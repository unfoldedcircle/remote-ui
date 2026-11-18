// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The page's requests of libucwebview 0.7.0 without the item: the request objects and WebViewRequests against stubbed
// uc_webview_request_reply()/uc_webview_request_auth(). Every request ends exactly once (the answer or cancelled()),
// a second answer and an answer after the end reach the library never, and nothing the page or the user wrote reaches
// the trace.

#include <QPointer>
#include <QSignalSpy>
#include <QtTest>

#include <memory>

#include "webview/webviewrequest.h"

using uc::webview::WebViewAuthRequest;
using uc::webview::WebViewDialogRequest;
using uc::webview::WebViewRequest;
using uc::webview::WebViewRequests;

namespace stub {

uc_webview *const kView = reinterpret_cast<uc_webview *>(0x1234);

QStringList calls;       // "reply(id,accept,text|null)", "auth(id,user|null,password|null)"
int         result = 0;  // what the stubs return (-1: the library refuses, the request had ended)

QString orNull(const char *text) {
    return text ? QStringLiteral("\"%1\"").arg(QString::fromUtf8(text)) : QStringLiteral("null");
}

int reply(uc_webview *view, uint64_t id, int accept, const char *text) {
    if (view != kView) {
        return -1;
    }
    calls.append(QStringLiteral("reply(%1,%2,%3)").arg(id).arg(accept).arg(orNull(text)));
    return result;
}

int auth(uc_webview *view, uint64_t id, const char *user, const char *password) {
    if (view != kView) {
        return -1;
    }
    calls.append(QStringLiteral("auth(%1,%2,%3)").arg(id).arg(orNull(user), orNull(password)));
    return result;
}

}  // namespace stub

// one table with its parent and the trace lines it wrote
struct Fixture {
    QObject                          parent;
    QStringList                      trace;  // before the table: its destructor still traces
    std::unique_ptr<WebViewRequests> requests;

    Fixture() : requests(new WebViewRequests(&parent)) {
        stub::calls.clear();
        stub::result = 0;
        requests->setView(stub::kView, &stub::reply, &stub::auth);
        requests->setTrace([this](const QString &line) { trace.append(line); });
    }

    WebViewDialogRequest *dialog(quint64 id, int type, const char *origin = "https://example.com",
                                 const char *message = "Hello", const char *defaultText = nullptr) {
        return requests->addDialog(id, type, origin, message, defaultText);
    }

    WebViewAuthRequest *login(quint64 id, const char *origin = "https://example.com", unsigned flags = 0,
                              int scheme = UC_WEBVIEW_AUTH_SCHEME_HTTP_BASIC) {
        return requests->addAuth(id, origin, "Router", scheme, flags);
    }
};

// the deleteLater() of ended requests
void processDeletes() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

class testWebViewRequests : public QObject {
    Q_OBJECT

 private slots:
    void dialogsCarryTheLibrarysStrings();
    void acceptAndRejectAnswerOnce();
    void promptAnswersCarryTheText();
    void cancelledEndsTheRequestWithoutAnAnswer();
    void answerTheLibraryRefusesEndsAsCancelled();
    void endedRequestsAreDeletedLater();
    void unhandledRequestsGetTheLibrarysDefault();
    void unknownTypesAndSchemesAreAnsweredNo();
    void loginsSendTheCredentialsOnce();
    void loginFlags();
    void retryGetsTheUserNameOfItsLogin();
    void keptUserNameEndsWithLoadsTimeoutAndClear();
    void noUserNameIsKeptWithoutCredentials();
    void leavePageIsAnsweredBeforeANavigation();
    void clearEndsEveryRequestWithoutASignal();
    void answersNeedAViewAndTheRightKind();
    void traceCarriesNoTextsOrCredentials();
};

