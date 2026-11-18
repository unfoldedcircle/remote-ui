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
 * come from new functions that take the frame handle.
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
 * got its focus. The second exception: uc_webview_free() ends the view's
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
 *   data_dir     website data directory (cookies, local storage); NULL or unwritable = ephemeral session.
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
/* UI thread. Deletes n_chars characters starting offset characters from the caret (offset -1, n 1 = backspace). */
void uc_webview_ime_delete_surrounding(uc_webview *, int offset, int n_chars);

#ifdef __cplusplus
}
#endif

#endif /* UC_WEBVIEW_H */
