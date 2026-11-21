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
 * Compatibility
 * =============
 * UC_WEBVIEW_API_VERSION is bumped on every incompatible change (removed or
 * changed functions, changed struct layouts, changed values). Hosts compare
 * uc_webview_api_version() with the value they were compiled against and
 * treat a mismatch as "web view unavailable". Within one API version the host
 * and the library come from different releases of this header, older or
 * newer in either direction (remote-ui custom UIs and the firmware that ships
 * the library update independently). Only these changes are additive and keep
 * the version:
 *  - New functions. Each names the library release that added it; hosts
 *    resolve such a function with dlsym() and cope with its absence.
 *  - New fields at the end of uc_webview_callbacks, the only struct the host
 *    allocates. The library sees them only through uc_webview_new_sized(),
 *    which takes the host's sizeof(uc_webview_callbacks), copies
 *    min(cb_size, its own size) bytes and treats the callbacks beyond cb_size
 *    as NULL. uc_webview_new() reads exactly the nine callbacks of API
 *    version 1 (frame_ready .. ime_surrounding), whatever header the host was
 *    built against. Library 0.6.0 appended pinch and magnification_changed,
 *    library 0.7.0 the six request callbacks (script_dialog ..
 *    response_refused). A host detects 0.7.0 with
 *    dlsym("uc_webview_request_reply"); an older library never reads the
 *    request callbacks, and the page's requests get the old behaviour.
 *  - New values of the constants the library reports (load events,
 *    termination reasons, IME purposes and hint bits, dialog types,
 *    authentication schemes and flags, new-window results, refusal reasons):
 *    hosts ignore values they do not know (an unknown dialog type or
 *    authentication scheme: answer it with uc_webview_request_reply(id, 0,
 *    NULL)). The library ignores input values it does not know
 *    (touch phases, buttons) with a warning and drops unknown modifier bits.
 * Nothing else changes within API version 1: no field is inserted, reordered,
 * retyped or removed, and uc_webview_frame does not grow at all. The library
 * allocates the frame and the host copies it with the sizeof of its own
 * header, which can be newer than the library's; per-frame data added later
 * come from new functions that take the frame handle. uc_webview_cookie
 * (library 0.8.0) does not grow either: the host passes an array of them, and
 * the library steps through it with its own sizeof.
 * Hosts therefore
 *  - zero-initialise uc_webview_callbacks (C: = { 0 }, C++: {}) before setting
 *    fields and create views with uc_webview_new_sized(..., cb,
 *    sizeof(uc_webview_callbacks), ...) of their own build; when dlsym() does
 *    not find it (library older than 0.4.0) they call uc_webview_new() with the
 *    same struct, from which such a library reads the API version 1 callbacks;
 *  - copy a uc_webview_frame by value or read it through the pointer, never
 *    allocate one for the library, and pass back only its handle;
 *  - take this header unchanged from one library release (a copy may add a
 *    comment), never mixing in declarations of their own.
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
 * call, with one exception: a change of the view's focus reports the page's
 * focused editable element at once, ime_focus(1) when the view gains the
 * focus and ime_focus(0) when it loses it (WebKit notifies its input method
 * from the view's has-focus change, WPEWebViewPlatform.cpp:100-110). That is
 * uc_webview_set_focused(), and also the pointer press or touch tap with which
 * WebKit focuses an unfocused view itself (WPEWebViewPlatform.cpp:368-370,
 * 468). The hints of such a report are those WebKit computed when the element
 * got its focus; since library 0.7.5 there is none for an element whose web
 * process is gone. The second exception: uc_webview_free() ends the view's
 * pending uc_webview_when_rendered() requests with
 * UC_WEBVIEW_RENDERED_CANCELLED inside the call (library 0.5.0). The pinch and
 * magnification_changed callbacks of library 0.6.0 and the request callbacks
 * of library 0.7.0 follow the rule without exception: uc_webview_free() ends
 * pending requests without any callback, and a request a navigation call ends
 * is reported with request_cancelled() from the main context after the call.
 * A host with a render thread (Qt's threaded scene graph) may touch the
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
 *
 * Pinch zoom (library 0.6.0)
 * ==========================
 * WebKit's WPE port has no zoom gesture of its own, and redrawing the page at
 * every finger move would take seconds per step on a large page. So the work
 * is split:
 *  1. The library recognises a two-finger pinch in the touches the host
 *     forwards with uc_webview_touch(), once the host sets the pinch callback
 *     (uc_webview_new_sized()). It sees only the touches the page did not
 *     handle: a page with a pinch of its own (a map) keeps it. While two
 *     fingers are down the page gets its DOM touch events as before, but no
 *     tap (click) and no scroll; the remaining finger of a pinch neither taps
 *     nor scrolls until every finger is up. A two-finger pan or tap does
 *     nothing.
 *  2. pinch(BEGIN, 1.0, x, y), pinch(UPDATE, scale, x, y) for every finger
 *     move, pinch(END, scale, x, y) when one of the two fingers lifts (the
 *     values of the last UPDATE), pinch(CANCEL, ...) when the gesture is
 *     abandoned (a touch cancel, the page took the touches over, a navigation
 *     committed). scale is the factor since BEGIN; (x, y) the midpoint of the
 *     two fingers in view pixels. The host scales its last frame on the GPU
 *     meanwhile: smooth, but blurry, and no new frame is needed.
 *  3. At END the host calls uc_webview_set_magnification() once with the
 *     magnification it wants (its own limits) and the point that stays where
 *     it is, then uc_webview_when_rendered(), and keeps showing its scaled
 *     frame until DONE: the first frame after DONE has the new scale, sharp.
 *     On TIMEOUT or CANCELLED it shows the next frame as it comes.
 * The magnification is WebKit's page scale: the page is drawn larger at the
 * same layout width (text keeps its line breaks), unlike the page zoom of
 * uc_webview_set_zoom(), which lays the page out again; the two multiply. Input stays in view pixels: WebKit
 * maps touches and clicks through the scale itself.
 * Navigation: a new page starts at 1.0 (WebKit resets the scale when a load
 * commits, WebPage.cpp:8233-8250); back, forward, reload and same-document
 * history steps restore the magnification the page had when it was left, with
 * its scroll position (WebKit keeps both per history item,
 * HistoryController.cpp:133, 192-193). Both arrive as magnification_changed().
 *
 * Requests (library 0.7.0)
 * ========================
 * A page asks for things a browser shows outside the page. With the request
 * callbacks set (uc_webview_new_sized()) the host gets them; without them the
 * library decides alone (see the callbacks for each default):
 *  - script_dialog(): alert(), confirm(), prompt() and the "leave this page?"
 *    question of a beforeunload handler. The page's script waits until the
 *    host answers with uc_webview_request_reply(): WebKit runs these dialogs
 *    as synchronous calls, so no script, timer or rendering update of the page
 *    runs meanwhile (uc_webview_when_rendered() waits as well).
 *  - authenticate(): HTTP Basic, Digest and the other login schemes of a server
 *    or proxy; the host answers with uc_webview_request_auth() (credentials, or
 *    none: the server's 401 page loads). Credentials are kept for the session
 *    (the process) only and never logged; client certificate and PIN requests
 *    never reach the host (continued without a certificate).
 *  - new_window(): window.open() and target=_blank. http and https load into
 *    this view (the page's window.open() returns null); other schemes are
 *    refused. close_requested(): window.close() of the page.
 *  - response_refused(): a download (Content-Disposition: attachment, <a
 *    download>) or a document WebKit cannot show (an unknown type, a PDF while
 *    the PDF viewer is off) in the main frame. Nothing is saved, the page stays,
 *    and no load_changed(UC_WEBVIEW_LOAD_FAILED) is reported for it.
 * Every script dialog and authentication request has an id (uint64_t, unique in
 * the process, never 0) and ends exactly once: by the host's answer, or by
 * request_cancelled() when the request went away first, after which the host
 * closes its dialog without answering. A request ends without request_cancelled
 * when the host answers it and when uc_webview_free() frees the view. The
 * library ends pending requests itself:
 *  - before a navigation call of the host (uc_webview_load_uri(), _load_html(),
 *    _reload(), _stop(), _go_back(), _go_forward()): dialogs get their "no"
 *    answer, authentication requests are cancelled; request_cancelled() follows
 *    from the main context (a pending beforeunload question answered this way
 *    means "stay": the navigation that asked it stops, the host's goes on).
 *    The page's script runs on after the dialog's "no" until the web process
 *    takes the new load; dialogs it opens meanwhile (not beforeunload) close at
 *    once with WebKit's default, until the next load event, at most 2 s;
 *  - when a main-frame load commits (dialogs of the old document);
 *  - when the web process terminates (all, before web_process_terminated());
 *  - when WebKit gives up an authentication request: at the start, the end or
 *    the failure of the next main-frame load, all pending ones.
 * An answer to an ended or unknown id returns -1 and does nothing, so a late
 * answer racing request_cancelled() is harmless. Strings passed to the request
 * callbacks are valid during the call only, like all callback strings.
 *
 * Site settings (library 0.8.0)
 * =============================
 * What a host configures per site (a remote-ui web view entity) or for the
 * whole process, on top of the address:
 *  - uc_webview_set_origin_headers(): extra request headers for one origin
 *    (a bearer token, an API key), added by the library's web process
 *    extension to the page, its subresources, fetch() and XMLHttpRequest;
 *  - uc_webview_set_user_agent(): the User-Agent of one view;
 *  - uc_webview_global_set_cookies(): cookies in the cookie jar of the
 *    process' network session;
 *  - uc_webview_global_allow_certificate(): a TLS certificate exception for
 *    one host, uc_webview_global_set_tls_errors_policy() for all hosts;
 *  - uc_webview_global_set_color_scheme(): prefers-color-scheme of every page;
 *  - uc_webview_global_clear_data(): the HTTP cache or all website data.
 * The process has one network session, one display and one set of system
 * settings, so the uc_webview_global_* functions act on every view. With a
 * data_dir (uc_webview_global_init()) cookies persist across restarts since
 * library 0.8.0 (data_dir/cookies.sqlite), as local storage, IndexedDB and the
 * HTTP cache did before. The asynchronous calls report with a
 * uc_webview_done_cb from the main context, never from inside a uc_webview_*
 * call. Secrets (header values, cookie values) are never logged.
 *
 * Device state for web pages (library 0.9.0)
 * ==========================================
 * A view whose host switches the device API on (uc_webview_set_device_api())
 * gives every page of its top frame, from the page's start, the state of the
 * device as the host reports it (uc_webview_set_device_state(),
 * uc_webview_device_event()):
 *  - navigator.getBattery(): a BatteryManager as in the W3C Battery Status
 *    API (WebKit has none);
 *  - window.deviceHost: version 1, screen ("on", "dim", "off"), powerSource
 *    ("ac", "battery") and powerProfile ("power-saver", "balanced",
 *    "performance"), with the events suspend, resume (detail.sleptMs), on-ac,
 *    on-battery, screenchange and powerprofilechange;
 *  - the events freeze and resume on document around a standby;
 *  - <html data-screen data-power-profile> for CSS.
 * Pages of a view without it have none of these. The package README ("Device
 * state for web pages") has the page side. Independent of it, every view's
 * hidden page (uc_webview_set_mapped(0)) is throttled since 0.9.0.
 */
#ifndef UC_WEBVIEW_H
#define UC_WEBVIEW_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UC_WEBVIEW_API_VERSION 1

/* Soname of the library the hosts dlopen(). */
#define UC_WEBVIEW_SONAME "libucwebview.so.1"

typedef struct uc_webview uc_webview;

/*
 * One rendered frame, allocated by the library. All fields are read-only for
 * the host and valid until uc_webview_frame_release(handle). The layout is
 * fixed for API version 1 (see "Compatibility").
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

/* pinch() phases (library 0.6.0, see "Pinch zoom") */
#define UC_WEBVIEW_PINCH_BEGIN  0   /* scale 1.0: the fingers moved apart or together by the drag threshold */
#define UC_WEBVIEW_PINCH_UPDATE 1
#define UC_WEBVIEW_PINCH_END    2   /* one of the two fingers lifted: scale and midpoint of the last UPDATE */
#define UC_WEBVIEW_PINCH_CANCEL 3   /* abandoned: forget the gesture */

/* uc_webview_set_magnification() range (library 0.6.0); hosts pick their own limits inside it */
#define UC_WEBVIEW_MAGNIFICATION_MIN 0.25
#define UC_WEBVIEW_MAGNIFICATION_MAX 10.0

/* script_dialog() types (library 0.7.0; values of WebKitScriptDialogType, WebKitScriptDialog.h.in:44-49) */
#define UC_WEBVIEW_DIALOG_ALERT         0   /* uc_webview_request_reply(): any answer closes it */
#define UC_WEBVIEW_DIALOG_CONFIRM       1   /* accept=1: confirm() returns true, 0: false */
#define UC_WEBVIEW_DIALOG_PROMPT        2   /* accept=1: prompt() returns text (NULL = ""), 0: null */
#define UC_WEBVIEW_DIALOG_BEFORE_UNLOAD 3   /* "leave this page?": accept=1 leave, 0 stay; message is NULL, show a
                                               fixed text (pages cannot word it, browsers stopped showing their text) */

/* authenticate() schemes (library 0.7.0; values of WebKitAuthenticationScheme, WebKitAuthenticationRequest.h.in:73-84).
 * Client certificate (7) and PIN (9) requests never reach the host. */
#define UC_WEBVIEW_AUTH_SCHEME_DEFAULT      1
#define UC_WEBVIEW_AUTH_SCHEME_HTTP_BASIC   2   /* the password travels unencrypted unless the origin is https */
#define UC_WEBVIEW_AUTH_SCHEME_HTTP_DIGEST  3
#define UC_WEBVIEW_AUTH_SCHEME_HTML_FORM    4
#define UC_WEBVIEW_AUTH_SCHEME_NTLM         5
#define UC_WEBVIEW_AUTH_SCHEME_NEGOTIATE    6
#define UC_WEBVIEW_AUTH_SCHEME_SERVER_TRUST 8
#define UC_WEBVIEW_AUTH_SCHEME_UNKNOWN      100

/* authenticate() flags (library 0.7.0) */
#define UC_WEBVIEW_AUTH_RETRY (1u << 0)     /* the previous credentials for this request were rejected */
#define UC_WEBVIEW_AUTH_PROXY (1u << 1)     /* a proxy asks, origin is the proxy's */

/* new_window() results (library 0.7.0) */
#define UC_WEBVIEW_NEW_WINDOW_LOADED  0     /* http(s): the page loads into this view */
#define UC_WEBVIEW_NEW_WINDOW_REFUSED 1     /* data:, javascript:, blob:, file:, about: and the like, or a form posted
                                               to a new window: nothing happens */

/* response_refused() reasons (library 0.7.0) */
#define UC_WEBVIEW_RESPONSE_DOWNLOAD    0   /* Content-Disposition: attachment or <a download>: nothing is saved */
#define UC_WEBVIEW_RESPONSE_UNSUPPORTED 1   /* a type WebKit cannot show, or a PDF while the PDF viewer is off */

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
 * Host callbacks. Zero-initialise the struct and set the fields the host
 * handles; every field may be NULL. Later releases append callbacks, which
 * reach the library only through uc_webview_new_sized() (see
 * "Compatibility"). All run on the UI thread from the GLib main context;
 * strings are owned by the library and valid only during the call. Do not
 * call uc_webview_free() from inside a callback.
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
     * while focused. The host shows/hides its keyboard here. Since library 0.7.5 also focused=0 when
     * the element's web process is gone: at a navigation to another site (before its COMMITTED) and
     * before web_process_terminated (package README, "Text input (IME)"). */
    void (*ime_focus)(uc_webview *, int focused, unsigned hints, unsigned purpose,
                      const char *surrounding, int cursor, void *user);
    /* Caret rectangle in view pixels (for keyboards that avoid covering the field). */
    void (*ime_cursor_area)(uc_webview *, int x, int y, int w, int h, void *user);
    /* CSS cursor name requested by the page ("default", "pointer", "text", ...). */
    void (*cursor_changed)(uc_webview *, const char *css_name, void *user);
    /* Surrounding text update while an editable element is focused: text around the caret, caret
     * character offset and selection anchor offset (equal to cursor without a selection). */
    void (*ime_surrounding)(uc_webview *, const char *text, int cursor, int anchor, void *user);

    /* ---- library 0.6.0: only through uc_webview_new_sized(), see "Compatibility" ---- */

    /* A two-finger pinch on the page, see "Pinch zoom": phase UC_WEBVIEW_PINCH_*, scale the factor since BEGIN
     * (1.0 at BEGIN), (x, y) the midpoint of the two fingers in view pixels. Setting it switches the library's pinch
     * recognition on for the view: two fingers then neither tap nor scroll. NULL keeps the behaviour of 0.5.0 (the
     * first finger taps or scrolls, the second one is ignored), also for every host of an older header. */
    void (*pinch)(uc_webview *, int phase, double scale, double x, double y, void *user);
    /* The view's magnification changed without uc_webview_set_magnification(): a main-frame navigation committed
     * (a new page starts at 1.0), or a history step restored the magnification the page had when it was left
     * (back, forward, reload, a same-document history step). The new value, as uc_webview_get_magnification()
     * reports it from now on. */
    void (*magnification_changed)(uc_webview *, double magnification, void *user);

    /* ---- library 0.7.0: only through uc_webview_new_sized(), see "Requests" and "Compatibility" ---- */

    /* A script dialog: type UC_WEBVIEW_DIALOG_*; origin "scheme://host[:port]" of the page's last committed main-frame
     * URI (NULL for pages without a host: about:blank, data:, uc_webview_load_html() without a base URI); WebKit does
     * not say which frame asked, so a frame of another origin shows the main page's. message as the page wrote it,
     * any length (NULL for BEFORE_UNLOAD); default_text the prompt's preset text (NULL for other types). Answer with
     * uc_webview_request_reply(), from here or later; until then the page waits. Not set: alert, confirm and prompt
     * close at once (WebKit's default: confirm() false, prompt() null) and BEFORE_UNLOAD leaves the page (library
     * default; WebKit's own would stay). Dialog loop guard: after three dialogs of one document that each came less
     * than 1 s after the previous answer, further dialogs of that document close at once with WebKit's default, until
     * the user taps or presses a key in the page (library 0.7.2, see "input") or the next page commits (beforeunload
     * excepted). */
    void (*script_dialog)(uc_webview *, uint64_t request, int type, const char *origin,
                          const char *message, const char *default_text, void *user);
    /* A server or proxy asks for a login: origin "scheme://host[:port]" of the server (of the proxy with
     * UC_WEBVIEW_AUTH_PROXY), realm the server's text for the protected area (may be "" or NULL), scheme
     * UC_WEBVIEW_AUTH_SCHEME_*, flags UC_WEBVIEW_AUTH_*. Answer with uc_webview_request_auth(). Several can be
     * pending (a page with several protected parts). Not set: the load continues without credentials and shows the
     * server's 401 page (library default; WebKit's own would hang until the next load). */
    void (*authenticate)(uc_webview *, uint64_t request, const char *origin, const char *realm,
                         int scheme, unsigned flags, void *user);
    /* A pending script dialog or authentication request went away before the host answered (see "Requests"): close
     * its dialog and do not answer it (an answer now returns -1). Never called from inside uc_webview_free(). */
    void (*request_cancelled)(uc_webview *, uint64_t request, void *user);
    /* The page called window.close() and WebKit allows it (a page opened by script, or one with at most one history
     * entry). The library does nothing else; a host may leave the page or go back. */
    void (*close_requested)(uc_webview *, void *user);
    /* The page asked for a new window (window.open(), target=_blank) with uri: UC_WEBVIEW_NEW_WINDOW_LOADED, it loads
     * into this view now (load_changed follows); UC_WEBVIEW_NEW_WINDOW_REFUSED, nothing happens. The page's
     * window.open() returns null either way. The same happens without the callback. */
    void (*new_window)(uc_webview *, const char *uri, int result, void *user);
    /* The main frame got a response the library does not show, reason UC_WEBVIEW_RESPONSE_*, mime_type as the server
     * sent it (NULL when not known yet, e.g. for <a download>). The page stays, nothing is saved; for a load the host
     * started, STARTED and FINISHED (with the URI of the page that stays) come without FAILED. The same happens without
     * the callback. Frames: refused silently. */
    void (*response_refused)(uc_webview *, const char *uri, const char *mime_type, int reason, void *user);
} uc_webview_callbacks;

/* ---- process-wide ------------------------------------------------------- */

/* Any thread. Returns UC_WEBVIEW_API_VERSION of the loaded library. */
int uc_webview_api_version(void);

/*
 * UI thread, once per process, before the first uc_webview_new(). Creates the
 * in-process WPEDisplay and the shared WebKit network session.
 *   egl_display  the host's EGLDisplay (Qt: nativeResourceForIntegration("egldisplay")); frames are
 *                imported on it. NULL selects the shared memory path (frames carry pixels, no EGLImage).
 *   data_dir     website data directory (local storage, IndexedDB, ...; since library 0.8.0 also the cookies with an
 *                expiry time, data_dir/cookies.sqlite); NULL or unwritable = ephemeral session.
 *   cache_dir    HTTP cache directory; NULL = data_dir/cache (ignored for an ephemeral session).
 * Returns 0 on success, -1 if the display could not be connected (EGL display unusable), 0 when
 * already initialised (the arguments of the second call are ignored).
 * Environment honoured: WPE_DRM_DEVICE, WPE_BUFFER_FORMAT, WPE_USE_EXPLICIT_SYNC, LIBGL_ALWAYS_SOFTWARE
 * (WPEPlatform), WEBKIT_INSPECTOR_SERVER (needs UC_WEBVIEW_DEVELOPER_EXTRAS=1), UC_WEBVIEW_CONSOLE_LOG=1
 * (page console to stderr), JavaScriptCoreUseJIT=0 (sandboxes with MemoryDenyWriteExecute),
 * UC_WEBVIEW_ALLOW_FILE_URLS=1 (lets uc_webview_load_uri()/load_html() use file: URIs; read here, once),
 * UC_WEBVIEW_REFRESH_RATE=<Hz> (display refresh rate, wins over uc_webview_global_set_refresh_rate(); read here,
 * once), UC_WEBVIEW_DRAG_THRESHOLD=<px> (touch drag threshold, see uc_webview_global_get_drag_threshold()),
 * UC_WEBVIEW_TRACE=1 (log lines about the display refresh ticks, key events, text input, the results of
 * uc_webview_when_rendered(), pinch begin and end and magnification changes with their render time, and requests by
 * kind, id and outcome; never with typed text, dialog texts, answers, user names or passwords, see the package
 * README "What the logs carry"),
 * UC_WEBVIEW_LOG_FULL_URLS=1 (log lines show URIs with query, fragment and data: payload; without it only scheme,
 * host, port and path; credentials never),
 * UC_WEBVIEW_PDF_VIEWER=1 (keeps WebKit's PDF viewer on, for crash investigations; read here, once). Since library
 * 0.6.1 the viewer is off: a PDF in the main frame is refused like a download, since 0.7.0 as
 * response_refused(UC_WEBVIEW_RESPONSE_UNSUPPORTED) without load_changed(UC_WEBVIEW_LOAD_FAILED) (0.6.1 reported
 * FAILED with the PDF's URI); a PDF in a frame stays empty.
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

/*
 * UI thread. Added in library 0.3.0 within API version 1: hosts resolve it with dlsym(); an older library lacks it
 * and leaves WPE's 8 px unless UC_WEBVIEW_DRAG_THRESHOLD is set.
 *
 * The touch drag threshold of the library's display in view pixels: WebKit's gesture recogniser turns a touch
 * sequence into a scroll instead of a click once a move is farther than this from the press position. 16 px
 * (measured on the remote's panel, see the package README) unless UC_WEBVIEW_DRAG_THRESHOLD=<px> (1..200) is set;
 * uc_webview_global_init() reads it once. A host that tells taps from drags itself (remote-ui asks for the
 * keyboard on a tap into a focused field) uses this value. Before uc_webview_global_init() it returns the value
 * the environment will give.
 */
int uc_webview_global_get_drag_threshold(void);

/* ---- view lifecycle ------------------------------------------------------ */

/*
 * UI thread. Creates a web view of width x height device pixels (minimum 1x1) that is mapped (visible to
 * the page) and focused; use uc_webview_set_mapped()/set_focused() to change that. cb is copied, but only its
 * nine API version 1 callbacks (frame_ready .. ime_surrounding), also when the host's header has more: a host
 * that sets later callbacks uses uc_webview_new_sized(). cb NULL = no callbacks. user is passed to every
 * callback. Returns NULL before uc_webview_global_init() or on failure.
 */
uc_webview *uc_webview_new(int width, int height, const uc_webview_callbacks *cb, void *user);

/*
 * UI thread. Added in library 0.4.0 within API version 1: hosts resolve it with dlsym() and call uc_webview_new()
 * with the same arguments but cb_size when an older library lacks it (such a library knows only the API version 1
 * callbacks and reads just those).
 *
 * uc_webview_new() for a callback struct of cb_size bytes: pass sizeof(uc_webview_callbacks) as compiled into the
 * host. The library copies min(cb_size, its own sizeof(uc_webview_callbacks)) bytes, so it reads nothing past the
 * host's struct, and treats the callbacks beyond cb_size as NULL; callbacks the host's newer header has and the
 * library does not are never called (logged once per process). cb NULL = no callbacks, cb_size is then ignored.
 * A cb_size smaller than the nine API version 1 callbacks is refused with a warning in the log: NULL. Otherwise
 * as uc_webview_new().
 */
uc_webview *uc_webview_new_sized(int width, int height, const uc_webview_callbacks *cb, size_t cb_size, void *user);

/* UI thread, not from a callback. Releases held frames, destroys the WebKitWebView. Pending requests end without a
 * callback (script dialogs with their "no" answer, authentication requests cancelled; library 0.7.0). NULL is
 * ignored. */
void uc_webview_free(uc_webview *);

/* UI thread. New size in device pixels (minimum 1x1); the next frame has the new size. */
void uc_webview_resize(uc_webview *, int width, int height);

/* UI thread. mapped=0 tells WebKit the view is hidden (display off, item invisible): the page's document becomes
 * hidden (visibilitychange), rendering and requestAnimationFrame stop, and since library 0.9.0 its timers fire at most
 * once a second and its CSS animations stop (WebKit's hidden page throttling; up to 0.8.x it was off on WPE and a
 * hidden page's timers kept their rate). mapped=1 resumes and triggers a new frame. */
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
/* UI thread. The navigation calls end pending script dialogs and authentication requests first (library 0.7.0,
 * see "Requests"); go_back/go_forward only when there is somewhere to go. */
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

/* UI thread. Page zoom factor (1.0 = 100 %, > 0): CSS pixels grow and the page is laid out again. WebKit keeps it
 * for the view across navigations. Independent of the magnification below; the two multiply. */
void uc_webview_set_zoom(uc_webview *, double zoom);

/*
 * UI thread. Added in library 0.6.0 within API version 1: hosts resolve it with dlsym() and offer no pinch zoom when
 * an older library lacks it.
 *
 * Sets the magnification (WebKit's page scale, see "Pinch zoom") to scale, keeping the content at (x, y) where it is.
 * scale: UC_WEBVIEW_MAGNIFICATION_MIN .. _MAX (1.0 = none); a host keeps its own limits inside (remote-ui: 1 to 4).
 * (x, y): the focal point in view pixels, clamped into the view. The content under it stays there within a CSS
 * pixel, unless the page cannot scroll that far: near the page's edges the content moves (WebKit clamps the scroll
 * position). The view centre is no special case: WebKit's own webkit_web_view_set_magnification() keeps no point
 * fixed in 2.54 (WebPage.cpp:2960-2970 scrolls to scale * (scroll - centre)), the library sets the scroll position
 * itself (README, "Pinch zoom").
 * Returns 0 when the change went to the page, also when scale is the current magnification (then nothing is sent),
 * -1 when refused (invalid view, another thread, scale out of range or not a number).
 * Cost: WebKit rebuilds the page's style and repaints it at the new scale (Page.cpp:1684-1741); the layout width stays
 * the view's (ScrollView.cpp:367-370), so line breaks stay, while innerWidth and visualViewport report the smaller
 * visible part. On a large page that takes seconds. Call uc_webview_when_rendered() right after it to learn when the new scale is on
 * screen; it runs after the change. No magnification_changed() follows for the call itself. The library runs two
 * small scripts in its own script world around WebKit's change (it reads and sets the scroll position); like every
 * script of WebKit's API they count as a user gesture for the page (see uc_webview_when_rendered()).
 */
int uc_webview_set_magnification(uc_webview *, double scale, double x, double y);

/* UI thread. Added in library 0.6.0. The view's magnification: 1.0, the last uc_webview_set_magnification() or what
 * magnification_changed() reported last. 0.0 for an invalid view. */
double uc_webview_get_magnification(uc_webview *);

/* UI thread. Colour shown where the page is transparent; components 0..1. Default opaque white (until library 0.6.1
 * opaque black), like WebKit and the browsers: a page without a background of its own expects white behind its text. */
void uc_webview_set_background_rgba(uc_webview *, float r, float g, float b, float a);

/* ---- rendering ----------------------------------------------------------- */

/* uc_webview_when_rendered() results (library 0.5.0) */
#define UC_WEBVIEW_RENDERED_DONE      0   /* a rendering update after the call finished and a frame reached the host */
#define UC_WEBVIEW_RENDERED_TIMEOUT   1   /* no rendering update within 60 s (view unmapped, page busy or hung) */
#define UC_WEBVIEW_RENDERED_CANCELLED 2   /* the document went away or the request could not run, see below */

/* The result of one uc_webview_when_rendered() request: status is UC_WEBVIEW_RENDERED_*, user the request's. */
typedef void (*uc_webview_rendered_cb)(uc_webview *, int status, void *user);

/*
 * UI thread. Added in library 0.5.0 within API version 1: hosts resolve it with dlsym() and use a heuristic of
 * their own when an older library lacks it.
 *
 * Calls cb(wv, status, user) exactly once, on the UI thread from the main context, when the page has finished a
 * whole rendering update that started after this call (style, layout and paint of everything requested before the
 * call, e.g. the relayout of a uc_webview_set_zoom() made just before it) and a frame has reached the host since
 * the call: status UC_WEBVIEW_RENDERED_DONE. For a busy indicator while a zoom step re-renders a large page.
 *
 * How: the library runs a small script in a script world of its own, which the page's scripts can neither see
 * nor change, and the script reports back after two requestAnimationFrame callbacks. WebKit runs the script after
 * every message the view sent to the page before it (a zoom rebuilds the style and lays the page out right in its
 * message, LocalFrame.cpp:1085-1130), runs requestAnimationFrame callbacks at the start of a rendering update
 * (Page.cpp:2329-2337) and starts the next update only after the compositor took the previous one
 * (LayerTreeHost.cpp:141-153, 438-457): the second callback means painted and composited. The update's frame can
 * reach the host shortly after the report (WebKit waits for its GPU fence, AcceleratedBackingStore.cpp:194-210),
 * so when no frame came since the call the library waits for the next one, at most 100 ms (an update that changes
 * nothing produces no frame). On a page that animates, a frame already on its way can count: DONE may come one
 * frame early.
 *
 * Other results:
 *  - UC_WEBVIEW_RENDERED_TIMEOUT 60 s after the call (15 s up to library 0.5.x): an unmapped view does not render
 *    (uc_webview_set_mapped(), the request completes when the view is mapped again within the time), a page keeps
 *    the web process busy. A zoom step of a large page took 11 to 15 s on a Remote 3 at 408 MHz (the low power
 *    mode), so 15 s was too short. A host shows the next frame as it comes after a timeout.
 *  - UC_WEBVIEW_RENDERED_CANCELLED when a navigation commits (reported before load_changed(COMMITTED): the
 *    document the request waited in is gone), the web process terminates (before web_process_terminated()), the
 *    page refuses the script, or the view is freed. For the last one cb runs inside uc_webview_free() (see the
 *    threading contract) and must not call any function on that view.
 * Returns 0 when the request is pending, -1 when it is refused (invalid view, cb NULL, another thread, from inside
 * uc_webview_free(), 32 requests of the view pending); cb is never called for a refused request.
 *
 * The script counts as a user gesture for the page, like every script of WebKit's JavaScript API (WebKit forces
 * one, WebKitWebView.cpp:4494-4502, ScriptController.cpp:654-663): the page gets sticky and history-action user
 * activation (LocalDOMWindow.h:145-150); the transient activation ends with the script. UC_WEBVIEW_TRACE=1 logs
 * one line per result with the time since the call, "ucwebview: trace <view>: rendered after 2840 ms (done)".
 */
int uc_webview_when_rendered(uc_webview *, uc_webview_rendered_cb cb, void *user);

/* ---- requests (library 0.7.0) ------------------------------------------------ */

/*
 * UI thread. Added in library 0.7.0 within API version 1: hosts resolve it with dlsym() and set the request callbacks
 * only when it exists (an older library ignores them anyway). May be called from inside a callback, also from the
 * request's own script_dialog().
 *
 * Answers a pending request (see "Requests"). Script dialogs: accept=1 is the host's yes: alert closed, confirm()
 * true, prompt() returns text (UTF-8; NULL = ""), beforeunload leaves the page. accept=0 is its no: alert closed,
 * confirm() false, prompt() null, beforeunload stays. text is used for prompts only. Authentication requests:
 * accept=0 continues without credentials (the server's 401 page loads), accept=1 is refused (use
 * uc_webview_request_auth()).
 * Returns 0 when answered, -1 when refused: invalid view, another thread, an unknown or ended id (also one that
 * request_cancelled() reported or is about to report), accept=1 for an authentication request, text not UTF-8 (the
 * request stays pending then).
 */
int uc_webview_request_reply(uc_webview *, uint64_t request, int accept, const char *text);

/*
 * UI thread. Added in library 0.7.0, like uc_webview_request_reply().
 *
 * Answers a pending authentication request: user and password (UTF-8; password NULL = "") go to the server; user
 * NULL continues without credentials, the server's 401 page loads (password is ignored). Credentials are kept for
 * the session only (WebKit's credential storage is in memory; nothing is written, the library never asks WebKit to
 * save them) and never logged. When the server rejects them, authenticate() comes again with UC_WEBVIEW_AUTH_RETRY.
 * Returns 0 when answered, -1 when refused: invalid view, another thread, an unknown or ended id, a script dialog's
 * id, a string that is not UTF-8 (the request stays pending then).
 */
int uc_webview_request_auth(uc_webview *, uint64_t request, const char *user, const char *password);

/* ---- site settings (library 0.8.0) --------------------------------------- */
/*
 * Every function of this section was added in library 0.8.0 within API version 1: hosts resolve each with dlsym()
 * and leave the setting alone (WebKit's default) when an older library lacks it. They return 0 when the setting is
 * taken and -1 when it is refused (a warning names the reason, never a secret); a refused call changes nothing.
 */

/* The end of an asynchronous request (uc_webview_global_set_cookies(), uc_webview_global_clear_data()): status
 * UC_WEBVIEW_DONE_OK or UC_WEBVIEW_DONE_FAILED, user as passed. Called exactly once for an accepted request, on the
 * UI thread from the main context, never from inside a uc_webview_* call; never for a refused one. */
typedef void (*uc_webview_done_cb)(int status, void *user);
#define UC_WEBVIEW_DONE_OK     0
#define UC_WEBVIEW_DONE_FAILED 1

/*
 * UI thread. Extra request headers for the requests of this view to origin: names[i]: values[i], i < count;
 * count 0 removes the view's headers for origin. For the "bearer" and "header" authentication of a site:
 * {"Authorization"}, {"Bearer <token>"}, or an API key header.
 *   origin  "http(s)://host[:port]" (an http or https URL is reduced to its origin; the default port is left out,
 *           so "https://host:443" is "https://host")
 *   names   header names (RFC 9110 tokens of at most 64 bytes); not Host, Cookie (use
 *           uc_webview_global_set_cookies()), Cookie2, Content-Length, Content-Type, Connection, Keep-Alive,
 *           Transfer-Encoding, Upgrade, TE, Trailer, Expect, Origin, Referer, User-Agent (use
 *           uc_webview_set_user_agent()) and no name starting with Proxy- or Sec- (case-insensitive; library 0.8.3,
 *           the Core-API's list and a few more); no name twice
 *   values  visible ASCII, spaces and tabs, at most 8192 bytes, no white space at either end; never logged
 * At most 32 headers per origin. The strings are copied. A later call for the same origin replaces the headers.
 * What gets them (the library's web process extension decides per request, WebKitWebPage::send-request):
 *  - the main resource of a document of origin, its subresources, and its fetch() and XMLHttpRequest calls to
 *    origin, while the view's top-level document is of origin. A redirect of another origin into origin gets them
 *    only where WebKit starts the redirected load in another web process, with origin's URI as the page URI: on WPE
 *    a cross-site redirect from a page whose web process has committed a document. A redirect in one process
 *    (same-site, the view's first navigation, the first load of a freshly started web process) starts origin's page
 *    without them (library 0.8.3: before, every request whose redirecting response had the page URI got them, also
 *    an image or frame of another site's page that loads its own URL, redirected into origin);
 *  - not a request of a page of another site to origin (an image, a frame, a fetch), not a request whose Origin
 *    header names another origin (a form another site posts to origin), not a request to another origin, and not
 *    the next hop of a redirect away from origin (the library removes the headers WebKit carries over);
 *  - they replace a header of the same name the page sets itself;
 *  - since library 0.8.1 a request that does not get them loses every header with the name and the exact value of
 *    one of them: WebKit copies headers into a redirect, and at a redirect to another site WPE moves the load to
 *    another web process, where it starts as a first request (0.8.0 leaked custom headers there). A page of another
 *    site that sends exactly a configured value itself loses it too.
 * While the main frame loads a new page of origin, the old document still runs and its requests to origin are
 * judged by the new page URI (WebKit sets it when the load starts); on WPE that old document is of the same site,
 * else it runs in another web process.
 * The headers take effect for the requests after the call: set them before uc_webview_load_uri(). Not covered:
 * WebSocket handshakes, requests of service workers and of the WebKit network process itself (favicon, DNS
 * prefetch), and the redirects of a keep-alive request (sendBeacon(), fetch(..., {keepalive: true})) that WebKit's
 * network process completes after its page went away, which no web process sees. Every web process gets the headers
 * of every view's origins in its memory (the library cannot tell which process a site will use); a page never sees
 * them, it can only send requests that carry them.
 * The headers of all views of the process go into one table; when two views set headers for the same origin, the
 * later call counts for both until one of them changes or is freed. uc_webview_free() removes the view's headers.
 * Returns 0, -1 when refused: invalid view, another thread, origin not http(s), an invalid name or value, more than
 * 32 headers, or the library's web process extension is not installed (then no headers are sent at all).
 */
int uc_webview_set_origin_headers(uc_webview *, const char *origin, const char *const *names,
                                  const char *const *values, size_t count);

/* uc_webview_set_user_agent() presets */
#define UC_WEBVIEW_USER_AGENT_DEFAULT 0   /* the Remote's: WebKit's own with the token UCRemote/<firmware version>
                                             ("Mozilla/5.0 (X11; Linux aarch64) AppleWebKit/605.1.15 (KHTML, like
                                             Gecko) Version/60.5 Safari/605.1.15 UCRemote/2.13.2" on the remote; up
                                             to library 0.8.1 WebKit's own without the token) */
#define UC_WEBVIEW_USER_AGENT_MOBILE  1   /* Safari on an iPhone: sites serve their phone layout */
#define UC_WEBVIEW_USER_AGENT_DESKTOP 2   /* Safari on a Mac: sites serve their desktop layout */
#define UC_WEBVIEW_USER_AGENT_CUSTOM  3   /* the custom text */

/*
 * UI thread. The User-Agent the view sends (and navigator.userAgent) from the next request on; reload or load the
 * page for a page that is already shown. preset UC_WEBVIEW_USER_AGENT_*; custom is used for CUSTOM only: 1..512
 * bytes of visible ASCII and spaces. The setting belongs to the view (the view gets WebKit settings of its own,
 * with the library's other settings unchanged).
 * DEFAULT, also the user agent of a view that never calls this function (library 0.8.2), identifies the Remote and
 * its firmware version: WebKit's own user agent, which keeps WebKit's platform part, followed by the product token
 * "UCRemote/<version>". The version is UC_OS_VERSION of the firmware's /etc/uc-release when the process can read it,
 * else UC_OS_VERSION of the environment (remote-ui's units load the file with EnvironmentFile=, their sandbox hides
 * the file itself); without a valid version (1..64 RFC 9110 tchar bytes) the token is "UCRemote". Read once per
 * process, by uc_webview_global_init(), which logs the result. MOBILE, DESKTOP and CUSTOM carry no token.
 * Returns 0, -1 when refused: invalid view, another thread, unknown preset, CUSTOM without a valid text.
 */
int uc_webview_set_user_agent(uc_webview *, int preset, const char *custom);

/*
 * One cookie of uc_webview_global_set_cookies(). The host allocates arrays of it; the layout is fixed for API
 * version 1 (see "Compatibility").
 *   name, value  name: an RFC 6265 token of 1..256 bytes; value: at most 4096 bytes of RFC 6265 cookie-octets
 *                (visible ASCII without '"', ',', ';' and '\'), optionally in double quotes; "" is valid
 *                (library 0.8.3; before, other bytes passed and WebKit dropped such a cookie silently). The value is
 *                never logged.
 *   domain       NULL or "": a host-only cookie of the URL's host. Else the URL's host or a domain it is in
 *                ("example.com" and ".example.com" both mean the domain and its subdomains, as the Domain attribute
 *                does); an IP address only as itself.
 *   path         NULL or "": "/". Else starts with '/', ASCII without ';' or control characters.
 *   secure       1: sent over https only. http_only 1: hidden from the page's scripts (document.cookie).
 *   expires      seconds since 1970-01-01 UTC; -1: a session cookie (until the host process ends; not written to
 *                the data directory). A time in the past deletes the cookie of that name, domain and path.
 * SameSite is Lax (libsoup's default since 3.4.0).
 */
typedef struct {
    const char *name, *value, *domain, *path;
    int secure, http_only;
    int64_t expires;
} uc_webview_cookie;

/*
 * UI thread, after uc_webview_global_init(). Puts count cookies for url (an http or https URL of the site they
 * belong to; its host decides host-only cookies and which domains are allowed) into the cookie jar of the process'
 * network session, which every view uses. A cookie with the name, domain and path of an existing one replaces it.
 * With a data_dir (uc_webview_global_init()) cookies with an expiry time are written to data_dir/cookies.sqlite and
 * come back after a restart. Asynchronous: cb (may be NULL) reports when WebKit's network process has them; load
 * the page in cb, a request made before can go without them. count 0: nothing to set, cb reports OK.
 * Returns 0 when the request is accepted, -1 when refused: another thread, before uc_webview_global_init(), url not
 * http(s), cookies NULL with a count, more than 64 cookies, a cookie that breaks a rule above, or a cookie for a public
 * suffix ("com", "co.uk"), which the cookie jar would drop (none is set then).
 * The cookie file is mode 0600 inside the 0700 data directory (library 0.8.3); its database overwrites deleted
 * cookies (libsoup's SQLite secure_delete), but SQLite's temporary journal and the flash storage are not wiped.
 */
int uc_webview_global_set_cookies(const char *url, const uc_webview_cookie *cookies, size_t count,
                                  uc_webview_done_cb cb, void *user);

/*
 * UI thread (may be called before uc_webview_global_init(); the exception is kept). Accepts an invalid TLS
 * certificate of host for page loads: the certificate whose DER encoding has the SHA-256 fingerprint sha256 (64 hex
 * digits; upper case and ':' between bytes, as openssl prints it, are accepted), or any certificate of host with
 * sha256 NULL. host: a host name or an IP address (IPv6 with or without brackets), no scheme, port or path; an
 * exception covers every port of the host. Calls add up: up to 16 fingerprints per host, 64 hosts.
 * How: when a main-frame load of host fails on its certificate (WebKit's load-failed-with-tls-errors), the library
 * checks the certificate against the exceptions, tells WebKit to accept it for host
 * (webkit_network_session_allow_tls_certificate_for_host()) and loads the URI again: the host sees STARTED, FINISHED
 * (no FAILED) and then the new load. Without a match the load fails as before (UC_WEBVIEW_LOAD_FAILED). Once
 * accepted, WebKit trusts that certificate for host for the rest of the process, for subresources too; a
 * subresource of host requested before the main frame accepted its certificate fails. A load the library repeats
 * is a GET of the URI (a form posted to such a host is not sent again). Exceptions cannot be removed.
 * Returns 0 (also for an exception that exists already), -1 when refused: another thread, invalid host or
 * fingerprint, too many.
 */
int uc_webview_global_allow_certificate(const char *host, const char *sha256);

/*
 * UI thread (may be called before uc_webview_global_init(); the value is kept for it). ignore=1: every host's
 * invalid certificate is accepted (WebKit's WEBKIT_TLS_ERRORS_POLICY_IGNORE for the network session); ignore=0
 * (WebKit's default): certificates are checked, uc_webview_global_allow_certificate() exceptions apply. Takes effect
 * for new connections: a connection that is open already (HTTP keep-alive) keeps the decision it was opened with.
 * Returns 0, -1 when refused (another thread).
 */
int uc_webview_global_set_tls_errors_policy(int ignore);

/* uc_webview_global_set_color_scheme() schemes */
#define UC_WEBVIEW_COLOR_SCHEME_LIGHT 0   /* WebKit's default */
#define UC_WEBVIEW_COLOR_SCHEME_DARK  1

/*
 * UI thread (may be called before uc_webview_global_init(); the value is kept for it). The color scheme every page
 * of the process sees: the CSS media feature prefers-color-scheme and the default colors of pages that declare
 * "color-scheme: light dark" (WPE's dark-mode setting, WPE_SETTING_DARK_MODE of the library's display, which WebKit
 * reads for the whole process, SystemSettingsManagerProxyWPE.cpp:103-106). Shown pages follow at once
 * (matchMedia() change events fire). Pages without a color-scheme of their own keep their look: their background is
 * uc_webview_set_background_rgba().
 * Returns 0, -1 when refused: another thread, unknown scheme.
 */
int uc_webview_global_set_color_scheme(int scheme);

/* uc_webview_global_clear_data() types */
#define UC_WEBVIEW_DATA_CACHE 0x1u          /* the memory cache (also pages kept for back/forward and cached web
                                               processes) and the HTTP disk cache */
#define UC_WEBVIEW_DATA_ALL   0xffffffffu   /* all website data: caches, cookies, local and session storage,
                                               IndexedDB, service workers, HSTS, ... (WEBKIT_WEBSITE_DATA_ALL) */

/*
 * UI thread, after uc_webview_global_init(). Clears website data of the process' network session (every view, every
 * site), in memory and in the data directory. types: UC_WEBVIEW_DATA_CACHE or UC_WEBVIEW_DATA_ALL. Shown pages stay
 * as they are; reload them for a fresh start. Not cleared, also with ALL: the credentials of logins answered with
 * uc_webview_request_auth() (kept in memory until the process ends; WebKit 2.54 has no API to drop them), the
 * certificate exceptions and the origin headers. Cleared cookies are overwritten in data_dir/cookies.sqlite (libsoup's
 * SQLite secure_delete); SQLite's temporary journal and the flash storage are not wiped (the 0700 directory and the
 * 0600 file keep them from other users). Asynchronous: cb (may be NULL) reports when WebKit is done.
 * Returns 0 when the request is accepted, -1 when refused: another thread, before uc_webview_global_init(), types
 * not one of the two values (a bit a later release defines).
 */
int uc_webview_global_clear_data(unsigned types, uc_webview_done_cb cb, void *user);

/* ---- device state for web pages (library 0.9.0) ------------------------------ */
/*
 * Every function of this section was added in library 0.9.0 within API version 1: hosts resolve each with dlsym()
 * and give pages no device state when one is missing. They return 0 when taken and -1 when refused (a warning names
 * the reason); a refused call changes nothing.
 *
 * How it reaches the page: while the device API is on, the library's script (device-host-shim.js, compiled in) runs
 * at the start of every document of the view's top frame, in the page's own script world, with the view's current
 * state; frames get nothing. A change of the state or an event goes to the page that is shown, through the script's
 * internal entry point with the values as arguments (WebKit's call_async_javascript_function, never script text), and
 * after every finished load the state goes there once more (a page from the back/forward cache kept the state of its
 * first load). Like every script of WebKit's API, such a delivery counts as a user gesture for the page: it gets
 * sticky user activation (see uc_webview_when_rendered()); the page's event handlers run later, in a task of their
 * own, without transient activation. The page sees events only for values that really change.
 */

/*
 * UI thread. enabled=1 switches the device API of the view on, 0 off (the default). It takes effect at the next page
 * load: a page that is shown keeps what it has (on: none; off: the API, without further updates). Switching it on
 * also sends the current state to a shown page that has the API from an earlier time.
 * Returns 0, -1 when refused: invalid view, another thread.
 */
int uc_webview_set_device_api (uc_webview *, int enabled);

/*
 * UI thread. The device state of the view as a JSON object (UTF-8, at most 4096 bytes), every member optional:
 *   {"screen": "on" | "dim" | "off", "powerSource": "ac" | "battery",
 *    "powerProfile": "power-saver" | "balanced" | "performance", "level": 0 .. 100}
 * level is the battery level in percent, an integer (the page sees 0.42 for 42). The page's battery is charging while
 * powerSource is "ac", also at full level (the Battery Status API's charging), and has no time estimates
 * (chargingTime 0 on power at level 100, else Infinity; dischargingTime Infinity). A member that is left out keeps its
 * value; a member with an invalid value too (a warning names it); unknown members are ignored. A new view starts with
 * screen "on", "ac", "balanced", level 100. The view keeps the state also while the device API is off and starts its
 * pages with it when it is switched on.
 * Returns 0 when json is taken (also with ignored members), -1 when refused: invalid view, another thread, json NULL,
 * longer than 4096 bytes, not UTF-8, not one JSON object (RFC 8259, no member name twice, at most 16 levels deep).
 */
int uc_webview_set_device_state (uc_webview *, const char *json);

/*
 * UI thread. A standby of the device, for the page that is shown while the device API is on (else nothing happens):
 *   "suspend"  right before the standby, after the state of the standby (uc_webview_set_device_state()): document
 *              freeze, then deviceHost suspend. The library does not delay the standby; a page has the time until the
 *              device sleeps (best effort).
 *   "resume"   after the wake, before the new state: document resume, then deviceHost resume with detail.sleptMs =
 *              slept_ms, the time the device slept in milliseconds (0 when unknown).
 * slept_ms is ignored for "suspend".
 * Returns 0, -1 when refused: invalid view, another thread, another type, slept_ms < 0 with "resume".
 */
int uc_webview_device_event (uc_webview *, const char *type, int64_t slept_ms);

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
 * Pass the user's input only: since library 0.7.2 a touch down, a button or
 * key press and text input that reaches a focused element (ime_commit,
 * ime_preedit with text, ime_delete_surrounding) end the dialog loop guard
 * (script_dialog()). Releases, moves and scrolling do not.
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
/* UI thread. Deletes n_chars characters starting offset characters from the caret (offset -1, n 1 = backspace).
 * WebKit counts both in UTF-16 code units, from the start of the selection when there is one (WebPage::
 * deleteSurrounding, documented on WPEInputMethodContext::delete-surrounding since 2.54.2): a character outside the
 * Basic Multilingual Plane, like most emoji, counts 2, as in a QString. n_chars <= 0 does nothing. */
void uc_webview_ime_delete_surrounding(uc_webview *, int offset, int n_chars);

#ifdef __cplusplus
}
#endif

#endif /* UC_WEBVIEW_H */
