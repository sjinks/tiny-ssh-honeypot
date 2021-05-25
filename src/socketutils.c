#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "socketutils.h"
#include "globals.h"
#include "utils.h"

void create_sockets(struct globals_t* g)
{
    size_t good = 0;
    const int on = 1;
    struct sockaddr_in sin;
    int res;

    g->sockets = calloc(g->sockets_count, sizeof(int));
    check_alloc(g->sockets, "calloc");

    uint16_t port = (uint16_t)atoi(g->bind_port);
    for (size_t i = 0; i < g->sockets_count; ++i) {
        memset(&sin, 0, sizeof(sin));
        sin.sin_port = htons(port);
        if (inet_pton(AF_INET, g->bind_addresses[i], &sin.sin_addr) == 1) {
            sin.sin_family = AF_INET;
        }
        else if (inet_pton(AF_INET6, g->bind_addresses[i], &sin.sin_addr) == 1) {
            sin.sin_family = AF_INET6;
        }
        else {
            g->sockets[i] = -1;
            fprintf(stderr, "ERROR: '%s' is not a valid address\n", g->bind_addresses[i]);
            free(g->bind_addresses[i]);
            g->bind_addresses[i] = NULL;
            continue;
        }

        g->sockets[i] = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (g->sockets[i] == -1) {
            fprintf(stderr, "ERROR: failed to create socket: %s\n", strerror(errno));
            free(g->bind_addresses[i]);
            g->bind_addresses[i] = NULL;
            continue;
        }

        if (setsockopt(g->sockets[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
            fprintf(stderr, "WARNING: setsockopt(SO_REUSEADDR) failed: %s\n", strerror(errno));
        }

        if (setsockopt(g->sockets[i], SOL_IP, IP_FREEBIND, &on, sizeof(int)) == -1) {
            fprintf(stderr, "WARNING: setsockopt(IP_FREEBIND) failed: %s\n", strerror(errno));
        }

        res = bind(g->sockets[i], (struct sockaddr*)&sin, sizeof(sin));
        if (-1 == res) {
            fprintf(stderr, "ERROR: failed to bind() to %s:%u: %s\n", g->bind_addresses[i], (unsigned int)port, strerror(errno));
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
