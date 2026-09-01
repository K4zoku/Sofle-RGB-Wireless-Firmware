FROM docker.io/zmkfirmware/zmk-dev-arm:3.5

USER root

RUN apt-get update \
    && apt-get install --no-install-recommends --yes meson ninja-build \
    && rm -rf /var/lib/apt/lists/*
