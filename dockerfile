#  Запуск:
#    docker run --rm lab3 --seq  999983 2 12345
#    docker run --rm lab3 --par  999983 2 12345
#    docker run --rm lab3 --both 9999991 2 987654
#    docker run --rm lab3 --both 9999991 2 987654 4
#    docker run --rm lab3 --benchmark
#    docker run --rm lab3 --help

FROM ubuntu:rolling AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        g++ \
        cmake \
        make \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel "$(nproc)"

FROM ubuntu:rolling AS runtime

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libstdc++6 \
        libc6 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/LAB3 /usr/local/bin/LAB3

ENTRYPOINT ["/usr/local/bin/LAB3"]
CMD ["--help"]