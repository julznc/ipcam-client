
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "utils.h"

display_context_t *new_display(void)
{
    display_context_t *dc = (display_context_t *)malloc(sizeof(display_context_t));
    if (NULL==dc) {
        return NULL;
    }
    dc->display = XOpenDisplay(NULL);
    if (NULL==dc->display) {
        free(dc);
        return NULL;
    }

    return dc;
}

void free_display(display_context_t *display)
{
    if (NULL==display) {
        return;
    }

    if (display->display) {
        XCloseDisplay(display->display);
    }

    free(display);
}
