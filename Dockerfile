FROM alpine:3.13 AS build
ARG BUILD_PARALLELISM=4
RUN apk add --no-cache cmake make musl-dev gcc file
WORKDIR /build
COPY . .
RUN \
    cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
        -DBUILD_STATIC_BINARY=ON \
        -DCMAKE_AR=/usr/bin/gcc-ar \
        -DCMAKE_RANLIB=/usr/bin/gcc-ranlib \
    && \
    cmake --build build --config MinSizeRel -j ${BUILD_PARALLELISM} && \
    strip build/tiny-ssh-honeypot

# COPY does not preserve extended attributes, therefore we cannot set the proper capabilities
# in the build image
FROM scratch
COPY --from=wildwildangel/setcap-static /setcap-static /!setcap-static
COPY --from=build /build/build/tiny-ssh-honeypot /tiny-ssh-honeypot
RUN ["/!setcap-static", "cap_net_bind_service=+ep", "/tiny-ssh-honeypot"]
ENTRYPOINT ["/tiny-ssh-honeypot"]