void testWebViewRequests::dialogsCarryTheLibrarysStrings() {
    Fixture               f;
    WebViewDialogRequest *prompt =
        f.dialog(7, UC_WEBVIEW_DIALOG_PROMPT, "https://example.com:8443", "Your name?", "Grüezi");
    QVERIFY(prompt);
    QCOMPARE(prompt->id(), quint64(7));
    QCOMPARE(prompt->type(), WebViewDialogRequest::Prompt);
    QCOMPARE(prompt->origin(), QStringLiteral("https://example.com:8443"));
    QCOMPARE(prompt->message(), QStringLiteral("Your name?"));
    QCOMPARE(prompt->defaultText(), QStringLiteral("Grüezi"));
    QVERIFY(prompt->isPending());
    QVERIFY(!prompt->isHandled());
    QCOMPARE(prompt->parent(), &f.parent);
    QCOMPARE(f.requests->pendingCount(), 1);

    // beforeunload has no message, a page without a host no origin
    WebViewDialogRequest *leave = f.dialog(8, UC_WEBVIEW_DIALOG_BEFORE_UNLOAD, nullptr, nullptr);
    QCOMPARE(leave->type(), WebViewDialogRequest::LeavePage);
    QVERIFY(leave->origin().isEmpty());
    QVERIFY(leave->message().isEmpty());
    QVERIFY(leave->defaultText().isEmpty());
    QCOMPARE(f.requests->pendingCount(), 2);
    QVERIFY(stub::calls.isEmpty());

    // a giant text is cut: a page cannot stall the UI's text layout with it
    const QByteArray      giant(3 * WebViewRequests::kMaxDialogTextLength, 'x');
    WebViewDialogRequest *cut = f.dialog(9, UC_WEBVIEW_DIALOG_PROMPT, nullptr, giant.constData(), giant.constData());
    QCOMPARE(cut->message().size(), WebViewRequests::kMaxDialogTextLength + 1);
    QVERIFY(cut->message().endsWith(QChar(0x2026)));
    QCOMPARE(cut->defaultText().size(), WebViewRequests::kMaxDialogTextLength + 1);

    QSignalSpy handled(prompt, &WebViewRequest::handledChanged);
    prompt->setHandled(true);
    prompt->setHandled(true);
    QVERIFY(prompt->isHandled());
    QCOMPARE(handled.count(), 1);
}

void testWebViewRequests::acceptAndRejectAnswerOnce() {
    Fixture               f;
    WebViewDialogRequest *confirm = f.dialog(3, UC_WEBVIEW_DIALOG_CONFIRM);
    QSignalSpy            finished(confirm, &WebViewRequest::finished);
    QSignalSpy            cancelled(confirm, &WebViewRequest::cancelled);
    QSignalSpy            pending(confirm, &WebViewRequest::pendingChanged);

    QVERIFY(confirm->accept());
    QCOMPARE(stub::calls, QStringList{QStringLiteral("reply(3,1,\"\")")});
    QVERIFY(!confirm->isPending());
    QCOMPARE(finished.count(), 1);
    QCOMPARE(cancelled.count(), 0);
    QCOMPARE(pending.count(), 1);
    QCOMPARE(f.requests->pendingCount(), 0);

    // a second answer, of either kind, never reaches the library
    QVERIFY(!confirm->accept());
    QVERIFY(!confirm->reject());
    QCOMPARE(stub::calls.size(), 1);
    QCOMPARE(finished.count(), 1);

    WebViewDialogRequest *alert = f.dialog(4, UC_WEBVIEW_DIALOG_ALERT);
    QVERIFY(alert->reject());
    QCOMPARE(stub::calls.last(), QStringLiteral("reply(4,0,null)"));
    QVERIFY(!alert->accept());
    QCOMPARE(stub::calls.size(), 2);

    // the library's late request_cancelled() of an answered request finds nothing
    f.requests->cancel(4);
    QCOMPARE(stub::calls.size(), 2);
}

