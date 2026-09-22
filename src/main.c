#include "server.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t shutdown_requested = 0;

static void request_shutdown(int signal_number)
{
    (void)signal_number;
    shutdown_requested = 1;
}

static int install_signal_handler(int signal_number, void (*handler)(int))
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handler;
    if (sigemptyset(&action.sa_mask) == -1) {
        fprintf(stderr, "sigemptyset: %s\n", strerror(errno));
        return -1;
    }
    if (sigaction(signal_number, &action, NULL) == -1) {
        fprintf(stderr, "sigaction: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int install_signal_handlers(void)
{
    if (install_signal_handler(SIGINT, request_shutdown) == -1 ||
        install_signal_handler(SIGTERM, request_shutdown) == -1 ||
        install_signal_handler(SIGPIPE, SIG_IGN) == -1) {
        return -1;
    }
    return 0;
}

static void print_usage(FILE *stream, const char *program_name)
{
    fprintf(stream, "Usage: %s [--host IPv4] [--port PORT]\n", program_name);
}

static int parse_port(const char *text, uint16_t *port)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value == 0UL ||
        value > 65535UL) {
        return -1;
    }
    *port = (uint16_t)value;
    return 0;
}

static int parse_arguments(int argc, char **argv, server_config_t *config)
{
    int index;

    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--help") == 0) {
            print_usage(stdout, argv[0]);
            return 1;
        }
        if (strcmp(argv[index], "--host") == 0) {
            if (++index >= argc) {
                fprintf(stderr, "--host requires an IPv4 address\n");
                return -1;
            }
            config->host = argv[index];
            continue;
        }
        if (strcmp(argv[index], "--port") == 0) {
            if (++index >= argc || parse_port(argv[index], &config->port) == -1) {
                fprintf(stderr, "--port requires a number from 1 to 65535\n");
                return -1;
            }
            continue;
        }

        fprintf(stderr, "unknown option: %s\n", argv[index]);
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    server_config_t config = {
        .host = "0.0.0.0",
        .port = 8080U,
        .backlog = 128
    };
    int parse_result = parse_arguments(argc, argv, &config);

    if (parse_result == 1) {
        return EXIT_SUCCESS;
    }
    if (parse_result == -1) {
        print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }
    if (install_signal_handlers() == -1) {
        return EXIT_FAILURE;
    }
    if (server_run(&config, &shutdown_requested) == -1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
