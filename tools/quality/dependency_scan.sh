#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
DEPENDENCY_SCAN_REQUIRE_SCANNER="${DEPENDENCY_SCAN_REQUIRE_SCANNER:-0}"
DEPENDENCY_SCAN_FAIL_ON_SEVERITY="${DEPENDENCY_SCAN_FAIL_ON_SEVERITY:-HIGH,CRITICAL}"
mkdir -p "${ARTIFACT_DIR}"

{
  echo "component,type,path"
  find thirdparty -maxdepth 3 -type f \( -name "LICENSE*" -o -name "CMakeLists.txt" -o -name "README*" \) 2>/dev/null | \
    awk '{print "thirdparty,file," $0}'
  find . -maxdepth 3 -type f \( -name "CMakeLists.txt" -o -name "*.cmake" \) \
    -not -path './build/*' -not -path './build-monorepo/*' | \
    awk '{print "build_config,file," $0}'
} > "${ARTIFACT_DIR}/dependency_inventory.csv"

if command -v trivy > /dev/null; then
  trivy fs \
    --quiet \
    --format json \
    --output "${ARTIFACT_DIR}/dependency_scan_trivy.json" \
    .

  python3 - "${ARTIFACT_DIR}/dependency_scan_trivy.json" "${DEPENDENCY_SCAN_FAIL_ON_SEVERITY}" > "${ARTIFACT_DIR}/dependency_scan_summary.json" <<'PY'
import json
import sys

report_path = sys.argv[1]
fail_levels = {lvl.strip().upper() for lvl in sys.argv[2].split(",") if lvl.strip()}
counts = {"UNKNOWN": 0, "LOW": 0, "MEDIUM": 0, "HIGH": 0, "CRITICAL": 0}

with open(report_path, "r", encoding="utf-8") as f:
    doc = json.load(f)

for result in doc.get("Results", []):
    for vuln in result.get("Vulnerabilities", []) or []:
        sev = str(vuln.get("Severity", "UNKNOWN")).upper()
        counts[sev] = counts.get(sev, 0) + 1

summary = {
    "tool": "trivy",
    "status": "passed",
    "fail_on_severity": sorted(fail_levels),
    "vulnerabilities_by_severity": counts,
}

if any(counts.get(level, 0) > 0 for level in fail_levels):
    summary["status"] = "failed"

print(json.dumps(summary))
PY

  if grep -q '"status": "failed"' "${ARTIFACT_DIR}/dependency_scan_summary.json"; then
    cat "${ARTIFACT_DIR}/dependency_scan_summary.json"
    exit 1
  fi
else
  if [[ "${DEPENDENCY_SCAN_REQUIRE_SCANNER}" == "1" ]]; then
    echo "trivy not installed; dependency scan cannot run" >&2
    exit 2
  fi
  echo '{"tool":"trivy","status":"skipped","reason":"trivy-not-installed"}' > "${ARTIFACT_DIR}/dependency_scan_summary.json"
fi