void testWebViewRequests::promptAnswersCarryTheText() {
    Fixture               f;
    WebViewDialogRequest *prompt = f.dialog(5, UC_WEBVIEW_DIALOG_PROMPT, "https://example.com", "Name?", "x");
    QVERIFY(prompt->accept(QStringLiteral("Grüezi 1")));
    QCOMPARE(stub::calls.last(), QStringLiteral("reply(5,1,\"Grüezi 1\")"));

    // an empty answer is "" (the library takes NULL as "" too), a rejected prompt returns null to the page
    WebViewDialogRequest *empty = f.dialog(6, UC_WEBVIEW_DIALOG_PROMPT);
    QVERIFY(empty->accept());
    QCOMPARE(stub::calls.last(), QStringLiteral("reply(6,1,\"\")"));
    WebViewDialogRequest *rejected = f.dialog(7, UC_WEBVIEW_DIALOG_PROMPT);
    QVERIFY(rejected->reject());
    QCOMPARE(stub::calls.last(), QStringLiteral("reply(7,0,null)"));
}

void testWebViewRequests::cancelledEndsTheRequestWithoutAnAnswer() {
    Fixture               f;
    WebViewDialogRequest *alert = f.dialog(9, UC_WEBVIEW_DIALOG_ALERT);
    WebViewAuthRequest   *login = f.login(10);
    QSignalSpy            alertCancelled(alert, &WebViewRequest::cancelled);
    QSignalSpy            alertFinished(alert, &WebViewRequest::finished);
    QSignalSpy            loginCancelled(login, &WebViewRequest::cancelled);

    f.requests->cancel(9);
    QCOMPARE(alertCancelled.count(), 1);
    QCOMPARE(alertFinished.count(), 0);
    QVERIFY(!alert->isPending());
    QVERIFY(login->isPending());
    QCOMPARE(f.requests->pendingCount(), 1);

    // the dialog's late answer is dropped here, not sent
    QVERIFY(!alert->accept());
    QVERIFY(stub::calls.isEmpty());

    // twice, and an id never seen: nothing
    f.requests->cancel(9);
    f.requests->cancel(4711);
    QCOMPARE(alertCancelled.count(), 1);

    f.requests->cancel(10);
    QCOMPARE(loginCancelled.count(), 1);
    QVERIFY(!login->login(QStringLiteral("user"), QStringLiteral("secret")));
    QVERIFY(!login->cancel());
    QVERIFY(stub::calls.isEmpty());
    QCOMPARE(f.requests->pendingCount(), 0);
}

void testWebViewRequests::answerTheLibraryRefusesEndsAsCancelled() {
    // the library ended the request a moment before the answer (its request_cancelled() comes from an idle): -1
    Fixture               f;
    WebViewDialogRequest *confirm = f.dialog(11, UC_WEBVIEW_DIALOG_CONFIRM);
    QSignalSpy            finished(confirm, &WebViewRequest::finished);
    QSignalSpy            cancelled(confirm, &WebViewRequest::cancelled);
    stub::result = -1;
    QVERIFY(!confirm->accept());
    QCOMPARE(stub::calls.size(), 1);
    QCOMPARE(finished.count(), 0);
    QCOMPARE(cancelled.count(), 1);
    QVERIFY(!confirm->isPending());

    // the library's report that follows finds nothing, the request ended once
    f.requests->cancel(11);
    QCOMPARE(cancelled.count(), 1);
    QVERIFY(!confirm->accept());
    QCOMPARE(stub::calls.size(), 1);
}

void testWebViewRequests::endedRequestsAreDeletedLater() {
    Fixture                        f;
    QPointer<WebViewDialogRequest> answered = f.dialog(1, UC_WEBVIEW_DIALOG_ALERT);
    QPointer<WebViewDialogRequest> cancelled = f.dialog(2, UC_WEBVIEW_DIALOG_ALERT);
    QPointer<WebViewAuthRequest>   pending = f.login(3);
    answered->accept();
    f.requests->cancel(2);
    // still there for the handlers of finished()/cancelled() and the rest of this event
    QVERIFY(answered && cancelled);
    processDeletes();
    QVERIFY(!answered);
    QVERIFY(!cancelled);
    QVERIFY(pending);
}

