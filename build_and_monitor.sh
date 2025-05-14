#!/bin/bash

#디렉ㅌ리및 타임스탬프 설정
LOG_DIR="./log"
TIMESTAMP=$(date +%y%m%d%H%M)
LOGFILE="$LOG_DIR/monitor_log_${TIMESTAMP}.txt"

# 1. 백그라운드로 BuildKit 데몬 실행
echo "[*] BuildKit 데몬 실행"
sudo buildkitd > buildkitd.log 2>&1 &
BUILDKITD_PID=$!
sleep 2  # 안정화 대기

# 2. open_monitor_user 실행 → 로그 7초간 수집
echo "[*] eBPF 감시 시작 (7초간)"
timeout 7s sudo ./open_monitor_user > "$LOGFILE" 2>&1 &

# 3. 도커 빌드 수행
echo "[*] Docker 빌드 시작"
sudo buildctl build \
  --frontend=dockerfile.v0 \
  --local context=./context \
  --local dockerfile=. \
  --opt filename=Dockerfile

# 4. BuildKit 데몬 종료
sleep 1
echo "[*] BuildKit 데몬 종료"
kill $BUILDKITD_PID
