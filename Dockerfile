FROM alpine:3.18.3@sha256:7144f7bab3d4c2648d7e59409f15ec52a18006a128c733fcff20d3a4a54ba44a AS build
RUN apk add --no-cache cmake make musl-dev gcc
WORKDIR /build
COPY --from=wildwildangel/tiny-ssh-honeypot-build-dependencies@sha256:865842990a57a343055f673f87e300fdb71d3b28be1e0f3d5b04af4e481d106f /usr /usr
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
