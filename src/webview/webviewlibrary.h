// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLibrary>
#include <QString>

#include "uc_webview.h"

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

    // uc_webview_global_init() once, with Qt's EGLDisplay; false when the library is unavailable or the display
    // could not be used (the web view is unavailable from then on).
    bool ensureInitialized();

    const WebViewFunctions &functions() const { return m_functions; }

    // WebKit's tap/drag distance in view pixels, as the library configured its display: what
    // uc_webview_global_get_drag_threshold() reports (library 0.3.0, 16 px unless UC_WEBVIEW_DRAG_THRESHOLD), or for
    // an older library UC_WEBVIEW_DRAG_THRESHOLD or WPE's 8 px (dragThresholdFromEnv). Read once the library is
    // initialised; before that the fallback.
    double dragThreshold();

    // The refresh rate of the screen the web views are shown on, for uc_webview_global_set_refresh_rate(): WebKit
    // ticks requestAnimationFrame and animations at this rate (60 Hz without it). Only after ensureInitialized()
    // and only with a library that has the function; a rate already passed is not passed again.
    void setDisplayRefreshRate(qreal hz);

    // Unit tests: use this function table instead of loading the library. The table counts as available and not
    // yet initialised, so ensureInitialized() calls its global_init.
    void setFunctionsForTest(const WebViewFunctions &functions);

 private:
    WebViewLibrary() = default;

    bool load();
    bool resolveAll(QString *missing);

    QLibrary         m_library;
    WebViewFunctions m_functions;
    QString          m_reason;
    bool             m_probed = false;
    bool             m_available = false;
    bool             m_initialized = false;
    qreal            m_refreshRate = 0;  // last rate passed to the library, 0 = none yet
    double           m_dragThreshold = 0;  // px, read by ensureInitialized(); 0 = not yet
};

}  // namespace webview
}  // namespace uc
