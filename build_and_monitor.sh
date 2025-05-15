#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

LOG_DIR="./log"
TIMESTAMP=$(date +%y%m%d%H%M)
LOGFILE="$LOG_DIR/monitor_log_${TIMESTAMP}.txt"
MONITOR_TIMEOUT=5   # seconds

mkdir -p "$LOG_DIR"

cleanup() {
  echo "[*] 스크립트 종료: 모니터 프로세스($MONITOR_PID) 정리"
  sudo kill "$MONITOR_PID" >/dev/null 2>&1 || true
}
trap cleanup EXIT

# 1) eBPF 모니터 직접 백그라운드 실행
echo "[*] eBPF 감시 시작 (${MONITOR_TIMEOUT}s) → $LOGFILE"
sudo ./open_monitor_user >"$LOGFILE" 2>&1 &
MONITOR_PID=$!

# 1-1) MONITOR_TIMEOUT 지나면 강제 종료 스케줄
(
  sleep "$MONITOR_TIMEOUT"
  echo "[*] 타임아웃(${MONITOR_TIMEOUT}s) 도달—모니터 종료"
  sudo kill "$MONITOR_PID" >/dev/null 2>&1 || true
) &

# 2) 컨테이너 실행
echo "[*] 컨테이너 실행: /host/etc/shadow 읽기"
sudo docker run --rm \
  --privileged \
  --security-opt seccomp=unconfined \
  -v /:/host:rw \
  alpine sh -c 'cat /host/etc/shadow' || echo "[!] docker run 실패" >&2

# 3) 모니터 프로세스가 남아 있으면 대기
wait "$MONITOR_PID" 2>/dev/null || true
echo "[*] eBPF 감시 로그 수집 완료 → $LOGFILE"
