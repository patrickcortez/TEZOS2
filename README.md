# 2D Engine

A dependency-free, cross-platform 2D game engine written in pure C. No external dependencies - just you, C, and the platform APIs.

## Features

- ✅ **Cross-platform**: Windows, Linux, macOS
- ✅ **Zero dependencies**: Uses native platform APIs directly
- ✅ **Pure C99**: Clean, portable code
- ✅ **Window management**: Create, resize, and manage windows
- ✅ **Event system**: Keyboard, mouse, and window events
- 🚧 **Graphics rendering**: Coming soon

## Platform Support

| Platform | API | Status |
|----------|-----|--------|
| Windows | Win32 | Planned |
| Linux | X11 | Planned |
| macOS | Cocoa | Planned |

## Quick Start

### Building

```bash
# Build library and example
make

# Build and run example
make run

# Clean build artifacts
make clean

# Show platform info
make info
```

### Options

- `DEBUG=1` - Build with debug symbols and logging (default)
- `DEBUG=0` - Build optimized release version

### Example Code

```c
#include "engine.h"

void on_event(const engine_event_t* event, void* user_data) {
    if (event->type == ENGINE_EVENT_WINDOW_CLOSE) {
        printf("Window closing!\n");
    }
}

int main(void) {
    /* Initialize engine */
    engine_config_t config = {
        .app_name = "My Game",
        .enable_logging = true,
    };
    engine_init(&config);
    
    /* Create window */
    engine_window_config_t window_config = {
        .title = "My Game Window",
        .width = 800,
        .height = 600,
        .resizable = true,
        .event_callback = on_event,
    };
    
    engine_window_t* window;
    engine_window_create(&window_config, &window);
    
    /* Main loop */
    while (!engine_window_should_close(window)) {
        engine_poll_events();
        
        /* Your game logic here */
    }
    
    /* Cleanup */
    engine_window_destroy(window);
    engine_shutdown();
    
    return 0;
}
```

## Project Structure

```
2D engine/
├── include/          # Public headers
│   ├── engine.h      # Main engine API
│   ├── platform.h    # Platform abstraction
│   └── types.h       # Common types
├── src/              # Source files
│   ├── engine.c      # Engine implementation
│   └── platform/     # Platform-specific code
│       ├── platform_win32.c
│       ├── platform_x11.c
│       └── platform_macos.c
├── examples/         # Example programs
│   └── basic_window.c
├── build/            # Build output
├── Makefile          # Build system
└── README.md
```

## Architecture

The engine is built in layers:

1. **Platform Layer** (`platform.h`) - Direct interface to OS APIs
2. **Engine Core** (`engine.h`) - High-level game engine API
3. **Your Game** - Built on top of the engine

### Platform Abstraction

Each platform implements the same interface defined in `platform.h`:

- **Windows**: Win32 API (CreateWindow, message pump)
- **Linux**: X11/Xlib (XCreateWindow, event loop)
- **macOS**: Cocoa (NSWindow, NSApplication)

The engine core wraps these implementations to provide a unified, easy-to-use API.

## Event System

The engine provides a callback-based event system:

```c
typedef enum {
    ENGINE_EVENT_WINDOW_CLOSE,
    ENGINE_EVENT_WINDOW_RESIZE,
    ENGINE_EVENT_KEY_PRESS,
    ENGINE_EVENT_KEY_RELEASE,
    ENGINE_EVENT_MOUSE_MOVE,
    ENGINE_EVENT_MOUSE_BUTTON_PRESS,
    ENGINE_EVENT_MOUSE_BUTTON_RELEASE,
    ENGINE_EVENT_MOUSE_WHEEL,
} engine_event_type_t;
```

Register an event callback when creating your window to receive events.

## Building on Different Platforms

### Linux

Requirements:
- GCC or Clang
- X11 development libraries (`libx11-dev` on Debian/Ubuntu)

```bash
sudo apt-get install libx11-dev  # Debian/Ubuntu
sudo dnf install libX11-devel     # Fedora
make
```

### Windows

Requirements:
- MinGW-w64 or MSVC
- Windows SDK (for Win32 APIs)

```bash
# With MinGW
make

# With MSVC (use nmake or Developer Command Prompt)
cl /I include src/engine.c src/platform/platform_win32.c /link user32.lib gdi32.lib
```

### macOS

Requirements:
- Xcode Command Line Tools
- Clang

```bash
xcode-select --install  # If needed
make
```

## Roadmap

### Phase 1: Foundation (Current)
- [x] Core type system and platform detection
- [x] Platform abstraction layer
- [x] Engine initialization system
- [ ] Window creation (Win32, X11, Cocoa)
- [ ] Event handling

### Phase 2: Graphics
- [ ] Software renderer
- [ ] Pixel buffer management
- [ ] Basic drawing primitives (lines, rectangles, circles)
- [ ] Image/sprite loading and rendering
- [ ] Text rendering

### Phase 3: Input
- [ ] Keyboard input system
- [ ] Mouse input with button states
- [ ] Gamepad/controller support

### Phase 4: Audio
- [ ] Audio playback system
- [ ] Sound effect support
- [ ] Music streaming

### Phase 5: Utilities
- [ ] Math library (vectors, matrices)
- [ ] Collision detection
- [ ] Resource management
- [ ] Scene system

## Contributing

This is a learning project focused on understanding how game engines work from scratch. Contributions are welcome!

## License

This project is in the public domain. Use it however you like!

## Why This Project?

Modern game engines come with lots of dependencies. This project explores:
- How operating systems handle windows and graphics
- How to write truly portable C code
- The fundamentals of game engine architecture
- Direct API usage without abstraction libraries

Perfect for learning, embedded systems, or situations where you can't use external libraries.
