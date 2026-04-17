#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
SOAK_SUMMARY_FILE="${SOAK_SUMMARY_FILE:-${ARTIFACT_DIR}/soak_summary.json}"
RELIABILITY_SUMMARY_FILE="${RELIABILITY_SUMMARY_FILE:-${ARTIFACT_DIR}/reliability_until_fail_summary.json}"
BENCHMARK_SUMMARY_FILE="${BENCHMARK_SUMMARY_FILE:-${ARTIFACT_DIR}/benchmark_summary.json}"
LATENCY_MATRIX_FILE="${LATENCY_MATRIX_FILE:-${ARTIFACT_DIR}/latency_scenario_matrix.json}"
RTO_SUMMARY_FILE="${RTO_SUMMARY_FILE:-${ARTIFACT_DIR}/recovery_rto_summary.json}"
VULNERABILITY_SUMMARY_FILE="${VULNERABILITY_SUMMARY_FILE:-${ARTIFACT_DIR}/vulnerability_sla_summary.json}"
RELEASE_CHECKLIST_FILE="${RELEASE_CHECKLIST_FILE:-docs/releases/M3-RC1_EXECUTION_CHECKLIST.md}"
LONG_SOAK_EVIDENCE_FILE="${LONG_SOAK_EVIDENCE_FILE:-docs/releases/GRADE1_LONG_SOAK_EVIDENCE.json}"
PILOT_EVIDENCE_FILE="${PILOT_EVIDENCE_FILE:-docs/releases/GRADE1_PRODUCTION_PILOT_EVIDENCE.md}"
GRADE1_FAIL_ON_BLOCKED="${GRADE1_FAIL_ON_BLOCKED:-0}"

mkdir -p "${ARTIFACT_DIR}"

python3 - \
  "${SOAK_SUMMARY_FILE}" \
  "${RELIABILITY_SUMMARY_FILE}" \
  "${BENCHMARK_SUMMARY_FILE}" \
  "${LATENCY_MATRIX_FILE}" \
  "${RTO_SUMMARY_FILE}" \
  "${VULNERABILITY_SUMMARY_FILE}" \
  "${RELEASE_CHECKLIST_FILE}" \
    "${LONG_SOAK_EVIDENCE_FILE}" \
  "${PILOT_EVIDENCE_FILE}" \
  "${ARTIFACT_DIR}" \
  "${GRADE1_FAIL_ON_BLOCKED}" <<'PY'
import json
import os
import re
import sys
from dataclasses import asdict, dataclass


(
    soak_file,
    reliability_file,
    benchmark_file,
    latency_matrix_file,
    rto_file,
    vuln_file,
    release_checklist_file,
    long_soak_evidence_file,
    pilot_evidence_file,
    artifact_dir,
    fail_on_blocked,
) = sys.argv[1:12]

fail_on_blocked = fail_on_blocked == "1"


def load_json(path):
    if not os.path.exists(path):
        return None
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def count_checked_markers(section_text):
    checked = len(re.findall(r"- \[x\]", section_text))
    unchecked = len(re.findall(r"- \[ \]", section_text))
    return checked, unchecked


@dataclass
class GateStatus:
    gate_id: str
    title: str
    status: str
    details: str


soak = load_json(soak_file)
reliability = load_json(reliability_file)
benchmark = load_json(benchmark_file)
latency_matrix = load_json(latency_matrix_file)
rto = load_json(rto_file)
vuln = load_json(vuln_file)
long_soak_evidence = load_json(long_soak_evidence_file)

release_text = ""
if os.path.exists(release_checklist_file):
    with open(release_checklist_file, "r", encoding="utf-8") as f:
        release_text = f.read()

pilot_text = ""
if os.path.exists(pilot_evidence_file):
    with open(pilot_evidence_file, "r", encoding="utf-8") as f:
        pilot_text = f.read()

gates = []

# Grade 1 reliability target: 30-day soak
required_soak_minutes = 30 * 24 * 60
soak_duration = None
if soak and isinstance(soak.get("duration_minutes"), (int, float)):
    soak_duration = float(soak.get("duration_minutes", 0))

long_soak_duration = None
long_soak_status = "missing"
if long_soak_evidence and isinstance(long_soak_evidence.get("duration_minutes"), (int, float)):
    long_soak_duration = float(long_soak_evidence.get("duration_minutes", 0))
    long_soak_status = str(long_soak_evidence.get("status", "")).strip().lower() or "missing"

soak_pass = soak_duration is not None and soak_duration >= required_soak_minutes
long_soak_pass = (
    long_soak_duration is not None
    and long_soak_duration >= required_soak_minutes
    and long_soak_status in {"completed", "verified"}
)

if soak_pass:
    soak_details = f"source=soak_summary duration_minutes={soak_duration} required>=43200"
