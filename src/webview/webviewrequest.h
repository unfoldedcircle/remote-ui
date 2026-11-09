// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

#include <functional>
#include <utility>

#include "uc_webview.h"

namespace uc {
namespace webview {

class WebViewRequests;

/**
 * A request of the page that waits for the user (libucwebview 0.7.0, docs/webview.md "Dialogs and logins"): a script
 * dialog (WebViewDialogRequest) or a login (WebViewAuthRequest). After Qt WebEngine's JavaScriptDialogRequest and
 * AuthenticationDialogRequest, with the `pending` state and the cancelled() signal Qt 5.15 lacks.
 *
 * - The WebView item emits it with dialogRequested() or authenticationRequested(). A handler that shows a dialog sets
 *   `handled` during the signal; the item answers a request nobody handled at once with the default.
 * - It ends exactly once: the answer (finished()), or cancelled() when it went away first (a navigation, a commit, a
 *   crash, WebKit gave the login up, or the library refused a late answer). `pending` turns false before either
 *   signal. A dialog closes on cancelled() without answering; an answer to an ended request does nothing and returns
 *   false.
 * - The item owns it (QObject parent, C++ ownership) and deletes it after it ended (deleteLater), or at once and
 *   without a signal when its view is freed. A QML property typed with the request type turns null then.
 * - The library's id never reaches QML; the texts are copies of the library's strings.
 */
class WebViewRequest : public QObject {
    Q_OBJECT
    // "scheme://host[:port]" of the page's last committed address (dialogs) or of the server or proxy (logins); empty
    // for a page without a host (about:blank, data:)
    Q_PROPERTY(QString origin READ origin CONSTANT)
    Q_PROPERTY(bool pending READ isPending NOTIFY pendingChanged)
    // set by the handler of the item's signal that shows the request; the item answers it otherwise
    Q_PROPERTY(bool handled READ isHandled WRITE setHandled NOTIFY handledChanged)

 public:
    quint64 id() const { return m_id; }
    QString origin() const { return m_origin; }
    bool    isPending() const { return m_pending; }
    bool    isHandled() const { return m_handled; }
    void    setHandled(bool handled);

 signals:
    void pendingChanged();
    void handledChanged();
    // the answer went to the page
    void finished();
    // the request went away before or instead of the answer: close its dialog, do not answer
    void cancelled();

 protected:
    WebViewRequest(WebViewRequests *owner, quint64 id, const QString &origin, QObject *parent);

    // the table that answers the request, nullptr once it ended
    WebViewRequests *owner() const { return m_owner; }

 private:
    friend class WebViewRequests;

    // WebViewRequests: the request ended with the answer (finished) or without (cancelled); emitSignals false when
    // the view is freed
    void end(bool answered, bool emitSignals);

    WebViewRequests *m_owner;
    const quint64    m_id;
    const QString    m_origin;
    bool             m_pending = true;
    bool             m_handled = false;
};

/**
 * alert(), confirm(), prompt() or the leave-page question of a beforeunload handler. The page waits until the answer:
 * no script, timer or rendering update of it runs meanwhile.
 */
class WebViewDialogRequest : public WebViewRequest {
    Q_OBJECT
    Q_PROPERTY(Type type READ type CONSTANT)
    // as the page wrote it, any length; empty for LeavePage (pages cannot word that question, show a fixed text)
    Q_PROPERTY(QString message READ message CONSTANT)
    // the prompt's preset text
    Q_PROPERTY(QString defaultText READ defaultText CONSTANT)

 public:
    // UC_WEBVIEW_DIALOG_ALERT .. _BEFORE_UNLOAD
    enum Type { Alert, Confirm, Prompt, LeavePage };
    Q_ENUM(Type)

    WebViewDialogRequest(WebViewRequests *owner, quint64 id, Type type, const QString &origin, const QString &message,
                         const QString &defaultText, QObject *parent);

    Type    type() const { return m_type; }
    QString message() const { return m_message; }
    QString defaultText() const { return m_defaultText; }

    // The user's yes: alert closed, confirm() true, prompt() returns text, leave the page. False when the request has
    // ended already.
    Q_INVOKABLE bool accept(const QString &text = QString());
    // The user's no: alert closed, confirm() false, prompt() null, stay on the page.
    Q_INVOKABLE bool reject();

 private:
    const Type    m_type;
    const QString m_message;
    const QString m_defaultText;
};

/**
 * A server or proxy asks for a user name and password (HTTP Basic, Digest, NTLM). The credentials are kept by WebKit
 * for the session only, never saved and never logged.
 */
class WebViewAuthRequest : public WebViewRequest {
    Q_OBJECT
    // the server's name for the protected area, its own text (show it attributed)
    Q_PROPERTY(QString realm READ realm CONSTANT)
    // the credentials sent for this login before were rejected
    Q_PROPERTY(bool retry READ isRetry CONSTANT)
    // On a retry: the user name sent with the rejected credentials, when this login is the retry of the login answered
    // here last with a user name (same origin, realm and proxy, WebViewRequests "Retry"); a dialog fills it in and asks
    // for the password only. Empty otherwise. Never the password.
    Q_PROPERTY(QString previousUser READ previousUser CONSTANT)
    // a proxy asks, origin is the proxy's
    Q_PROPERTY(bool proxy READ isProxy CONSTANT)
    // the origin is http: the connection is not encrypted (with Basic the password travels readable)
    Q_PROPERTY(bool insecure READ isInsecure CONSTANT)

 public:
    WebViewAuthRequest(WebViewRequests *owner, quint64 id, const QString &origin, const QString &realm, unsigned flags,
                       const QString &previousUser, QObject *parent);

