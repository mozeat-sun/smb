# ==============================================================================
# ZOO Thread Pool - Deployment Configuration
# Automated deployment and CI/CD configuration
# ==============================================================================

# Deployment options
option(ZOO_ENABLE_AUTO_DEPLOY "Enable automatic deployment" OFF)
option(ZOO_DEPLOY_TO_STAGING "Deploy to staging environment" OFF)
option(ZOO_DEPLOY_TO_PRODUCTION "Deploy to production environment" OFF)
option(ZOO_ENABLE_DOCKER_BUILD "Build Docker containers" OFF)

# Deployment environments
set(ZOO_STAGING_PREFIX "/opt/zoo-staging" CACHE STRING "Staging deployment prefix")
set(ZOO_PRODUCTION_PREFIX "/opt/zoo" CACHE STRING "Production deployment prefix")
set(ZOO_DOCKER_REGISTRY "registry.company.com/zoo" CACHE STRING "Docker registry URL")

# ==============================================================================
# DEPLOYMENT VALIDATION
# ==============================================================================

function(zoo_validate_deployment_environment)
    if(ZOO_DEPLOY_TO_PRODUCTION AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(FATAL_ERROR "Production deployment requires Release build type")
    endif()
    
    if(ZOO_DEPLOY_TO_PRODUCTION AND ZOO_THREAD_POOL_DEBUG)
        message(FATAL_ERROR "Production deployment cannot have debug logging enabled")
    endif()
    
    if(ZOO_ENABLE_AUTO_DEPLOY AND NOT DEFINED ENV{CI})
        message(WARNING "Automatic deployment should only be used in CI/CD environments")
    endif()
endfunction()

# ==============================================================================
# STAGING DEPLOYMENT
# ==============================================================================

function(zoo_configure_staging_deployment)
    if(ZOO_DEPLOY_TO_STAGING)
        message(STATUS "Configuring staging deployment")
        
        # Override install prefix for staging
        set(CMAKE_INSTALL_PREFIX ${ZOO_STAGING_PREFIX} CACHE STRING "Install prefix" FORCE)
        
        # Create staging-specific configuration
        add_custom_target(deploy-staging
            COMMAND ${CMAKE_COMMAND} --build . --target install
            COMMAND ${CMAKE_COMMAND} -E echo "Deployed to staging: ${ZOO_STAGING_PREFIX}"
            COMMENT "Deploying ZOO Thread Pool to staging environment"
            VERBATIM
        )
        
        # Staging validation tests
        add_custom_target(validate-staging
            COMMAND ${CMAKE_COMMAND} -E echo "Running staging validation..."
            COMMAND ${ZOO_STAGING_PREFIX}/bin/zoo_thread_pool_tests --gtest_filter="*Basic*"
            DEPENDS deploy-staging
            COMMENT "Validating staging deployment"
            VERBATIM
        )
    endif()
endfunction()

# ==============================================================================
# PRODUCTION DEPLOYMENT
# ==============================================================================

function(zoo_configure_production_deployment)
    if(ZOO_DEPLOY_TO_PRODUCTION)
        message(STATUS "Configuring production deployment")
        
        # Validate production readiness
        zoo_validate_deployment_environment()
        
        # Override install prefix for production
        set(CMAKE_INSTALL_PREFIX ${ZOO_PRODUCTION_PREFIX} CACHE STRING "Install prefix" FORCE)
        
        # Production deployment with backup
        add_custom_target(deploy-production
            COMMAND ${CMAKE_COMMAND} -E echo "Backing up existing installation..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory 
                ${ZOO_PRODUCTION_PREFIX} 
                ${ZOO_PRODUCTION_PREFIX}.backup.${PROJECT_VERSION}
            COMMAND ${CMAKE_COMMAND} --build . --target install
            COMMAND ${CMAKE_COMMAND} -E echo "Deployed to production: ${ZOO_PRODUCTION_PREFIX}"
            COMMENT "Deploying ZOO Thread Pool to production environment"
            VERBATIM
        )
        
        # Production smoke tests
        add_custom_target(validate-production
            COMMAND ${CMAKE_COMMAND} -E echo "Running production smoke tests..."
            COMMAND ${ZOO_PRODUCTION_PREFIX}/bin/zoo_thread_pool_tests --gtest_filter="*Smoke*"
            DEPENDS deploy-production
            COMMENT "Validating production deployment"
            VERBATIM
        )
        
        # Rollback capability
        add_custom_target(rollback-production
            COMMAND ${CMAKE_COMMAND} -E echo "Rolling back production deployment..."
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${ZOO_PRODUCTION_PREFIX}
            COMMAND ${CMAKE_COMMAND} -E copy_directory 
                ${ZOO_PRODUCTION_PREFIX}.backup.${PROJECT_VERSION}
                ${ZOO_PRODUCTION_PREFIX}
            COMMENT "Rolling back production deployment"
            VERBATIM
        )
    endif()
endfunction()

# ==============================================================================
# DOCKER DEPLOYMENT
# ==============================================================================

function(zoo_configure_docker_deployment)
    if(ZOO_ENABLE_DOCKER_BUILD)
        message(STATUS "Configuring Docker deployment")
        
        # Find Docker
        find_program(DOCKER_EXECUTABLE docker)
        if(NOT DOCKER_EXECUTABLE)
            message(WARNING "Docker not found, skipping Docker deployment configuration")
            return()
        endif()
        
        # Generate Dockerfile
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile.in"
            "${CMAKE_CURRENT_BINARY_DIR}/Dockerfile"
            @ONLY
        )
        
        # Build Docker image
        add_custom_target(docker-build
            COMMAND ${DOCKER_EXECUTABLE} build 
                -t ${ZOO_DOCKER_REGISTRY}/zoo-thread-pool:${PROJECT_VERSION}
                -t ${ZOO_DOCKER_REGISTRY}/zoo-thread-pool:latest
                .
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Building Docker image for ZOO Thread Pool"
            VERBATIM
        )
        
        # Push Docker image
        add_custom_target(docker-push
            COMMAND ${DOCKER_EXECUTABLE} push ${ZOO_DOCKER_REGISTRY}/zoo-thread-pool:${PROJECT_VERSION}
            COMMAND ${DOCKER_EXECUTABLE} push ${ZOO_DOCKER_REGISTRY}/zoo-thread-pool:latest
            DEPENDS docker-build
            COMMENT "Pushing Docker image to registry"
            VERBATIM
        )
        
        # Docker compose for testing
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/docker/docker-compose.yml.in"
            "${CMAKE_CURRENT_BINARY_DIR}/docker-compose.yml"
            @ONLY
        )
        
        add_custom_target(docker-test
            COMMAND docker-compose up --abort-on-container-exit
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS docker-build
            COMMENT "Running Docker-based tests"
            VERBATIM
        )
    endif()
