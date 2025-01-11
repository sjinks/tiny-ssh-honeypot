FROM --platform=${BUILDPLATFORM} tonistiigi/xx:latest@sha256:923441d7c25f1e2eb5789f82d987693c47b8ed987c4ab3b075d6ed2b5d6779a3 AS xx

FROM --platform=${BUILDPLATFORM} alpine:3.21.2@sha256:56fa17d2a7e7f168a043a2712e63aed1f8543aeafdcee47c58dcffe38ed51099 AS build
COPY --from=xx / /
ARG TARGETPLATFORM
RUN \
    apk add --no-cache clang llvm lld make cmake file pkgconf autoconf automake libcap && \
    xx-apk add --no-cache gcc musl-dev libev-dev

WORKDIR /src
COPY . .

RUN \
    set -x && \
    export ARCHITECTURE=$(xx-info alpine-arch) && \
    export XX_CC_PREFER_LINKER=ld && \
    export SYSROOT=$(xx-info sysroot) && \
    export HOSTSPEC=$(xx-info triple) && \
    xx-clang --setup-target-triple && \
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=toolchain -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_STATIC_BINARY=ON && \
    cmake --build build -j 2 && \
    ${HOSTSPEC}-strip build/tiny-ssh-honeypot && \
    setcap cap_net_bind_service=ep build/tiny-ssh-honeypot

FROM scratch
COPY --from=build /src/build/tiny-ssh-honeypot /tiny-ssh-honeypot
ENTRYPOINT ["/tiny-ssh-honeypot"]
