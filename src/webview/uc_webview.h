/* remote-ui copy of uc_webview.h of the firmware's libucwebview library (Unfolded OS): the libucwebview ABI contract. Keep it identical to the firmware header apart from this line; UC_WEBVIEW_API_VERSION is checked at runtime. */
/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Unfolded Circle AG
 *
 * libucwebview: an in-process WPE WebKit (WPEPlatform) web view with a plain C ABI.
 *
 * The library owns everything WPE: it implements a WPEDisplay whose views hand
 * their rendered frames to the embedding host instead of a screen, creates the
 * WebKitWebView, and exposes the result through this header only. The header
 * is the contract between the firmware (which ships libucwebview.so.1) and the
 * hosts that dlopen() it (remote-ui's WebViewItem, a flutter-pi plugin,
 * ucwebview-test). It uses no GLib, WPE or EGL types: EGLDisplay/EGLImageKHR
 * travel as void *.
 *
 * Compatibility: UC_WEBVIEW_API_VERSION is bumped on every incompatible change
 * (removed or changed functions, changed struct layouts, changed enum values).
 * Hosts compare uc_webview_api_version() with the value they were compiled
 * against and treat a mismatch as "web view unavailable". Additive changes
 * (new functions, new callback fields at the end of uc_webview_callbacks,
 * new frame fields at the end of uc_webview_frame) keep the version.
 *
 * Threading contract
 * ==================
 * Every function in this header, including uc_webview_frame_rendered() and
 * uc_webview_frame_release(), must be called from the thread that called
 * uc_webview_global_init() ("the UI thread"). That thread must iterate the
 * GLib main context that is the default context for that thread
 * (g_main_context_default(), which Qt's QPAEventDispatcherGlib drives on the
 * GUI thread; WPEPlatform and WebKit deliver all their events through it,
 * see WPEPlatform/docs/tutorial-browser.md). All callbacks arrive on that
 * thread while it iterates the context, never from inside a uc_webview_*
 * call. A host with a render thread (Qt's threaded scene graph) may touch the
 * EGLImage and GL objects of a frame from the render thread, but must marshal
 * the uc_webview_frame_rendered()/uc_webview_frame_release() calls back to the
 * UI thread.
 *
 * Frame lifecycle
 * ===============
 *  1. WebKit's web process renders into a DMA-BUF (or, without a GPU, a shared
 *     memory buffer) and hands it to the library.
 *  2. The library imports it into an EGLImageKHR on the EGLDisplay given to
 *     uc_webview_global_init() and calls callbacks.frame_ready() with a
 *     uc_webview_frame. The frame (and its EGLImage) stays valid until the
 *     host calls uc_webview_frame_release() for it.
 *  3. The host shows the frame and calls uc_webview_frame_rendered(handle)
 *     once it is on screen (Qt: after QQuickWindow::frameSwapped). WebKit does
 *     not send the next frame before this call, so a host that never calls it
 *     stalls rendering after one frame.
 *  4. When the host no longer samples the frame (it has shown a newer one) it
 *     calls uc_webview_frame_release(handle); the buffer goes back to the web
 *     process. Up to three frames can be in flight; keep at most two.
 *  uc_webview_free() releases every frame still held by the host; the host
 *  must not touch their EGLImages afterwards.
 */
#ifndef UC_WEBVIEW_H
#define UC_WEBVIEW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UC_WEBVIEW_API_VERSION 1

/* Soname of the library the hosts dlopen(). */
#define UC_WEBVIEW_SONAME "libucwebview.so.1"

typedef struct uc_webview uc_webview;

/*
 * One rendered frame. All fields are read-only for the host and valid until
 * uc_webview_frame_release(handle).
 */
typedef struct {
    int width, height;          /* buffer size in device pixels */
    void *egl_image;            /* EGLImageKHR imported on the host's EGLDisplay, NULL for a shared memory frame */
    uint32_t fourcc;            /* DRM fourcc of the buffer (DMA-BUF: as allocated by the web process; SHM: 'AR24') */
    uint64_t modifier;          /* DRM format modifier (DMA-BUF only, 0 = linear, 0x00ffffffffffffff = none/unknown) */
    int n_planes;               /* informational; the dma-buf fds stay inside the library */
    void *handle;               /* opaque, pass back to uc_webview_frame_rendered()/_release() */
    /* Shared memory fallback (web process without a GPU, LIBGL_ALWAYS_SOFTWARE, host EGLDisplay NULL):
     * egl_image is NULL and the pixels are here, ARGB8888 little endian (B,G,R,A bytes), top row first.
     * NULL/0 for DMA-BUF frames. */
    const void *pixels;
    int stride;                 /* bytes per row of pixels */
} uc_webview_frame;