    QString realm() const { return m_realm; }
    bool    isRetry() const { return m_retry; }
    bool    isProxy() const { return m_proxy; }
    bool    isInsecure() const;
    QString previousUser() const { return m_previousUser; }

    // Sends the credentials. False when the request has ended already.
    Q_INVOKABLE bool login(const QString &user, const QString &password);
    // Continues without credentials: the server's own page for a refused login (401) loads.
    Q_INVOKABLE bool cancel();

 private:
    const QString m_realm;
    const bool    m_retry;
    const bool    m_proxy;
    const QString m_previousUser;
};

/**
 * The item's table of pending requests: creates the request objects for the library's callbacks, sends their answers
 * with uc_webview_request_reply()/uc_webview_request_auth() and ends them on request_cancelled(). GUI thread only,
 * like every library call. The request objects are children of `parent` (the WebView item).
 *
 * The trace (setTrace) gets one line per request event: kind, id and outcome, never a message, a default text, an
 * answer, a realm, an origin, a user name or a password.
 *
 * Retry: after a wrong login the user does not type the user name again. When a login is
 * answered with a non-empty user name and the library takes it, the table keeps that user name with the login's
 * origin, realm and proxy flag. The next login request with the same three takes it when it is a retry (its
 * previousUser) and ends it otherwise (a new login); a login request for another origin, realm or proxy leaves it
 * alone (several logins can be under way). A rejected login comes back before any load event: libsoup emits the
 * retry's "authenticate" on the server's next 401, before WebKit sees a response (NetworkDataTaskSoup.cpp:570-579).
 * So every load event of the view (forgetUser(): the page went on, with the login accepted or given up),
 * kKeptUserTimeoutMs without a retry and the view freed (clear()) drop it. The password is never kept.
 */
class WebViewRequests {
 public:
    using ReplyFunction = int (*)(uc_webview *, uint64_t, int, const char *);
    using AuthFunction = int (*)(uc_webview *, uint64_t, const char *, const char *);
    using Trace = std::function<void(const QString &line)>;

    // A dialog's message and preset text keep at most this many characters (plus an ellipsis): more than a dialog on
    // the remote's screen can show, and a page cannot stall the UI with a giant text.
    static constexpr int kMaxDialogTextLength = 10000;
    // How long the user name of a login waits for its retry: the server rejects credentials within one HTTP round trip;
    // a retry that comes later starts with an empty user name.
    static constexpr int kKeptUserTimeoutMs = 30000;

    explicit WebViewRequests(QObject *parent);
    ~WebViewRequests();

    WebViewRequests(const WebViewRequests &) = delete;
    WebViewRequests &operator=(const WebViewRequests &) = delete;

    // The view the answers go to and the library's reply functions (library 0.7.0); a null view refuses answers.
    void setView(uc_webview *view, ReplyFunction replyFunction, AuthFunction authFunction);
    void setTrace(Trace trace) { m_trace = std::move(trace); }

    // script_dialog(): the request of a known type, pending; an unknown type is answered "no" at once (nullptr)
    WebViewDialogRequest *addDialog(quint64 id, int type, const char *origin, const char *message,
                                    const char *defaultText);
    // authenticate(): the request of a login scheme (user name and password), pending; other schemes are answered
    // without credentials at once (nullptr)
    WebViewAuthRequest *addAuth(quint64 id, const char *origin, const char *realm, int scheme, unsigned flags);
    // After the item's signal: a request no handler took gets the library's default without a host (alert, confirm and
    // prompt closed with "no", leave page: leave, login: without credentials).
    void answerIfUnhandled(WebViewRequest *request);
    // request_cancelled(): the request ends with cancelled(); an unknown or ended id is ignored
    void cancel(quint64 id);
    // Before the host navigates: a pending leave-page question is answered here, leave (the user goes elsewhere) or
    // stay (stop), instead of the library's "stay" for every navigation call.
    void answerLeavePage(bool leave);
    // The view is freed (uc_webview_free() ends its requests without a callback): every request ends without a signal
    // and is deleted. The kept user name goes too.
    void clear();
    // A load event of the view: the login whose user name waits for a retry is over (see "Retry" above).
    void forgetUser();

    int  pendingCount() const { return m_pending.size(); }
    // a user name waits for the retry of its login
    bool keepsUser() const { return !m_keptUser.isEmpty(); }
    void setKeptUserTimeoutForTest(int msec) { m_keptUserTimer.setInterval(msec); }

    // The answers of the request objects; false when the request is not pending here.
    bool reply(WebViewRequest *request, bool accept, const QString &text);
    bool authenticate(WebViewRequest *request, const QString *user, const QString *password);

 private:
    bool take(WebViewRequest *request);
    bool sendReply(WebViewRequest *request, bool accept, const QString &text, const char *how);
    bool sendAuth(WebViewRequest *request, const QString *user, const QString *password, const char *how);
    void finish(WebViewRequest *request, int result, const char *how);
    void trace(const QString &line) const;

    void keepUser(const QString &user, const QString &origin, const QString &realm, bool proxy);

    QObject                         *m_parent;
    uc_webview                      *m_view = nullptr;
    ReplyFunction                    m_reply = nullptr;
    AuthFunction                     m_auth = nullptr;
    Trace                            m_trace;
    QHash<quint64, WebViewRequest *> m_pending;

    // the user name of the last login answered with credentials and the login it is kept for (see "Retry" above)
    QString m_keptUser;
    QString m_keptOrigin;
    QString m_keptRealm;
    bool    m_keptProxy = false;
    QTimer  m_keptUserTimer;
};

}  // namespace webview
}  // namespace uc
