
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "session.h"
#include "utils.h"

session_context_t *new_session(const char *host, const char *port)
{
    session_context_t *sc = (session_context_t *)malloc(sizeof(session_context_t));
    if (NULL==sc) {
        return NULL;
    }

    memset(sc, 0, sizeof(session_context_t));
    sc->server.sin_family = AF_INET;
    sc->server.sin_port = htons(atoi(port));

    if (inet_pton(AF_INET, host, &sc->server.sin_addr) < 1) {
        ERR("unable to convert host address");
        return NULL;
    }

    sc->sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sc->sock_fd < 1) {
        ERR("unable to create socket");
        return NULL;
    }

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(sc->sock_fd, SOL_SOCKET, SO_RCVTIMEO,
                   (char *)&timeout, sizeof(timeout)) < 0) {
        close(sc->sock_fd);
        ERR("set receive timeout failed");
        return NULL;
    }

    if (setsockopt(sc->sock_fd, SOL_SOCKET, SO_SNDTIMEO,
                   (char *)&timeout, sizeof(timeout)) < 0) {
        close(sc->sock_fd);
        ERR("set send timeout failed");
        return NULL;
    }

    if (0 != connect(sc->sock_fd, (struct sockaddr *)&sc->server, sizeof(sc->server))) {
        ERR("unable to connect to host");
        close(sc->sock_fd);
        return NULL;
    }

    return sc;
}

void free_session(session_context_t *session)
{
    if (NULL==session) {
        return;
    }

    if (session->sock_fd > 0) {
        close(session->sock_fd);
    }

    free(session);
}

int session_send(session_context_t *session, const void *data, size_t length)
{
    if ((NULL==session) || (session->sock_fd < 1)) {
        return -1;
    }
    return write(session->sock_fd, data, length);
}

int session_read(session_context_t *session, void *data, size_t max_length)
{
    if ((NULL==session) || (session->sock_fd < 1)) {
        return -1;
    }
    return read(session->sock_fd, data, max_length);
}
