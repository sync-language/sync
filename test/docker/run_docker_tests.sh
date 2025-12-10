docker run --privileged --rm tonistiigi/binfmt --install all
docker buildx create --name multiarch --use
docker buildx inspect --bootstrap

docker buildx build --platform linux/amd64 --load --no-cache -t sync:ubuntu-x64 -f test/docker/ubuntu-x64.Dockerfile .
docker buildx build --platform linux/arm64 --load --no-cache -t sync:ubuntu-arm64 -f test/docker/ubuntu-arm64.Dockerfile .
docker buildx build --platform linux/riscv64 --load --no-cache -t sync:ubuntu-riscv64 -f test/docker/ubuntu-riscv64.Dockerfile .
docker build --no-cache -t sync:wasm -f test/docker/wasm.Dockerfile .