# ZOO Log Module

A comprehensive and configurable logging module for the ZOO library that provides multi-level logging capabilities with various output targets and formatting options.

## Features

- **Multi-level logging**: Support for TRACE, DEBUG, INFO, WARN, ERROR, and FATAL levels
- **Multiple output targets**: Console, file, syslog (on supported platforms)
- **Thread-safe**: Built with thread safety in mind for multi-threaded applications
- **Configurable formatting**: Customizable log message formats with timestamps, thread IDs, and source location
- **Platform-agnostic**: Works across different operating systems with platform-specific optimizations
- **Performance optimized**: Minimal overhead with efficient buffering and async logging options
- **Runtime configuration**: Dynamic log level and target configuration

## Architecture

The ZOO Log module is designed with the following components:

- **Core logging engine**: Handles message formatting and routing
- **Output handlers**: Pluggable output targets (console, file, syslog)
- **Level management**: Configurable logging levels with filtering
- **Thread synchronization**: Thread-safe operations with minimal locking
- **Configuration management**: Runtime and compile-time configuration options

## Getting Started

### Prerequisites

- CMake 3.14 or higher
- C11 compatible compiler
- ZOO platform module (../platform)
- ZOO utility module (../util)

### Building

```bash
mkdir build
cd build
cmake ..
make
```

### Integration

To use the ZOO Log module in your project:

```c
#include "zoo_log.h"

int main() {
    // Initialize logging system
    zoo_log_init();
    
    // Configure log level
    zoo_log_set_level(ZOO_LOG_LEVEL_INFO);
    
    // Basic logging
    ZOO_LOG_INFO("Application started");
    ZOO_LOG_DEBUG("Debug information: %d", 42);
    ZOO_LOG_ERROR("Error occurred: %s", "Sample error");
    
    // Cleanup
    zoo_log_cleanup();
    return 0;
}
```

## API Reference

### Core Functions

- `zoo_result_t zoo_log_init(void)` - Initialize the logging system
- `void zoo_log_cleanup(void)` - Cleanup and shutdown logging
- `zoo_result_t zoo_log_set_level(zoo_log_level_t level)` - Set minimum log level
- `zoo_result_t zoo_log_add_target(zoo_log_target_t *target)` - Add output target

### Logging Macros

- `ZOO_LOG_TRACE(format, ...)` - Trace level logging
- `ZOO_LOG_DEBUG(format, ...)` - Debug level logging  
- `ZOO_LOG_INFO(format, ...)` - Information level logging
- `ZOO_LOG_WARN(format, ...)` - Warning level logging
- `ZOO_LOG_ERROR(format, ...)` - Error level logging
- `ZOO_LOG_FATAL(format, ...)` - Fatal error level logging

## Configuration

The logging system can be configured through:

1. **Compile-time macros**: Define build-time behavior
2. **Runtime API**: Dynamic configuration during execution
3. **Configuration files**: External configuration support

### Build Options

- `ZOO_LOG_ENABLE_COLORS` - Enable colored console output
- `ZOO_LOG_ENABLE_TIMESTAMPS` - Include timestamps in log messages
- `ZOO_LOG_ENABLE_THREAD_ID` - Include thread IDs in log messages
- `ZOO_LOG_ENABLE_SOURCE_LOCATION` - Include file/line information

## Performance

The ZOO Log module is designed for high performance:

- Minimal overhead when logging is disabled
- Efficient string formatting
- Optional asynchronous logging
- Memory pool for reduced allocations

## Thread Safety

All logging operations are thread-safe by default. The module uses:

- Lock-free algorithms where possible
- Minimal critical sections
- Per-thread buffers for performance

## Testing

Run the test suite:

```bash
make test
```

## Contributing

When contributing to the ZOO Log module:

1. Follow the existing code style
2. Add unit tests for new functionality
3. Update documentation as needed
4. Ensure thread safety for all operations

## License

Copyright (C) 2025, Basic Software Research Institute ltd. All rights reserved.

## Authors

- weiwang.sun - Initial implementation
- GitHub Copilot - Implementation completion and enhancements

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/index.html)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.
