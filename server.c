#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ev.h>
#include <assh/assh_buffer.h>
#include <assh/assh_session.h>
#include <assh/assh_userauth_server.h>
#include <assh/assh_event.h>
#include <assh/helper_io.h>
#include "server.h"
#include "globals.h"
#include "log.h"
#include "socketutils.h"

enum connection_state_t {
    PRE_KEX,
    PRE_AUTH,
    TRIED_AUTH
};

struct conn_data_t {
    struct assh_session_s* session;
    ev_io socket_notifier;
    ev_timer watchdog;
    char ipstr[INET6_ADDRSTRLEN];
    char my_ipstr[INET6_ADDRSTRLEN];
    int port;
    int my_port;
    enum connection_state_t state;
};

static void signal_callback(struct ev_loop* loop, ev_signal* w, int revents)
{
    fprintf(stderr, "Got signal %d, shutting down", w->signum);
    ev_break(loop, EVBREAK_ALL);
}

static int ssh_loop(struct conn_data_t* data, int fd, int events)
{
    struct assh_session_s* session = data->session;

    time_t ts = time(NULL);
    while (1) {
        struct assh_event_s event;
        if (!assh_event_get(session, &event, ts)) {
            return 0;
        }

        switch ((int)event.id) {
            case ASSH_EVENT_READ:
                if (!(events & EV_READ)) {
                    assh_event_done(session, &event, ASSH_OK);
                    return 1;
                }

                asshh_fd_event(session, &event, fd);
                events &= ~EV_READ;
                break;

            case ASSH_EVENT_WRITE:
                if (!(events & EV_WRITE)) {
                    assh_event_done(session, &event, ASSH_OK);
                    return 1;
                }

                asshh_fd_event(session, &event, fd);
                events &= ~EV_WRITE;
                break;

            case ASSH_EVENT_SESSION_ERROR:
                my_log(
                    "[%s:%d => %s:%d]: SSH error: %s",
                    data->ipstr, data->port, data->my_ipstr, data->my_port,
                    assh_error_str(event.session.error.code)
                );

                assh_event_done(session, &event, ASSH_ERR_INPUT_OVERFLOW);
                break;

            case ASSH_EVENT_KEX_DONE:
                data->state = PRE_AUTH;
                assh_event_done(session, &event, ASSH_OK);
                break;

            case ASSH_EVENT_USERAUTH_SERVER_METHODS: {
                struct assh_event_userauth_server_methods_s* methods = &event.userauth_server.methods;
                if ((int)methods->failed == 0) {
                    methods->retries = 5;
                }

                methods->methods = ASSH_USERAUTH_METHOD_PASSWORD;
                assh_event_done(session, &event, ASSH_OK);
                break;
            }

            case ASSH_EVENT_USERAUTH_SERVER_PASSWORD: {
                struct assh_event_userauth_server_password_s* auth = &event.userauth_server.password;
                if (auth->username.size > INT_MAX || auth->password.size > INT_MAX) {
                    my_log(
                        "[%s:%d => %s:%d]: input overflow",
                        data->ipstr, data->port, data->my_ipstr, data->my_port
                    );

                    assh_event_done(session, &event, ASSH_ERR_INPUT_OVERFLOW);
                }
                else {
                    my_log(
                        "[%s:%d => %s:%d]: login attempt for user: %.*s (password: %.*s)",
                        data->ipstr,
                        data->port,
                        data->my_ipstr,
                        data->my_port,
                        (int)auth->username.size, auth->username.str,
                        (int)auth->password.size, auth->password.str
                    );

                    auth->result = ASSH_SERVER_PWSTATUS_FAILURE;
                    data->state  = TRIED_AUTH;
                    assh_event_done(session, &event, ASSH_OK);
                }

                break;
            }

            default:
                assh_event_done(session, &event, ASSH_OK);
                break;
        }
    }
}

