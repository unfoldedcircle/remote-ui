// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLibrary>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QUrl>

#include <functional>
#include <utility>

#include "uc_webview.h"
#include "webviewsite.h"

namespace uc {
namespace webview {

/**
 * The functions of libucwebview.so.1 (uc_webview.h without the uc_webview_ prefix; new/free are create/destroy).
 * Every pointer is set once the library is available, except the optional ones, which a library of the same API
 * version but an older release lacks (nullptr then). A unit test fills the table with stubs.
 */
struct WebViewFunctions {
    int (*api_version)(void) = nullptr;
    int (*global_init)(void *, const char *, const char *) = nullptr;
    void (*global_shutdown)(void) = nullptr;
    // optional, library 0.2.0: the display refresh rate WebKit paces animations with
    int (*global_set_refresh_rate)(double) = nullptr;
    // optional, library 0.3.0: the touch drag threshold of the library's display (16 px by default)
    int (*global_get_drag_threshold)(void) = nullptr;
    uc_webview *(*create)(int, int, const uc_webview_callbacks *, void *) = nullptr;
    void (*destroy)(uc_webview *) = nullptr;
    void (*resize)(uc_webview *, int, int) = nullptr;
    void (*set_mapped)(uc_webview *, int) = nullptr;
    void (*set_focused)(uc_webview *, int) = nullptr;
    void (*set_scale)(uc_webview *, double) = nullptr;
    void (*load_uri)(uc_webview *, const char *) = nullptr;
    void (*load_html)(uc_webview *, const char *, const char *) = nullptr;
    void (*reload)(uc_webview *) = nullptr;
    void (*stop)(uc_webview *) = nullptr;
    void (*go_back)(uc_webview *) = nullptr;
    void (*go_forward)(uc_webview *) = nullptr;
    int (*can_go_back)(uc_webview *) = nullptr;
    int (*can_go_forward)(uc_webview *) = nullptr;
    const char *(*get_uri)(uc_webview *) = nullptr;
    const char *(*get_title)(uc_webview *) = nullptr;
    void (*run_javascript)(uc_webview *, const char *) = nullptr;
    void (*set_zoom)(uc_webview *, double) = nullptr;
    void (*set_background_rgba)(uc_webview *, float, float, float, float) = nullptr;
    void (*frame_rendered)(uc_webview *, void *) = nullptr;
    void (*frame_release)(uc_webview *, void *) = nullptr;
    void (*touch)(uc_webview *, int, int, double, double, uint32_t) = nullptr;
    void (*pointer_button)(uc_webview *, int, int, double, double, unsigned, uint32_t) = nullptr;
    void (*pointer_move)(uc_webview *, double, double, unsigned, uint32_t) = nullptr;
    void (*scroll)(uc_webview *, double, double, double, double, int, uint32_t) = nullptr;
    void (*key)(uc_webview *, uint32_t, uint32_t, int, unsigned, uint32_t) = nullptr;
    void (*ime_commit)(uc_webview *, const char *) = nullptr;
    void (*ime_preedit)(uc_webview *, const char *, int) = nullptr;
    void (*ime_delete_surrounding)(uc_webview *, int, int) = nullptr;
    // optional, library 0.5.0: calls back once the page rendered everything requested before (the zoom indicator)
    int (*when_rendered)(uc_webview *, uc_webview_rendered_cb, void *) = nullptr;
    // optional, library 0.4.0: uc_webview_new() for a callback struct of the given size, the only way the library
    // reads callbacks appended after the API version 1 ones (pinch, magnification_changed of 0.6.0)
    uc_webview *(*create_sized)(int, int, const uc_webview_callbacks *, size_t, void *) = nullptr;
    // optional, library 0.6.0: pinch zoom, WebKit's page scale with a focal point
    int (*set_magnification)(uc_webview *, double, double, double) = nullptr;
    double (*get_magnification)(uc_webview *) = nullptr;
    // optional, library 0.7.0: the answers to the page's script dialogs and logins (the request callbacks appended to
    // uc_webview_callbacks, which reach the library only through uc_webview_new_sized())
    int (*request_reply)(uc_webview *, uint64_t, int, const char *) = nullptr;
    int (*request_auth)(uc_webview *, uint64_t, const char *, const char *) = nullptr;
    // optional, library 0.8.0: the site settings of a web view entity, each resolved on its own (docs/webview.md,
    // "Options and the library")
    int (*set_origin_headers)(uc_webview *, const char *, const char *const *, const char *const *, size_t) = nullptr;
    int (*set_user_agent)(uc_webview *, int, const char *) = nullptr;
    int (*global_set_cookies)(const char *, const uc_webview_cookie *, size_t, uc_webview_done_cb, void *) = nullptr;
    int (*global_allow_certificate)(const char *, const char *) = nullptr;
    int (*global_set_tls_errors_policy)(int) = nullptr;
    int (*global_set_color_scheme)(int) = nullptr;
    int (*global_clear_data)(unsigned, uc_webview_done_cb, void *) = nullptr;
};

/**
 * Loads libucwebview.so.1 at runtime and owns the process-wide state of the web view: the function table, the
 * API version check and the lazy uc_webview_global_init().
 *
 * The library is optional: on the desktop simulator, on a firmware without the uc-webview package or with another
 * UC_WEBVIEW_API_VERSION, isAvailable() is false and every WebView item stays an empty placeholder. The library
 * also needs Qt's GLib event dispatcher (QPAEventDispatcherGlib on eglfs, QXcbGlibEventDispatcher on a desktop):
 * WebKit delivers its events through the default GLib main context, which only that dispatcher runs on the GUI
 * thread (qgenericunixeventdispatcher.cpp:46-54; QT_NO_GLIB=1 would silently stall WebKit).
 *
 * All functions must be called from the GUI thread; the first call loads the library.
 */
class WebViewLibrary {
 public:
    static WebViewLibrary *instance();

