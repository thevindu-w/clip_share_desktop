ARG DISTRO
ARG VERSION="latest"

FROM ubuntu:${VERSION} AS ubuntu_builder

# Install dependencies
RUN apt-get update && \
    apt-get install --no-install-recommends -y coreutils gcc make libc6-dev libx11-dev libxmu-dev libxfixes-dev libunistring-dev libmicrohttpd-dev libssl-dev xxd && \
    apt-get clean -y

FROM fedora:42 AS fedora_builder

# Install dependencies
RUN dnf install --setopt=install_weak_deps=False -y coreutils gcc make glibc-devel libX11-devel libXmu-devel libXfixes-devel libunistring-devel libmicrohttpd-devel openssl-devel xxd

FROM archlinux:base AS arch_builder

# Install dependencies
RUN pacman -Sy && \
    pacman -S --needed --noconfirm coreutils gcc make glibc libx11 libxmu libxfixes libunistring libmicrohttpd openssl tinyxxd

FROM debian:bullseye-slim AS debian_builder

# Install dependencies
RUN apt-get update && apt-get install --no-install-recommends -y coreutils gcc make libc6-dev libx11-dev libxmu-dev libxfixes-dev libunistring-dev libmicrohttpd-dev libssl-dev xxd && apt-get clean -y

# hadolint ignore=DL3006
FROM ${DISTRO}_builder

RUN useradd -mU -s /bin/bash user
USER user
ENV HOME=/home/user

RUN mkdir -p "$HOME/src"
WORKDIR "$HOME/src"

# Copy the source files
COPY Makefile ./
COPY --chown=user ./src src

# Build
RUN make clean && make

COPY --chmod=755 ./docker/release/copy_release.sh ./

CMD ["./copy_release.sh"]
