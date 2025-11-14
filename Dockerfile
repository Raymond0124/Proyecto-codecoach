FROM debian:stable-slim

RUN apt-get update && apt-get install -y \
    g++ cmake make git curl \
    time procps coreutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN mkdir -p build && cd build && cmake .. && cmake --build . -j

EXPOSE 8080
CMD ["/app/build/codecoach_motor"]
