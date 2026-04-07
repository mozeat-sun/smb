#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
CATALOG="docs/REQUIREMENTS_CATALOG.md"
mkdir -p "${ARTIFACT_DIR}"

if [[ ! -f "${CATALOG}" ]]; then
  echo "Requirements catalog not found: ${CATALOG}" >&2
  exit 1
fi

REQ_IDS=$(grep -oE '`REQ-[A-Z]+-[0-9]+`' "${CATALOG}" | tr -d '`' | sort -u)

{
  echo "requirement_id,linked_occurrences,status"
  while IFS= read -r req_id; do
    [[ -z "${req_id}" ]] && continue
    count=$(grep -R -I -n --exclude-dir=.git --exclude-dir=build --exclude-dir=build-monorepo --exclude="traceability_matrix.csv" "${req_id}" . | wc -l)
    status="unverified"
    if [[ ${count} -gt 1 ]]; then
      status="linked"
    fi
    echo "${req_id},${count},${status}"
  done <<< "${REQ_IDS}"
} > "${ARTIFACT_DIR}/traceability_matrix.csv"

awk -F',' 'NR>1 {total++; if ($3 == "linked") linked++} END {printf("{\"total\":%d,\"linked\":%d}\n", total, linked)}' \
  "${ARTIFACT_DIR}/traceability_matrix.csv" > "${ARTIFACT_DIR}/traceability_summary.json"

cat "${ARTIFACT_DIR}/traceability_matrix.csv"
