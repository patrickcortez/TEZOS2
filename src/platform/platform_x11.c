#include "../../include/platform.h"

#ifdef ENGINE_PLATFORM_LINUX

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/* Platform window structure */
struct platform_window {
    Display* display;
    Window window;
    Atom wm_delete_window;
    i32 width;
    i32 height;
    bool should_close;
    engine_event_callback_t event_callback;
    void* user_data;
};

/* Platform state */
struct platform_state {
    bool initialized;
    Display* display;
    int screen;
    struct timeval start_time;
};

static platform_state_t g_platform_state = {0};

/* Helper: Translate X11 key to engine key */
static engine_key_t translate_key(KeySym keysym) {
    switch (keysym) {
        case XK_space: return ENGINE_KEY_SPACE;
        case XK_Escape: return ENGINE_KEY_ESCAPE;
        case XK_Return: return ENGINE_KEY_ENTER;
        case XK_Tab: return ENGINE_KEY_TAB;
        case XK_BackSpace: return ENGINE_KEY_BACKSPACE;
        case XK_Insert: return ENGINE_KEY_INSERT;
        case XK_Delete: return ENGINE_KEY_DELETE;
        case XK_Right: return ENGINE_KEY_RIGHT;
        case XK_Left: return ENGINE_KEY_LEFT;
        case XK_Down: return ENGINE_KEY_DOWN;
        case XK_Up: return ENGINE_KEY_UP;
        
        /* Modifier keys */
        case XK_Shift_L: return ENGINE_KEY_LEFT_SHIFT;
        case XK_Control_L: return ENGINE_KEY_LEFT_CONTROL;
        case XK_Alt_L: return ENGINE_KEY_LEFT_ALT;
        case XK_Shift_R: return ENGINE_KEY_RIGHT_SHIFT;
        case XK_Control_R: return ENGINE_KEY_RIGHT_CONTROL;
        case XK_Alt_R: return ENGINE_KEY_RIGHT_ALT;
        
        case XK_F1: return ENGINE_KEY_F1;
        case XK_F2: return ENGINE_KEY_F2;
        case XK_F3: return ENGINE_KEY_F3;
        case XK_F4: return ENGINE_KEY_F4;
        case XK_F5: return ENGINE_KEY_F5;
        case XK_F6: return ENGINE_KEY_F6;
        case XK_F7: return ENGINE_KEY_F7;
        case XK_F8: return ENGINE_KEY_F8;
        case XK_F9: return ENGINE_KEY_F9;
        case XK_F10: return ENGINE_KEY_F10;
        case XK_F11: return ENGINE_KEY_F11;
        case XK_F12: return ENGINE_KEY_F12;
        
        /* Numbers */
        case XK_0: return ENGINE_KEY_0;
        case XK_1: return ENGINE_KEY_1;
        case XK_2: return ENGINE_KEY_2;
        case XK_3: return ENGINE_KEY_3;
        case XK_4: return ENGINE_KEY_4;
        case XK_5: return ENGINE_KEY_5;
        case XK_6: return ENGINE_KEY_6;
        case XK_7: return ENGINE_KEY_7;
        case XK_8: return ENGINE_KEY_8;
        case XK_9: return ENGINE_KEY_9;
        
        /* Letters */
        case XK_a: case XK_A: return ENGINE_KEY_A;
        case XK_b: case XK_B: return ENGINE_KEY_B;
        case XK_c: case XK_C: return ENGINE_KEY_C;
        case XK_d: case XK_D: return ENGINE_KEY_D;
        case XK_e: case XK_E: return ENGINE_KEY_E;
        case XK_f: case XK_F: return ENGINE_KEY_F;
        case XK_g: case XK_G: return ENGINE_KEY_G;
        case XK_h: case XK_H: return ENGINE_KEY_H;
        case XK_i: case XK_I: return ENGINE_KEY_I;
        case XK_j: case XK_J: return ENGINE_KEY_J;
        case XK_k: case XK_K: return ENGINE_KEY_K;
        case XK_l: case XK_L: return ENGINE_KEY_L;
        case XK_m: case XK_M: return ENGINE_KEY_M;
        case XK_n: case XK_N: return ENGINE_KEY_N;
        case XK_o: case XK_O: return ENGINE_KEY_O;
        case XK_p: case XK_P: return ENGINE_KEY_P;
        case XK_q: case XK_Q: return ENGINE_KEY_Q;
        case XK_r: case XK_R: return ENGINE_KEY_R;
        case XK_s: case XK_S: return ENGINE_KEY_S;
        case XK_t: case XK_T: return ENGINE_KEY_T;
        case XK_u: case XK_U: return ENGINE_KEY_U;
        case XK_v: case XK_V: return ENGINE_KEY_V;
        case XK_w: case XK_W: return ENGINE_KEY_W;
        case XK_x: case XK_X: return ENGINE_KEY_X;
        case XK_y: case XK_Y: return ENGINE_KEY_Y;
        case XK_z: case XK_Z: return ENGINE_KEY_Z;
        
        /* Symbols */
        case XK_apostrophe: return ENGINE_KEY_APOSTROPHE;
        case XK_comma: return ENGINE_KEY_COMMA;
        case XK_minus: return ENGINE_KEY_MINUS;
        case XK_period: return ENGINE_KEY_PERIOD;
        case XK_slash: return ENGINE_KEY_SLASH;
        case XK_semicolon: return ENGINE_KEY_SEMICOLON;
        case XK_equal: return ENGINE_KEY_EQUALS;
        case XK_bracketleft: return ENGINE_KEY_LEFT_BRACKET;
        case XK_backslash: return ENGINE_KEY_BACKSLASH;
        case XK_bracketright: return ENGINE_KEY_RIGHT_BRACKET;
        
        default: return ENGINE_KEY_UNKNOWN;
    }
}

