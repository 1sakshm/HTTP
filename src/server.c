#include "server.h"

#include "response.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum {
    RECEIVE_BUFFER_SIZE = 4096
};

static void close_descriptor(int descriptor, const char *description)
{
    if (close(descriptor) == -1) {
        fprintf(stderr, "close(%s): %s\n", description, strerror(errno));
    }
}

static int send_all(int client_fd, const char *data, size_t length,
                    const volatile sig_atomic_t *shutdown_requested)
{
    size_t sent = 0U;

    while (sent < length) {
        ssize_t result = send(client_fd, data + sent, length - sent, 0);

        if (result > 0) {
            sent += (size_t)result;
            continue;
        }
        if (result == -1 && errno == EINTR && !*shutdown_requested) {
            continue;
        }
        if (result == -1) {
            fprintf(stderr, "send: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "send: wrote zero bytes\n");
        }
        return -1;
    }

    return 0;
}

static int handle_client(int client_fd,
                         const volatile sig_atomic_t *shutdown_requested)
{
    unsigned char buffer[RECEIVE_BUFFER_SIZE];
    ssize_t received;

    do {
        received = recv(client_fd, buffer, sizeof(buffer), 0);
    } while (received == -1 && errno == EINTR && !*shutdown_requested);

    if (received == 0) {
        return 0;
    }
    if (received == -1) {
        if (!*shutdown_requested) {
            fprintf(stderr, "recv: %s\n", strerror(errno));
        }
        return -1;
    }
    if (*shutdown_requested) {
        return 0;
    }

    return send_all(client_fd, CSERVE_HELLO_RESPONSE,
                    CSERVE_HELLO_RESPONSE_LENGTH, shutdown_requested);
}

static int create_listener(const server_config_t *config)
{
    struct sockaddr_in address;
    int listener_fd;
    int reuse_address = 1;
    int conversion_result;

    listener_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_fd == -1) {
        fprintf(stderr, "socket: %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
                   sizeof(reuse_address)) == -1) {
        fprintf(stderr, "setsockopt(SO_REUSEADDR): %s\n", strerror(errno));
        close_descriptor(listener_fd, "listener");
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(config->port);
    conversion_result = inet_pton(AF_INET, config->host, &address.sin_addr);
    if (conversion_result != 1) {
        if (conversion_result == 0) {
            fprintf(stderr, "invalid IPv4 address: %s\n", config->host);
        } else {
            fprintf(stderr, "inet_pton: %s\n", strerror(errno));
        }
        close_descriptor(listener_fd, "listener");
        return -1;
    }

    if (bind(listener_fd, (const struct sockaddr *)&address,
             sizeof(address)) == -1) {
        fprintf(stderr, "bind %s:%u: %s\n", config->host,
                (unsigned int)config->port, strerror(errno));
        close_descriptor(listener_fd, "listener");
        return -1;
    }
    if (listen(listener_fd, config->backlog) == -1) {
        fprintf(stderr, "listen: %s\n", strerror(errno));
        close_descriptor(listener_fd, "listener");
        return -1;
    }

    return listener_fd;
}

int server_run(const server_config_t *config,
               const volatile sig_atomic_t *shutdown_requested)
{
    int listener_fd = create_listener(config);
    int result = 0;

    if (listener_fd == -1) {
        return -1;
    }

    fprintf(stderr, "cserve listening on %s:%u\n", config->host,
            (unsigned int)config->port);

    while (!*shutdown_requested) {
        int client_fd = accept(listener_fd, NULL, NULL);

        if (client_fd == -1) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "accept: %s\n", strerror(errno));
            result = -1;
            break;
        }

        (void)handle_client(client_fd, shutdown_requested);
        close_descriptor(client_fd, "client");
    }

    close_descriptor(listener_fd, "listener");
    return result;
}
