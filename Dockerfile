# syntax = docker/dockerfile:1.2
FROM --platform=amd64 wildwildangel/linux-musl-cross-compilers@sha256:47867c3664f53b99132899671323b4eeffab46a8c3fc1445dead0af6b20c643c AS build-base
RUN apk add --no-cache libcap file patch
COPY toolchain /toolchain

FROM build-base AS build
ARG TARGETPLATFORM
WORKDIR /src
COPY . .
RUN \
    $(setvars ${TARGETPLATFORM}) && \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_STATIC_BINARY=ON && \
    cmake --build build --config MinSizeRel -j 2 && \
    strip build/tiny-ssh-honeypot && \
    setcap cap_net_bind_service=ep build/tiny-ssh-honeypot

FROM scratch
COPY --from=build /src/build/tiny-ssh-honeypot /tiny-ssh-honeypot
ENTRYPOINT ["/tiny-ssh-honeypot"]