elif long_soak_pass:
    soak_details = (
        "source=long_soak_evidence "
        f"duration_minutes={long_soak_duration} status={long_soak_status} required>=43200"
    )
else:
    soak_details = (
        f"soak_duration_minutes={soak_duration} "
        f"long_soak_duration_minutes={long_soak_duration} "
        f"long_soak_status={long_soak_status} required>=43200"
    )

gates.append(
    GateStatus(
        gate_id="G1-REL-001",
        title="30-day soak evidence",
        status="passed" if (soak_pass or long_soak_pass) else "blocked",
        details=soak_details,
    )
)

# Grade 1 reliability target: restart RTO < 5s
if rto and isinstance(rto.get("restart_cycle_max_ms"), (int, float)):
    max_ms = float(rto.get("restart_cycle_max_ms", 0))
    rto_pass = max_ms <= 5000.0 and rto.get("status") == "passed"
    gates.append(
        GateStatus(
            gate_id="G1-REL-002",
            title="Restart recovery under 5 seconds",
            status="passed" if rto_pass else "blocked",
            details=f"restart_cycle_max_ms={max_ms}",
        )
    )
else:
    gates.append(
        GateStatus(
            gate_id="G1-REL-002",
            title="Restart recovery under 5 seconds",
            status="blocked",
            details="recovery RTO summary missing or invalid",
        )
    )

# Grade 1 exit gate: public benchmark report with p50/p95/p99/throughput
bench_ok = False
bench_detail = "benchmark summary missing"
if benchmark:
    required_keys = ["throughput_ops", "p50_latency_ms", "p95_latency_ms", "p99_latency_ms"]
    missing = [k for k in required_keys if benchmark.get(k) is None]
    scenarios = 0
    if latency_matrix:
        scenarios = len(latency_matrix.get("scenarios", []))
    bench_ok = not missing and scenarios >= 2
    bench_detail = f"missing_metrics={missing} scenarios={scenarios}"

gates.append(
    GateStatus(
        gate_id="G1-EXIT-001",
        title="Benchmark curves and throughput evidence",
        status="passed" if bench_ok else "blocked",
        details=bench_detail,
    )
)

# Grade 1 exit gate: signed release checklist including reliability/security evidence
approval_section = ""
if "## Approval record" in release_text:
    approval_section = release_text.split("## Approval record", 1)[1]
    if "## " in approval_section:
        approval_section = approval_section.split("## ", 1)[0]

checked, unchecked = count_checked_markers(approval_section)
release_signed = unchecked == 0 and checked >= 3
gates.append(
    GateStatus(
        gate_id="G1-EXIT-002",
        title="Signed release checklist with reliability/security evidence",
        status="passed" if release_signed else "blocked",
        details=f"approval_checked={checked} approval_unchecked={unchecked}",
    )
)

# Grade 1 exit gate: production pilot with >=2 external industrial users
pilot_users = re.findall(r"^- External user:\s*(.+)$", pilot_text, flags=re.MULTILINE)
pilot_ok = len(pilot_users) >= 2
gates.append(
    GateStatus(
        gate_id="G1-EXIT-003",
        title="Production pilot with at least two external industrial users",
        status="passed" if pilot_ok else "blocked",
        details=f"external_users={len(pilot_users)} source={os.path.basename(pilot_evidence_file)}",
    )
)

# Supplemental signal: vulnerability posture
if vuln:
    overdue = len(vuln.get("overdue_findings", []))
    gates.append(
        GateStatus(
            gate_id="G1-SEC-001",
            title="No overdue vulnerability SLA findings",
            status="passed" if overdue == 0 else "blocked",
            details=f"overdue_findings={overdue}",
        )
    )

blocked = [g for g in gates if g.status == "blocked"]
overall = "passed" if not blocked else "blocked"

summary = {
    "status": overall,
    "blocked_gate_count": len(blocked),
    "gates": [asdict(g) for g in gates],
}

json_path = os.path.join(artifact_dir, "grade1_exit_summary.json")
with open(json_path, "w", encoding="utf-8") as f:
    json.dump(summary, f)

report_path = os.path.join(artifact_dir, "grade1_exit_report.md")
with open(report_path, "w", encoding="utf-8") as f:
    f.write("# Grade 1 Exit Gate Report\n\n")
    f.write(f"- Overall status: `{overall}`\n")
    f.write(f"- Blocked gates: {len(blocked)}\n\n")
    f.write("| Gate | Title | Status | Details |\n")
    f.write("|---|---|---|---|\n")
    for g in gates:
        f.write(
            f"| {g.gate_id} | {g.title} | {g.status} | {g.details.replace('|', '/')} |\n"
        )

print(json.dumps(summary))
if overall == "blocked" and fail_on_blocked:
    sys.exit(1)
PY