void testWebViewRequests::unhandledRequestsGetTheLibrarysDefault() {
    Fixture f;
    // as the library answers without a host callback: WebKit's "no" for the three dialogs, leave for beforeunload,
    // without credentials for a login
    f.requests->answerIfUnhandled(f.dialog(1, UC_WEBVIEW_DIALOG_ALERT));
    f.requests->answerIfUnhandled(f.dialog(2, UC_WEBVIEW_DIALOG_CONFIRM));
    f.requests->answerIfUnhandled(f.dialog(3, UC_WEBVIEW_DIALOG_PROMPT, "https://example.com", "?", "preset"));
    f.requests->answerIfUnhandled(f.dialog(4, UC_WEBVIEW_DIALOG_BEFORE_UNLOAD, "https://example.com", nullptr));
    f.requests->answerIfUnhandled(f.login(5));
    QCOMPARE(stub::calls, (QStringList{QStringLiteral("reply(1,0,null)"), QStringLiteral("reply(2,0,null)"),
                                       QStringLiteral("reply(3,0,null)"), QStringLiteral("reply(4,1,\"\")"),
                                       QStringLiteral("auth(5,null,null)")}));
    QCOMPARE(f.requests->pendingCount(), 0);

    // a handled request waits for its dialog; an answered one and nullptr are left alone
    WebViewDialogRequest *handled = f.dialog(6, UC_WEBVIEW_DIALOG_CONFIRM);
    handled->setHandled(true);
    f.requests->answerIfUnhandled(handled);
    WebViewAuthRequest *answered = f.login(7);
    answered->setHandled(true);
    answered->cancel();
    answered->setHandled(false);
    f.requests->answerIfUnhandled(answered);
    f.requests->answerIfUnhandled(nullptr);
    QCOMPARE(stub::calls.size(), 6);
    QVERIFY(handled->isPending());
}

void testWebViewRequests::unknownTypesAndSchemesAreAnsweredNo() {
    // values of a newer library: uc_webview_request_reply(id, 0, NULL) at once (uc_webview.h, "Compatibility")
    Fixture f;
    QVERIFY(!f.dialog(20, 9));
    QVERIFY(!f.dialog(21, -1));
    QVERIFY(!f.login(22, "https://example.com", 0, UC_WEBVIEW_AUTH_SCHEME_UNKNOWN));
    QVERIFY(!f.login(23, "https://example.com", 0, 42));
    // no user name and password: Negotiate takes the system's ticket, server trust is no login
    QVERIFY(!f.login(24, "https://example.com", 0, UC_WEBVIEW_AUTH_SCHEME_NEGOTIATE));
    QVERIFY(!f.login(25, "https://example.com", 0, UC_WEBVIEW_AUTH_SCHEME_SERVER_TRUST));
    QVERIFY(!f.login(26, "https://example.com", 0, UC_WEBVIEW_AUTH_SCHEME_HTML_FORM));
    QCOMPARE(stub::calls, (QStringList{QStringLiteral("reply(20,0,null)"), QStringLiteral("reply(21,0,null)"),
                                       QStringLiteral("reply(22,0,null)"), QStringLiteral("reply(23,0,null)"),
                                       QStringLiteral("reply(24,0,null)"), QStringLiteral("reply(25,0,null)"),
                                       QStringLiteral("reply(26,0,null)")}));
    QCOMPARE(f.requests->pendingCount(), 0);

    // the user name and password schemes get a request
    QVERIFY(f.login(30, "https://example.com", 0, UC_WEBVIEW_AUTH_SCHEME_DEFAULT));
    QVERIFY(f.login(31, "https://example.com", 0, UC_WEBVIEW_AUTH_SCHEME_HTTP_BASIC));
    QVERIFY(f.login(32, "https://example.com", 0, UC_WEBVIEW_AUTH_SCHEME_HTTP_DIGEST));
    QVERIFY(f.login(33, "https://example.com", 0, UC_WEBVIEW_AUTH_SCHEME_NTLM));
    QCOMPARE(f.requests->pendingCount(), 4);
}

