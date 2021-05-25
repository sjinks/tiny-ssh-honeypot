#ifndef EE4C5BE8_5E82_44ED_B79D_0E782FB8E70A
#define EE4C5BE8_5E82_44ED_B79D_0E782FB8E70A

#include <stdint.h>
#include <netinet/in.h>

struct globals_t {
    struct ev_loop* loop;
    struct assh_context_s* context;
    char** bind_addresses;
    char* bind_port;

    int* sockets;
    size_t sockets_count;
    size_t sockets_allocated;

    uint8_t seed[128];
};

void init_globals(struct globals_t* g);
void free_globals(struct globals_t* g);

#endif /* EE4C5BE8_5E82_44ED_B79D_0E782FB8E70A */
