
#include "media.h"
#include "bubble.h"
#include "utils.h"

media_context_t *new_media(session_context_t *session)
{
    media_context_t *media = (media_context_t *)malloc(sizeof(media_context_t));
    if (NULL==media) {
        return NULL;
    }

    memset(media, 0, sizeof(media_context_t));
    media->session = session;

    media->display = new_display(640, 360);
    if (NULL==media->display) {
        free(media);
        return NULL;
    }

    avcodec_register_all();
    return media;
}

void free_media(media_context_t *media)
{
    if (NULL==media) {
        return;
    }

    if (media->display) {
        free_display(media->display);
    }

    free(media);
}

int process_packet(void *packet)
{
    PackHead *packhead = (PackHead *)packet;
    uint32_t uiPackLen = ntohl(packhead->uiLength);
    size_t packsize = packsize = uiPackLen + STRUCT_MEMBER_POS(PackHead, cPackType);

    MediaPackData *media_packhead = (MediaPackData *)packhead->pData;
    uint32_t uiMediaPackLen = ntohl(media_packhead->uiLength);
    if (uiMediaPackLen + STRUCT_MEMBER_POS(PackHead, pData) +
            STRUCT_MEMBER_POS(MediaPackData, pData) > packsize) {
        ERR("Frame data is larger than packet size");
        return -1;
    }

    //DBG("media packet chl: %d type: %d", media_packhead->cId, media_packhead->cMediaType);
    //unsigned char *framedata = (unsigned char *)media_packhead->pData;
    switch (media_packhead->cMediaType)
    {
    case MT_IDR:
        //DBG("MT_IDR");
        //dumpbytes(framedata, uiMediaPackLen);
        break;
    case MT_PSLICE:
        //DBG("MT_PSLICE");
        //dumpbytes(framedata, uiMediaPackLen);
        break;
    case MT_AUDIO:
        //DBG("MT_AUDIO");
        //dumpbytes(framedata, uiMediaPackLen);
        break;
    default:
        ERR("unknown media pack type");
        return -1;
    }

    return 0;
}

int run_media(media_context_t *media)
{
    fd_set readfds;
    struct timeval tv;

    if (NULL==media) {
        return -1;
    }

    int max_fd = media->session->sock_fd;
    if (media->display->x11_fd > max_fd) {
        max_fd = media->display->x11_fd;
    }

    media->running = 1;
    while (media->running) {

        FD_ZERO(&readfds);
        FD_SET(media->session->sock_fd, &readfds);
        FD_SET(media->display->x11_fd, &readfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int num_ready_fds = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if (num_ready_fds < 0) {
            //ERR("select() failed");
            return -1;
        } else if (num_ready_fds == 0) {
            //DBG("select() timed out");
            continue;
        }

        if (FD_ISSET(media->session->sock_fd, &readfds)) {
            int nbytes = receive_packet(media->session, media->recvbuf, sizeof(media->recvbuf));
            if (-2 == nbytes) {
                media->running = 0;
            } else if (nbytes > 0) {
                process_packet(media->recvbuf);
            }
        }

        if (FD_ISSET(media->display->x11_fd, &readfds)) {
            while(XPending(media->display->display)) {
                XEvent event;
                XNextEvent(media->display->display, &event);
                //DBG("event %d", event.type);
                if (ClientMessage == event.type) {
                    // todo: check if "WM_DELETE_WINDOW"
                    media->running = 0;
                }
            }
        }
    }

    return 0;
}

void stop_media(media_context_t *media)
{
    DBG("%s()", __func__);
    if (NULL==media) {
        return;
    }
    media->running = 0;
}