    // Loads the library on the first call. False when it is missing, incompatible or the dispatcher is wrong.
    bool isAvailable();
    // Why isAvailable() is false (empty when it is true).
    QString unavailableReason() const { return m_reason; }

    // Whether the web view can be expected to work, without loading the library: before the first load, Qt's GLib
    // dispatcher runs and installedPath() finds the library; after it, whether the library loaded (and initialised).
    bool isInstalled() const;
    // The path of libucwebview.so.1 in the directories the dynamic linker searches, in its order: those of
    // LD_LIBRARY_PATH, then /usr/lib, where the firmware installs it, and /lib. The first readable file counts; empty
    // without one. A stat() per directory, the library is not opened. Not checked: directories that only
    // /etc/ld.so.cache names, and a RUNPATH (the device's binary has none).
    static QString installedPath();

    // Called with the outcome of the first load (isAvailable()), and with false when uc_webview_global_init() fails
    // later: the web view controller declares web views to the core while the library is usable. Called on the GUI
    // thread from inside the call that found it; one handler, nullptr removes it.
    using AvailabilityHandler = std::function<void(bool available)>;
    void setAvailabilityHandler(AvailabilityHandler handler) { m_availabilityHandler = std::move(handler); }

    // uc_webview_global_init() once, with Qt's EGLDisplay; false when the library is unavailable or the display
    // could not be used (the web view is unavailable from then on).
    bool ensureInitialized();

    const WebViewFunctions &functions() const { return m_functions; }

    // WebKit's tap/drag distance in view pixels, as the library configured its display: what
    // uc_webview_global_get_drag_threshold() reports (library 0.3.0, 16 px unless UC_WEBVIEW_DRAG_THRESHOLD), or for
    // an older library UC_WEBVIEW_DRAG_THRESHOLD or WPE's 8 px (dragThresholdFromEnv). Read once the library is
    // initialised; before that the fallback.
    double dragThreshold();

    // Pinch zoom works: the library has uc_webview_new_sized() (the pinch callbacks reach it), set_magnification()
    // (0.6.0) and when_rendered() (the hold of the scaled frame). Without, two fingers behave as before 0.6.0.
    bool pinchZoomAvailable() const {
        return m_functions.create_sized && m_functions.set_magnification && m_functions.when_rendered;
    }

    // The page's dialogs, logins, new windows and refused responses reach the item (library 0.7.0, detected by
    // uc_webview_request_reply()): the request callbacks go through uc_webview_new_sized() and are answered with
    // uc_webview_request_reply()/_auth(). Without, the library decides alone, as before 0.7.0.
    bool requestsAvailable() const {
        return m_functions.create_sized && m_functions.request_reply && m_functions.request_auth;
    }

    // The refresh rate of the screen the web views are shown on, for uc_webview_global_set_refresh_rate(): WebKit
    // ticks requestAnimationFrame and animations at this rate (60 Hz without it). Only after ensureInitialized()
    // and only with a library that has the function; a rate already passed is not passed again.
    void setDisplayRefreshRate(qreal hz);