void testWebViewRequests::loginsSendTheCredentialsOnce() {
    Fixture             f;
    WebViewAuthRequest *login = f.login(40);
    QSignalSpy          finished(login, &WebViewRequest::finished);
    QVERIFY(login->login(QStringLiteral("ucuser"), QStringLiteral("ücsecret")));
    QCOMPARE(stub::calls, QStringList{QStringLiteral("auth(40,\"ucuser\",\"ücsecret\")")});
    QCOMPARE(finished.count(), 1);
    QVERIFY(!login->login(QStringLiteral("other"), QStringLiteral("other")));
    QVERIFY(!login->cancel());
    QCOMPARE(stub::calls.size(), 1);

    // an empty password is "", the user's cancel continues without credentials (the server's 401 page)
    WebViewAuthRequest *empty = f.login(41);
    QVERIFY(empty->login(QStringLiteral("admin"), QString()));
    QCOMPARE(stub::calls.last(), QStringLiteral("auth(41,\"admin\",\"\")"));
    WebViewAuthRequest *cancelled = f.login(42);
    QVERIFY(cancelled->cancel());
    QCOMPARE(stub::calls.last(), QStringLiteral("auth(42,null,null)"));
}

void testWebViewRequests::loginFlags() {
    Fixture             f;
    WebViewAuthRequest *plain = f.login(50, "https://example.com");
    QCOMPARE(plain->realm(), QStringLiteral("Router"));
    QVERIFY(!plain->isRetry());
    QVERIFY(!plain->isProxy());
    QVERIFY(!plain->isInsecure());

    WebViewAuthRequest *retry = f.login(51, "http://192.168.1.1", UC_WEBVIEW_AUTH_RETRY);
    QVERIFY(retry->isRetry());
    QVERIFY(!retry->isProxy());
    QVERIFY(retry->isInsecure());

    WebViewAuthRequest *proxy = f.login(52, "HTTP://proxy:3128", UC_WEBVIEW_AUTH_PROXY | UC_WEBVIEW_AUTH_RETRY);
    QVERIFY(proxy->isRetry());
    QVERIFY(proxy->isProxy());
    QVERIFY(proxy->isInsecure());

    // a realm may be missing
    WebViewAuthRequest *noRealm =
        f.requests->addAuth(53, "https://example.com", nullptr, UC_WEBVIEW_AUTH_SCHEME_HTTP_DIGEST, 0);
    QVERIFY(noRealm->realm().isEmpty());
}

void testWebViewRequests::retryGetsTheUserNameOfItsLogin() {
    // after a wrong login the dialog asks for the password only
    Fixture             f;
    WebViewAuthRequest *first = f.login(70);
    QVERIFY(first->previousUser().isEmpty());
    QVERIFY(!f.requests->keepsUser());
    QVERIFY(first->login(QStringLiteral("ucuser"), QStringLiteral("wrong")));
    QVERIFY(f.requests->keepsUser());

    // the server rejects it: the retry of the same origin, realm and proxy gets the user name, which leaves the table
    WebViewAuthRequest *retry = f.login(71, "https://example.com", UC_WEBVIEW_AUTH_RETRY);
    QCOMPARE(retry->previousUser(), QStringLiteral("ucuser"));
    QVERIFY(!f.requests->keepsUser());
    // the user may correct the user name too: the one sent last is kept
    QVERIFY(retry->login(QStringLiteral("ucuser2"), QStringLiteral("wrong again")));
    QVERIFY(f.requests->keepsUser());

    // logins of another origin, realm or proxy get nothing and leave it alone (several logins can be under way)
    QVERIFY(f.login(72, "https://other.example", UC_WEBVIEW_AUTH_RETRY)->previousUser().isEmpty());
    QVERIFY(f.requests
                ->addAuth(73, "https://example.com", "Other", UC_WEBVIEW_AUTH_SCHEME_HTTP_BASIC, UC_WEBVIEW_AUTH_RETRY)
                ->previousUser()
                .isEmpty());
    QVERIFY(f.login(74, "https://example.com", UC_WEBVIEW_AUTH_RETRY | UC_WEBVIEW_AUTH_PROXY)
                ->previousUser()
                .isEmpty());
    QVERIFY(f.requests->keepsUser());
    // another scheme for the same area is still its retry (Digest after Basic)
    WebViewAuthRequest *digest = f.requests->addAuth(75, "https://example.com", "Router",
                                                     UC_WEBVIEW_AUTH_SCHEME_HTTP_DIGEST, UC_WEBVIEW_AUTH_RETRY);
    QCOMPARE(digest->previousUser(), QStringLiteral("ucuser2"));
    QVERIFY(!f.requests->keepsUser());
    QCOMPARE(stub::calls.size(), 2);

    // a new login of the same area that is no retry (the chain ended) gets nothing and ends it
    QVERIFY(digest->login(QStringLiteral("ucuser3"), QStringLiteral("pw")));
    WebViewAuthRequest *fresh = f.login(76);
    QVERIFY(fresh->previousUser().isEmpty());
    QVERIFY(!f.requests->keepsUser());
    QVERIFY(f.login(77, "https://example.com", UC_WEBVIEW_AUTH_RETRY)->previousUser().isEmpty());

    // a proxy's login and its retry
    WebViewAuthRequest *proxy = f.login(78, "http://proxy:3128", UC_WEBVIEW_AUTH_PROXY);
    QVERIFY(proxy->login(QStringLiteral("proxyuser"), QStringLiteral("pw")));
    QCOMPARE(f.login(79, "http://proxy:3128", UC_WEBVIEW_AUTH_PROXY | UC_WEBVIEW_AUTH_RETRY)->previousUser(),
             QStringLiteral("proxyuser"));
}