endfunction()

# ==============================================================================
# ARTIFACT PACKAGING
# ==============================================================================

function(zoo_configure_artifact_packaging)
    # Create release artifacts
    add_custom_target(package-artifacts
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/artifacts
        COMMENT "Creating release artifacts"
    )
    
    # Source archive
    add_custom_command(TARGET package-artifacts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf 
            ${CMAKE_CURRENT_BINARY_DIR}/artifacts/zoo-thread-pool-${PROJECT_VERSION}-src.tar.gz
            --exclude=build --exclude=.git --exclude=artifacts
            ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating source archive"
    )
    
    # Binary packages for different platforms
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_custom_command(TARGET package-artifacts POST_BUILD
            COMMAND ${CMAKE_COMMAND} --build . --target package
            COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                ${CMAKE_CURRENT_BINARY_DIR}/*.deb
                ${CMAKE_CURRENT_BINARY_DIR}/*.rpm
                ${CMAKE_CURRENT_BINARY_DIR}/*.tar.gz
                ${CMAKE_CURRENT_BINARY_DIR}/artifacts/
            COMMENT "Creating Linux binary packages"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        add_custom_command(TARGET package-artifacts POST_BUILD
            COMMAND ${CMAKE_COMMAND} --build . --target package
            COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                ${CMAKE_CURRENT_BINARY_DIR}/*.zip
                ${CMAKE_CURRENT_BINARY_DIR}/*.exe
                ${CMAKE_CURRENT_BINARY_DIR}/artifacts/
            COMMENT "Creating Windows binary packages"
        )
    endif()
    
    # Checksums
    add_custom_command(TARGET package-artifacts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/artifacts
            ${CMAKE_COMMAND} -E sha256sum * > checksums.sha256
        COMMENT "Generating checksums"
    )
endfunction()

# ==============================================================================
# CI/CD INTEGRATION
# ==============================================================================

function(zoo_configure_cicd)
    # Generate CI/CD pipeline configuration
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.github")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/ci/github-actions.yml.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/ci.yml"
            @ONLY
        )
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.gitlab-ci.yml.in")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/.gitlab-ci.yml.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/.gitlab-ci.yml"
            @ONLY
        )
    endif()
    
    # Quality gates
    add_custom_target(quality-gate
        COMMAND ${CMAKE_COMMAND} -E echo "Running quality gate checks..."
        COMMENT "Quality gate validation"
    )
    
    if(ZOO_ENABLE_TESTS)
        add_custom_command(TARGET quality-gate POST_BUILD
            COMMAND ${CMAKE_COMMAND} --build . --target test
            COMMENT "Running unit tests"
        )
    endif()
    
    if(ZOO_ENABLE_COVERAGE)
        add_custom_command(TARGET quality-gate POST_BUILD
            COMMAND ${CMAKE_COMMAND} --build . --target coverage
            COMMENT "Generating coverage report"
        )
    endif()
    
    # Static analysis
    find_program(CPPCHECK_EXECUTABLE cppcheck)
    if(CPPCHECK_EXECUTABLE)
        add_custom_command(TARGET quality-gate POST_BUILD
            COMMAND ${CPPCHECK_EXECUTABLE} --enable=all --error-exitcode=1 
                ${CMAKE_CURRENT_SOURCE_DIR}/src
            COMMENT "Running static analysis"
        )
    endif()
endfunction()

# ==============================================================================
# DEPLOYMENT ORCHESTRATION
# ==============================================================================

function(zoo_configure_deployment)
    message(STATUS "Configuring ZOO Thread Pool deployment")
    
    # Validate environment
    zoo_validate_deployment_environment()
    
    # Configure deployment targets
    zoo_configure_staging_deployment()
    zoo_configure_production_deployment()
    zoo_configure_docker_deployment()
    zoo_configure_artifact_packaging()
    zoo_configure_cicd()
    
    # Master deployment target
    add_custom_target(deploy-all
        COMMENT "Complete deployment pipeline"
    )
    
    if(ZOO_DEPLOY_TO_STAGING)
        add_dependencies(deploy-all deploy-staging validate-staging)
    endif()
    
    if(ZOO_DEPLOY_TO_PRODUCTION)
        add_dependencies(deploy-all deploy-production validate-production)
    endif()
    
    if(ZOO_ENABLE_DOCKER_BUILD)
        add_dependencies(deploy-all docker-build docker-push)
    endif()
    
    add_dependencies(deploy-all package-artifacts quality-gate)
    
    message(STATUS "Deployment configuration complete")
endfunction()

message(STATUS "ZOO Thread Pool deployment configuration loaded")
