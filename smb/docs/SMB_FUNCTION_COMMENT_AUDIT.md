# SMB Function Comment Audit

Date: 2026-04-05
Scope: `src`, `inc`, `tests`, `examples`
Rule: function definitions must have an immediately preceding Doxygen-style block comment.

## Audit Result

- Total missing function comments: 0
- Status: PASS

## What Was Done

1. Added missing function comments in previously flagged files under:
- `tests/unit`
- `tests/integration`
- `tests/common`
- `examples/common`

2. Added an automated checker script:
- `tools/check_function_comments.sh`

3. Added CI/build integration:
- Custom target: `check_function_comments`
- CTest test: `smb_comment_compliance`

## How To Run

### Direct script

```bash
cd smb
./tools/check_function_comments.sh
```

### Via CMake target

```bash
cmake --build . --target check_function_comments
```

### Via CTest

```bash
ctest -R smb_comment_compliance
```

## Notes

- The checker ignores build artifacts and third-party source by scanning only `src`, `inc`, `tests`, and `examples`.
- The checker exits non-zero when any missing function comments are found, making it suitable for CI gating.
