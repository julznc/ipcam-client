#ifndef MEDIA_H
#define MEDIA_H

#include <libavcodec/avcodec.h>
#include "session.h"
#include "display.h"

typedef struct media_context {
    AVCodec *codec;
    session_context_t *session;
    display_context_t *display;
    char recvbuf[128*1024 + FF_INPUT_BUFFER_PADDING_SIZE];
    int running;
} media_context_t;

media_context_t *new_media(session_context_t *session);
void free_media(media_context_t *media);

int process_packet(void *packet);

int run_media(media_context_t *media);
void stop_media(media_context_t *media);

#endif // MEDIA_H
