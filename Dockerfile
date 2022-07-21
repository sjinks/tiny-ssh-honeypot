FROM alpine:3.16.1@sha256:7580ece7963bfa863801466c0a488f11c86f85d9988051a9f9c68cb27f6b7872 AS build
RUN apk add --no-cache cmake make musl-dev gcc
WORKDIR /build
COPY --from=wildwildangel/tiny-ssh-honeypot-build-dependencies /usr /usr
COPY . .
RUN \
    cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
        -DBUILD_STATIC_BINARY=ON \
        -DCMAKE_AR=/usr/bin/gcc-ar \
        -DCMAKE_RANLIB=/usr/bin/gcc-ranlib \
    && \
    cmake --build build --config MinSizeRel -j 2 && \
    strip build/tiny-ssh-honeypot

# COPY does not preserve extended attributes, therefore we cannot set the proper capabilities
# in the build image
FROM scratch
COPY --from=wildwildangel/setcap-static /setcap-static /!setcap-static
COPY --from=build /build/build/tiny-ssh-honeypot /tiny-ssh-honeypot
RUN ["/!setcap-static", "cap_net_bind_service=ep", "/tiny-ssh-honeypot"]
ENTRYPOINT ["/tiny-ssh-honeypot"]
