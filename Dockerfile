FROM alpine:3.16.2@sha256:bc41182d7ef5ffc53a40b044e725193bc10142a1243f395ee852a8d9730fc2ad AS build
RUN apk add --no-cache cmake make musl-dev gcc
WORKDIR /build
COPY --from=wildwildangel/tiny-ssh-honeypot-build-dependencies@sha256:063d01963402ae200add46184aa0dff2e8b6baeb36e11d88324644c11e124cf0 /usr /usr
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
COPY --from=wildwildangel/setcap-static@sha256:dd8997ef3340ad43e459c210d0ebea44e26bfbf4adf34d777ea3a7a9c3cefeda /setcap-static /!setcap-static
COPY --from=build /build/build/tiny-ssh-honeypot /tiny-ssh-honeypot
RUN ["/!setcap-static", "cap_net_bind_service=ep", "/tiny-ssh-honeypot"]
ENTRYPOINT ["/tiny-ssh-honeypot"]
