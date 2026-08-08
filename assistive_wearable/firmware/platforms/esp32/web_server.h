#ifndef WEB_SERVER_H
#define WEB_SERVER_H
#include <stdbool.h>
void web_server_init(void);
bool web_server_is_connected(void);
const char *web_server_ip(void);
#endif
