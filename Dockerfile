FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ca-certificates \
    libasio-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 --branch v1.3.4 \
    https://github.com/CrowCpp/Crow.git /tmp/crow \
    && cmake -S /tmp/crow -B /tmp/crow/build \
        -DCROW_BUILD_EXAMPLES=OFF \
        -DCROW_BUILD_TESTS=OFF \
        -DCROW_BUILD_DOCS=OFF \
    && cmake --build /tmp/crow/build \
    && cmake --install /tmp/crow/build \
    && rm -rf /tmp/crow

WORKDIR /app

COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target vectordb_server

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    ca-certificates \
    libssl3 \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/vectordb_server /app/vectordb_server

RUN mkdir -p /app/data

COPY vectordb.db /app/data/vectordb.db

ENV VECTORDB_DATA_PATH=/app/data/vectordb.db

EXPOSE 10000

CMD ["./vectordb_server"]
