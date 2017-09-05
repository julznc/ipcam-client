
#include "bubble.h"
#include "utils.h"

int main(int argc, char **argv)
{
    DBG("start %s", argv[0]);

    if (argc != 5) {
        ERR("wrong arguments");
        return -1;
    }

    session_context_t *session = new_session(argv[1], argv[2]);
    if (NULL==session) {
        ERR("session not created");
        return -1;
    }

    if (0 != session_init(session)) {
        ERR("failed to initialize session");
        free_session(session);
        return -1;
    }

    if (0 != verify_user(session, argv[3], argv[4])) {
        DBG("login failed");
    }

    free_session(session);
    DBG("end %s", argv[0]);
    return 0;
}
