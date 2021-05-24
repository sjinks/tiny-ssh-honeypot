FROM alpine:3.13 AS build
RUN apk add --no-cache cmake make musl-dev libev-dev gcc
RUN mkdir -p /build/build
COPY . /build/
WORKDIR /build/build
RUN cmake -DBUILD_STATIC_BINARY=ON ..
RUN make
RUN strip tiny-ssh-honeypot

FROM scratch
COPY --from=build /build/build/tiny-ssh-honeypot /tiny-ssh-honeypot
