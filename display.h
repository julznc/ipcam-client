#ifndef DISPLAY_H
#define DISPLAY_H

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <X11/Xlib.h>

typedef struct display_context {
    Display *display;
    Window window;
    int x11_fd;
    struct SwsContext *swsContext;
    uint8_t *rgbBuffer;
    AVFrame *rgbFrame;
    AVFrame *prevFrame;
} display_context_t;

display_context_t *new_display(unsigned int width, unsigned int height);
void free_display(display_context_t *display);

int init_conversion(display_context_t *display, AVFrame *src);

int display_frame(display_context_t *display, AVFrame *frame);

#endif // DISPLAY_H