/* Window list management */
#define MAX_WINDOWS 16
static platform_window_t* g_windows[MAX_WINDOWS] = {0};

static void register_window(platform_window_t* window) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!g_windows[i]) {
            g_windows[i] = window;
            return;
        }
    }
    ENGINE_LOG_WARN("Maximum number of windows reached");
}

static void unregister_window(platform_window_t* window) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i] == window) {
            g_windows[i] = NULL;
            return;
        }
    }
}

static platform_window_t* find_window(Window xwindow) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i] && g_windows[i]->window == xwindow) {
            return g_windows[i];
        }
    }
    return NULL;
}

/* Platform implementation */

engine_result_t platform_init(void) {
    if (g_platform_state.initialized) {
        ENGINE_LOG_WARN("Platform already initialized");
        return ENGINE_SUCCESS;
    }

    /* Open X11 display */
    g_platform_state.display = XOpenDisplay(NULL);
    if (!g_platform_state.display) {
        ENGINE_LOG_ERROR("Failed to open X11 display");
        return ENGINE_ERROR_PLATFORM_INIT_FAILED;
    }

    g_platform_state.screen = DefaultScreen(g_platform_state.display);
    
    /* Record start time */
    gettimeofday(&g_platform_state.start_time, NULL);
    
    g_platform_state.initialized = true;
    ENGINE_LOG_INFO("X11 platform initialized");
    
    return ENGINE_SUCCESS;
}

void platform_shutdown(void) {
    if (!g_platform_state.initialized) {
        return;
    }

    if (g_platform_state.display) {
        XCloseDisplay(g_platform_state.display);
        g_platform_state.display = NULL;
    }

    g_platform_state.initialized = false;
    ENGINE_LOG_INFO("X11 platform shutdown");
}

