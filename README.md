# tiny-ssh-honeypot

tiny-ssh-honeypot is a lightweight low-interaction SSH honeypot. It is a spin-off of [ssh-honeypotd](https://github.com/sjinks/ssh-honeypotd) with fewer features but lower resource consumption.

Unlike ssh-honeypotd, tiny-ssh-honeypot is a single-threaded event-driven application. Its architecture allows for handling much more connections while consuming much less memory. tiny-ssh-honeypot uses [libassh 1.0](http://www.nongnu.org/libassh/) and [libev 4.x](http://software.schmorp.de/pkg/libev.html) under the hood.

## Compilation

tiny-ssh-honeypot uses [CMake](https://cmake.org/documentation/) as its build system.

The project has two optional dependencies:

  1. libassh 1.0
  2. libev 4.x

If those are not present, the build script will try to download and build them.

The download locations are:
  * libassh: http://download.savannah.nongnu.org/releases/libassh/libassh-1.0.tar.gz
  * libev: http://dist.schmorp.de/libev/Attic/libev-4.33.tar.gz

**NOTE:** the build script builds static versions of the downloaded libraries.

The build script defines the following command-line options:

  * `BUILD_STATIC_BINARY` (`OFF` by default): whether to build a static binary;
  * `FORCE_EXTERNAL_LIBEV` (`OFF` by default): whether to ignore the system `libev`;
  * `FORCE_EXTERNAL_LIBASSH` (`OFF` by default): whether to ignore the system `libassh`.

To build the program, you will need `cmake`, a C compiler (say, `clang` or `gcc`), and a make tool (`make`). The program builds on Linux and may (or may not) build on other systems (like Windows).

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

tiny-ssh-honeypot [options]

Mandatory arguments to long options are mandatory for short options too.

  * `-k`, `--host-key FILE`: the file containing the private host key (RSA, DSA, ECDSA, ED25519). As a fallback mechanism, the program will try to generate the RSA and ED25519 keys automatically.
  * `-b`, `--address ADDRESS`: the IP address to bind to (default: 0.0.0.0). You can specify this option multiple times.
  * `-p`, `--port PORT`: the port to bind to (default: 22).
  * `-h`, `--help`: display the help screen and exit.
  * `-v`, `--version`: output version information and exit.

## Usage with Docker

```bash
docker run -d \
    --network=host \
    --cap-add=NET_ADMIN \
    --restart=always \
    --read-only \
    wildwildangel/tiny-ssh-honeypot:latest
```

These variables make it easy to have several honeypots running on the same machine, should the need arise.

## Usage with Kubernetes

`tiny-ssh-honeypot.yaml`:
```yaml
---
apiVersion: v1
kind: Namespace
metadata:
  name: honeypots
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: tiny-ssh-honeypot
  namespace: honeypots
spec:
  selector:
    matchLabels:
      name: tiny-ssh-honeypot
  template:
    metadata:
      labels:
        name: tiny-ssh-honeypot
    spec:
      hostNetwork: true
      containers:
        - name: tiny-ssh-honeypot
          image: wildwildangel/tiny-ssh-honeypot
          resources:
            limits:
              memory: 8Mi
            requests:
              cpu: 100m
              memory: 8Mi
          securityContext:
            capabilities:
              add:
                - NET_ADMIN
            readOnlyRootFilesystem: true
          ports:
            - containerPort: 22
              hostPort: 22
              protocol: TCP
```

```bash
kubectl apply -f tiny-ssh-honeypot.yaml
```
