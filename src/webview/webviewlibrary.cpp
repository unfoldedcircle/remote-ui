// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewlibrary.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMetaObject>
#include <QPointer>
#include <QThread>
#include <qpa/qplatformnativeinterface.h>

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

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
UC_WEBVIEW_CHECK_SIGNATURE(when_rendered, uc_webview_when_rendered);
UC_WEBVIEW_CHECK_SIGNATURE(create_sized, uc_webview_new_sized);
UC_WEBVIEW_CHECK_SIGNATURE(set_magnification, uc_webview_set_magnification);
UC_WEBVIEW_CHECK_SIGNATURE(get_magnification, uc_webview_get_magnification);
UC_WEBVIEW_CHECK_SIGNATURE(request_reply, uc_webview_request_reply);
UC_WEBVIEW_CHECK_SIGNATURE(request_auth, uc_webview_request_auth);
UC_WEBVIEW_CHECK_SIGNATURE(set_origin_headers, uc_webview_set_origin_headers);
UC_WEBVIEW_CHECK_SIGNATURE(set_user_agent, uc_webview_set_user_agent);
UC_WEBVIEW_CHECK_SIGNATURE(global_set_cookies, uc_webview_global_set_cookies);
UC_WEBVIEW_CHECK_SIGNATURE(global_allow_certificate, uc_webview_global_allow_certificate);
UC_WEBVIEW_CHECK_SIGNATURE(global_set_tls_errors_policy, uc_webview_global_set_tls_errors_policy);
UC_WEBVIEW_CHECK_SIGNATURE(global_set_color_scheme, uc_webview_global_set_color_scheme);
UC_WEBVIEW_CHECK_SIGNATURE(global_clear_data, uc_webview_global_clear_data);

#undef UC_WEBVIEW_CHECK_SIGNATURE

namespace {

// QLibrary("ucwebview", 1) tries lib<name>.so.1 on Linux (qlibrary.h:70, qlibrary_unix.cpp:84 adds ".so.<version>").
constexpr const char *kLibraryName = "ucwebview";
constexpr int         kLibraryVersion = 1;
// The GLib dispatchers of Qt 5.15: QPAEventDispatcherGlib (eglfs and the other generic Unix QPA plugins,
// qeventdispatcher_glib_p.h:61) and QXcbGlibEventDispatcher (xcb). Both derive from QEventDispatcherGlib.
constexpr const char *kGlibDispatcherMarker = "Glib";

// the class name of this thread's event dispatcher, empty without one
QString eventDispatcherName() {
    QAbstractEventDispatcher *dispatcher = QAbstractEventDispatcher::instance();
    return dispatcher ? QLatin1String(dispatcher->metaObject()->className()) : QString();
}

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

// An asynchronous request of the library (uc_webview_done_cb): the library calls onDone() exactly once for an accepted
// request, on the GUI thread from an idle of the GLib main context, never for a refused one.
struct DoneCall {
    QPointer<QObject>    context;
    WebViewLibrary::Done done;
    quint64             *clearedGeneration = nullptr;  // a clear of all website data: counted when it ends
};

void onDone(int status, void *user) {
    std::unique_ptr<DoneCall> call(static_cast<DoneCall *>(user));
    if (call->clearedGeneration) {
        ++*call->clearedGeneration;
    }
    if (!call->context || !call->done) {
        return;  // the requester went meanwhile
    }
    // Into Qt's event loop: the handler runs on the context's thread after the GLib dispatch of the library's idle
    // has returned, and Qt drops the call when the context goes before it runs.
    const bool ok = status == UC_WEBVIEW_DONE_OK;
    QMetaObject::invokeMethod(
        call->context.data(), [done = std::move(call->done), ok] { done(ok); }, Qt::QueuedConnection);
}

// the library functions of the site settings (library 0.8.0) that are missing, for the log
QStringList missingSiteFunctions(const WebViewFunctions &f) {
    QStringList missing;
    const std::pair<bool, const char *> functions[] = {
        {f.set_origin_headers != nullptr, "uc_webview_set_origin_headers"},
        {f.set_user_agent != nullptr, "uc_webview_set_user_agent"},
        {f.global_set_cookies != nullptr, "uc_webview_global_set_cookies"},
        {f.global_allow_certificate != nullptr, "uc_webview_global_allow_certificate"},
        {f.global_set_tls_errors_policy != nullptr, "uc_webview_global_set_tls_errors_policy"},
        {f.global_set_color_scheme != nullptr, "uc_webview_global_set_color_scheme"},
        {f.global_clear_data != nullptr, "uc_webview_global_clear_data"},
    };
    for (const auto &function : functions) {
        if (!function.first) {
            missing.append(QLatin1String(function.second));
        }
    }
    return missing;
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
        if (m_availabilityHandler) {
            m_availabilityHandler(m_available);
        }
    }
    return m_available;
}

