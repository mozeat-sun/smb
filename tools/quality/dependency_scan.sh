#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
mkdir -p "${ARTIFACT_DIR}"

{
  echo "component,type,path"
  find third_party -maxdepth 3 -type f \( -name "LICENSE*" -o -name "CMakeLists.txt" -o -name "README*" \) 2>/dev/null | \
    awk '{print "third_party,file," $0}'
  find . -maxdepth 3 -type f \( -name "CMakeLists.txt" -o -name "*.cmake" \) \
    -not -path './build/*' -not -path './build-monorepo/*' | \
    awk '{print "build_config,file," $0}'
} > "${ARTIFACT_DIR}/dependency_inventory.csv"

echo '{"status":"inventory-generated"}' > "${ARTIFACT_DIR}/dependency_scan_summary.json"