/* load_changed() events (values of WebKitLoadEvent, WebKitWebView.h.in:157-160, plus FAILED) */
#define UC_WEBVIEW_LOAD_STARTED     0   /* a new load request was made */
#define UC_WEBVIEW_LOAD_REDIRECTED  1   /* server redirect, uri is the new one */
#define UC_WEBVIEW_LOAD_COMMITTED   2   /* content started arriving, uri is final */
#define UC_WEBVIEW_LOAD_FINISHED    3   /* load finished (also after a failure) */
#define UC_WEBVIEW_LOAD_FAILED      4   /* load failed, uri is the failing uri; FINISHED follows, except for a
                                           URI the scheme allowlist rejected (uc_webview_load_uri()), which is
                                           reported alone and leaves the current page and load untouched */

/* web_process_terminated() reasons (values of WebKitWebProcessTerminationReason, WebKitWebView.h.in:236-238) */
#define UC_WEBVIEW_TERMINATED_CRASHED               0
#define UC_WEBVIEW_TERMINATED_EXCEEDED_MEMORY_LIMIT 1
#define UC_WEBVIEW_TERMINATED_BY_API                2

/* uc_webview_touch() phases */
#define UC_WEBVIEW_TOUCH_DOWN   0
#define UC_WEBVIEW_TOUCH_MOVE   1
#define UC_WEBVIEW_TOUCH_UP     2
#define UC_WEBVIEW_TOUCH_CANCEL 3

/* uc_webview_pointer_button() buttons (values of WPE_BUTTON_*, WPEEvent.h:138-153) */
#define UC_WEBVIEW_BUTTON_PRIMARY   1   /* left */
#define UC_WEBVIEW_BUTTON_MIDDLE    2
#define UC_WEBVIEW_BUTTON_SECONDARY 3   /* right */

/* modifier bits for the input functions (values of WPEModifiers, WPEEvent.h:118-130) */
#define UC_WEBVIEW_MOD_CONTROL   (1u << 0)
#define UC_WEBVIEW_MOD_SHIFT     (1u << 1)
#define UC_WEBVIEW_MOD_ALT       (1u << 2)
#define UC_WEBVIEW_MOD_META      (1u << 3)
#define UC_WEBVIEW_MOD_CAPS_LOCK (1u << 4)
#define UC_WEBVIEW_MOD_BUTTON1   (1u << 8)   /* primary button held (pointer move while dragging) */
#define UC_WEBVIEW_MOD_BUTTON2   (1u << 9)
#define UC_WEBVIEW_MOD_BUTTON3   (1u << 10)
#define UC_WEBVIEW_MOD_BUTTON4   (1u << 11)
#define UC_WEBVIEW_MOD_BUTTON5   (1u << 12)

/* ime_focus() purpose (values of WPEInputPurpose, WPEInputMethodContext.h:66-88) */
#define UC_WEBVIEW_IME_PURPOSE_FREE_FORM 0
#define UC_WEBVIEW_IME_PURPOSE_ALPHA     1
#define UC_WEBVIEW_IME_PURPOSE_DIGITS    2
#define UC_WEBVIEW_IME_PURPOSE_NUMBER    3
#define UC_WEBVIEW_IME_PURPOSE_PHONE     4
#define UC_WEBVIEW_IME_PURPOSE_URL       5
#define UC_WEBVIEW_IME_PURPOSE_EMAIL     6
#define UC_WEBVIEW_IME_PURPOSE_NAME      7
#define UC_WEBVIEW_IME_PURPOSE_PASSWORD  8
#define UC_WEBVIEW_IME_PURPOSE_PIN       9
#define UC_WEBVIEW_IME_PURPOSE_TERMINAL  10
#define UC_WEBVIEW_IME_PURPOSE_SEARCH    11

