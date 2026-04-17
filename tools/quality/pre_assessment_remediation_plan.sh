#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
FINDINGS_LOG_FILE="${PRE_ASSESSMENT_FINDINGS_LOG_FILE:-docs/PRE_ASSESSMENT_FINDINGS_LOG.csv}"

mkdir -p "${ARTIFACT_DIR}"

if [[ ! -f "${FINDINGS_LOG_FILE}" ]]; then
  echo "Pre-assessment findings log not found: ${FINDINGS_LOG_FILE}" >&2
  exit 1
fi

python3 - "${FINDINGS_LOG_FILE}" "${ARTIFACT_DIR}" <<'PY'
import csv
import datetime as dt
import json
import os
import sys


findings_path, artifact_dir = sys.argv[1:3]

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

severity_rank = {
    "BLOCKER": 0,
    "MAJOR": 1,
    "MINOR": 2,
    "OBSERVATION": 3,
}
open_statuses = {"OPEN", "IN_PROGRESS", "DEFERRED"}
today = dt.date.today()


def parse_date(value: str):
    value = (value or "").strip()
    if not value:
        return None
    try:
        return dt.date.fromisoformat(value)
    except ValueError:
        return None


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

open_items = []
for row in rows:
    status = (row.get("status") or "").strip().upper()
    if status not in open_statuses:
        continue

    severity = (row.get("severity") or "").strip().upper()
    target_date = parse_date(row.get("target_date") or "")
    due_state = "unset"
    if target_date is not None:
        if target_date < today:
            due_state = "overdue"
        elif target_date == today:
            due_state = "due-today"
        else:
            due_state = "on-track"

    open_items.append(
        {
            "id": (row.get("id") or "").strip(),
            "assessment_round": (row.get("assessment_round") or "").strip(),
            "area": (row.get("area") or "").strip(),
            "severity": severity,
            "status": status,
            "owner": (row.get("owner") or "").strip(),
            "target_date": (row.get("target_date") or "").strip(),
            "summary": (row.get("summary") or "").strip(),
            "remediation_notes": (row.get("remediation_notes") or "").strip(),
            "due_state": due_state,
        }
    )

open_items.sort(
    key=lambda item: (
        severity_rank.get(item["severity"], 99),
        item["target_date"] or "9999-12-31",
        item["id"] or "",
    )
)

counts = {
    "total_open": len(open_items),
    "by_severity": {},
    "by_due_state": {"overdue": 0, "due-today": 0, "on-track": 0, "unset": 0},
}

for item in open_items:
    sev = item["severity"] or "UNKNOWN"
    counts["by_severity"][sev] = counts["by_severity"].get(sev, 0) + 1
    counts["by_due_state"][item["due_state"]] += 1

summary = {
    "status": "generated",
    "source_findings_log": os.path.basename(findings_path),
    "generated_utc": dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
    "open_findings": counts["total_open"],
    "open_findings_by_severity": dict(sorted(counts["by_severity"].items())),
    "open_findings_by_due_state": counts["by_due_state"],
}

json_path = os.path.join(artifact_dir, "pre_assessment_remediation_plan_summary.json")
with open(json_path, "w", encoding="utf-8") as f:
    json.dump(summary, f)

report_path = os.path.join(artifact_dir, "pre_assessment_remediation_plan.md")
with open(report_path, "w", encoding="utf-8") as f:
    f.write("# Pre-Assessment Remediation Plan\n\n")
    f.write(f"- Source findings log: `{os.path.basename(findings_path)}`\n")
    f.write(f"- Generated (UTC): `{summary['generated_utc']}`\n")
    f.write(f"- Open findings: {counts['total_open']}\n")
    f.write(f"- Overdue actions: {counts['by_due_state']['overdue']}\n\n")

    f.write("## Open findings by severity\n\n")
    if counts["by_severity"]:
        for severity, value in sorted(counts["by_severity"].items()):
            f.write(f"- {severity}: {value}\n")
    else:
        f.write("- None\n")

    f.write("\n## Open findings by due state\n\n")
    for key in ["overdue", "due-today", "on-track", "unset"]:
        f.write(f"- {key}: {counts['by_due_state'][key]}\n")

    f.write("\n## Remediation actions\n\n")
    if open_items:
        f.write("| Finding | Round | Area | Severity | Status | Owner | Target date | Due state | Action summary | Remediation notes |\n")
        f.write("|---|---|---|---|---|---|---|---|---|---|\n")
        for item in open_items:
            f.write(
                "| "
                f"{item['id'] or '<no-id>'} | "
                f"{item['assessment_round'] or '<unset>'} | "
                f"{item['area'] or '<unset>'} | "
                f"{item['severity'] or '<unset>'} | "
                f"{item['status'] or '<unset>'} | "
                f"{item['owner'] or '<unassigned>'} | "
                f"{item['target_date'] or '<unset>'} | "
                f"{item['due_state']} | "
                f"{(item['summary'] or '<none>').replace('|', '/')} | "
                f"{(item['remediation_notes'] or '<none>').replace('|', '/')} |\n"
            )
    else:
        f.write("No open findings are currently tracked.\n")

print(json.dumps(summary))
PY