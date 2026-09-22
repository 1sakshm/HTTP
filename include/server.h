#ifndef CSERVE_SERVER_H
#define CSERVE_SERVER_H

#include <signal.h>
#include <stdint.h>

typedef struct {
    const char *host;
    uint16_t port;
    int backlog;
} server_config_t;

int server_run(const server_config_t *config,
               const volatile sig_atomic_t *shutdown_requested);

#endif