/* ime_focus() hint bits (values of WPEInputHints, WPEInputMethodContext.h:119-134) */
#define UC_WEBVIEW_IME_HINT_NONE                0u
#define UC_WEBVIEW_IME_HINT_SPELLCHECK          (1u << 0)
#define UC_WEBVIEW_IME_HINT_NO_SPELLCHECK       (1u << 1)
#define UC_WEBVIEW_IME_HINT_WORD_COMPLETION     (1u << 2)
#define UC_WEBVIEW_IME_HINT_LOWERCASE           (1u << 3)
#define UC_WEBVIEW_IME_HINT_UPPERCASE_CHARS     (1u << 4)
#define UC_WEBVIEW_IME_HINT_UPPERCASE_WORDS     (1u << 5)
#define UC_WEBVIEW_IME_HINT_UPPERCASE_SENTENCES (1u << 6)
#define UC_WEBVIEW_IME_HINT_INHIBIT_OSK         (1u << 7)   /* page asked for no on-screen keyboard */
#define UC_WEBVIEW_IME_HINT_VERTICAL_WRITING    (1u << 8)
#define UC_WEBVIEW_IME_HINT_EMOJI               (1u << 9)
#define UC_WEBVIEW_IME_HINT_NO_EMOJI            (1u << 10)
#define UC_WEBVIEW_IME_HINT_PRIVATE             (1u << 11)

/*
 * Host callbacks. Every field may be NULL. All run on the UI thread from the
 * GLib main context; strings are owned by the library and valid only during
 * the call. Do not call uc_webview_free() from inside a callback.
 */
typedef struct {
    /* A new frame is available (see "Frame lifecycle" above). The uc_webview_frame pointer is only
     * valid during the call; copy the struct. */
    void (*frame_ready)(uc_webview *, const uc_webview_frame *, void *user);
    /* UC_WEBVIEW_LOAD_* with the current (or failing) URI. */
    void (*load_changed)(uc_webview *, int event, const char *uri, void *user);
    void (*title_changed)(uc_webview *, const char *title, void *user);
    /* 0.0 .. 1.0 */
    void (*progress_changed)(uc_webview *, double progress, void *user);
    /* UC_WEBVIEW_TERMINATED_*; the view is blank afterwards, uc_webview_load_uri()/reload() recover. */
    void (*web_process_terminated)(uc_webview *, int reason, void *user);
    /* An editable element gained (focused=1) or lost (0) focus. hints/purpose are UC_WEBVIEW_IME_*;
     * surrounding/cursor are the text around the caret and the caret's character offset if known
     * (surrounding may be NULL, cursor -1). Called again with focused=1 when hints or purpose change
     * while focused. The host shows/hides its keyboard here. */
    void (*ime_focus)(uc_webview *, int focused, unsigned hints, unsigned purpose,
                      const char *surrounding, int cursor, void *user);
    /* Caret rectangle in view pixels (for keyboards that avoid covering the field). */
    void (*ime_cursor_area)(uc_webview *, int x, int y, int w, int h, void *user);
    /* CSS cursor name requested by the page ("default", "pointer", "text", ...). */
    void (*cursor_changed)(uc_webview *, const char *css_name, void *user);
    /* Surrounding text update while an editable element is focused: text around the caret, caret
     * character offset and selection anchor offset (equal to cursor without a selection). */
    void (*ime_surrounding)(uc_webview *, const char *text, int cursor, int anchor, void *user);
} uc_webview_callbacks;

/* ---- process-wide ------------------------------------------------------- */

/* Any thread. Returns UC_WEBVIEW_API_VERSION of the loaded library. */
int uc_webview_api_version(void);

/*
 * UI thread, once per process, before the first uc_webview_new(). Creates the
 * in-process WPEDisplay and the shared WebKit network session.
 *   egl_display  the host's EGLDisplay (Qt: nativeResourceForIntegration("egldisplay")); frames are
 *                imported on it. NULL selects the shared memory path (frames carry pixels, no EGLImage).
 *   data_dir     website data directory (cookies, local storage); NULL or unwritable = ephemeral session.
 *   cache_dir    HTTP cache directory; NULL = data_dir/cache (ignored for an ephemeral session).
 * Returns 0 on success, -1 if the display could not be connected (EGL display unusable), 0 when
 * already initialised (the arguments of the second call are ignored).
 * Environment honoured: WPE_DRM_DEVICE, WPE_BUFFER_FORMAT, WPE_USE_EXPLICIT_SYNC, LIBGL_ALWAYS_SOFTWARE
 * (WPEPlatform), WEBKIT_INSPECTOR_SERVER (needs UC_WEBVIEW_DEVELOPER_EXTRAS=1), UC_WEBVIEW_CONSOLE_LOG=1
 * (page console to stderr), JavaScriptCoreUseJIT=0 (sandboxes with MemoryDenyWriteExecute),
 * UC_WEBVIEW_ALLOW_FILE_URLS=1 (lets uc_webview_load_uri()/load_html() use file: URIs; read here, once),
 * UC_WEBVIEW_REFRESH_RATE=<Hz> (display refresh rate, wins over uc_webview_global_set_refresh_rate(); read here,
 * once), UC_WEBVIEW_TRACE=1 (log lines about the display refresh ticks).
 */
