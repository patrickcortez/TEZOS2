#define _POSIX_C_SOURCE 199309L

#include "../include/platform.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <termios.h>
#include <time.h>
#include <dirent.h>

/* Platform window structure */
struct platform_window {
    i32 width;
    i32 height;
    bool should_close;
    
    /* Framebuffer */
    i32 fb_fd;
    u8* fb_ptr;
    u32 fb_size;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    
    /* Input devices */
    i32 kbd_fd;
    i32 mouse_fd;
    
    /* Terminal state */
    struct termios orig_termios;
    i32 orig_kbd_mode;
    
    /* Event callback */
    engine_event_callback_t event_callback;
    void* user_data;
};

/* Global state */
static bool g_platform_initialized = false;
static platform_window_t* g_windows[16] = {0};
static i32 g_window_count = 0;

/* Helper: Register window */
static void register_window(platform_window_t* window) {
    if (g_window_count < 16) {
        g_windows[g_window_count++] = window;
    }
}

/* Helper: Unregister window */
static void unregister_window(platform_window_t* window) {
    for (i32 i = 0; i < g_window_count; i++) {
        if (g_windows[i] == window) {
            for (i32 j = i; j < g_window_count - 1; j++) {
                g_windows[j] = g_windows[j + 1];
            }
            g_windows[--g_window_count] = NULL;
            return;
        }
    }
}

/* Helper: Find input device by name pattern */
static i32 find_input_device(const char* name_pattern) {
    DIR* dir = opendir("/dev/input");
    if (!dir) return -1;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;
        
        char path[256];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
        
        i32 fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        
        char device_name[256] = {0};
        if (ioctl(fd, EVIOCGNAME(sizeof(device_name)), device_name) >= 0) {
            if (strstr(device_name, name_pattern)) {
                closedir(dir);
                return fd;
            }
        }
        
        close(fd);
    }
    
    closedir(dir);
    return -1;
}

/* Helper: Translate Linux key code to engine key */
static engine_key_t translate_key(u32 linux_key) {
    /* Simple mapping - expand as needed */
    if (linux_key >= KEY_A && linux_key <= KEY_Z) {
        return ENGINE_KEY_A + (linux_key - KEY_A);
    }
    
    switch (linux_key) {
        case KEY_SPACE: return ENGINE_KEY_SPACE;
        case KEY_ESC: return ENGINE_KEY_ESCAPE;
        case KEY_ENTER: return ENGINE_KEY_ENTER;
        case KEY_BACKSPACE: return ENGINE_KEY_BACKSPACE;
        case KEY_TAB: return ENGINE_KEY_TAB;
        case KEY_LEFT: return ENGINE_KEY_LEFT;
        case KEY_RIGHT: return ENGINE_KEY_RIGHT;
        case KEY_UP: return ENGINE_KEY_UP;
        case KEY_DOWN: return ENGINE_KEY_DOWN;
        default: return -1;
    }
}

/* Platform initialization */
engine_result_t platform_init(void) {
    if (g_platform_initialized) return ENGINE_SUCCESS;
    
    printf("[INFO] Initializing framebuffer platform\n");
    g_platform_initialized = true;
    return ENGINE_SUCCESS;
}

void platform_shutdown(void) {
    if (!g_platform_initialized) return;
    
    printf("[INFO] Shutting down framebuffer platform\n");
    g_platform_initialized = false;
}

