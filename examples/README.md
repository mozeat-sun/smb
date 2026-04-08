# Examples

Top-level examples entry.

Current module-specific examples remain under each module directory, such as:
- socket/examples
- smb/examples

This folder is reserved for cross-module examples and quick-start demos.

## Build

Enable examples and build from repository root:

```bash
cmake -S . -B build -DZOO_BUILD_EXAMPLES=ON
cmake --build build -j"$(nproc)"
```

## Generated Targets

- zoo_example_client
- zoo_example_server
- zoo_example_publisher
- zoo_example_subscriber

Executables are generated in the repository root bin directory.

## Run

From repository root:

```bash
./bin/zoo_example_server
./bin/zoo_example_client
./bin/zoo_example_publisher
./bin/zoo_example_subscriber
```