void testWebViewRequests::keptUserNameEndsWithLoadsTimeoutAndClear() {
    Fixture f;
    // a load event of the view (the item calls forgetUser()): the page went on, the login is over
    QVERIFY(f.login(80)->login(QStringLiteral("ucuser"), QStringLiteral("pw")));
    QVERIFY(f.requests->keepsUser());
    f.requests->forgetUser();
    QVERIFY(!f.requests->keepsUser());
    QVERIFY(f.login(81, "https://example.com", UC_WEBVIEW_AUTH_RETRY)->previousUser().isEmpty());

    // no retry within the time: the login was accepted without a load (a script's request), or the server is gone
    QCOMPARE(WebViewRequests::kKeptUserTimeoutMs, 30000);
    f.requests->setKeptUserTimeoutForTest(50);
    QVERIFY(f.login(82)->login(QStringLiteral("ucuser"), QStringLiteral("pw")));
    QVERIFY(f.requests->keepsUser());
    QTRY_VERIFY_WITH_TIMEOUT(!f.requests->keepsUser(), 2000);
    QVERIFY(f.login(83, "https://example.com", UC_WEBVIEW_AUTH_RETRY)->previousUser().isEmpty());

    // a retry in time stops the clock: the user name lives on in the request only
    f.requests->setKeptUserTimeoutForTest(200);
    QVERIFY(f.login(84)->login(QStringLiteral("ucuser"), QStringLiteral("pw")));
    WebViewAuthRequest *retry = f.login(85, "https://example.com", UC_WEBVIEW_AUTH_RETRY);
    QCOMPARE(retry->previousUser(), QStringLiteral("ucuser"));
    QTest::qWait(300);
    QCOMPARE(retry->previousUser(), QStringLiteral("ucuser"));
    QVERIFY(!f.requests->keepsUser());

    // the view is freed
    QVERIFY(retry->login(QStringLiteral("ucuser"), QStringLiteral("pw")));
    QVERIFY(f.requests->keepsUser());
    f.requests->clear();
    QVERIFY(!f.requests->keepsUser());
}