int uc_webview_global_init(void *egl_display, const char *data_dir, const char *cache_dir);

/*
 * UI thread. Drops the library's display, network session and settings after every view has been freed
 * (views still alive are logged and nothing is dropped). WebKit's helper processes exit with the last view.
 * Not required before process exit; meant for leak checks. WebKit keeps process-wide state (screen manager,
 * settings proxy), so a second uc_webview_global_init() in the same process is supported for tests only.
 */
void uc_webview_global_shutdown(void);

/*
 * UI thread (the thread that runs or will run uc_webview_global_init(); may be called before it, the value is
 * kept for it). Added in library 0.2.0 within API version 1: hosts resolve it with dlsym() and skip it when an
 * older library lacks it (WebKit then paces at 60 Hz as before).
 *
 * The refresh rate of the display the host presents the frames on, in Hz (decimals allowed: 49.999, 59.6), valid
 * range 20..240. The library reports it to WebKit as the screen's refresh rate (WebKit's nominal frame rate) and
 * ticks WebKit's display link, which drives requestAnimationFrame and the page's rendering updates, from a thread
 * of its own at this rate plus 1 % (a link slower than the panel would leave refreshes without a frame; a surplus
 * tick is absorbed while the host reports frames as it presents them). Without it the link runs at 60 Hz. Qt hosts
 * pass QScreen::refreshRate() of the view's window.
 * Precedence: UC_WEBVIEW_REFRESH_RATE (environment, read by uc_webview_global_init()) > this value > 60 Hz.
 * Call it before the first page load: WebKit takes its nominal frame rate from the screen once, when it sets up
 * the display link at the first load in the process; later calls change the tick rate at once but not that
 * nominal value.
 * Returns 0 when the value was taken (also when UC_WEBVIEW_REFRESH_RATE overrides it), -1 when it is not a number
 * in 20..240 or the call comes from another thread (the previous value stays).
 */
int uc_webview_global_set_refresh_rate(double hz);

/* UI thread. Added in library 0.2.0 like the setter. The display refresh rate in Hz the library uses after the
 * precedence above (the display link ticks 1 % faster; before uc_webview_global_init() the environment is not read
 * yet). */
double uc_webview_global_get_refresh_rate(void);

/* ---- view lifecycle ------------------------------------------------------ */

/*
 * UI thread. Creates a web view of width x height device pixels (minimum 1x1) that is mapped (visible to
 * the page) and focused; use uc_webview_set_mapped()/set_focused() to change that. cb is copied; user is
 * passed to every callback. Returns NULL before uc_webview_global_init() or on failure.
 */
uc_webview *uc_webview_new(int width, int height, const uc_webview_callbacks *cb, void *user);

/* UI thread, not from a callback. Releases held frames, destroys the WebKitWebView. NULL is ignored. */
void uc_webview_free(uc_webview *);

/* UI thread. New size in device pixels (minimum 1x1); the next frame has the new size. */
void uc_webview_resize(uc_webview *, int width, int height);

/* UI thread. mapped=0 tells WebKit the view is hidden: rendering stops and timers throttle (display off,
 * item invisible); mapped=1 resumes and triggers a new frame. */
void uc_webview_set_mapped(uc_webview *, int mapped);

/* UI thread. Keyboard focus of the view: unfocused views do not get IME focus and their caret blinks off. */
void uc_webview_set_focused(uc_webview *, int focused);

/* UI thread. Device scale factor (CSS px to device px, default 1.0, > 0); changes trigger a re-layout. */
void uc_webview_set_scale(uc_webview *, double scale);

/* ---- navigation ---------------------------------------------------------- */

/*
 * UI thread. Starts loading uri. Scheme allowlist: http, https, about and data; file only when the process
 * set UC_WEBVIEW_ALLOW_FILE_URLS=1 before uc_webview_global_init() (blocked by default). Any other scheme,
 * and a string without a scheme (a bare path, "example.com"), is refused with a warning in the log and a
 * load_changed(UC_WEBVIEW_LOAD_FAILED, uri) callback from the main context (no STARTED/FINISHED); the
 * current page and a running load stay as they are.
 */