engine_result_t platform_window_create(
    const platform_window_config_t* config,
    platform_window_t** out_window
) {
    if (!g_platform_state.initialized) {
        ENGINE_LOG_ERROR("Platform not initialized");
        return ENGINE_ERROR_NOT_INITIALIZED;
    }

    if (!config || !out_window) {
        ENGINE_LOG_ERROR("Invalid parameters");
        return ENGINE_ERROR_INVALID_PARAM;
    }

    /* Allocate window structure */
    platform_window_t* window = (platform_window_t*)malloc(sizeof(platform_window_t));
    if (!window) {
        ENGINE_LOG_ERROR("Failed to allocate window");
        return ENGINE_ERROR_OUT_OF_MEMORY;
    }

    memset(window, 0, sizeof(platform_window_t));
    window->display = g_platform_state.display;
    window->width = config->width;
    window->height = config->height;
    window->event_callback = config->event_callback;
    window->user_data = config->user_data;
    window->should_close = false;

    /* Get root window */
    Window root = RootWindow(g_platform_state.display, g_platform_state.screen);

    /* Create window */
    XSetWindowAttributes attrs;
    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
                      ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                      StructureNotifyMask | FocusChangeMask;
    attrs.background_pixel = BlackPixel(g_platform_state.display, g_platform_state.screen);

    window->window = XCreateWindow(
        g_platform_state.display,
        root,
        0, 0,  /* x, y position (will be centered by WM if not specified) */
        config->width,
        config->height,
        0,  /* border width */
        CopyFromParent,  /* depth */
        InputOutput,  /* class */
        CopyFromParent,  /* visual */
        CWBackPixel | CWEventMask,
        &attrs
    );

    if (!window->window) {
        ENGINE_LOG_ERROR("Failed to create X11 window");
        free(window);
        return ENGINE_ERROR_WINDOW_CREATION_FAILED;
    }

    /* Set window title */
    XStoreName(g_platform_state.display, window->window, config->title);

    /* Set size hints for resizable window */
    XSizeHints* size_hints = XAllocSizeHints();
    if (size_hints) {
        if (!config->resizable) {
            size_hints->flags = PMinSize | PMaxSize;
            size_hints->min_width = size_hints->max_width = config->width;
            size_hints->min_height = size_hints->max_height = config->height;
        }
        XSetWMNormalHints(g_platform_state.display, window->window, size_hints);
        XFree(size_hints);
    }

    /* Register for window close event */
    window->wm_delete_window = XInternAtom(g_platform_state.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_platform_state.display, window->window, &window->wm_delete_window, 1);

    /* Show window if visible */
    if (config->visible) {
        XMapWindow(g_platform_state.display, window->window);
    }

    XFlush(g_platform_state.display);

    /* Register window for event handling */
    register_window(window);

    *out_window = window;
    ENGINE_LOG_INFO("X11 window created: %dx%d", config->width, config->height);

    return ENGINE_SUCCESS;
}

void platform_window_destroy(platform_window_t* window) {
    if (!window) {
        return;
    }

    /* Unregister window */
    unregister_window(window);

    if (window->window && window->display) {
        XDestroyWindow(window->display, window->window);
        XFlush(window->display);
    }

    free(window);
    ENGINE_LOG_INFO("X11 window destroyed");
}

bool platform_window_should_close(const platform_window_t* window) {
    if (!window) {
        return true;
    }

    return window->should_close;
}

void platform_poll_events(void) {
    if (!g_platform_state.initialized || !g_platform_state.display) {
        return;
    }

    /* Process all pending events */
    while (XPending(g_platform_state.display) > 0) {
        XEvent xevent;
        XNextEvent(g_platform_state.display, &xevent);

        /* Find the window this event belongs to */
        platform_window_t* window = find_window(xevent.xany.window);
        if (!window) {
            continue;  /* Event for unknown window, ignore */
        }

        engine_event_t event = {0};

        switch (xevent.type) {
            case ClientMessage:
                /* Window close requested */
                if ((Atom)xevent.xclient.data.l[0] == window->wm_delete_window) {
                    event.type = ENGINE_EVENT_WINDOW_CLOSE;
                    window->should_close = true;
                    if (window->event_callback) {
                        window->event_callback(&event, window->user_data);
                    }
                }
                break;

            case ConfigureNotify:
                /* Window resized */
                if (xevent.xconfigure.width != window->width ||
                    xevent.xconfigure.height != window->height) {
                    window->width = xevent.xconfigure.width;
                    window->height = xevent.xconfigure.height;
                    
                    event.type = ENGINE_EVENT_WINDOW_RESIZE;
                    event.data.resize.width = window->width;
                    event.data.resize.height = window->height;
                    
                    if (window->event_callback) {
                        window->event_callback(&event, window->user_data);
                    }
                }
                break;

            case KeyPress: {
                KeySym keysym = XLookupKeysym(&xevent.xkey, 0);
                event.type = ENGINE_EVENT_KEY_PRESS;
                event.data.key.key = translate_key(keysym);
                event.data.key.repeat = false;
                
                if (window->event_callback) {
                    window->event_callback(&event, window->user_data);
                }
                break;
            }

            case KeyRelease: {
                KeySym keysym = XLookupKeysym(&xevent.xkey, 0);
                event.type = ENGINE_EVENT_KEY_RELEASE;
                event.data.key.key = translate_key(keysym);
                event.data.key.repeat = false;
                
                if (window->event_callback) {
                    window->event_callback(&event, window->user_data);
                }
                break;
            }

            case MotionNotify:
                event.type = ENGINE_EVENT_MOUSE_MOVE;
                event.data.mouse_move.x = xevent.xmotion.x;
                event.data.mouse_move.y = xevent.xmotion.y;
                
                if (window->event_callback) {
                    window->event_callback(&event, window->user_data);
                }
                break;

            case ButtonPress:
                event.type = ENGINE_EVENT_MOUSE_BUTTON_PRESS;
                event.data.mouse_button.button = (engine_mouse_button_t)(xevent.xbutton.button - 1);
                
                if (window->event_callback) {
                    window->event_callback(&event, window->user_data);
                }
                break;

            case ButtonRelease:
                event.type = ENGINE_EVENT_MOUSE_BUTTON_RELEASE;
                event.data.mouse_button.button = (engine_mouse_button_t)(xevent.xbutton.button - 1);
                
                if (window->event_callback) {
                    window->event_callback(&event, window->user_data);
                }
                break;

            case FocusIn:
                event.type = ENGINE_EVENT_WINDOW_FOCUS;
                if (window->event_callback) {
                    window->event_callback(&event, window->user_data);
                }
                break;

            case FocusOut:
                event.type = ENGINE_EVENT_WINDOW_UNFOCUS;
                if (window->event_callback) {
                    window->event_callback(&event, window->user_data);
                }
                break;

            default:
                break;
        }
    }
}

