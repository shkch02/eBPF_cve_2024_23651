sudo buildkitd 
으로 빌드킷 실행후 

다른테미널에서 빌드
명령어:
buildctl build \
  --frontend=dockerfile.v0 \
  --local context=./context \
  --local dockerfile=. \
  --opt filename=Dockerfile
