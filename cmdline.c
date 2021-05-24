#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <assh/assh_context.h>
#include <assh/helper_key.h>
#include "cmdline.h"
#include "globals.h"
#include "utils.h"
#include "version.h"

static struct option long_options[] = {
    { "host-key",   required_argument, NULL, 'k' },
    { "address",    required_argument, NULL, 'b' },
    { "port",       required_argument, NULL, 'p' },
    { "help",       no_argument,       NULL, 'h' },
    { "version",    no_argument,       NULL, 'v' },
    { NULL,         0,                 NULL, 0   }
};

#if defined(__clang__) || defined(__GNUC__)
__attribute__((noreturn))
#endif
static int usage(void)
{
    puts(
        "Usage: tiny-ssh-honeypot [options]...\n\n"
        "Low-interaction SSH honeypot\n\n"
        "Mandatory arguments to long options are mandatory for short options too.\n"
        "  -k, --host-key FILE   the file containing the private host key (RSA, DSA, ECDSA, ED25519)\n"
        "                        Will be automatically generated if not provided\n"
        "  -b, --address ADDRESS the IP address to bind to (default: 0.0.0.0)\n"
        "  -p, --port PORT       the port to bind to (default: 22)\n"
        "  -h, --help            display this help and exit\n"
        "  -v, --version         output version information and exit\n\n"
        "Please report bugs here: <https://github.com/sjinks/ssh-honeypotd/issues>\n"
    );
    exit(EXIT_SUCCESS);
}

#if defined(__clang__) || defined(__GNUC__)
__attribute__((noreturn))
#endif
static int version(void)
{
    puts(
        "tiny-ssh-honeypot " PROGRAM_VERSION "\n"
        "Copyright (c) 2021, Volodymyr Kolesnykov <volodymyr@wildwolf.name>\n"
        "License: MIT <http://opensource.org/licenses/MIT>\n"
    );
    exit(EXIT_SUCCESS);
}

void parse_command_line(int argc, char** argv, struct globals_t* g)
{
    while (1) {
        int option_index = 0;
        int c = getopt_long(
            argc,
            argv,
            "k:b:p:hv",
            long_options,
            &option_index
        );

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'k': {
                assh_status_t st = asshh_key_load_filename(g->context, assh_context_keys(g->context), NULL, ASSH_ALGO_SIGN, optarg, ASSH_KEY_FMT_NONE, NULL, 0);
                if (st != 0) {
                    fprintf(stderr, "WARNING: failed to load key '%s'\n", optarg);
                }

                break;
            }

            case 'b':
                ++g->sockets_count;
                if (g->sockets_count > g->sockets_allocated) {
                    g->sockets_allocated += 16;
                    if (g->sockets_allocated > SIZE_MAX / sizeof(char*)) {
                        fprintf(stderr, "FATAL ERROR: unsigned overflow\n");
                        exit(EXIT_FAILURE);
                    }

                    char** tmp = realloc(g->bind_addresses, g->sockets_allocated * sizeof(char*));
                    check_alloc(tmp, "realloc");
                    g->bind_addresses = tmp;
                }

                g->bind_addresses[g->sockets_count - 1] = my_strdup(optarg);
                break;

            case 'p':
                free(g->bind_port);
                g->bind_port = my_strdup(optarg);
                break;

            case 'h':
                usage();

            case 'v':
                version();

            case '?':
            default:
                break;
        }
    }

    while (optind < argc) {
        fprintf(stderr, "WARNING: unrecognized option: %s\n", argv[optind]);
        ++optind;
    }

    struct assh_key_s** keys = assh_context_keys(g->context);
    if (*keys == NULL) {
        assh_status_t s1 = asshh_key_create(g->context, keys, 0, "ssh-ed25519", ASSH_ALGO_SIGN);
        assh_status_t s2 = asshh_key_create(g->context, keys, 0, "ssh-rsa", ASSH_ALGO_SIGN);
        if (s1 && s2) {
            fprintf(stderr, "FATAL ERROR: unable to generate server keys\n");
            exit(EXIT_FAILURE);
        }
    }
}
