
#include <stdlib.h>

#include "session.h"
#include "utils.h"

session_context_t *new_session(const char *host, const char *port)
{
    session_context_t *sc = (session_context_t *)malloc(sizeof(session_context_t));
    if (NULL==sc) {
        return NULL;
    }

    return sc;
}

void free_session(session_context_t *session)
{
    if (NULL==session) {
        return;
    }

    free(session);
}
