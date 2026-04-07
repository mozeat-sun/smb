# Contributing to ZOO

Thanks for contributing to ZOO.

## Development Setup

1. Install CMake (3.16+) and a C compiler (GCC or Clang).
2. Configure the project:

```bash
cmake -S . -B build
```

3. Build all targets:

```bash
cmake --build build -j"$(nproc)"
```

4. Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Coding Guidelines

- Keep changes focused and minimal.
- Follow the existing naming and style of each module.
- Add or update tests for behavior changes.
- Avoid breaking public APIs unless discussed in an issue first.

## Commit and PR Guidelines

- Use clear commit messages describing what changed and why.
- Link related issues in pull requests.
- Include test evidence (commands and results).
- Update documentation when interfaces or behavior changes.

## Pull Request Checklist

- [ ] Builds successfully
- [ ] Tests pass locally
- [ ] Documentation updated if needed
- [ ] No unrelated file changes

## Reporting Bugs

Open an issue with:

- Expected behavior
- Actual behavior
- Reproduction steps
- Platform/compiler details
- Relevant logs or stack traces
