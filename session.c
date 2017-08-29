
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
