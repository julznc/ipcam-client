#ifndef SESSION_H
#define SESSION_H

#include <stdint.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef struct session_context {
    int sock_fd;
    struct sockaddr_in server;
} session_context_t;

session_context_t *new_session(const char *host, const char *port);
void free_session(session_context_t *session);

int session_send(session_context_t *session, const void *data, size_t length);
int session_read(session_context_t *session, void *data, size_t max_length);

int session_init(session_context_t *session);

#endif // SESSION_H
