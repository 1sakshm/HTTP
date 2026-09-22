#include "response.h"

const char CSERVE_HELLO_RESPONSE[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 13\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Hello, world!";

const size_t CSERVE_HELLO_RESPONSE_LENGTH = sizeof(CSERVE_HELLO_RESPONSE) - 1U;