bool WebViewLibrary::isInstalled() const {
    if (m_probed) {
        return m_available;
    }
    return eventDispatcherName().contains(QLatin1String(kGlibDispatcherMarker)) && !installedPath().isEmpty();
}

QString WebViewLibrary::installedPath() {
    // QLibrary passes the bare file name to dlopen(), which looks in the directories of LD_LIBRARY_PATH first
    QStringList directories =
        QString::fromLocal8Bit(qgetenv("LD_LIBRARY_PATH")).split(QLatin1Char(':'), Qt::SkipEmptyParts);
    directories << QStringLiteral("/usr/lib") << QStringLiteral("/lib");
    for (const QString &directory : directories) {
        const QFileInfo file(QDir(directory), QLatin1String(UC_WEBVIEW_SONAME));
        if (file.isFile() && file.isReadable()) {
            return file.absoluteFilePath();
        }
    }
    return QString();
}

bool WebViewLibrary::load() {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    const QString dispatcherName = eventDispatcherName();
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
    if (!m_functions.when_rendered) {
        qCInfo(lcWebView()) << m_library.fileName()
                            << "has no uc_webview_when_rendered (older than 0.5.0): a zoom step counts as rendered at "
                               "the first frame after it";
    }
    if (!m_functions.set_magnification) {
        qCInfo(lcWebView()) << m_library.fileName()
                            << "has no uc_webview_set_magnification (older than 0.6.0): no pinch zoom";
    }
    if (!m_functions.request_reply) {
        qCInfo(lcWebView()) << m_library.fileName()
                            << "has no uc_webview_request_reply (older than 0.7.0): no dialogs or logins of the page";
    }
    const QStringList missingSite = missingSiteFunctions(m_functions);
    if (!missingSite.isEmpty()) {
        qCInfo(lcWebView()).noquote() << m_library.fileName() << "has no" << missingSite.join(QStringLiteral(", "))
                                      << "(older than 0.8.0): web view entities load without these site settings";
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
    m_functions.when_rendered =
        reinterpret_cast<decltype(m_functions.when_rendered)>(m_library.resolve("uc_webview_when_rendered"));
    m_functions.create_sized =
        reinterpret_cast<decltype(m_functions.create_sized)>(m_library.resolve("uc_webview_new_sized"));
    m_functions.set_magnification =
        reinterpret_cast<decltype(m_functions.set_magnification)>(m_library.resolve("uc_webview_set_magnification"));
    m_functions.get_magnification =
        reinterpret_cast<decltype(m_functions.get_magnification)>(m_library.resolve("uc_webview_get_magnification"));
    // library 0.7.0: uc_webview_request_reply() is the one the header names for detecting it
    m_functions.request_reply =
        reinterpret_cast<decltype(m_functions.request_reply)>(m_library.resolve("uc_webview_request_reply"));
    m_functions.request_auth =
        reinterpret_cast<decltype(m_functions.request_auth)>(m_library.resolve("uc_webview_request_auth"));
    // library 0.8.0: the site settings, each on its own (the header names no detection function for them)
    m_functions.set_origin_headers =
        reinterpret_cast<decltype(m_functions.set_origin_headers)>(m_library.resolve("uc_webview_set_origin_headers"));
    m_functions.set_user_agent =
        reinterpret_cast<decltype(m_functions.set_user_agent)>(m_library.resolve("uc_webview_set_user_agent"));
    m_functions.global_set_cookies =
        reinterpret_cast<decltype(m_functions.global_set_cookies)>(m_library.resolve("uc_webview_global_set_cookies"));
    m_functions.global_allow_certificate = reinterpret_cast<decltype(m_functions.global_allow_certificate)>(
        m_library.resolve("uc_webview_global_allow_certificate"));
    m_functions.global_set_tls_errors_policy = reinterpret_cast<decltype(m_functions.global_set_tls_errors_policy)>(
        m_library.resolve("uc_webview_global_set_tls_errors_policy"));
    m_functions.global_set_color_scheme = reinterpret_cast<decltype(m_functions.global_set_color_scheme)>(
        m_library.resolve("uc_webview_global_set_color_scheme"));
    m_functions.global_clear_data =
        reinterpret_cast<decltype(m_functions.global_clear_data)>(m_library.resolve("uc_webview_global_clear_data"));
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
        if (m_availabilityHandler) {
            m_availabilityHandler(false);
        }
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
    applyGlobalSettings();
    return true;
}

void WebViewLibrary::setGlobalSettings(const QString &colorScheme, bool allowInvalidCertificates) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    if (colorScheme == m_colorScheme && allowInvalidCertificates == m_allowInvalidCertificates) {
        return;
    }
    m_colorScheme = colorScheme;
    m_allowInvalidCertificates = allowInvalidCertificates;
    if (m_initialized) {
        applyGlobalSettings();
    }
}

