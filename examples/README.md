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
- zoo_example_shm_server
- zoo_example_shm_client

Executables are generated in the repository root bin directory.

## Run

From repository root:

```bash
./bin/zoo_example_server
./bin/zoo_example_client
./bin/zoo_example_publisher
./bin/zoo_example_subscriber
./bin/zoo_example_shm_server
./bin/zoo_example_shm_client
```

## SHM Transport Demo

Run server and client in separate terminals.

Terminal 1:

```bash
./bin/zoo_example_shm_server --channel zoo_shm_demo --server-name shm_demo_server
```

Terminal 2:

```bash
./bin/zoo_example_shm_client --channel zoo_shm_demo --server-name shm_demo_server --payload "hello shm"
```

Useful flags:

- `--topic <name>`
- `--timeout-ms <ms>` (client only)
- `--log-level <0-6>`
