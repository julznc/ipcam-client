
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "utils.h"

display_context_t *new_display(unsigned int width, unsigned int height)
{
    DBG("%s(%u, %u)", __func__, width, height);
    display_context_t *dc = (display_context_t *)malloc(sizeof(display_context_t));
    if (NULL==dc) {
        ERR("failed to allocate display context");
        return NULL;
    }

    dc->display = XOpenDisplay(NULL);
    if (NULL==dc->display) {
        ERR("XOpenDisplay() failed. Try \"$ export DISPLAY=:0.0\"");
        free(dc);
        return NULL;
    }

    dc->window = XCreateSimpleWindow(dc->display,
                                     XDefaultRootWindow(dc->display),
                                     100, 100, width, height, 4, 0, 0);

    //DBG("dc->window=%lu", dc->window);
    XMapWindow(dc->display, dc->window);
    XFlush(dc->display);

    XSelectInput(dc->display, dc->window,
                 KeyPressMask | ButtonPressMask | ExposureMask);

    Atom delwin = XInternAtom(dc->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dc->display, dc->window, &delwin, 1);

    dc->x11_fd = ConnectionNumber(dc->display);
    //DBG("ConnectionNumber()=%d", dc->x11_fd);

    return dc;
}

void free_display(display_context_t *display)
{
    if (NULL==display) {
        return;
    }

    if (display->display) {
        XDestroyWindow(display->display, display->window);
        XCloseDisplay(display->display);
    }

    free(display);
}