void WebViewLibrary::applyGlobalSettings() {
    const bool dark = m_colorScheme == QLatin1String("dark");
    if (m_functions.global_set_color_scheme) {
        if (m_functions.global_set_color_scheme(dark ? UC_WEBVIEW_COLOR_SCHEME_DARK : UC_WEBVIEW_COLOR_SCHEME_LIGHT) !=
            0) {
            qCWarning(lcWebView()) << "The web view library refused the colour scheme" << m_colorScheme;
        }
    } else if (dark) {
        // WebKit's default needs nothing
        qCInfo(lcWebView()) << "The web view library cannot set the colour scheme dark (older than 0.8.0): pages see "
                               "the light scheme";
    }
    if (m_functions.global_set_tls_errors_policy) {
        if (m_functions.global_set_tls_errors_policy(m_allowInvalidCertificates ? 1 : 0) != 0) {
            qCWarning(lcWebView()) << "The web view library refused the policy for invalid certificates";
        }
    } else if (m_allowInvalidCertificates) {
        qCInfo(lcWebView()) << "The web view library cannot accept invalid certificates of all hosts (older than "
                               "0.8.0): the certificate check stays on";
    }
}

bool WebViewLibrary::allowCertificate(const QString &host, const QString &sha256) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    if (!m_available || !m_functions.global_allow_certificate || host.isEmpty()) {
        return false;
    }
    const QString key = host + QLatin1Char(' ') + sha256;
    if (m_certificateExceptions.contains(key)) {
        return true;
    }
    const QByteArray hostBytes = host.toUtf8();
    const QByteArray sha256Bytes = sha256.toLatin1();
    // the library logs the exception it takes, or why it refused it
    if (m_functions.global_allow_certificate(hostBytes.constData(),
                                             sha256.isEmpty() ? nullptr : sha256Bytes.constData()) != 0) {
        qCWarning(lcWebView()) << "The web view library refused the certificate exception of a web view";
        return false;
    }
    m_certificateExceptions.insert(key);
    return true;
}

bool WebViewLibrary::setCookies(const QUrl &url, const QList<WebViewSite::Cookie> &cookies, QObject *context,
                                Done done) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    if (!m_initialized || !m_functions.global_set_cookies) {
        return false;
    }
    // the library copies the strings during the call
    std::vector<QByteArray>        strings;
    std::vector<uc_webview_cookie> list(static_cast<size_t>(cookies.size()));
    strings.reserve(list.size() * 4);
    auto keep = [&strings](const QString &text) {
        strings.push_back(text.toUtf8());
        return strings.back().constData();
    };
    for (int i = 0; i < cookies.size(); ++i) {
        const WebViewSite::Cookie &cookie = cookies.at(i);
        uc_webview_cookie         &c = list[static_cast<size_t>(i)];
        c.name = keep(cookie.name);
        c.value = keep(cookie.value);
        c.domain = cookie.domain.isEmpty() ? nullptr : keep(cookie.domain);
        c.path = cookie.path.isEmpty() ? nullptr : keep(cookie.path);
        c.secure = cookie.secure ? 1 : 0;
        c.http_only = cookie.httpOnly ? 1 : 0;
        c.expires = cookie.expires.isValid() ? cookie.expires.toSecsSinceEpoch() : -1;
    }
    const QByteArray address = url.toEncoded();
    auto            *call = new DoneCall{context, std::move(done), nullptr};
    // the library logs the URL and the count, or the rule a cookie breaks, never a value
    if (m_functions.global_set_cookies(address.constData(), list.data(), list.size(), &onDone, call) != 0) {
        delete call;
        return false;
    }
    return true;
}

WebViewLibrary::ClearStart WebViewLibrary::clearWebsiteData(bool all, QObject *context, Done done) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    if (!isAvailable() || !m_functions.global_clear_data) {
        bool &logged = all ? m_clearAllGapLogged : m_clearCacheGapLogged;
        if (!logged) {
            logged = true;
            qCInfo(lcWebView()) << "The web view library cannot clear" << (all ? "the website data" : "the HTTP cache")
                                << "(older than 0.8.0): nothing is cleared";
        }
        return ClearStart::Unsupported;
    }
    if (!ensureInitialized()) {
        return ClearStart::Refused;
    }
    auto *call = new DoneCall{context, std::move(done), all ? &m_websiteDataGeneration : nullptr};
    if (m_functions.global_clear_data(all ? UC_WEBVIEW_DATA_ALL : UC_WEBVIEW_DATA_CACHE, &onDone, call) != 0) {
        delete call;
        qCWarning(lcWebView()) << "The web view library refused to clear"
                               << (all ? "the website data" : "the HTTP cache");
        return ClearStart::Refused;
    }
    qCInfo(lcWebView()) << "Clearing" << (all ? "the website data" : "the HTTP cache") << "of every web view";
    return ClearStart::Started;
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
    m_colorScheme = QStringLiteral("light");
    m_allowInvalidCertificates = false;
    m_clearCacheGapLogged = false;
    m_clearAllGapLogged = false;
    m_certificateExceptions.clear();
    m_websiteDataGeneration = 0;
}

}  // namespace webview
}  // namespace uc
