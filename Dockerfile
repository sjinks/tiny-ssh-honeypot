FROM --platform=${BUILDPLATFORM} tonistiigi/xx:latest@sha256:c64defb9ed5a91eacb37f96ccc3d4cd72521c4bd18d5442905b95e2226b0e707 AS xx

FROM --platform=${BUILDPLATFORM} alpine:3.23.2@sha256:865b95f46d98cf867a156fe4a135ad3fe50d2056aa3f25ed31662dff6da4eb62 AS build
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
