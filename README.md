# tiny-ssh-honeypot

[![Build](https://github.com/sjinks/tiny-ssh-honeypot/actions/workflows/build.yml/badge.svg)](https://github.com/sjinks/tiny-ssh-honeypot/actions/workflows/build.yml)
[![Docker CI/CD](https://github.com/sjinks/tiny-ssh-honeypot/actions/workflows/docker.yml/badge.svg)](https://github.com/sjinks/tiny-ssh-honeypot/actions/workflows/docker.yml)

`tiny-ssh-honeypot` is a lightweight low-interaction SSH honeypot. It is a spin-off of [`ssh-honeypotd`](https://github.com/sjinks/ssh-honeypotd) with fewer features but lower resource consumption.

Unlike `ssh-honeypotd`, `tiny-ssh-honeypot` is a single-threaded event-driven application. Its architecture allows for handling much more connections while consuming much less memory. `tiny-ssh-honeypot` uses [libassh 1.0](http://www.nongnu.org/libassh/) and [libev 4.x](http://software.schmorp.de/pkg/libev.html) under the hood.

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
cmake -S . -B build
cmake --build build
```

## Build Docker Image

If you have [Buildx](https://docs.docker.com/buildx/working-with-buildx/) installed:

```bash
docker buildx build --pull --tag wildwildangel/tiny-ssh-honeypot --load -f Dockerfile.buildx .
```

If you have [BuildKit](https://docs.docker.com/develop/develop-images/build_enhancements/) enabled:

```bash
DOCKER_BUILDKIT=1 docker build --pull --tag wildwildangel/tiny-ssh-honeypot --load -f Dockerfile.buildx .
```

Otherwise,

```bash
docker build --pull -t wildwildangel/tiny-ssh-honeypot -f Dockerfile .
```

The difference is that without BuildKit or Buildx, Docker does not copy the [extended file attributes](https://en.wikipedia.org/wiki/Extended_file_attributes#Linux) between images; therefore, several extra steps are necessary to build the final image.

## Usage

tiny-ssh-honeypot [options]

Mandatory arguments to long options are mandatory for short options too.

  * `-k`, `--host-key FILE`: the file containing the private host key (RSA, DSA, ECDSA, ED25519). As a fallback mechanism, the program will try to generate the RSA and ED25519 keys automatically.
  * `-b`, `--address ADDRESS`: the IP address to bind to (default: 0.0.0.0). You can specify this option multiple times.
  * `-p`, `--port PORT`: the port to bind to (default: 22).
  * `-h`, `--help`: display the help screen and exit.
  * `-v`, `--version`: output version information and exit.

## Log Format

Unlike `ssh-honeypotd`, `tiny-ssh-honeypot` does not try to mimic the output of `sshd` but logs its messages differently. The reason behind this decision is that some developers find it hard to [write a robust regular expression for a failed SSH login](https://wildwolf.name/configservers-login-failure-daemon-is-vulnerable-to-denial-of-service-attacks/), and thus leave their systems open to a possible Denial of Service attack.

All actionable messages generated by `tiny-ssh-honeypot` have the following format:

```
[source_ip:source_port => target_ip:target_port]: message
```

`tiny-ssh-honeypot` generates the following messages:

  * **incoming connection**: there is a new incoming connection;
  * **closing connection**: the client has disconnected. If there are no messages about failed login attempts between these two messages, this could mean that somebody scans the ports of your server (reads service identification strings);
  * **did not receive identification string**: the client has failed to identify itself and disconnected. This message can be an indicator of a scanner;
  * **login attempt for user: [user] (password: [password])**: failed login attempt;
  * **connection closed by authenticating user**: the client has identified itself and passed the KEX (key exchange) stage but disconnected before the user has identified themselves;
  * **SSH error: [error]**: protocol error occurred. For example, errors can happen when the client unexpectedly disconnects (*IO error*);
  * **input overflow**: this error should not happen: this means that the length of the username or the password is greater than the value of `INT_MAX` constant.

## Usage with Docker

```bash
docker run -d \
    --network=host \
    --cap-drop=all \
    --cap-add=NET_BIND_SERVICE \
    --restart=always \
    --read-only \
    --user=10001:10001 \
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
              cpu: 100m
              memory: 8Mi
            requests:
              cpu: 50m
              memory: 8Mi
          securityContext:
            capabilities:
              drop:
                - all
              add:
                - NET_BIND_SERVICE
            readOnlyRootFilesystem: true
            runAsNonRoot: true
            runAsUser: 10001
            runAsGroup: 10001
            allowPrivilegeEscalation: false
          ports:
            - containerPort: 22
              hostPort: 22
              protocol: TCP
```

```bash
kubectl apply -f tiny-ssh-honeypot.yaml
```