/* Window creation */
engine_result_t platform_window_create(const platform_window_config_t* config, platform_window_t** out_window) {
    if (!config || !out_window) return ENGINE_ERROR_INVALID_PARAM;
    
    platform_window_t* window = (platform_window_t*)calloc(1, sizeof(platform_window_t));
    if (!window) return ENGINE_ERROR_OUT_OF_MEMORY;
    
    /* Open framebuffer */
    window->fb_fd = open("/dev/fb0", O_RDWR);
    if (window->fb_fd < 0) {
        fprintf(stderr, "[ERROR] Failed to open /dev/fb0. Are you running with sudo?\n");
        free(window);
        return ENGINE_ERROR_WINDOW_CREATION_FAILED;
    }
    
    /* Get framebuffer info */
    if (ioctl(window->fb_fd, FBIOGET_VSCREENINFO, &window->vinfo) < 0 ||
        ioctl(window->fb_fd, FBIOGET_FSCREENINFO, &window->finfo) < 0) {
        fprintf(stderr, "[ERROR] Failed to get framebuffer info\n");
        close(window->fb_fd);
        free(window);
        return ENGINE_ERROR_WINDOW_CREATION_FAILED;
    }
    
    window->width = window->vinfo.xres;
    window->height = window->vinfo.yres;
    window->fb_size = window->vinfo.yres_virtual * window->finfo.line_length;
    
    /* Map framebuffer to memory */
    window->fb_ptr = (u8*)mmap(0, window->fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, window->fb_fd, 0);
    if (window->fb_ptr == MAP_FAILED) {
        fprintf(stderr, "[ERROR] Failed to mmap framebuffer\n");
        close(window->fb_fd);
        free(window);
        return ENGINE_ERROR_WINDOW_CREATION_FAILED;
    }
    
    /* Clear screen */
    memset(window->fb_ptr, 0, window->fb_size);
    
    /* Open keyboard device */
    window->kbd_fd = find_input_device("keyboard");
    if (window->kbd_fd < 0) {
        window->kbd_fd = find_input_device("Keyboard");
    }
    if (window->kbd_fd < 0) {
        fprintf(stderr, "[WARN] No keyboard found, trying /dev/input/event0\n");
        window->kbd_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
    }
    
    /* Open mouse device */
    window->mouse_fd = find_input_device("mouse");
    if (window->mouse_fd < 0) {
        window->mouse_fd = find_input_device("Mouse");
    }
    if (window->mouse_fd < 0) {
        fprintf(stderr, "[WARN] No mouse found\n");
    }
    
    /* Save terminal state */
    tcgetattr(STDIN_FILENO, &window->orig_termios);
    ioctl(STDIN_FILENO, KDGKBMODE, &window->orig_kbd_mode);
    
    /* Set terminal to raw mode */
    struct termios raw = window->orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
   /* Store event callback from config */
    if (config->event_callback) {
        window->event_callback = config->event_callback;
        window->user_data = config->user_data;
    }
    
    printf("[INFO] Framebuffer platform initialized: %dx%d, %d bpp\n", 
           window->width, window->height, window->vinfo.bits_per_pixel);
    
    register_window(window);
    *out_window = window;
    return ENGINE_SUCCESS;
}

void platform_window_destroy(platform_window_t* window) {
    if (!window) return;
    
    unregister_window(window);
    
    /* Restore terminal */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &window->orig_termios);
    ioctl(STDIN_FILENO, KDSKBMODE, window->orig_kbd_mode);
    
    /* Close input devices */
    if (window->kbd_fd >= 0) close(window->kbd_fd);
    if (window->mouse_fd >= 0) close(window->mouse_fd);
    
    /* Unmap and close framebuffer */
    if (window->fb_ptr != MAP_FAILED) {
        munmap(window->fb_ptr, window->fb_size);
    }
    if (window->fb_fd >= 0) {
        close(window->fb_fd);
    }
    
    free(window);
    printf("[INFO] Framebuffer platform cleaned up\n");
}

/* Window properties */
i32 platform_window_get_width(const platform_window_t* window) {
    return window ? window->width : 0;
}

i32 platform_window_get_height(const platform_window_t* window) {
    return window ? window->height : 0;
}

bool platform_window_should_close(const platform_window_t* window) {
    return window ? window->should_close : true;
}

void platform_window_set_should_close(platform_window_t* window, bool should_close) {
    if (window) window->should_close = should_close;
}

void platform_window_set_event_callback(platform_window_t* window, engine_event_callback_t callback, void* user_data) {
    if (!window) return;
    window->event_callback = callback;
    window->user_data = user_data;
}

void platform_window_set_title(platform_window_t* window, const char* title) {
    /* Framebuffer mode doesn't support window titles */
    (void)window;
    (void)title;
}

void platform_window_set_visible(platform_window_t* window, bool visible) {
    /* Framebuffer is always visible */
    (void)window;
    (void)visible;
}

f64 platform_get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
}

