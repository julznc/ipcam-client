
#include "session.h"
#include "utils.h"

int main(int argc, char **argv)
{
    DBG("start %s", argv[0]);

    if (argc != 3) {
        ERR("wrong arguments");
        return -1;
    }

    session_context_t *session = new_session(argv[1], argv[2]);
    if (NULL==session) {
        ERR("session not created");
        return -1;
    }

    free_session(session);
    DBG("end %s", argv[0]);
    return 0;
}
