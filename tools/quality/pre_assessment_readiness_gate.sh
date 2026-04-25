#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
FINDINGS_LOG_FILE="${PRE_ASSESSMENT_FINDINGS_LOG_FILE:-docs/readiness/PRE_ASSESSMENT_FINDINGS_LOG.csv}"
SUMMARY_FILE="${PRE_ASSESSMENT_READINESS_SUMMARY_FILE:-docs/readiness/PRE_ASSESSMENT_READINESS_SUMMARY.md}"
FAIL_ON_OPEN_BLOCKERS="${PRE_ASSESSMENT_FAIL_ON_OPEN_BLOCKERS:-0}"
FAIL_ON_OPEN_MAJORS="${PRE_ASSESSMENT_FAIL_ON_OPEN_MAJORS:-0}"

mkdir -p "${ARTIFACT_DIR}"

if [[ ! -f "${FINDINGS_LOG_FILE}" ]]; then
  echo "Pre-assessment findings log not found: ${FINDINGS_LOG_FILE}" >&2
  exit 1
fi

if [[ ! -f "${SUMMARY_FILE}" ]]; then
  echo "Pre-assessment readiness summary not found: ${SUMMARY_FILE}" >&2
  exit 1
fi

python3 - "${FINDINGS_LOG_FILE}" "${SUMMARY_FILE}" "${ARTIFACT_DIR}" "${FAIL_ON_OPEN_BLOCKERS}" "${FAIL_ON_OPEN_MAJORS}" <<'PY'
import csv
import datetime as dt
import json
import os
import re
import sys
from collections import Counter, defaultdict


findings_path, summary_path, artifact_dir, fail_blockers, fail_majors = sys.argv[1:6]
fail_blockers = fail_blockers == "1"
fail_majors = fail_majors == "1"

expected_fields = [
    "id",
    "assessment_round",
    "reported_utc",
    "area",
    "severity",
    "status",
    "owner",
    "target_date",
    "closure_date",
    "summary",
    "remediation_notes",
]

allowed_severities = {"BLOCKER", "MAJOR", "MINOR", "OBSERVATION"}
allowed_statuses = {"OPEN", "IN_PROGRESS", "CLOSED", "DEFERRED"}
open_statuses = {"OPEN", "IN_PROGRESS", "DEFERRED"}


def parse_iso_date(value: str, field: str, row_id: str, errors: list[str]) -> bool:
    if not value:
        return True
    try:
        dt.date.fromisoformat(value)
        return True
    except ValueError:
        errors.append(f"row {row_id}: invalid {field} date '{value}', expected YYYY-MM-DD")
        return False


with open(findings_path, newline="", encoding="utf-8") as f:
    reader = csv.DictReader(f)
    if reader.fieldnames != expected_fields:
        print(
            f"Unexpected findings log header in {findings_path}.\n"
            f"Expected: {','.join(expected_fields)}\n"
            f"Actual:   {','.join(reader.fieldnames or [])}",
            file=sys.stderr,
        )
        sys.exit(1)

    rows = list(reader)

errors: list[str] = []
counts_by_severity = Counter()
counts_by_status = Counter()
open_by_severity = Counter()
open_items = []
by_round = defaultdict(lambda: {"total": 0, "open": 0, "closed": 0})

for row in rows:
    row_id = (row.get("id") or "").strip()
    round_name = (row.get("assessment_round") or "").strip()
    area = (row.get("area") or "").strip()
    severity = (row.get("severity") or "").strip().upper()
    status = (row.get("status") or "").strip().upper()
    summary = (row.get("summary") or "").strip()

    if not row_id:
        errors.append("row with empty id")
    if not round_name:
        errors.append(f"row {row_id or '<unknown>'}: assessment_round is required")
    if not area:
        errors.append(f"row {row_id or '<unknown>'}: area is required")
    if not summary:
        errors.append(f"row {row_id or '<unknown>'}: summary is required")

    if severity not in allowed_severities:
        errors.append(
            f"row {row_id or '<unknown>'}: invalid severity '{severity}', allowed values are {sorted(allowed_severities)}"
        )
    if status not in allowed_statuses:
        errors.append(
            f"row {row_id or '<unknown>'}: invalid status '{status}', allowed values are {sorted(allowed_statuses)}"
        )

    parse_iso_date((row.get("reported_utc") or "").strip(), "reported_utc", row_id or "<unknown>", errors)
    parse_iso_date((row.get("target_date") or "").strip(), "target_date", row_id or "<unknown>", errors)
    closure_date = (row.get("closure_date") or "").strip()
    parse_iso_date(closure_date, "closure_date", row_id or "<unknown>", errors)

    if status == "CLOSED" and not closure_date:
        errors.append(f"row {row_id or '<unknown>'}: closure_date is required for CLOSED status")
    if status in open_statuses and closure_date:
        errors.append(
            f"row {row_id or '<unknown>'}: closure_date must be empty for non-CLOSED status ({status})"
        )

    if severity:
        counts_by_severity[severity] += 1
    if status:
        counts_by_status[status] += 1

    if round_name:
        by_round[round_name]["total"] += 1
        if status == "CLOSED":
            by_round[round_name]["closed"] += 1
        elif status in open_statuses:
            by_round[round_name]["open"] += 1

    if status in open_statuses:
        if severity:
            open_by_severity[severity] += 1
        open_items.append(
            {
                "id": row_id,
                "assessment_round": round_name,
                "area": area,
                "severity": severity,
                "status": status,
                "owner": (row.get("owner") or "").strip(),
                "target_date": (row.get("target_date") or "").strip(),
                "summary": summary,
            }
        )

