#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# name=image-tag
TARGETS=(
  "ubuntu22-gcc=zoo-build:ubuntu22-gcc"
  "ubuntu24-clang=zoo-build:ubuntu24-clang"
  "debian12-gcc=zoo-build:debian12-gcc"
  "fedora40-gcc=zoo-build:fedora40-gcc"
  "alpine320-gcc=zoo-build:alpine320-gcc"
  "ubuntu22-aarch64-cross=zoo-build:arm64-cross"
  "ubuntu22-mingw-cross=zoo-build:mingw-cross"
)

usage() {
  cat <<'EOF'
Usage:
  docker/build-all.sh [options] [target ...]

Examples:
  docker/build-all.sh
  docker/build-all.sh ubuntu22-gcc ubuntu24-clang
  docker/build-all.sh --list

Options:
  -l, --list      List available targets and exit
  -h, --help      Show this help
EOF
}

list_targets() {
  for entry in "${TARGETS[@]}"; do
    local name="${entry%%=*}"
    local tag="${entry#*=}"
    printf "%-24s -> %s\n" "${name}" "${tag}"
  done
}

has_target() {
  local needle="$1"
  for entry in "${TARGETS[@]}"; do
    if [[ "${entry%%=*}" == "${needle}" ]]; then
      return 0
    fi
  done
  return 1
}

build_target() {
  local name="$1"
  local tag=""
  for entry in "${TARGETS[@]}"; do
    if [[ "${entry%%=*}" == "${name}" ]]; then
      tag="${entry#*=}"
      break
    fi
  done

  if [[ -z "${tag}" ]]; then
    echo "Unknown target: ${name}" >&2
    return 1
  fi

  local dockerfile="${SCRIPT_DIR}/${name}/Dockerfile"
  if [[ ! -f "${dockerfile}" ]]; then
    echo "Missing Dockerfile: ${dockerfile}" >&2
    return 1
  fi

  echo "==> Building ${name} as ${tag}"
  docker build -t "${tag}" -f "${dockerfile}" "${REPO_ROOT}"
}

if [[ $# -eq 0 ]]; then
  requested=()
  for entry in "${TARGETS[@]}"; do
    requested+=("${entry%%=*}")
  done
else
  requested=()
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -l|--list)
        list_targets
        exit 0
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        requested+=("$1")
        ;;
    esac
    shift
  done
fi

for name in "${requested[@]}"; do
  if ! has_target "${name}"; then
    echo "Invalid target: ${name}" >&2
    echo "Run 'docker/build-all.sh --list' to see valid targets." >&2
    exit 1
  fi
done

for name in "${requested[@]}"; do
  build_target "${name}"
done

echo "All requested Docker images built successfully."
