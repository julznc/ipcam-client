#ifndef MEDIA_H
#define MEDIA_H

#include <libavcodec/avcodec.h>
#include "session.h"
#include "display.h"

typedef struct media_context {
    AVCodec *codec;
    session_context_t *session;
    display_context_t *display;
} media_context_t;

media_context_t *new_media(session_context_t *session);
void free_media(media_context_t *media);

int process_packet(void *packet);

#endif // MEDIA_H
