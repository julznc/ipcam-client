
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "utils.h"

//#define DISPLAY_FORMAT      AV_PIX_FMT_BGR24
#define DISPLAY_FORMAT      AV_PIX_FMT_RGB24
#define DISPLAY_DEPTH      24

display_context_t *new_display(unsigned int width, unsigned int height)
{
    DBG("%s(%u, %u)", __func__, width, height);
    display_context_t *dc = (display_context_t *)malloc(sizeof(display_context_t));
    if (NULL==dc) {
        ERR("failed to allocate display context");
        return NULL;
    }

    memset(dc, 0, sizeof(display_context_t));

    dc->display = XOpenDisplay(NULL);
    if (NULL==dc->display) {
        ERR("XOpenDisplay() failed. Try \"$ export DISPLAY=:0.0\"");
        free(dc);
        return NULL;
    }

    int screen = DefaultScreen(dc->display);
    assert(DISPLAY_DEPTH == DefaultDepth(dc->display, screen));

    unsigned long black = BlackPixel(dc->display, screen);
    unsigned long white = WhitePixel(dc->display, screen);

    dc->window = XCreateSimpleWindow(dc->display,
                                     XDefaultRootWindow(dc->display),
                                     100, 100, width, height, 4,
                                     white, black);

    if (dc->window < 0) {
        ERR("XCreateSimpleWindow() failed.");
        free_display(dc);
        return NULL;
    }

    dc->gc = XCreateGC(dc->display, dc->window, 0, NULL);
    if (NULL==dc->gc) {
        ERR("XCreateGC() failed.");
        free_display(dc);
        return NULL;
    }

    XSetBackground(dc->display, dc->gc, white);
    XSetForeground(dc->display, dc->gc, black);

    //DBG("dc->window=%lu", dc->window);
    XMapWindow(dc->display, dc->window);
    XFlush(dc->display);

    XSelectInput(dc->display, dc->window,
                 KeyPressMask | ButtonPressMask | ExposureMask);

    Atom delwin = XInternAtom(dc->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dc->display, dc->window, &delwin, 1);

    dc->x11_fd = ConnectionNumber(dc->display);
    //DBG("ConnectionNumber()=%d", dc->x11_fd);

    dc->rgbFrame = av_frame_alloc();
    if (NULL==dc->rgbFrame) {
        ERR("failed to allocate RGB frame");
        free_display(dc);
        return NULL;
    }

    dc->rgbFrame->width = width;
    dc->rgbFrame->height = height;
    dc->rgbFrame->format = DISPLAY_FORMAT;

    dc->prevFrame = av_frame_alloc();
    if (NULL==dc->prevFrame) {
        ERR("failed to allocate a temporary frame");
        free_display(dc);
        return NULL;
    }

    return dc;
}

void free_display(display_context_t *display)
{
    if (NULL==display) {
        return;
    }

    if (display->swsContext) {
        sws_freeContext(display->swsContext);
        display->swsContext = NULL;
    }

    if (display->rgbFrame) {
        av_frame_free(&display->rgbFrame);
        display->rgbFrame = NULL;
    }

    if (display->rgbBuffer) {
        free(display->rgbBuffer);
        display->rgbBuffer = NULL;
    }

    if (display->display) {
        if (display->gc) {
            XFreeGC (display->display, display->gc);
            display->gc = NULL;
        }
        if (display->window > 0) {
            XDestroyWindow(display->display, display->window);
            display->window = 0;
        }
        XCloseDisplay(display->display);
    }

    free(display);
}

int init_conversion(display_context_t *display, AVFrame *src)
{
    if ((NULL!=display->swsContext) &&
        (src->format==display->prevFrame->format) &&
        (src->width==display->prevFrame->width) &&
        (src->height==display->prevFrame->height)) {
        //DBG("reused");
        return 0;
    }

    if (NULL!=display->swsContext) {
        // free up previous
        sws_freeContext(display->swsContext);
        display->swsContext = NULL;
    }

    if (NULL!=display->rgbBuffer) {
        // free up previous
        free(display->rgbBuffer);
        display->rgbBuffer = NULL;
    }

    display->swsContext = sws_getContext(src->width, src->height, src->format,
                                         display->rgbFrame->width,
                                         display->rgbFrame->height,
                                         display->rgbFrame->format,
                                         SWS_BICUBIC | SWS_PRINT_INFO,
                                         NULL, NULL, NULL);

    if (NULL==display->swsContext) {
        ERR("failed to allocate sws context");
        return -1;
    }

    int nbytes = av_image_get_buffer_size(display->rgbFrame->format,
                                          display->rgbFrame->width,
                                          display->rgbFrame->height, 1);
    //DBG("buffer size = %d", nbytes);
    display->rgbBuffer = (uint8_t *)av_malloc(nbytes);
    if (NULL==display->rgbBuffer) {
        ERR("failed to allocate rgb buffer");
        sws_freeContext(display->swsContext);
        display->swsContext = NULL;
        return -1;
    }

    av_image_fill_arrays(display->rgbFrame->data, display->rgbFrame->linesize,
                         display->rgbBuffer, display->rgbFrame->format,
                         display->rgbFrame->width, display->rgbFrame->height, 1);

    display->prevFrame->format = src->format;
    display->prevFrame->width = src->width;
    display->prevFrame->height = src->height;
    return 0;
}

int display_frame(display_context_t *display, AVFrame *frame)
{
    //DBG("%s %dx%d #%u", __func__,
    //    frame->width, frame->height, display->frameCount);

    if (0 != init_conversion(display, frame)) {
        return -1;
    }

    int height = sws_scale(display->swsContext,
                           (uint8_t const* const*)frame->data,
                           frame->linesize, 0, frame->height,
                           display->rgbFrame->data,
                           display->rgbFrame->linesize);
    if (height <= 0) {
        ERR("failed to scale image");
        return -1;
    }

    XImage *image = XCreateImage(display->display, NULL,
                                 DISPLAY_DEPTH, ZPixmap, 0,
                                 (char *)display->rgbFrame->data,
                                 display->rgbFrame->width,
                                 display->rgbFrame->height,
                                 8, 0);//display->rgbFrame->linesize[0]);

    if (NULL==image) {
        ERR("XCreateImage() failed.");
        return -1;
    }

    Pixmap pixmap = XCreatePixmap(display->display, display->window,
                                  display->rgbFrame->width,
                                  display->rgbFrame->height, DISPLAY_DEPTH);
    if (pixmap <= 0) {
        ERR("XCreatePixmap() failed.");
        return -1;
    }

    XPutImage(display->display, pixmap,
              display->gc, image, 0, 0, 0, 0,
              display->rgbFrame->width,
              display->rgbFrame->height);

    XSetWindowBackgroundPixmap(display->display,
                               display->window, pixmap);
    XClearWindow(display->display, display->window);
    XSync(display->display, 0);

#if 0 // will also destroy 'rgbFrame->data'!
    XDestroyImage(image);
#else
    free(image);
#endif
    XFreePixmap(display->display, pixmap);

    display->frameCount++;
    return 0;
}
