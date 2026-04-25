#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
CATALOG="docs/requirements/REQUIREMENTS_CATALOG.md"
TRACEABILITY_TEST_REQUIRED_REQS="${TRACEABILITY_TEST_REQUIRED_REQS:-REQ-REL-001,REQ-REL-002,REQ-REL-003,REQ-REL-004,REQ-PERF-001,REQ-PERF-002,REQ-PERF-003,REQ-SAFE-001,REQ-SAFE-002,REQ-SAFE-003,REQ-COMP-001}"

mkdir -p "${ARTIFACT_DIR}"

bash tools/quality/generate_traceability_matrix.sh > /dev/null

REQ_IDS=$(grep -oE '`REQ-[A-Z]+-[0-9]+`' "${CATALOG}" | tr -d '`' | sort -u)
AUDIT_FILE="${ARTIFACT_DIR}/traceability_audit_sample.md"
SUMMARY_FILE="${ARTIFACT_DIR}/traceability_gate_summary.json"
required_list=$(printf '%s' "${TRACEABILITY_TEST_REQUIRED_REQS}" | tr ',' ' ')
unlinked_count=0
missing_test_links=()

{
  echo "# Traceability Audit Sample"
  echo
  echo "| Requirement | Design or implementation evidence | Verification evidence |"
  echo "|---|---|---|"

  while IFS= read -r req_id; do
    [[ -z "${req_id}" ]] && continue

    repo_count=$({ grep -R -I -n --exclude-dir=.git --exclude-dir=build --exclude-dir=build-monorepo --exclude="traceability_matrix.csv" --exclude="traceability_test_matrix.csv" "${req_id}" . || true; } | grep -v "docs/requirements/REQUIREMENTS_CATALOG.md" | wc -l)
    design_match=$(grep -R -I -n --exclude-dir=.git --exclude-dir=build --exclude-dir=build-monorepo "${req_id}" docs include src tools 2>/dev/null | head -n 1 || true)
    verification_match=$(grep -R -I -n --exclude-dir=.git --exclude-dir=build --exclude-dir=build-monorepo "${req_id}" tests tools/quality 2>/dev/null | head -n 1 || true)
    test_count=$({ grep -R -I -n --exclude-dir=.git --exclude-dir=build --exclude-dir=build-monorepo "${req_id}" tests 2>/dev/null || true; } | wc -l)

    if [[ ${repo_count} -eq 0 ]]; then
      unlinked_count=$((unlinked_count + 1))
    fi

    for required_req in ${required_list}; do
      if [[ "${req_id}" == "${required_req}" && ${test_count} -eq 0 ]]; then
        missing_test_links+=("${req_id}")
      fi
    done

    if [[ -z "${design_match}" ]]; then
      design_match="missing"
    fi
    if [[ -z "${verification_match}" ]]; then
      verification_match="missing"
    fi

    printf '| `%s` | `%s` | `%s` |\n' "${req_id}" "${design_match}" "${verification_match}"
  done <<< "${REQ_IDS}"
} > "${AUDIT_FILE}"

python3 - "${SUMMARY_FILE}" "${unlinked_count}" "${#missing_test_links[@]}" "${TRACEABILITY_TEST_REQUIRED_REQS}" <<'PY'
import json
import sys

summary_path = sys.argv[1]
unlinked_count = int(sys.argv[2])
missing_test_count = int(sys.argv[3])
required = [req for req in sys.argv[4].split(',') if req]

summary = {
    "status": "passed" if unlinked_count == 0 and missing_test_count == 0 else "failed",
    "unlinked_requirements": unlinked_count,
    "required_test_link_requirements": required,
    "missing_required_test_links": missing_test_count,
    "audit_sample": "traceability_audit_sample.md",
}

with open(summary_path, "w", encoding="utf-8") as f:
    json.dump(summary, f)

print(json.dumps(summary))
PY

if [[ ${unlinked_count} -ne 0 || ${#missing_test_links[@]} -ne 0 ]]; then
  printf 'Traceability gate failed. Missing test links for: %s\n' "${missing_test_links[*]:-none}" >&2
  exit 1
fi