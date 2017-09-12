
#include <signal.h>


#include "bubble.h"
#include "utils.h"

session_context_t *session = NULL;
media_context_t *media = NULL;

static void handle_sigint(int signum)
{
    //DBG("%s(%d)", __func__, signum);
    if (NULL==media)
        return;

    stop_media(media);
}

int main(int argc, char **argv)
{
    DBG("start %s", argv[0]);

    if (argc != 5) {
        ERR("wrong arguments");
        return -1;
    }

    session = new_session(argv[1], argv[2]);
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
        ERR("login failed");
        free_session(session);
        return -1;
    }

    if (0 != open_stream(session, 0, 0)) {
        ERR("failed to open stream");
        free_session(session);
        return -1;
    }

    media = new_media(session);
    if (NULL == media) {
        ERR("media context not created");
        free_session(session);
        return -1;
    }


    signal(SIGINT, handle_sigint);
    run_media(media);

    free_media(media);
    free_session(session);
    DBG("end %s", argv[0]);
    return 0;
}
