#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
mkdir -p "${ARTIFACT_DIR}"

if command -v cppcheck > /dev/null; then
  cppcheck --enable=warning,style,performance,portability --inconclusive \
    --quiet --error-exitcode=1 \
    --exclude=third_party --exclude=build --exclude=build-monorepo \
    . 2> "${ARTIFACT_DIR}/cppcheck.txt"
  echo '{"tool":"cppcheck","status":"passed"}' > "${ARTIFACT_DIR}/static_analysis_summary.json"
else
  echo "cppcheck not installed; static analysis skipped" | tee "${ARTIFACT_DIR}/cppcheck.txt"
  echo '{"tool":"cppcheck","status":"skipped"}' > "${ARTIFACT_DIR}/static_analysis_summary.json"
fi