void uc_webview_load_uri(uc_webview *, const char *uri);
/* UI thread. Loads html (UTF-8, NUL terminated) with base_uri (NULL or "" = about:blank) for relative URLs.
 * A base_uri outside the allowlist of uc_webview_load_uri() refuses the load the same way (FAILED with
 * base_uri). */
void uc_webview_load_html(uc_webview *, const char *html, const char *base_uri);
void uc_webview_reload(uc_webview *);
void uc_webview_stop(uc_webview *);
void uc_webview_go_back(uc_webview *);
void uc_webview_go_forward(uc_webview *);
int  uc_webview_can_go_back(uc_webview *);
int  uc_webview_can_go_forward(uc_webview *);

/* UI thread. Current URI / title; owned by the library, valid until the next library call or callback,
 * NULL before the first load. */
const char *uc_webview_get_uri(uc_webview *);
const char *uc_webview_get_title(uc_webview *);

/* UI thread. Runs script in the page's main world, result discarded (errors go to the log). */
void uc_webview_run_javascript(uc_webview *, const char *script);

/* UI thread. Page zoom factor (1.0 = 100 %, > 0). */
void uc_webview_set_zoom(uc_webview *, double zoom);

/* UI thread. Colour shown where the page is transparent (default opaque black); components 0..1. */
void uc_webview_set_background_rgba(uc_webview *, float r, float g, float b, float a);

/* ---- frames -------------------------------------------------------------- */

/* UI thread. The host has displayed the frame: WebKit may render the next one. Unknown or already
 * reported handles are ignored with a warning. */
void uc_webview_frame_rendered(uc_webview *, void *frame_handle);
/* UI thread. The host no longer uses the frame or its EGLImage: the buffer returns to the web process.
 * Implies frame_rendered if that was not called. */
void uc_webview_frame_release(uc_webview *, void *frame_handle);

/* ---- input --------------------------------------------------------------- */
/*
 * Coordinates are view pixels (origin top-left). ms is the event time in
 * milliseconds from any monotonic source (used for double-click and gesture
 * timing; 0 = now). mods are UC_WEBVIEW_MOD_* bits.
 */

/* UI thread. One touch point; id is the touch sequence (finger) id. Touches drive WebKit's own gesture
 * recogniser (tap -> click, drag -> scroll) and the page's touch events. */
void uc_webview_touch(uc_webview *, int id, int phase, double x, double y, uint32_t ms);
/* UI thread. Mouse button press/release (UC_WEBVIEW_BUTTON_*); click counts are computed internally. */
void uc_webview_pointer_button(uc_webview *, int button, int pressed, double x, double y, unsigned mods, uint32_t ms);
void uc_webview_pointer_move(uc_webview *, double x, double y, unsigned mods, uint32_t ms);
/* UI thread. Wheel/scroll at (x, y): precise=1 means dx/dy are pixels, 0 means wheel ticks (120 Qt units
 * = 1 tick). Positive dy scrolls the content up (towards the top), like a wheel rolled away from the user. */
void uc_webview_scroll(uc_webview *, double dx, double dy, double x, double y, int precise, uint32_t ms);
/* UI thread. Key event with an XKB keysym (XKB_KEY_* / WPE_KEY_*, e.g. 0xff0d Return, 0xff08 BackSpace,
 * 0x0061 'a'); keycode is the hardware (evdev + 8) code or 0 if unknown. Printable keysyms insert text. */
void uc_webview_key(uc_webview *, uint32_t xkb_keysym, uint32_t keycode, int pressed, unsigned mods, uint32_t ms);

/* ---- text input (IME) ---------------------------------------------------- */
/* Valid between ime_focus(1) and ime_focus(0); ignored otherwise. */

/* UI thread. Inserts utf8 at the caret (ends a running pre-edit first). */
void uc_webview_ime_commit(uc_webview *, const char *utf8);
/* UI thread. Shows utf8 as composition text with the caret at character offset cursor; "" or NULL ends the
 * pre-edit without inserting. */
void uc_webview_ime_preedit(uc_webview *, const char *utf8, int cursor);
/* UI thread. Deletes n_chars characters starting offset characters from the caret (offset -1, n 1 = backspace). */
void uc_webview_ime_delete_surrounding(uc_webview *, int offset, int n_chars);

#ifdef __cplusplus
}
#endif

#endif /* UC_WEBVIEW_H */
