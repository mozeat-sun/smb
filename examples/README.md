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
- zoo_example_udp_client
- zoo_example_udp_server
- zoo_example_publisher
- zoo_example_subscriber
- zoo_example_shm_server
- zoo_example_shm_client

Executables are generated in the repository root stage/bin directory.

## Run

From repository root:

```bash
./stage/bin/zoo_example_server
./stage/bin/zoo_example_client
./stage/bin/zoo_example_udp_server
./stage/bin/zoo_example_udp_client
./stage/bin/zoo_example_publisher
./stage/bin/zoo_example_subscriber
./stage/bin/zoo_example_shm_server
./stage/bin/zoo_example_shm_client
```

## UDP Transport Demo

Run server and client in separate terminals.

Terminal 1:

```bash
./stage/bin/zoo_example_udp_server -t demo_udp -l 2
```

Terminal 2:

```bash
./stage/bin/zoo_example_udp_client -t demo_udp -p "hello udp" -l 2
```

## SHM Transport Demo

Run server and client in separate terminals.

Terminal 1:

```bash
./stage/bin/zoo_example_shm_server --channel zoo_shm_demo --server-name shm_demo_server
```

Terminal 2:

```bash
./stage/bin/zoo_example_shm_client --channel zoo_shm_demo --server-name shm_demo_server --payload "hello shm"
```

Useful flags:

- `--topic <name>`
- `--timeout-ms <ms>` (client only)
- `--log-level <0-6>`

## Local Runner Tool

For one-command local example runs (including auto-build with examples enabled), use:

```bash
bash tools/run_local_examples.sh all
```

Supported modes:

- `tcp`
- `udp`
- `pubsub`
- `shm`
- `all`

Quick examples:

```bash
bash tools/run_local_examples.sh udp
SKIP_BUILD=1 bash tools/run_local_examples.sh shm
EXAMPLES_LOG_LEVEL=1 bash tools/run_local_examples.sh all
```