void testWebViewRequests::noUserNameIsKeptWithoutCredentials() {
    Fixture f;
    // Cancel (continue without credentials), an empty user name, the default of an unhandled login
    QVERIFY(f.login(86)->cancel());
    QVERIFY(f.login(87)->login(QString(), QStringLiteral("pw")));
    f.requests->answerIfUnhandled(f.login(88));
    QCOMPARE(stub::calls.size(), 3);
    QVERIFY(!f.requests->keepsUser());

    // an answer the library refused (the request had ended): no retry will come
    stub::result = -1;
    QVERIFY(!f.login(89)->login(QStringLiteral("ucuser"), QStringLiteral("pw")));
    QVERIFY(!f.requests->keepsUser());
    stub::result = 0;

    // the kept user name of one login survives another login's Cancel
    QVERIFY(f.login(90)->login(QStringLiteral("ucuser"), QStringLiteral("pw")));
    QVERIFY(f.login(91, "https://other.example")->cancel());
    QCOMPARE(f.login(92, "https://example.com", UC_WEBVIEW_AUTH_RETRY)->previousUser(), QStringLiteral("ucuser"));
}

void testWebViewRequests::leavePageIsAnsweredBeforeANavigation() {
    Fixture               f;
    WebViewDialogRequest *leave = f.dialog(60, UC_WEBVIEW_DIALOG_BEFORE_UNLOAD, "https://example.com", nullptr);
    WebViewAuthRequest   *login = f.login(61);
    leave->setHandled(true);
    login->setHandled(true);
    QSignalSpy finished(leave, &WebViewRequest::finished);

    // the user goes elsewhere: leave; the login is the library's to cancel
    f.requests->answerLeavePage(true);
    QCOMPARE(stub::calls, QStringList{QStringLiteral("reply(60,1,\"\")")});
    QCOMPARE(finished.count(), 1);
    QVERIFY(login->isPending());

    // nothing pending: nothing to answer
    f.requests->answerLeavePage(true);
    QCOMPARE(stub::calls.size(), 1);

    // the user stops: stay; other dialogs are the library's to end
    WebViewDialogRequest *stay = f.dialog(62, UC_WEBVIEW_DIALOG_BEFORE_UNLOAD, "https://example.com", nullptr);
    WebViewDialogRequest *confirm = f.dialog(63, UC_WEBVIEW_DIALOG_CONFIRM);
    f.requests->answerLeavePage(false);
    QCOMPARE(stub::calls.last(), QStringLiteral("reply(62,0,null)"));
    QVERIFY(!stay->isPending());
    QVERIFY(confirm->isPending());
    QCOMPARE(stub::calls.size(), 2);
}

void testWebViewRequests::clearEndsEveryRequestWithoutASignal() {
    Fixture                        f;
    QPointer<WebViewDialogRequest> alert = f.dialog(70, UC_WEBVIEW_DIALOG_ALERT);
    QPointer<WebViewAuthRequest>   login = f.login(71);
    QSignalSpy                     alertFinished(alert.data(), &WebViewRequest::finished);
    QSignalSpy                     alertCancelled(alert.data(), &WebViewRequest::cancelled);
    QSignalSpy                     alertPending(alert.data(), &WebViewRequest::pendingChanged);
    QSignalSpy                     loginCancelled(login.data(), &WebViewRequest::cancelled);

    // uc_webview_free() ends them in the library without a callback; the objects go at once, without a signal
    f.requests->clear();
    QVERIFY(!alert);
    QVERIFY(!login);
    QCOMPARE(alertFinished.count(), 0);
    QCOMPARE(alertCancelled.count(), 0);
    QCOMPARE(alertPending.count(), 0);
    QCOMPARE(loginCancelled.count(), 0);
    QVERIFY(stub::calls.isEmpty());
    QCOMPARE(f.requests->pendingCount(), 0);
    QVERIFY(f.trace.contains(QStringLiteral("request 70: ended (view freed)")));

    // the table's destructor does the same
    QPointer<WebViewDialogRequest> left = f.dialog(72, UC_WEBVIEW_DIALOG_CONFIRM);
    f.requests.reset();
    QVERIFY(!left);
    QVERIFY(stub::calls.isEmpty());
}

