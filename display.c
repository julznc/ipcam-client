
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "utils.h"

#define DISPLAY_FORMAT      AV_PIX_FMT_RGBA

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
    unsigned long black = BlackPixel(dc->display, screen);
    unsigned long white = WhitePixel(dc->display, screen);

    dc->depth = DefaultDepth(dc->display, screen);
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

void free_display(display_context_t *dc)
{
    if (NULL==dc) {
        return;
    }

    if (dc->swsContext) {
        sws_freeContext(dc->swsContext);
        dc->swsContext = NULL;
    }

    if (dc->rgbFrame) {
        av_frame_free(&dc->rgbFrame);
        dc->rgbFrame = NULL;
    }

    if (dc->rgbBuffer) {
        free(dc->rgbBuffer);
        dc->rgbBuffer = NULL;
    }

    if (dc->display) {
        if (dc->gc) {
            XFreeGC (dc->display, dc->gc);
            dc->gc = NULL;
        }
        if (dc->window > 0) {
            XDestroyWindow(dc->display, dc->window);
            dc->window = 0;
        }
        XCloseDisplay(dc->display);
    }

    free(dc);
}

int init_conversion(display_context_t *dc, AVFrame *src)
{
    if ((NULL!=dc->swsContext) &&
        (src->format==dc->prevFrame->format) &&
        (src->width==dc->prevFrame->width) &&
        (src->height==dc->prevFrame->height)) {
        //DBG("reused");
        return 0;
    }

    if (NULL!=dc->swsContext) {
        // free up previous
        sws_freeContext(dc->swsContext);
        dc->swsContext = NULL;
    }

    if (NULL!=dc->rgbBuffer) {
        // free up previous
        free(dc->rgbBuffer);
        dc->rgbBuffer = NULL;
    }

    dc->swsContext = sws_getContext(src->width, src->height, src->format,
                                         dc->rgbFrame->width,
                                         dc->rgbFrame->height,
                                         dc->rgbFrame->format,
                                         SWS_BICUBIC | SWS_PRINT_INFO,
                                         NULL, NULL, NULL);

    if (NULL==dc->swsContext) {
        ERR("failed to allocate sws context");
        return -1;
    }

    int nbytes = av_image_get_buffer_size(dc->rgbFrame->format,
                                          dc->rgbFrame->width,
                                          dc->rgbFrame->height, 1);
    //DBG("buffer size = %d", nbytes);
    dc->rgbBuffer = (uint8_t *)av_malloc(nbytes);
    if (NULL==dc->rgbBuffer) {
        ERR("failed to allocate rgb buffer");
        sws_freeContext(dc->swsContext);
        dc->swsContext = NULL;
        return -1;
    }

    av_image_fill_arrays(dc->rgbFrame->data, dc->rgbFrame->linesize,
                         dc->rgbBuffer, dc->rgbFrame->format,
                         dc->rgbFrame->width, dc->rgbFrame->height, 1);

    dc->prevFrame->format = src->format;
    dc->prevFrame->width = src->width;
    dc->prevFrame->height = src->height;
    return 0;
}

int display_frame(display_context_t *dc, AVFrame *frame)
{
    //DBG("%s %dx%d #%u", __func__,
    //    frame->width, frame->height, dc->frameCount);

    if (0 != init_conversion(dc, frame)) {
        return -1;
    }

    int height = sws_scale(dc->swsContext,
                           (uint8_t const* const*)frame->data,
                           frame->linesize, 0, frame->height,
                           dc->rgbFrame->data,
                           dc->rgbFrame->linesize);
    if (height <= 0) {
        ERR("failed to scale image");
        return -1;
    }

    XImage *image = XCreateImage(dc->display, NULL,
                                 dc->depth, ZPixmap, 0,
                                 (char *)dc->rgbFrame->data[0],
                                 dc->rgbFrame->width,
                                 dc->rgbFrame->height,
                                 8, 0);

    if (NULL==image) {
        ERR("XCreateImage() failed.");
        return -1;
    }

    Pixmap pixmap = XCreatePixmap(dc->display, dc->window,
                                  dc->rgbFrame->width,
                                  dc->rgbFrame->height,
                                  dc->depth);
    if (pixmap <= 0) {
        ERR("XCreatePixmap() failed.");
        return -1;
    }

    XPutImage(dc->display, pixmap,
              dc->gc, image, 0, 0, 0, 0,
              dc->rgbFrame->width,
              dc->rgbFrame->height);

    XSetWindowBackgroundPixmap(dc->display,
                               dc->window, pixmap);
    XClearWindow(dc->display, dc->window);
    XSync(dc->display, 0);

#if 0 // will also destroy 'rgbFrame->data'!
    XDestroyImage(image);
#else
    free(image);
#endif
    XFreePixmap(dc->display, pixmap);

    dc->frameCount++;
    return 0;
}
