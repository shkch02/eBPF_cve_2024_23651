빌드킷 실행
$sudo buildkitd 

다른테미널에서 빌드
$buildctl build \
  --frontend=dockerfile.v0 \
  --local context=./context \
  --local dockerfile=. \
  --opt filename=Dockerfile

eBPF

eBPF 컴파일
$make

필터실행
$./open_monitor_user
