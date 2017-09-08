
#include <signal.h>


#include "bubble.h"
#include "utils.h"

static int app_exit = 0;

static void handle_sigint(int signum)
{
    DBG("%s", __func__);
    if (app_exit)
        return;
    app_exit = 1;
}

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
        ERR("login failed");
        free_session(session);
        return -1;
    }

    if (0 != open_stream(session, 0, 1)) {
        ERR("failed to open stream");
        free_session(session);
        return -1;
    }

    media_context_t *media = new_media(session);
    if (NULL == media) {
        ERR("media context not created");
        free_session(session);
        return -1;
    }

    app_exit = 0;
    signal(SIGINT, handle_sigint);
    while (0==app_exit) {
        unsigned char tmpbuff[512*1024 + 16];
        int nbytes = receive_packet(session, tmpbuff, sizeof(tmpbuff));
        if (-2 == nbytes) {
            app_exit = 1;
        } else if (nbytes > 0) {
            process_packet(tmpbuff);
        }
    }

    free_media(media);
    free_session(session);
    DBG("end %s", argv[0]);
    return 0;
}
