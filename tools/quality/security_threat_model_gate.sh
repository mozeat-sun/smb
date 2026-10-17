#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
THREAT_MODEL_FILE="docs/security/THREAT_MODEL.md"
REMEDIATION_SLA_FILE="docs/security/VULNERABILITY_REMEDIATION_SLA.md"
REMEDIATION_LOG_FILE="docs/security/VULNERABILITY_REMEDIATION_LOG.csv"

mkdir -p "${ARTIFACT_DIR}"

for required_file in "${THREAT_MODEL_FILE}" "${REMEDIATION_SLA_FILE}" "${REMEDIATION_LOG_FILE}"; do
  if [[ ! -f "${required_file}" ]]; then
    echo "Missing required security evidence file: ${required_file}" >&2
    exit 1
  fi
done

required_sections=(
  "## Scope"
  "## Assets"
  "## Trust Boundaries"
  "## Threat Actors"
  "## Threat Scenarios"
  "## Control Mapping"
  "## Review Cadence"
)

for section in "${required_sections[@]}"; do
  if ! grep -q "${section}" "${THREAT_MODEL_FILE}"; then
    echo "Threat model is missing required section: ${section}" >&2
    exit 1
  fi
done

required_controls=(
  "REQ-SEC-001"
  "REQ-SEC-002"
  "REQ-SEC-003"
  "REQ-SEC-004"
)

for control in "${required_controls[@]}"; do
  if ! grep -q "${control}" "${THREAT_MODEL_FILE}"; then
    echo "Threat model does not reference required control: ${control}" >&2
    exit 1
  fi
done

required_threat_ids=("TM-001" "TM-002" "TM-003" "TM-004" "TM-005")
for tid in "${required_threat_ids[@]}"; do
  if ! grep -q "${tid}" "${THREAT_MODEL_FILE}"; then
    echo "Threat model scenario table missing: ${tid}" >&2
    exit 1
  fi
done

if ! head -n 1 "${REMEDIATION_LOG_FILE}" | grep -q "^id,reported_utc,severity,status,owner,target_sla_days,closed_utc,source,notes$"; then
  echo "Vulnerability remediation log CSV header is invalid (expected canonical schema)" >&2
  exit 1
fi

open_findings=$(
  tail -n +2 "${REMEDIATION_LOG_FILE}" \
  | awk -F',' 'toupper($4) != "CLOSED" && toupper($4) != "RESOLVED" { count++ } END { print count + 0 }'
)

summary_json="${ARTIFACT_DIR}/security_threat_model_summary.json"
summary_md="${ARTIFACT_DIR}/security_threat_model_summary.md"

cat > "${summary_json}" <<JSON
{
  "gate": "security-threat-model",
  "status": "passed",
  "threat_model_file": "${THREAT_MODEL_FILE}",
  "remediation_sla_file": "${REMEDIATION_SLA_FILE}",
  "remediation_log_file": "${REMEDIATION_LOG_FILE}",
  "open_findings": ${open_findings},
  "required_scenarios": ["TM-001", "TM-002", "TM-003", "TM-004", "TM-005"]
}
JSON

cat > "${summary_md}" <<MD
# Security Threat-Model Gate Summary

- Gate: \`security-threat-model\`
- Status: \`passed\`
- Threat model: \`${THREAT_MODEL_FILE}\`
- Vulnerability SLA policy: \`${REMEDIATION_SLA_FILE}\`
- Vulnerability remediation log: \`${REMEDIATION_LOG_FILE}\`
- Open findings (status not CLOSED/RESOLVED): ${open_findings}

## Required scenario coverage check

- TM-001: present
- TM-002: present
- TM-003: present
- TM-004: present
- TM-005: present
MD

echo "security threat-model gate passed (open findings: ${open_findings})"