void testWebViewRequests::answersNeedAViewAndTheRightKind() {
    Fixture               f;
    WebViewDialogRequest *alert = f.dialog(80, UC_WEBVIEW_DIALOG_ALERT);
    WebViewAuthRequest   *login = f.login(81);
    // the wrong function for the kind is refused before the library
    QVERIFY(!f.requests->authenticate(alert, nullptr, nullptr));
    QVERIFY(!f.requests->reply(login, false, QString()));
    QVERIFY(alert->isPending());
    QVERIFY(login->isPending());
    QVERIFY(stub::calls.isEmpty());

    // a request of another table is not answered by this one
    Fixture other;
    QVERIFY(!other.requests->reply(alert, true, QString()));
    QVERIFY(alert->isPending());

    // without a view (freed) an answer cannot go anywhere: the request ends as cancelled
    f.requests->setView(nullptr, nullptr, nullptr);
    QSignalSpy cancelled(alert, &WebViewRequest::cancelled);
    QVERIFY(!alert->accept());
    QCOMPARE(cancelled.count(), 1);
    QVERIFY(stub::calls.isEmpty());
}

void testWebViewRequests::traceCarriesNoTextsOrCredentials() {
    // UC_WEBVIEW_TRACE=1 output is shared in test reports: kind, id and outcome only
    Fixture f;
    f.dialog(90, UC_WEBVIEW_DIALOG_ALERT, "https://uc-origin.example", "uc-alert")->accept();
    f.dialog(91, UC_WEBVIEW_DIALOG_PROMPT, "https://uc-origin.example", "uc-prompt", "uc-default")
        ->accept(QStringLiteral("uc-answer"));
    f.dialog(92, UC_WEBVIEW_DIALOG_CONFIRM, "https://uc-origin.example", "uc-confirm")->reject();
    f.requests->answerIfUnhandled(f.dialog(93, UC_WEBVIEW_DIALOG_BEFORE_UNLOAD, "https://uc-origin.example", nullptr));
    f.requests
        ->addAuth(94, "https://uc-origin.example", "uc-realm", UC_WEBVIEW_AUTH_SCHEME_HTTP_BASIC, UC_WEBVIEW_AUTH_RETRY)
        ->login(QStringLiteral("ucuser"), QStringLiteral("ucsecret"));
    // its retry carries the user name, the trace only says so
    f.requests
        ->addAuth(95, "https://uc-origin.example", "uc-realm", UC_WEBVIEW_AUTH_SCHEME_HTTP_DIGEST,
                  UC_WEBVIEW_AUTH_RETRY)
        ->cancel();
    f.dialog(96, UC_WEBVIEW_DIALOG_ALERT, "https://uc-origin.example", "uc-alert");
    f.requests->cancel(96);
    f.dialog(97, UC_WEBVIEW_DIALOG_CONFIRM, "https://uc-origin.example", "uc-confirm");
    f.requests->clear();

    QCOMPARE(f.trace, (QStringList{
                          QStringLiteral("request 90: alert"),
                          QStringLiteral("request 90: answered yes"),
                          QStringLiteral("request 91: prompt"),
                          QStringLiteral("request 91: answered yes with text"),
                          QStringLiteral("request 92: confirm"),
                          QStringLiteral("request 92: answered no"),
                          QStringLiteral("request 93: beforeunload"),
                          QStringLiteral("request 93: answered leave (not handled)"),
                          QStringLiteral("request 94: auth scheme 2, retry"),
                          QStringLiteral("request 94: answered with credentials"),
                          QStringLiteral("request 95: auth scheme 3, retry, previous user name"),
                          QStringLiteral("request 95: answered without credentials"),
                          QStringLiteral("request 96: alert"),
                          QStringLiteral("request 96: cancelled"),
                          QStringLiteral("request 97: confirm"),
                          QStringLiteral("request 97: ended (view freed)"),
                      }));
    for (const QString &line : qAsConst(f.trace)) {
        QVERIFY2(!line.contains(QLatin1String("uc-")) && !line.contains(QLatin1String("ucuser")) &&
                     !line.contains(QLatin1String("ucsecret")),
                 qPrintable(line));
    }
}

QTEST_GUILESS_MAIN(testWebViewRequests)
#include "test_webview_requests.moc"
