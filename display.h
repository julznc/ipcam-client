#ifndef DISPLAY_H
#define DISPLAY_H

#include <X11/Xlib.h>

typedef struct display_context {
    Display *display;
    Window window;
} display_context_t;

display_context_t *new_display(void);
void free_display(display_context_t *display);

#endif // DISPLAY_H
