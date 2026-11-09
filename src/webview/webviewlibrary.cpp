// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewlibrary.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QThread>
#include <qpa/qplatformnativeinterface.h>

#include <type_traits>

#include "../logging.h"
#include "webviewinput.h"

namespace uc {
namespace webview {

// The function table must match the ABI header it is resolved against.
#define UC_WEBVIEW_CHECK_SIGNATURE(member, symbol)                                                       \
    static_assert(std::is_same<decltype(WebViewFunctions::member), decltype(&symbol)>::value, \
                  "WebViewFunctions::" #member " does not match " #symbol)

UC_WEBVIEW_CHECK_SIGNATURE(api_version, uc_webview_api_version);
UC_WEBVIEW_CHECK_SIGNATURE(global_init, uc_webview_global_init);
UC_WEBVIEW_CHECK_SIGNATURE(global_shutdown, uc_webview_global_shutdown);
UC_WEBVIEW_CHECK_SIGNATURE(global_set_refresh_rate, uc_webview_global_set_refresh_rate);
UC_WEBVIEW_CHECK_SIGNATURE(global_get_drag_threshold, uc_webview_global_get_drag_threshold);
UC_WEBVIEW_CHECK_SIGNATURE(create, uc_webview_new);
UC_WEBVIEW_CHECK_SIGNATURE(destroy, uc_webview_free);
UC_WEBVIEW_CHECK_SIGNATURE(resize, uc_webview_resize);
UC_WEBVIEW_CHECK_SIGNATURE(set_mapped, uc_webview_set_mapped);
UC_WEBVIEW_CHECK_SIGNATURE(set_focused, uc_webview_set_focused);
UC_WEBVIEW_CHECK_SIGNATURE(set_scale, uc_webview_set_scale);
UC_WEBVIEW_CHECK_SIGNATURE(load_uri, uc_webview_load_uri);
UC_WEBVIEW_CHECK_SIGNATURE(load_html, uc_webview_load_html);
UC_WEBVIEW_CHECK_SIGNATURE(reload, uc_webview_reload);
UC_WEBVIEW_CHECK_SIGNATURE(stop, uc_webview_stop);
UC_WEBVIEW_CHECK_SIGNATURE(go_back, uc_webview_go_back);
UC_WEBVIEW_CHECK_SIGNATURE(go_forward, uc_webview_go_forward);
UC_WEBVIEW_CHECK_SIGNATURE(can_go_back, uc_webview_can_go_back);
UC_WEBVIEW_CHECK_SIGNATURE(can_go_forward, uc_webview_can_go_forward);
UC_WEBVIEW_CHECK_SIGNATURE(get_uri, uc_webview_get_uri);
UC_WEBVIEW_CHECK_SIGNATURE(get_title, uc_webview_get_title);
UC_WEBVIEW_CHECK_SIGNATURE(run_javascript, uc_webview_run_javascript);
UC_WEBVIEW_CHECK_SIGNATURE(set_zoom, uc_webview_set_zoom);
UC_WEBVIEW_CHECK_SIGNATURE(set_background_rgba, uc_webview_set_background_rgba);
UC_WEBVIEW_CHECK_SIGNATURE(frame_rendered, uc_webview_frame_rendered);
UC_WEBVIEW_CHECK_SIGNATURE(frame_release, uc_webview_frame_release);
UC_WEBVIEW_CHECK_SIGNATURE(touch, uc_webview_touch);
UC_WEBVIEW_CHECK_SIGNATURE(pointer_button, uc_webview_pointer_button);
UC_WEBVIEW_CHECK_SIGNATURE(pointer_move, uc_webview_pointer_move);
UC_WEBVIEW_CHECK_SIGNATURE(scroll, uc_webview_scroll);
UC_WEBVIEW_CHECK_SIGNATURE(key, uc_webview_key);
UC_WEBVIEW_CHECK_SIGNATURE(ime_commit, uc_webview_ime_commit);
UC_WEBVIEW_CHECK_SIGNATURE(ime_preedit, uc_webview_ime_preedit);
UC_WEBVIEW_CHECK_SIGNATURE(ime_delete_surrounding, uc_webview_ime_delete_surrounding);

#undef UC_WEBVIEW_CHECK_SIGNATURE

namespace {

// QLibrary("ucwebview", 1) tries lib<name>.so.1 on Linux (qlibrary.h:70, qlibrary_unix.cpp:84 adds ".so.<version>").
constexpr const char *kLibraryName = "ucwebview";
constexpr int         kLibraryVersion = 1;
// The GLib dispatchers of Qt 5.15: QPAEventDispatcherGlib (eglfs and the other generic Unix QPA plugins,
// qeventdispatcher_glib_p.h:61) and QXcbGlibEventDispatcher (xcb). Both derive from QEventDispatcherGlib.
constexpr const char *kGlibDispatcherMarker = "Glib";

template <typename F>
bool resolveSymbol(QLibrary *library, F *target, const char *symbol, QString *missing) {
    *target = reinterpret_cast<F>(library->resolve(symbol));
    if (*target) {
        return true;
    }
    if (!missing->isEmpty()) {
        missing->append(QLatin1String(", "));
    }
    missing->append(QLatin1String(symbol));
    return false;
}

}  // namespace

WebViewLibrary *WebViewLibrary::instance() {
    static WebViewLibrary library;
    return &library;
}

bool WebViewLibrary::isAvailable() {
    if (!m_probed) {
        m_probed = true;
        m_available = load();
        if (m_available) {
            qCInfo(lcWebView()) << "Web view available: libucwebview API" << UC_WEBVIEW_API_VERSION;
        } else {
            qCInfo(lcWebView()) << "Web view unavailable:" << m_reason;
        }
    }
    return m_available;
}

bool WebViewLibrary::load() {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    QAbstractEventDispatcher *dispatcher = QAbstractEventDispatcher::instance();
    const QString dispatcherName = dispatcher ? QLatin1String(dispatcher->metaObject()->className()) : QString();
    if (!dispatcherName.contains(QLatin1String(kGlibDispatcherMarker))) {
        m_reason = QStringLiteral("the event dispatcher is %1, WebKit needs Qt's GLib dispatcher (is QT_NO_GLIB set?)")
                       .arg(dispatcherName.isEmpty() ? QStringLiteral("missing") : dispatcherName);
        return false;
    }

    m_library.setFileNameAndVersion(QLatin1String(kLibraryName), kLibraryVersion);
    if (!m_library.load()) {
        m_reason = m_library.errorString();
        return false;
    }

    QString missing;
    if (!resolveAll(&missing)) {
        m_reason = QStringLiteral("%1 lacks %2").arg(m_library.fileName(), missing);
        m_library.unload();
        return false;
    }

    const int version = m_functions.api_version();
    if (version != UC_WEBVIEW_API_VERSION) {
        m_reason = QStringLiteral("%1 has API version %2, this build needs %3")
                       .arg(m_library.fileName())
                       .arg(version)
                       .arg(UC_WEBVIEW_API_VERSION);
        m_library.unload();
        return false;
    }

    qCDebug(lcWebView()) << "Loaded" << m_library.fileName() << "with the" << dispatcherName << "dispatcher";
    if (!m_functions.global_set_refresh_rate) {
        qCInfo(lcWebView()) << m_library.fileName()
                            << "has no uc_webview_global_set_refresh_rate (older than 0.2.0): pages animate at 60 Hz";
    }
    return true;
}

bool WebViewLibrary::resolveAll(QString *missing) {
    bool ok = true;
#define UC_WEBVIEW_RESOLVE(member, symbol) ok = resolveSymbol(&m_library, &m_functions.member, #symbol, missing) && ok
    UC_WEBVIEW_RESOLVE(api_version, uc_webview_api_version);
    UC_WEBVIEW_RESOLVE(global_init, uc_webview_global_init);
    UC_WEBVIEW_RESOLVE(global_shutdown, uc_webview_global_shutdown);
    UC_WEBVIEW_RESOLVE(create, uc_webview_new);
    UC_WEBVIEW_RESOLVE(destroy, uc_webview_free);
    UC_WEBVIEW_RESOLVE(resize, uc_webview_resize);
    UC_WEBVIEW_RESOLVE(set_mapped, uc_webview_set_mapped);
    UC_WEBVIEW_RESOLVE(set_focused, uc_webview_set_focused);
    UC_WEBVIEW_RESOLVE(set_scale, uc_webview_set_scale);
    UC_WEBVIEW_RESOLVE(load_uri, uc_webview_load_uri);
    UC_WEBVIEW_RESOLVE(load_html, uc_webview_load_html);
    UC_WEBVIEW_RESOLVE(reload, uc_webview_reload);
    UC_WEBVIEW_RESOLVE(stop, uc_webview_stop);
    UC_WEBVIEW_RESOLVE(go_back, uc_webview_go_back);
    UC_WEBVIEW_RESOLVE(go_forward, uc_webview_go_forward);
    UC_WEBVIEW_RESOLVE(can_go_back, uc_webview_can_go_back);
    UC_WEBVIEW_RESOLVE(can_go_forward, uc_webview_can_go_forward);
    UC_WEBVIEW_RESOLVE(get_uri, uc_webview_get_uri);
    UC_WEBVIEW_RESOLVE(get_title, uc_webview_get_title);
    UC_WEBVIEW_RESOLVE(run_javascript, uc_webview_run_javascript);
    UC_WEBVIEW_RESOLVE(set_zoom, uc_webview_set_zoom);
    UC_WEBVIEW_RESOLVE(set_background_rgba, uc_webview_set_background_rgba);
    UC_WEBVIEW_RESOLVE(frame_rendered, uc_webview_frame_rendered);
    UC_WEBVIEW_RESOLVE(frame_release, uc_webview_frame_release);
    UC_WEBVIEW_RESOLVE(touch, uc_webview_touch);
    UC_WEBVIEW_RESOLVE(pointer_button, uc_webview_pointer_button);
    UC_WEBVIEW_RESOLVE(pointer_move, uc_webview_pointer_move);
    UC_WEBVIEW_RESOLVE(scroll, uc_webview_scroll);
    UC_WEBVIEW_RESOLVE(key, uc_webview_key);
    UC_WEBVIEW_RESOLVE(ime_commit, uc_webview_ime_commit);
    UC_WEBVIEW_RESOLVE(ime_preedit, uc_webview_ime_preedit);
    UC_WEBVIEW_RESOLVE(ime_delete_surrounding, uc_webview_ime_delete_surrounding);
#undef UC_WEBVIEW_RESOLVE

    // Optional: added after the first release within the same API version, which the header allows for new
    // functions; an older library simply lacks them.
    m_functions.global_set_refresh_rate = reinterpret_cast<decltype(m_functions.global_set_refresh_rate)>(
        m_library.resolve("uc_webview_global_set_refresh_rate"));
    m_functions.global_get_drag_threshold = reinterpret_cast<decltype(m_functions.global_get_drag_threshold)>(
        m_library.resolve("uc_webview_global_get_drag_threshold"));
    return ok;
}

bool WebViewLibrary::ensureInitialized() {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    if (!isAvailable()) {
        return false;
    }
    if (m_initialized) {
        return true;
    }

    // Qt's EGLDisplay: eglfs exposes it as the "egldisplay" integration resource (qeglfsintegration.cpp:300,323).
    // Platforms without one (the desktop simulator on xcb, offscreen) get NULL: the library then renders into
    // shared memory frames, which the item uploads as images.
    void *eglDisplay = nullptr;
    if (QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface()) {
        eglDisplay = native->nativeResourceForIntegration(QByteArrayLiteral("egldisplay"));
    }

    // Website data (cookies, local storage) only when the firmware names a writable directory inside the
    // sandbox; otherwise the session is ephemeral (nothing is written to disk).
    const QByteArray dataDir = qgetenv("UC_WEBVIEW_DATA_DIR");

    const int result = m_functions.global_init(eglDisplay, dataDir.isEmpty() ? nullptr : dataDir.constData(), nullptr);
    if (result != 0) {
        m_available = false;
        m_reason = QStringLiteral("uc_webview_global_init failed (%1)").arg(result);
        qCWarning(lcWebView()) << "Web view unavailable:" << m_reason;
        return false;
    }

    m_initialized = true;
    qCInfo(lcWebView()) << "Web view initialised" << (eglDisplay ? "with Qt's EGL display" : "without an EGL display")
                        << (dataDir.isEmpty() ? "and an ephemeral session" : "and persistent website data");

    // the library sets the threshold on its display in uc_webview_global_init() and reports it from then on
    const int  reported = m_functions.global_get_drag_threshold ? m_functions.global_get_drag_threshold() : 0;
    const bool valid = reported >= 1 && reported <= 200;
    m_dragThreshold = valid ? reported : dragThresholdFromEnv(qgetenv("UC_WEBVIEW_DRAG_THRESHOLD"));
    qCInfo(lcWebView()) << "Touch drag threshold" << m_dragThreshold << "px"
                        << (valid ? "(reported by the library)"
                                  : "(not reported by the library: UC_WEBVIEW_DRAG_THRESHOLD or WPE's default)");
    return true;
}

double WebViewLibrary::dragThreshold() {
    return m_dragThreshold > 0 ? m_dragThreshold : dragThresholdFromEnv(qgetenv("UC_WEBVIEW_DRAG_THRESHOLD"));
}

void WebViewLibrary::setDisplayRefreshRate(qreal hz) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    if (!m_initialized || !m_functions.global_set_refresh_rate || qFuzzyCompare(hz, m_refreshRate)) {
        return;
    }
    m_refreshRate = hz;
    // The library logs which rate it uses: UC_WEBVIEW_REFRESH_RATE in the environment wins over this one.
    if (m_functions.global_set_refresh_rate(static_cast<double>(hz)) == 0) {
        qCInfo(lcWebView()) << "Display refresh rate" << hz << "Hz passed to the web view";
    } else {
        qCWarning(lcWebView()) << "The web view refused the display refresh rate" << hz << "Hz (valid: 20..240)";
    }
}

void WebViewLibrary::setFunctionsForTest(const WebViewFunctions &functions) {
    m_functions = functions;
    m_probed = true;
    m_available = true;
    m_initialized = false;
    m_refreshRate = 0;
    m_dragThreshold = 0;
    m_reason.clear();
}

}  // namespace webview
}  // namespace uc
