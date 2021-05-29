# syntax = docker/dockerfile:1.2
FROM alpine:3.13 AS build
RUN apk add --no-cache cmake make musl-dev gcc file ccache
RUN mkdir -p /build/build /build/.ccache
COPY . /build/
ENV CCACHE_BASEDIR=/build/.ccache
WORKDIR /build/build
RUN --mount=type=cache,target=/build/.ccache \
    cmake \
        -DBUILD_STATIC_BINARY=ON \
        -DCMAKE_AR=/usr/bin/gcc-ar \
        -DCMAKE_RANLIB=/usr/bin/gcc-ranlib \
        ..
RUN make
RUN strip tiny-ssh-honeypot

FROM scratch
COPY --from=build /build/build/tiny-ssh-honeypot /tiny-ssh-honeypot
ENTRYPOINT ["/tiny-ssh-honeypot"]