static void run_ssh_loop(struct ev_loop* loop, struct conn_data_t* data, int revents)
{
    int fd = data->socket_notifier.fd;
    int rc = ssh_loop(data, fd, revents);

    if (rc == 1) {
        ev_io_set(&data->socket_notifier, fd, EV_READ | (assh_transport_has_output(data->session) ? EV_WRITE : 0));
        ev_io_start(loop, &data->socket_notifier);

        assh_time_t timeout = assh_session_delay(data->session, time(NULL));
        data->watchdog.repeat = timeout ? timeout : 0.005;
        ev_timer_again(loop, &data->watchdog);
    }
    else {
        ev_timer_stop(loop, &data->watchdog);
        ev_io_stop(loop, &data->socket_notifier);
        assh_session_release(data->session);
        close(fd);

        if (data->state == PRE_KEX) {
            my_log(
                "[%s:%d => %s:%d]: did not receive identification string",
                data->ipstr, data->port, data->my_ipstr, data->my_port
            );
        }
        else if (data->state == PRE_AUTH) {
            my_log(
                "[%s:%d => %s:%d]: connection closed by authenticating user",
                data->ipstr, data->port, data->my_ipstr, data->my_port
            );
        }

        my_log(
            "[%s:%d => %s:%d]: closing connection",
            data->ipstr, data->port, data->my_ipstr, data->my_port
        );

        free(data);
    }
}

static void watchdog_handler(struct ev_loop* loop, ev_timer* w, int revents)
{
    run_ssh_loop(loop, w->data, 0);
}

static void socket_handler(struct ev_loop* loop, ev_io* w, int revents)
{
    struct conn_data_t* data = w->data;
    ev_timer_stop(loop, &data->watchdog);
    run_ssh_loop(loop, data, revents);
}

static void connection_handler(struct ev_loop* loop, ev_io* w, int revents)
{
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    int fd = accept4(w->fd, (struct sockaddr*)&addr, &len, SOCK_NONBLOCK);
    if (fd < 0) {
        perror("accept4");
        return;
    }

    struct conn_data_t* data = calloc(1, sizeof(struct conn_data_t));
    if (!data) {
        perror("calloc");
        close(fd);
        return;
    }

    get_ip_port(&addr, data->ipstr, &data->port);

    len = sizeof(addr);
    if (!getsockname(fd, (struct sockaddr*)&addr, &len)) {
        get_ip_port(&addr, data->my_ipstr, &data->my_port);
    }
    else {
        data->my_ipstr[0] = '?';
        data->my_port     = -1;
    }

    my_log(
        "[%s:%d => %s:%d]: incoming connection",
        data->ipstr, data->port, data->my_ipstr, data->my_port
    );

    struct globals_t* g = ev_userdata(loop);
    assh_status_t st;
    st = assh_session_create(g->context, &data->session);
    if (st) {
        fprintf(stderr, "ERROR: assh_session_create() failed\n");
        close(fd);
        return;
    }

    data->state = PRE_KEX;

    ev_io_init(&data->socket_notifier, socket_handler, fd, EV_READ | (assh_transport_has_output(data->session) ? EV_WRITE : 0));
    data->socket_notifier.data = data;
    ev_io_start(g->loop, &data->socket_notifier);

    assh_time_t timeout = assh_session_delay(data->session, time(NULL));
    ev_init(&data->watchdog, watchdog_handler);
    data->watchdog.repeat = timeout ? timeout : 0.05;
    data->watchdog.data   = data;
    ev_timer_again(g->loop, &data->watchdog);
}

int run(struct globals_t* g)
{
    ev_signal sigterm_watcher;
    ev_signal sigint_watcher;
    ev_signal sigquit_watcher;
    ev_io* accept_watchers;

    ev_set_userdata(g->loop, g);

    ev_signal_init(&sigterm_watcher, signal_callback, SIGTERM);
    ev_signal_init(&sigint_watcher,  signal_callback, SIGINT);
    ev_signal_init(&sigquit_watcher, signal_callback, SIGQUIT);
    ev_signal_start(g->loop, &sigterm_watcher);
    ev_signal_start(g->loop, &sigint_watcher);
    ev_signal_start(g->loop, &sigquit_watcher);

    accept_watchers = calloc(g->sockets_count, sizeof(ev_io));
    if (!accept_watchers) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < g->sockets_count; ++i) {
        ev_io_init(&accept_watchers[i], connection_handler, g->sockets[i], EV_READ);
        if (g->sockets[i] != -1) {
            ev_io_start(g->loop, &accept_watchers[i]);
        }
    }

    ev_run(g->loop, 0);

    for (size_t i = 0; i < g->sockets_count; ++i) {
        if (accept_watchers[i].fd != -1) {
            ev_io_stop(g->loop, &accept_watchers[i]);
        }
    }

    free(accept_watchers);
    return EXIT_SUCCESS;
}
