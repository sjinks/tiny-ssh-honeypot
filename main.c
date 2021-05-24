#include <stdlib.h>
#include "cmdline.h"
#include "globals.h"
#include "server.h"
#include "socketutils.h"

static struct globals_t g;

static void cleanup(void)
{
    free_globals(&g);
}

int main(int argc, char** argv)
{
    atexit(cleanup);
    init_globals(&g);
    parse_command_line(argc, argv, &g);
    create_sockets(&g);
    run(&g);
    return EXIT_SUCCESS;
}
