# GyroRank multi-stage containerization
# Build: docker build -t gyorank .
# Run:   docker run --rm gyorank

# ---- Builder stage ----
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ cmake make curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# Official Orson Peters pdqsort (optional but preferred)
RUN curl -fsSL https://raw.githubusercontent.com/orlp/pdqsort/master/pdqsort.h -o pdqsort.h || true

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --target gyro_rank_demo -j"$(nproc)"

# ---- Runtime stage ----
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/gyro_rank_demo /usr/local/bin/gyro_rank_demo

# Non-root for basic hygiene
RUN useradd -m -u 10001 gyro
USER gyro

ENTRYPOINT ["/usr/local/bin/gyro_rank_demo"]
