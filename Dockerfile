FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libasio-dev \
    libssl-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Crow 1.3.4 from source
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

RUN cmake -S . -B build \
    && cmake --build build --target vectordb_server

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libasio-dev \
    libssl3 \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/vectordb_server /app/vectordb_server

EXPOSE 8080

CMD ["./vectordb_server"]
