#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "socketutils.h"
#include "globals.h"
#include "utils.h"

void create_sockets(struct globals_t* g)
{
    size_t good = 0;
    const int on = 1;

    union {
        struct sockaddr sa;
        struct sockaddr_in sa_in;
        struct sockaddr_in6 sa_in6;
    } sin;

    int res;

    g->sockets = calloc(g->sockets_count, sizeof(int));
    check_alloc(g->sockets, "calloc");

    char* endptr = NULL;
    errno = 0;
    long int p = strtol(g->bind_port, &endptr, 10);
    if (errno != 0 || endptr == g->bind_port || *endptr != '\0' || p <= 0 || p > 65535) {
        fprintf(stderr, "FATAL ERROR: invalid port: %s\n", g->bind_port);
        exit(EXIT_FAILURE);
    }

    uint16_t port = (uint16_t)p;

    for (size_t i = 0; i < g->sockets_count; ++i) {
        memset(&sin, 0, sizeof(sin));
        if (inet_pton(AF_INET, g->bind_addresses[i], &sin.sa_in.sin_addr) == 1) {
            sin.sa_in.sin_family = AF_INET;
            sin.sa_in.sin_port = htons(port);
        }
        else if (inet_pton(AF_INET6, g->bind_addresses[i], &sin.sa_in6.sin6_addr) == 1) {
            sin.sa_in6.sin6_family = AF_INET6;
            sin.sa_in6.sin6_port = htons(port);
        }
        else {
            g->sockets[i] = -1;
            fprintf(stderr, "ERROR: '%s' is not a valid address\n", g->bind_addresses[i]);
            free(g->bind_addresses[i]);
            g->bind_addresses[i] = NULL;
            continue;
        }

#if defined(SOCK_NONBLOCK)
        g->sockets[i] = socket(sin.sa.sa_family, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
#else
        g->sockets[i] = socket(sin.sa.sa_family, SOCK_STREAM, IPPROTO_TCP);
#endif
        if (g->sockets[i] == -1) {
            fprintf(stderr, "ERROR: failed to create socket: %s\n", strerror(errno));
            free(g->bind_addresses[i]);
            g->bind_addresses[i] = NULL;
            continue;
        }

#if !defined(SOCK_NONBLOCK)
        fcntl(g->sockets[i], F_SETFL, fcntl(g->sockets[i], F_GETFL, 0) | O_NONBLOCK);
#endif

        if (setsockopt(g->sockets[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
            fprintf(stderr, "WARNING: setsockopt(SO_REUSEADDR) failed: %s\n", strerror(errno));
        }

#if defined(SOL_IP) && defined(IP_FREEBIND)
        if (setsockopt(g->sockets[i], SOL_IP, IP_FREEBIND, &on, sizeof(int)) == -1) {
            fprintf(stderr, "WARNING: setsockopt(IP_FREEBIND) failed: %s\n", strerror(errno));
        }
#endif

        if (sin.sa.sa_family == AF_INET6 && setsockopt(g->sockets[i], IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(int)) == -1) {
            fprintf(stderr, "WARNING: setsockopt(IPV6_V6ONLY) failed: %s\n", strerror(errno));
        }

        res = bind(g->sockets[i], &sin.sa, sizeof(sin));
        if (-1 == res) {
            int saved_errno = errno;
            fprintf(stderr, "ERROR: failed to bind() to %s:%u: %s\n", g->bind_addresses[i], (unsigned int)port, strerror(saved_errno));
            if (saved_errno == EACCES && port < 1024) {
                fputs("Hint: try running as root to bind to ports below 1024 or give the CAP_NET_BIND_SERVICE capability to the executable\n", stderr);
            }
            close(g->sockets[i]);
            g->sockets[i] = -1;
            free(g->bind_addresses[i]);
            g->bind_addresses[i] = NULL;
            continue;
        }

        res = listen(g->sockets[i], 1024);
        if (-1 == res) {
            fprintf(stderr, "ERROR: failed to listen(%s:%u): %s\n", g->bind_addresses[i], (unsigned int)port, strerror(errno));
            close(g->sockets[i]);
            g->sockets[i] = -1;
            free(g->bind_addresses[i]);
            g->bind_addresses[i] = NULL;
            continue;
        }

        free(g->bind_addresses[i]);
        g->bind_addresses[i] = NULL;

        ++good;
    }

    if (!good) {
        fprintf(stderr, "FATAL ERROR: no listening sockets available\n");
        exit(EXIT_FAILURE);
    }

    free(g->bind_port);
    free(g->bind_addresses);
    g->bind_port = NULL;
    g->bind_addresses = NULL;
}

void get_ip_port(const struct sockaddr_storage* addr, char* ipstr, int* port)
{
    if (addr->ss_family == AF_INET) {
        const struct sockaddr_in* s = (const struct sockaddr_in*)addr;
        *port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, INET6_ADDRSTRLEN);
    }
    else if (addr->ss_family == AF_INET6) {
        const struct sockaddr_in6* s = (const struct sockaddr_in6*)addr;
        *port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, INET6_ADDRSTRLEN);
    }
    else {
        /* Should not happen */
        ipstr[0] = '?';
        ipstr[1] = 0;
        *port    = -1;
    }
}
