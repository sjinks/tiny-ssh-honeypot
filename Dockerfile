FROM --platform=${BUILDPLATFORM} tonistiigi/xx:latest@sha256:0c6a569797744e45955f39d4f7538ac344bfb7ebf0a54006a0a4297b153ccf0f AS xx

FROM --platform=${BUILDPLATFORM} alpine:3.20.3@sha256:beefdbd8a1da6d2915566fde36db9db0b524eb737fc57cd1367effd16dc0d06d AS build
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
    if [ "${ARCHITECTURE}" = "ppc64le" ]; then export XX_CC_PREFER_LINKER=ld; fi && \
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
