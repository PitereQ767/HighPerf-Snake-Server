FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
g++ \
cmake \
make \
iproute2 \
libpqxx-dev \
&& rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p build && cd build && cmake -DBUILD_CLIENT=OFF .. && make


#Drugi etap aby obraz mniej wazyl (wyrzucamy narzedzia do kompilowania i budowania kodu)
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    iproute2 \
    libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -r -s /bin/false snakeuser

WORKDIR /app
#Skopiowanie z 1 etapu tylko pliku binarnego i przypisanie go do stworzonego uzytkownika, aby zapobiec atakom z zewnatrz
COPY --from=builder --chown=snakeuser:snakeuser /app/build/server/snake-server .

USER snakeuser

EXPOSE 8080

CMD ["./snake-server"]