i32 platform_window_get_width(const platform_window_t* window) {
    if (!window) {
        return 0;
    }
    return window->width;
}

i32 platform_window_get_height(const platform_window_t* window) {
    if (!window) {
        return 0;
    }
    return window->height;
}

void platform_window_set_title(platform_window_t* window, const char* title) {
    if (!window || !window->window || !window->display || !title) {
        return;
    }

    XStoreName(window->display, window->window, title);
    XFlush(window->display);
}

void platform_window_set_visible(platform_window_t* window, bool visible) {
    if (!window || !window->window || !window->display) {
        return;
    }

    if (visible) {
        XMapWindow(window->display, window->window);
    } else {
        XUnmapWindow(window->display, window->window);
    }
    
    XFlush(window->display);
}

f64 platform_get_time(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    
    f64 seconds = (f64)(now.tv_sec - g_platform_state.start_time.tv_sec);
    f64 microseconds = (f64)(now.tv_usec - g_platform_state.start_time.tv_usec);
    
    return seconds + (microseconds / 1000000.0);
}

void platform_window_present_buffer(
    platform_window_t* window,
    const u32* pixels,
    i32 width,
    i32 height
) {
    if (!window || !window->window || !window->display || !pixels) {
        return;
    }
    
    /* Create XImage from pixel buffer */
    Visual* visual = DefaultVisual(window->display, DefaultScreen(window->display));
    i32 depth = DefaultDepth(window->display, DefaultScreen(window->display));
    
    /* Create XImage with proper format */
    XImage* ximage = XCreateImage(
        window->display,
        visual,
        depth,
        ZPixmap,
        0,
        (char*)pixels,
        width,
        height,
        32,  /* bitmap_pad - 32-bit pixels */
        0    /* bytes_per_line (auto-calculate) */
    );
    
    if (!ximage) {
        ENGINE_LOG_ERROR("Failed to create XImage");
        return;
    }
    
    /* Set the byte order and pixel format to match our RGBA layout */
    /* Our pixels are: u32 = (A << 24) | (B << 16) | (G << 8) | R */
    ximage->byte_order = LSBFirst;  /* Little-endian byte order */
    ximage->bitmap_bit_order = LSBFirst;
    
    /* Set RGB masks for 32-bit RGBA format */
    /* On most systems with 24/32-bit depth, we need to specify the masks */
    if (depth == 24 || depth == 32) {
        ximage->red_mask = 0x000000FF;    /* R is in byte 0 */
        ximage->green_mask = 0x0000FF00;  /* G is in byte 1 */
        ximage->blue_mask = 0x00FF0000;   /* B is in byte 2 */
    }
    
    /* Put image to window */
    GC gc = DefaultGC(window->display, DefaultScreen(window->display));
    XPutImage(
        window->display,
        window->window,
        gc,
        ximage,
        0, 0,      /* src x, y */
        0, 0,      /* dest x, y */
        width, height
    );
    
    /* Clean up (don't free pixel data, it's owned by caller) */
    ximage->data = NULL;
    XDestroyImage(ximage);
    
    XFlush(window->display);
}

void platform_sleep(u32 milliseconds) {
    usleep(milliseconds * 1000);
}

#endif /* ENGINE_PLATFORM_LINUX */
