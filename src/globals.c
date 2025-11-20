#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/syslog.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#ifndef __APPLE__
#   include <sys/random.h>
#endif
#include <ev.h>
#include <assh/assh_algo.h>
#include <assh/assh_buffer.h>
#include <assh/assh_context.h>
#include <assh/assh_service.h>
#include "globals.h"
#include "log.h"
#include "utils.h"

void init_globals(struct globals_t* g)
{
    openlog("tiny-ssh-honeypot", LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_DAEMON);
    memset(g, 0, sizeof(*g));
    tzset();

    if (assh_deps_init() != ASSH_OK) {
        my_log("FATAL ERROR: Failed to initialize ASSH");
        exit(EXIT_FAILURE);
    }

    g->loop = ev_default_loop(0);
    if (!g->loop) {
        my_log("FATAL ERROR: Failed to initialize the event loop");
        exit(EXIT_FAILURE);
    }

    g->bind_port         = my_strdup("22");
    g->sockets_allocated = 1;
    g->sockets_count     = 0;
    g->bind_addresses    = calloc(g->sockets_allocated, sizeof(char*));
    check_alloc(g->bind_addresses, "calloc");
    g->sockets           = NULL;

    signal(SIGPIPE, SIG_IGN);

    struct assh_buffer_s buf;
    buf.data = g->seed;
    buf.len  = sizeof(g->seed);
#ifdef __APPLE__
    arc4random_buf(g->seed, sizeof(g->seed));
#else
    /* Reads of up to 256 bytes will always return as many bytes as requested and will not be interrupted by signals */
    if (getrandom(g->seed, sizeof(g->seed), 0) != sizeof(g->seed)) {
        my_log("WARNING: unable to seed the CSPRNG");
    }
#endif

    if (
        assh_context_create(&g->context, ASSH_SERVER, NULL, NULL, NULL, &buf) ||
        assh_service_register_default(g->context) ||
        assh_algo_register_default(g->context, ASSH_SAFETY_WEAK)
    ) {
        my_log("FATAL ERROR: unable to create the server context");
        exit(EXIT_FAILURE);
    }
}

void free_globals(struct globals_t* g)
{
    if (g->loop) {
        ev_loop_destroy(g->loop);
    }

    if (g->sockets) {
        for (size_t i = 0; i < g->sockets_count; ++i) {
            if (g->sockets[i] >= 0) {
                shutdown(g->sockets[i], SHUT_RDWR);
                close(g->sockets[i]);
            }
        }

        free(g->sockets);
    }

    free(g->bind_port);

    if (g->bind_addresses) {
        for (size_t i=0; i < g->sockets_count; ++i) {
            free(g->bind_addresses[i]);
        }

        free(g->bind_addresses);
    }

    // Release the context only if there are no more sessions; `assh_context_release()` is unconditional.
    if (assh_context_refcount(g->context) == 0) {
        assh_context_release(g->context);
    }

    closelog();
}