/* Event polling */
void platform_poll_events(void) {
    /* Poll all windows */
    for (i32 w = 0; w < g_window_count; w++) {
        platform_window_t* window = g_windows[w];
        if (!window) continue;
        
        struct input_event ev;
        static i32 mouse_x = 0, mouse_y = 0;
        
        /* Poll keyboard */
        if (window->kbd_fd >= 0) {
            while (read(window->kbd_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    engine_key_t key = translate_key(ev.code);
                    if (key >= 0) {
                        engine_event_t event = {0};
                        event.type = ev.value ? ENGINE_EVENT_KEY_PRESS : ENGINE_EVENT_KEY_RELEASE;
                        event.data.key.key = key;
                        event.data.key.repeat = false;
                        
                        if (window->event_callback) {
                            window->event_callback(&event, window->user_data);
                        }
                        
                        /* ESC to quit */
                        if (key == ENGINE_KEY_ESCAPE && ev.value) {
                            window->should_close = true;
                        }
                    }
                }
            }
        }
    
        /* Poll mouse */
        if (window->mouse_fd >= 0) {
            while (read(window->mouse_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_REL) {
                    if (ev.code == REL_X) {
                        mouse_x += ev.value;
                        if (mouse_x < 0) mouse_x = 0;
                        if (mouse_x >= window->width) mouse_x = window->width - 1;
                    } else if (ev.code == REL_Y) {
                        mouse_y += ev.value;
                        if (mouse_y < 0) mouse_y = 0;
                        if (mouse_y >= window->height) mouse_y = window->height - 1;
                    }
                    
                    engine_event_t event = {0};
                    event.type = ENGINE_EVENT_MOUSE_MOVE;
                    event.data.mouse_move.x = mouse_x;
                    event.data.mouse_move.y = mouse_y;
                    
                    if (window->event_callback) {
                        window->event_callback(&event, window->user_data);
                    }
                } else if (ev.type == EV_KEY) {
                    if (ev.code >= BTN_LEFT && ev.code <= BTN_MIDDLE) {
                        engine_event_t event = {0};
                        event.type = ev.value ? ENGINE_EVENT_MOUSE_BUTTON_PRESS : ENGINE_EVENT_MOUSE_BUTTON_RELEASE;
                        event.data.mouse_button.button = ev.code - BTN_LEFT;
                        
                        if (window->event_callback) {
                            window->event_callback(&event, window->user_data);
                        }
                    }
                } else if (ev.type == EV_REL && ev.code == REL_WHEEL) {
                    engine_event_t event = {0};
                    event.type = ENGINE_EVENT_MOUSE_WHEEL;
                    event.data.mouse_wheel.delta = ev.value > 0 ? 1 : -1;
                    
                    if (window->event_callback) {
                        window->event_callback(&event, window->user_data);
                    }
                }
            }
        }
    }
}

/* Buffer presentation */
void platform_window_present_buffer(platform_window_t* window, const u32* buffer, i32 width, i32 height) {
    if (!window || !buffer) return;
    
    /* Convert RGBA to framebuffer format and write */
    i32 bpp = window->vinfo.bits_per_pixel / 8;
    
    for (i32 y = 0; y < height && y < window->height; y++) {
        for (i32 x = 0; x < width && x < window->width; x++) {
            u32 pixel = buffer[y * width + x];
            
            u8 r = pixel & 0xFF;
            u8 g = (pixel >> 8) & 0xFF;
            u8 b = (pixel >> 16) & 0xFF;
            
            i32 fb_offset = y * window->finfo.line_length + x * bpp;
            
            if (bpp == 4) {
                /* 32-bit: BGRA */
                window->fb_ptr[fb_offset + 0] = b;
                window->fb_ptr[fb_offset + 1] = g;
                window->fb_ptr[fb_offset + 2] = r;
                window->fb_ptr[fb_offset + 3] = 0xFF;
            } else if (bpp == 3) {
                /* 24-bit: BGR */
                window->fb_ptr[fb_offset + 0] = b;
                window->fb_ptr[fb_offset + 1] = g;
                window->fb_ptr[fb_offset + 2] = r;
            } else if (bpp == 2) {
                /* 16-bit: RGB565 */
                u16 rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                *(u16*)(window->fb_ptr + fb_offset) = rgb565;
            }
        }
    }
}

/* Timing */
void platform_sleep(u32 milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

u32 platform_get_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