summary_text = open(summary_path, "r", encoding="utf-8").read()
outcome_match = re.search(r"^- Outcome:\s*(.+?)\s*$", summary_text, re.MULTILINE)
summary_outcome = outcome_match.group(1).strip() if outcome_match else ""

gate_failures = []
if errors:
    gate_failures.append("validation-errors")
if fail_blockers and open_by_severity.get("BLOCKER", 0) > 0:
    gate_failures.append("open-blockers")
if fail_majors and open_by_severity.get("MAJOR", 0) > 0:
    gate_failures.append("open-majors")

status = "failed" if gate_failures else "passed"
result = {
    "status": status,
    "gate_failures": gate_failures,
    "validation_error_count": len(errors),
    "tracked_findings": len(rows),
    "open_findings": len(open_items),
    "open_findings_by_severity": dict(sorted(open_by_severity.items())),
    "findings_by_severity": dict(sorted(counts_by_severity.items())),
    "findings_by_status": dict(sorted(counts_by_status.items())),
    "assessment_rounds": dict(sorted(by_round.items())),
    "summary_outcome": summary_outcome,
    "source_findings_log": os.path.basename(findings_path),
    "source_summary": os.path.basename(summary_path),
}

json_path = os.path.join(artifact_dir, "pre_assessment_readiness_summary.json")
with open(json_path, "w", encoding="utf-8") as f:
    json.dump(result, f)

report_path = os.path.join(artifact_dir, "pre_assessment_readiness_report.md")
with open(report_path, "w", encoding="utf-8") as f:
    f.write("# Pre-Assessment Readiness Gate Report\n\n")
    f.write(f"- Source findings log: `{os.path.basename(findings_path)}`\n")
    f.write(f"- Source readiness summary: `{os.path.basename(summary_path)}`\n")
    f.write(f"- Gate status: `{status}`\n")
    f.write(f"- Validation errors: {len(errors)}\n")
    f.write(f"- Tracked findings: {len(rows)}\n")
    f.write(f"- Open findings: {len(open_items)}\n")
    f.write(f"- Declared summary outcome: `{summary_outcome or 'Not declared'}`\n\n")

    f.write("## Findings by severity\n\n")
    if counts_by_severity:
        for severity, count in sorted(counts_by_severity.items()):
            f.write(f"- {severity}: {count}\n")
    else:
        f.write("- None\n")

    f.write("\n## Findings by status\n\n")
    if counts_by_status:
        for finding_status, count in sorted(counts_by_status.items()):
            f.write(f"- {finding_status}: {count}\n")
    else:
        f.write("- None\n")

    f.write("\n## Assessment rounds\n\n")
    if by_round:
        for round_name, stats in sorted(by_round.items()):
            f.write(
                f"- {round_name}: total={stats['total']} open={stats['open']} closed={stats['closed']}\n"
            )
    else:
        f.write("- None\n")

    f.write("\n## Open blocker and major findings\n\n")
    open_critical = [
        item for item in open_items if item["severity"] in {"BLOCKER", "MAJOR"}
    ]
    if open_critical:
        for item in open_critical:
            f.write(
                "- "
                f"{item['id'] or '<no-id>'} "
                f"[{item['severity']}/{item['status']}] "
                f"area={item['area'] or '<no-area>'} "
                f"owner={item['owner'] or '<unassigned>'} "
                f"target={item['target_date'] or '<unset>'} "
                f"summary={item['summary'] or '<none>'}\n"
            )
    else:
        f.write("- None\n")

    f.write("\n## Validation errors\n\n")
    if errors:
        for err in errors:
            f.write(f"- {err}\n")
    else:
        f.write("- None\n")

print(json.dumps(result))
if gate_failures:
    sys.exit(1)
PY