    // ---- web view entity (docs/webview.md, "Web view entity") -------------------------------------------------------
    // The site settings of library 0.8.0. Each function is optional: what a 0.7.x library lacks degrades to a log line.
    // The asynchronous ones report through `done` on the context's thread, from Qt's event loop: never from inside
    // the call, and not at all once the context is gone.

    // the outcome of an asynchronous library request: true when the library reported UC_WEBVIEW_DONE_OK
    using Done = std::function<void(bool ok)>;

    /**
     * @brief The global web view settings of the core (get_webview_cfg): the colour scheme pages see in the CSS media
     * feature prefers-color-scheme ("light" or "dark") and whether invalid TLS certificates of every host are
     * accepted. Kept until the library is initialised, never a reason to load it; applied then and on every change
     * (uc_webview_global_set_color_scheme(), uc_webview_global_set_tls_errors_policy()). Without these functions the
     * page keeps WebKit's light scheme and the certificate check: logged.
     */
    void setGlobalSettings(const QString &colorScheme, bool allowInvalidCertificates);

    /**
     * @brief Accepts an invalid TLS certificate of host for page loads (uc_webview_global_allow_certificate()): the one
     * whose SHA-256 fingerprint is sha256 (64 hex digits), any with an empty sha256. Exceptions add up for the rest of
     * the process, the library cannot remove them; one passed before is not passed again.
     * @return false when the library lacks the function or refused the exception
     */
    bool allowCertificate(const QString &host, const QString &sha256);

    /**
     * @brief Puts cookies for url into the cookie jar every web view shares (uc_webview_global_set_cookies()); needs
     * the initialised library. done(true) once WebKit has them, done(false) when it failed.
     * @return false when the library lacks the function, is not initialised or refused the cookies; done is not
     * called then
     */
    bool setCookies(const QUrl &url, const QList<WebViewSite::Cookie> &cookies, QObject *context, Done done);

    enum class ClearStart {
        Started,      // done reports the outcome
        Unsupported,  // the library has no uc_webview_global_clear_data() (older than 0.8.0)
        Refused,      // the library refused the request, or could not be initialised
    };

    /**
     * @brief Clears the website data of every web view (uc_webview_global_clear_data()): the HTTP cache (all false), or
     * all website data (all true). The credentials of logins answered in a dialog stay until the process ends, the
     * library cannot drop them. Initialises the library first: the data of an earlier run is on disk. done(ok) when
     * WebKit is done; not called unless Started.
     */
    ClearStart clearWebsiteData(bool all, QObject *context, Done done);

    // clearWebsiteData() can start: the library is available and has uc_webview_global_clear_data() (0.8.0). Loads the
    // library on the first call, like isAvailable().
    bool canClearWebsiteData() { return isAvailable() && m_functions.global_clear_data; }
    // Counts the clears of all website data that ended: the cookies a web view set before are gone after one.
    quint64 websiteDataGeneration() const { return m_websiteDataGeneration; }

    // Unit tests: use this function table instead of loading the library. The table counts as available and not
    // yet initialised, so ensureInitialized() calls its global_init.
    void setFunctionsForTest(const WebViewFunctions &functions);

 private:
    WebViewLibrary() = default;

    bool load();
    bool resolveAll(QString *missing);
    void applyGlobalSettings();

    QLibrary         m_library;
    WebViewFunctions m_functions;
    QString          m_reason;
    bool             m_probed = false;
    bool             m_available = false;
    bool             m_initialized = false;
    qreal            m_refreshRate = 0;  // last rate passed to the library, 0 = none yet
    double           m_dragThreshold = 0;  // px, read by ensureInitialized(); 0 = not yet
    // told the outcome of the load (setAvailabilityHandler)
    AvailabilityHandler m_availabilityHandler;
    // the global settings of the core (setGlobalSettings), WebKit's defaults until the core's arrive
    QString m_colorScheme = QStringLiteral("light");
    bool    m_allowInvalidCertificates = false;
    bool    m_clearCacheGapLogged = false;
    bool    m_clearAllGapLogged = false;
    // the certificate exceptions passed to the library, "host sha256" (allowCertificate)
    QSet<QString> m_certificateExceptions;
    quint64       m_websiteDataGeneration = 0;
};

}  // namespace webview
}  // namespace uc
