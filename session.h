#ifndef SESSION_H
#define SESSION_H

typedef struct session_context {
    int sock_fd;
} session_context_t;

session_context_t *new_session(const char *host, const char *port);

void free_session(session_context_t *session);

#endif // SESSION_H
