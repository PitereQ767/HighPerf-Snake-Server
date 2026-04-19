FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
g++ \
cmake \
make \
iproute2 \
&& rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p build && cd build && cmake -DBUILD_CLIENT=OFF .. && make

EXPOSE 8080

CMD ["./build/server/snake-server"]
