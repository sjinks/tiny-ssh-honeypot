#ifndef D7568C94_0B23_4E27_85F9_084BF9D8B2EC
#define D7568C94_0B23_4E27_85F9_084BF9D8B2EC

struct globals_t;

void create_sockets(struct globals_t* g);
void get_ip_port(const struct sockaddr_storage* addr, char* ipstr, int* port);

#endif /* D7568C94_0B23_4E27_85F9_084BF9D8B2EC */
