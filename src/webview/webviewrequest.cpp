// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewrequest.h"

#include <QByteArray>

#include <utility>

#include "../logging.h"

namespace uc {
namespace webview {

static_assert(WebViewDialogRequest::Alert == UC_WEBVIEW_DIALOG_ALERT, "dialog types follow uc_webview.h");
static_assert(WebViewDialogRequest::Confirm == UC_WEBVIEW_DIALOG_CONFIRM, "dialog types follow uc_webview.h");
static_assert(WebViewDialogRequest::Prompt == UC_WEBVIEW_DIALOG_PROMPT, "dialog types follow uc_webview.h");
static_assert(WebViewDialogRequest::LeavePage == UC_WEBVIEW_DIALOG_BEFORE_UNLOAD, "dialog types follow uc_webview.h");

namespace {

QString fromLibrary(const char *text) {
    return text ? QString::fromUtf8(text) : QString();
}

// A page's dialog text as the user can read it: the library passes any length, and laying out megabytes of text (a
// page's alert('x'.repeat(1e7))) would stall the UI. Longer texts are cut with an ellipsis.
QString dialogText(const char *text) {
    QString result = fromLibrary(text);
    if (result.size() > WebViewRequests::kMaxDialogTextLength) {
        result.truncate(WebViewRequests::kMaxDialogTextLength);
        result.append(QChar(0x2026));
    }
    return result;
}

// the library's names of the kinds (its trace says "request 7: beforeunload" too)
const char *dialogName(WebViewDialogRequest::Type type) {
    switch (type) {
        case WebViewDialogRequest::Alert:
            return "alert";
        case WebViewDialogRequest::Confirm:
            return "confirm";
        case WebViewDialogRequest::Prompt:
            return "prompt";
        case WebViewDialogRequest::LeavePage:
            return "beforeunload";
    }
    return "?";
}

// The schemes that take a user name and password from a dialog. Negotiate (Kerberos) takes the system's ticket, not
// credentials (libsoup's SoupAuthNegotiate), HTML_FORM is not used by libsoup, SERVER_TRUST is no login, and a scheme
// of a newer library is unknown here: those are answered without credentials (uc_webview.h, "Compatibility").
bool isLoginScheme(int scheme) {
    switch (scheme) {
        case UC_WEBVIEW_AUTH_SCHEME_DEFAULT:
        case UC_WEBVIEW_AUTH_SCHEME_HTTP_BASIC:
        case UC_WEBVIEW_AUTH_SCHEME_HTTP_DIGEST:
        case UC_WEBVIEW_AUTH_SCHEME_NTLM:
            return true;
        default:
            return false;
    }
}

// what is left of a secret in memory we own once it went to the library
void wipe(QByteArray *bytes) {
    bytes->fill('\0');
}

}  // namespace

// ---- WebViewRequest ---------------------------------------------------------------------------------------------

WebViewRequest::WebViewRequest(WebViewRequests *owner, quint64 id, const QString &origin, QObject *parent)
    : QObject(parent), m_owner(owner), m_id(id), m_origin(origin) {}

void WebViewRequest::setHandled(bool handled) {
    if (handled == m_handled) {
        return;
    }
    m_handled = handled;
    emit handledChanged();
}

void WebViewRequest::end(bool answered, bool emitSignals) {
    m_owner = nullptr;
    m_pending = false;
    if (!emitSignals) {
        return;
    }
    emit pendingChanged();
    if (answered) {
        emit finished();
    } else {
        emit cancelled();
    }
    // a QML dialog may still read it in the handlers above; the guarded properties that hold it turn null later
    deleteLater();
}

// ---- WebViewDialogRequest ---------------------------------------------------------------------------------------

WebViewDialogRequest::WebViewDialogRequest(WebViewRequests *owner, quint64 id, Type type, const QString &origin,
                                           const QString &message, const QString &defaultText, QObject *parent)
    : WebViewRequest(owner, id, origin, parent), m_type(type), m_message(message), m_defaultText(defaultText) {}

bool WebViewDialogRequest::accept(const QString &text) {
    WebViewRequests *requests = owner();
    if (!requests) {
        qCDebug(lcWebView()) << "Request" << id() << "has ended: the answer is dropped";
        return false;
    }
    return requests->reply(this, true, text);
}

bool WebViewDialogRequest::reject() {
    WebViewRequests *requests = owner();
    if (!requests) {
        qCDebug(lcWebView()) << "Request" << id() << "has ended: the answer is dropped";
        return false;
    }
    return requests->reply(this, false, QString());
}

// ---- WebViewAuthRequest -----------------------------------------------------------------------------------------

WebViewAuthRequest::WebViewAuthRequest(WebViewRequests *owner, quint64 id, const QString &origin, const QString &realm,
                                       unsigned flags, QObject *parent)
    : WebViewRequest(owner, id, origin, parent),
      m_realm(realm),
      m_retry((flags & UC_WEBVIEW_AUTH_RETRY) != 0),
      m_proxy((flags & UC_WEBVIEW_AUTH_PROXY) != 0) {}

bool WebViewAuthRequest::isInsecure() const {
    return origin().startsWith(QLatin1String("http:"), Qt::CaseInsensitive);
}

bool WebViewAuthRequest::login(const QString &user, const QString &password) {
    WebViewRequests *requests = owner();
    if (!requests) {
        qCDebug(lcWebView()) << "Request" << id() << "has ended: the credentials are dropped";
        return false;
    }
    return requests->authenticate(this, &user, &password);
}

bool WebViewAuthRequest::cancel() {
    WebViewRequests *requests = owner();
    if (!requests) {
        qCDebug(lcWebView()) << "Request" << id() << "has ended: the answer is dropped";
        return false;
    }
    return requests->authenticate(this, nullptr, nullptr);
}

// ---- WebViewRequests --------------------------------------------------------------------------------------------

WebViewRequests::WebViewRequests(QObject *parent) : m_parent(parent) {}

WebViewRequests::~WebViewRequests() {
    clear();
}

void WebViewRequests::setView(uc_webview *view, ReplyFunction replyFunction, AuthFunction authFunction) {
    m_view = view;
    m_reply = replyFunction;
    m_auth = authFunction;
}

void WebViewRequests::trace(const QString &line) const {
    if (m_trace) {
        m_trace(line);
    }
}

WebViewDialogRequest *WebViewRequests::addDialog(quint64 id, int type, const char *origin, const char *message,
                                                 const char *defaultText) {
    if (type < UC_WEBVIEW_DIALOG_ALERT || type > UC_WEBVIEW_DIALOG_BEFORE_UNLOAD) {
        // a dialog type of a newer library: its "no" (uc_webview.h, "Compatibility")
        trace(QStringLiteral("request %1: dialog type %2 unknown, answered no").arg(id).arg(type));
        if (m_view && m_reply) {
            m_reply(m_view, id, 0, nullptr);
        }
        return nullptr;
    }
    const auto dialogType = static_cast<WebViewDialogRequest::Type>(type);
    auto      *request = new WebViewDialogRequest(this, id, dialogType, fromLibrary(origin), dialogText(message),
                                                  dialogText(defaultText), m_parent);
    m_pending.insert(id, request);
    trace(QStringLiteral("request %1: %2").arg(id).arg(QLatin1String(dialogName(dialogType))));
    return request;
}

WebViewAuthRequest *WebViewRequests::addAuth(quint64 id, const char *origin, const char *realm, int scheme,
                                             unsigned flags) {
    if (!isLoginScheme(scheme)) {
        trace(QStringLiteral("request %1: auth scheme %2 takes no user name, answered without credentials")
                  .arg(id)
                  .arg(scheme));
        if (m_view && m_reply) {
            m_reply(m_view, id, 0, nullptr);
        }
        return nullptr;
    }
    auto *request = new WebViewAuthRequest(this, id, fromLibrary(origin), fromLibrary(realm), flags, m_parent);
    m_pending.insert(id, request);
    trace(QStringLiteral("request %1: auth scheme %2%3%4")
              .arg(id)
              .arg(scheme)
              .arg(request->isRetry() ? QLatin1String(", retry") : QLatin1String())
              .arg(request->isProxy() ? QLatin1String(", proxy") : QLatin1String()));
    return request;
}

void WebViewRequests::answerIfUnhandled(WebViewRequest *request) {
    if (!request || !request->isPending() || request->isHandled()) {
        return;
    }
    if (auto *dialog = qobject_cast<WebViewDialogRequest *>(request)) {
        // as the library answers without a script_dialog callback: WebKit's "no" for alert, confirm and prompt, and
        // "leave" for the leave-page question (WebKit's own "stay" would keep the user on the page silently)
        const bool leave = dialog->type() == WebViewDialogRequest::LeavePage;
        sendReply(dialog, leave, QString(), leave ? "leave (not handled)" : "no (not handled)");
    } else {
        sendAuth(request, nullptr, nullptr, "without credentials (not handled)");
    }
}

void WebViewRequests::cancel(quint64 id) {
    WebViewRequest *request = m_pending.take(id);
    if (!request) {
        // answered here a moment ago, the library had ended it already (its answer returned -1)
        trace(QStringLiteral("request %1: cancelled, not pending here").arg(id));
        return;
    }
    trace(QStringLiteral("request %1: cancelled").arg(id));
    request->end(false, true);
}

void WebViewRequests::answerLeavePage(bool leave) {
    const QList<WebViewRequest *> pending = m_pending.values();
    for (WebViewRequest *request : pending) {
        auto *dialog = qobject_cast<WebViewDialogRequest *>(request);
        if (dialog && dialog->type() == WebViewDialogRequest::LeavePage && dialog->isPending()) {
            sendReply(dialog, leave, QString(), leave ? "leave (the host navigates)" : "stay (the host stops)");
        }
    }
}

void WebViewRequests::clear() {
    const QHash<quint64, WebViewRequest *> pending = std::exchange(m_pending, {});
    for (WebViewRequest *request : pending) {
        trace(QStringLiteral("request %1: ended (view freed)").arg(request->id()));
        request->end(false, false);
        delete request;
    }
}

bool WebViewRequests::take(WebViewRequest *request) {
    if (!request || request->owner() != this || !request->isPending() || m_pending.value(request->id()) != request) {
        return false;
    }
    m_pending.remove(request->id());
    return true;
}

bool WebViewRequests::reply(WebViewRequest *request, bool accept, const QString &text) {
    if (!qobject_cast<WebViewDialogRequest *>(request)) {
        return false;
    }
    const bool prompt = static_cast<WebViewDialogRequest *>(request)->type() == WebViewDialogRequest::Prompt;
    return sendReply(request, accept, text, accept ? (prompt ? "yes with text" : "yes") : "no");
}

bool WebViewRequests::authenticate(WebViewRequest *request, const QString *user, const QString *password) {
    if (!qobject_cast<WebViewAuthRequest *>(request)) {
        return false;
    }
    return sendAuth(request, user, password, user ? "with credentials" : "without credentials");
}

bool WebViewRequests::sendReply(WebViewRequest *request, bool accept, const QString &text, const char *how) {
    if (!take(request)) {
        return false;
    }
    int result = -1;
    if (m_view && m_reply) {
        // the text counts for a prompt only; NULL is "" there
        QByteArray utf8 = text.toUtf8();
        result = m_reply(m_view, request->id(), accept ? 1 : 0, accept ? utf8.constData() : nullptr);
        wipe(&utf8);
    }
    finish(request, result, how);
    return result == 0;
}

bool WebViewRequests::sendAuth(WebViewRequest *request, const QString *user, const QString *password, const char *how) {
    if (!take(request)) {
        return false;
    }
    int result = -1;
    if (m_view && m_auth) {
        // user NULL: continue without credentials; the copies are wiped once the library has them
        QByteArray userUtf8 = user ? user->toUtf8() : QByteArray();
        QByteArray passwordUtf8 = password ? password->toUtf8() : QByteArray();
        result = m_auth(m_view, request->id(), user ? userUtf8.constData() : nullptr,
                        user ? passwordUtf8.constData() : nullptr);
        wipe(&userUtf8);
        wipe(&passwordUtf8);
    }
    finish(request, result, how);
    return result == 0;
}

void WebViewRequests::finish(WebViewRequest *request, int result, const char *how) {
    if (result == 0) {
        trace(QStringLiteral("request %1: answered %2").arg(request->id()).arg(QLatin1String(how)));
        request->end(true, true);
        return;
    }
    // the library had ended it (its request_cancelled() is on the way and finds nothing here), or there is no view
    trace(QStringLiteral("request %1: answer refused by the library, the request had ended").arg(request->id()));
    qCInfo(lcWebView()) << "Request" << request->id() << "had ended in the web view: the answer was dropped";
    request->end(false, true);
}

}  // namespace webview
}  // namespace uc
