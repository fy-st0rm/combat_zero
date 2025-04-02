   // .. Other #pragmas

   // Permissive Code here.

   // Restore normal processing.
//#define _GLFW_BUILD_DLL           // To build shared version
// Ref: http://www.glfw.org/docs/latest/compile.html#compile_manual

// Platform options:
// _GLFW_WIN32      to use the Win32 API
// _GLFW_X11        to use the X Window System
// _GLFW_WAYLAND    to use the Wayland API (experimental and incomplete)
// _GLFW_COCOA      to use the Cocoa frameworks
// _GLFW_OSMESA     to use the OSMesa API (headless and non-interactive)
// _GLFW_MIR        experimental, not supported at this moment

#if defined(_WIN32) || defined(__CYGWIN__)
    #define _GLFW_WIN32
#endif
#if defined(__linux__)
    #if !defined(_GLFW_WAYLAND)     // Required for Wayland windowing
        #define _GLFW_X11
    #endif
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    #define _GLFW_X11
#endif
#if defined(__APPLE__)
    #define _GLFW_COCOA
    #define _GLFW_USE_MENUBAR       // To create and populate the menu bar when the first window is created
    #define _GLFW_USE_RETINA        // To have windows use the full resolution of Retina displays
#endif
#if defined(__TINYC__)
    #define _WIN32_WINNT_WINXP      0x0501
#endif

// Common modules to all platforms
#include "init.c"
#include "platform.c"
#include "context.c"
#include "monitor.c"
#include "window.c"
#include "input.c"
#include "vulkan.c"

#if defined(_WIN32) || defined(__CYGWIN__)
    #include "win32_init.c"
    #include "win32_module.c"
    #include "win32_monitor.c"
    #include "win32_window.c"
    #include "win32_joystick.c"
    #include "win32_time.c"
    #include "win32_thread.c"
    #include "wgl_context.c"

    #include "egl_context.c"
    #include "osmesa_context.c"
#endif

#if defined(__linux__)
    #include "posix_module.c"
    #include "posix_thread.c"
    #include "posix_time.c"
    #include "posix_poll.c"
    #include "linux_joystick.c"
    #include "xkb_unicode.c"

    #include "egl_context.c"
    #include "osmesa_context.c"

    #if defined(_GLFW_WAYLAND)
        #include "wl_init.c"
        #include "wl_monitor.c"
        #include "wl_window.c"
    #endif
    #if defined(_GLFW_X11)
        #include "x11_init.c"
        #include "x11_monitor.c"
        #include "x11_window.c"
        #include "glx_context.c"
    #endif
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__)
    #include "posix_module.c"
    #include "posix_thread.c"
    #include "posix_time.c"
    #include "posix_poll.c"
    #include "null_joystick.c"
    #include "xkb_unicode.c"

    #include "x11_init.c"
    #include "x11_monitor.c"
    #include "x11_window.c"
    #include "glx_context.c"

    #include "egl_context.c"
    #include "osmesa_context.c"
#endif

#if defined(__APPLE__)
    #include "posix_module.c"
    #include "posix_thread.c"
    #include "cocoa_init.m"
    #include "cocoa_joystick.m"
    #include "cocoa_monitor.m"
    #include "cocoa_window.m"
    #include "cocoa_time.c"
    #include "nsgl_context.m"

    #include "egl_context.c"
    #include "osmesa_context.c"
#endif

