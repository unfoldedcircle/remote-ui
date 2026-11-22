// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonDocument>
#include <QtTest>

#include "hardware/hardwareModel.h"
#include "ui/entity/webview.h"
#include "ui/entity/webviewButtons.h"
#include "webview/webviewsite.h"

using uc::hw::HardwareModel;
using uc::ui::entity::Webview;
using uc::ui::entity::WebviewButtons;
using uc::ui::entity::WebviewStates;
using uc::webview::WebViewSite;

/**
 * The web view entity (core-api doc/entities/entity_webview.md): its options with the user overrides, the values the
 * core would refuse, the secret options that only the UI's session receives, and the site settings the WebView item
 * applies (credentials only for the start page's origin).
 */
class testWebviewEntity : public QObject {
    Q_OBJECT

 private slots:
    void defaults();
    void options_areApplied();
    void userOverrides_winOverTheOptions();
    void userOverrides_nullOrInvalidAreIgnored();
    void deviceApi_optionAndUserOverride();
    void invalidValues_fallBackToTheDefaults();
    void url_onlyHttpAndHttpsWithoutCredentials();
    void state_andTileText();

    void site_basicAuth();
    void site_bearerAndHeaderAuth();
    void site_cookiesWithDefaults();
    void site_cookieWithoutValueIsSkipped();
    void site_tlsException();
    void site_changeIsSignalledOnlyWhenItChanged();

    void secretsInOptions_tellsAnEventFromTheUisAnswer();

    void login_onlyForTheStartPagesOrigin();
    void login_neverForAProxyARetryOrAnotherScheme();
    void origin_defaultPortsAndCase();
    void sha256_isNormalised();
    void requestOptions_nameWhatTheLibraryCannotApply();

    void buttons_ofEachModel();
    void buttons_defaultActions();
    void buttons_validActions_data();
    void buttons_validActions();
    void buttons_parseKeepsOnlyValidEntries();
    void buttons_withDefaultsHasEveryButton();
    void buttons_heldRepeatsKeysAndScrolling();
    void buttons_optionAndUserOverrides();

 private:
    static QVariantMap fromJson(const char *json) { return QJsonDocument::fromJson(json).toVariant().toMap(); }
    static Webview    *make(const QVariantMap &options, const QVariantMap &attributes = QVariantMap());
};

Webview *testWebviewEntity::make(const QVariantMap &options, const QVariantMap &attributes) {
    return new Webview(QStringLiteral("uc_web.main.1"), QVariantMap({{"en", "Dashboard"}}), QStringLiteral("en"),
                       QString(), QString(), true, attributes, options, QStringLiteral("uc_web.main"), nullptr);
}

void testWebviewEntity::defaults() {
    QScopedPointer<Webview> w(make(fromJson(R"({"url": "https://dash.example.com/"})")));
    QCOMPARE(w->getType(), uc::ui::entity::Base::Type::Webview);
    QCOMPARE(w->getIcon(), QStringLiteral("uc:globe"));
    QCOMPARE(w->getDeviceClass(), QStringLiteral("Webview"));
    QCOMPARE(w->getUrl(), QUrl("https://dash.example.com/"));
    QCOMPARE(w->getZoom(), 100);
    QCOMPARE(w->getBackgroundColor(), QStringLiteral("#FFFFFF"));
    QVERIFY(!w->getShowNavigation());
    QCOMPARE(w->getReloadInterval(), 0);
    QVERIFY(!w->getDeviceApi());
    QVERIFY(!w->isEditable());
    QCOMPARE(w->getSite().auth(), WebViewSite::Auth::None);
    QCOMPARE(w->getSite().userAgent(), QStringLiteral("default"));
    QVERIFY(w->getSite().cookies().isEmpty());
    QVERIFY(!w->getSite().hasTlsException());
}

void testWebviewEntity::options_areApplied() {
    QScopedPointer<Webview> w(make(fromJson(R"({
        "editable": true, "url": "http://inverter.home.arpa/status", "zoom": 70, "background_color": "#102030",
        "show_navigation": true, "reload_interval": 60, "user_agent": "desktop"
    })")));
    QCOMPARE(w->getUrl(), QUrl("http://inverter.home.arpa/status"));
    QCOMPARE(w->getZoom(), 70);
    QCOMPARE(w->getBackgroundColor(), QStringLiteral("#102030"));
    QVERIFY(w->getShowNavigation());
    QCOMPARE(w->getReloadInterval(), 60);
    QVERIFY(w->isEditable());
    QCOMPARE(w->getSite().userAgent(), QStringLiteral("desktop"));
}

void testWebviewEntity::userOverrides_winOverTheOptions() {
    QScopedPointer<Webview> w(make(fromJson(R"({
        "url": "https://a.example/", "zoom": 70, "show_navigation": true, "reload_interval": 60,
        "user": {"zoom": 60, "show_navigation": false, "reload_interval": 0}
    })")));
    QCOMPARE(w->getZoom(), 60);
    QVERIFY(!w->getShowNavigation());
    QCOMPARE(w->getReloadInterval(), 0);

    // removing an override brings the option back
    QSignalSpy changed(w.data(), &Webview::optionsChanged);
    w->updateOptions(fromJson(R"({"url": "https://a.example/", "zoom": 70, "show_navigation": true,
                                  "reload_interval": 60, "user": {"zoom": 60}})"));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(w->getZoom(), 60);
    QVERIFY(w->getShowNavigation());
    QCOMPARE(w->getReloadInterval(), 60);
}

void testWebviewEntity::userOverrides_nullOrInvalidAreIgnored() {
    QScopedPointer<Webview> w(make(fromJson(R"({
        "url": "https://a.example/", "zoom": 80, "reload_interval": 30,
        "user": {"zoom": null, "reload_interval": 5, "show_navigation": null}
    })")));
    QCOMPARE(w->getZoom(), 80);
    QCOMPARE(w->getReloadInterval(), 30);
    QVERIFY(!w->getShowNavigation());
}

void testWebviewEntity::deviceApi_optionAndUserOverride() {
    // a user-defined web view: the option itself
    QScopedPointer<Webview> w(make(fromJson(R"({"editable": true, "url": "https://a.example/", "device_api": true})")));
    QVERIFY(w->getDeviceApi());

    // a web view of a driver: its default, and the user's override over it, both ways
    QSignalSpy changed(w.data(), &Webview::optionsChanged);
    w->updateOptions(fromJson(R"({"url": "https://a.example/", "device_api": true, "user": {"device_api": false}})"));
    QVERIFY(!w->getDeviceApi());
    w->updateOptions(fromJson(R"({"url": "https://a.example/", "device_api": false, "user": {"device_api": true}})"));
    QVERIFY(w->getDeviceApi());
    // null and a missing override leave the option
    w->updateOptions(fromJson(R"({"url": "https://a.example/", "device_api": true, "user": {"device_api": null}})"));
    QVERIFY(w->getDeviceApi());
    w->updateOptions(fromJson(R"({"url": "https://a.example/", "user": {"zoom": 80}})"));
    QVERIFY(!w->getDeviceApi());
    QCOMPARE(changed.count(), 4);
}

void testWebviewEntity::invalidValues_fallBackToTheDefaults() {
    QScopedPointer<Webview> w(make(fromJson(R"({
        "url": "https://a.example/", "zoom": 500, "reload_interval": 5, "background_color": "red"
    })")));
    QCOMPARE(w->getZoom(), 100);
    QCOMPARE(w->getReloadInterval(), 0);
    QCOMPARE(w->getBackgroundColor(), QStringLiteral("#FFFFFF"));

    QCOMPARE(Webview::validZoom(25), 25);
    QCOMPARE(Webview::validZoom(400), 400);
    QCOMPARE(Webview::validZoom(24), -1);
    QCOMPARE(Webview::validZoom(QStringLiteral("x")), -1);
    QCOMPARE(Webview::validReloadInterval(0), 0);
    QCOMPARE(Webview::validReloadInterval(10), 10);
    QCOMPARE(Webview::validReloadInterval(86400), 86400);
    QCOMPARE(Webview::validReloadInterval(9), -1);
    QCOMPARE(Webview::validReloadInterval(86401), -1);
}

void testWebviewEntity::url_onlyHttpAndHttpsWithoutCredentials() {
    QCOMPARE(Webview::validUrl(QStringLiteral("https://example.com/a?b=c")), QUrl("https://example.com/a?b=c"));
    QCOMPARE(Webview::validUrl(QStringLiteral("HTTP://example.com")), QUrl("HTTP://example.com"));
    QVERIFY(Webview::validUrl(QStringLiteral("file:///etc/passwd")).isEmpty());
    QVERIFY(Webview::validUrl(QStringLiteral("javascript:alert(1)")).isEmpty());
    QVERIFY(Webview::validUrl(QStringLiteral("data:text/html,hi")).isEmpty());
    QVERIFY(Webview::validUrl(QStringLiteral("https://user:secret@example.com/")).isEmpty());
    QVERIFY(Webview::validUrl(QStringLiteral("example.com")).isEmpty());
    QVERIFY(Webview::validUrl(QStringLiteral("https://example.com/") + QString(2048, 'a')).isEmpty());
    QVERIFY(Webview::validUrl(QString()).isEmpty());

    QScopedPointer<Webview> w(make(fromJson(R"({"url": "ftp://example.com/"})")));
    QVERIFY(w->getUrl().isEmpty());
    QVERIFY(w->getSite().origin().isEmpty());
}

void testWebviewEntity::state_andTileText() {
    QScopedPointer<Webview> w(make(fromJson(R"({"url": "https://a.example/"})"), fromJson(R"({"state": "ON"})")));
    QCOMPARE(w->getState(), static_cast<int>(WebviewStates::On));
    // an available web view shows its icon and name only
    QVERIFY(w->getStateInfo().isEmpty());

    QSignalSpy stateChanged(w.data(), &Webview::stateChanged);
    QVERIFY(w->updateAttribute(QStringLiteral("State"), QStringLiteral("UNAVAILABLE")));
    QCOMPARE(stateChanged.count(), 1);
    QCOMPARE(w->getState(), static_cast<int>(WebviewStates::Unavailable));
    QCOMPARE(w->getStateInfo(), QStringLiteral("Unavailable"));
    QVERIFY(!w->isEnabled());

    // a state the UI does not know is skipped (ADR 0018)
    QVERIFY(!w->updateAttribute(QStringLiteral("State"), QStringLiteral("SOMETHING_NEW")));
    QCOMPARE(w->getState(), static_cast<int>(WebviewStates::Unavailable));
}

void testWebviewEntity::site_basicAuth() {
    QScopedPointer<Webview> w(make(fromJson(R"({
        "url": "https://nas.home.arpa:5001/status",
        "auth": {"type": "basic", "username": "viewer", "password": "my-secret"}
    })")));
    const WebViewSite       site = w->getSite();
    QCOMPARE(site.auth(), WebViewSite::Auth::Basic);
    QCOMPARE(site.authUser(), QStringLiteral("viewer"));
    QCOMPARE(site.origin(), QStringLiteral("https://nas.home.arpa:5001"));
    // basic goes as the answer to a login, not as a header
    QVERIFY(site.authHeaderName().isEmpty());
    QVERIFY(site.requestOptions().isEmpty());

    QString user;
    QString password;
    QVERIFY(site.loginFor(QStringLiteral("https://nas.home.arpa:5001"), true, false, false, &user, &password));
    QCOMPARE(user, QStringLiteral("viewer"));
    QCOMPARE(password, QStringLiteral("my-secret"));
}

void testWebviewEntity::site_bearerAndHeaderAuth() {
    QScopedPointer<Webview> bearer(make(fromJson(R"({
        "url": "https://dashboard.example.com/energy?kiosk=1", "auth": {"type": "bearer", "token": "abc.def"}
    })")));
    QCOMPARE(bearer->getSite().authHeaderName(), QStringLiteral("Authorization"));
    QCOMPARE(bearer->getSite().authHeaderValue(), QStringLiteral("Bearer abc.def"));
    QCOMPARE(bearer->getSite().requestOptions(), QStringList{QStringLiteral("auth.bearer")});
    QVERIFY(!bearer->getSite().loginFor(QStringLiteral("https://dashboard.example.com"), true, false, false, nullptr,
                                        nullptr));

    QScopedPointer<Webview> header(make(fromJson(R"({
        "url": "http://inverter.home.arpa/status",
        "auth": {"type": "header", "header_name": "X-API-Key", "header_value": "3b1f0c9e7a"}
    })")));
    QCOMPARE(header->getSite().authHeaderName(), QStringLiteral("X-API-Key"));
    QCOMPARE(header->getSite().authHeaderValue(), QStringLiteral("3b1f0c9e7a"));
    QCOMPARE(header->getSite().requestOptions(), QStringList{QStringLiteral("auth.header")});

    // the flag of a session without the secrets is not the secret: no header to send
    QScopedPointer<Webview> flagged(make(fromJson(R"({
        "url": "https://a.example/", "auth": {"type": "bearer", "has_token": true}
    })")));
    QVERIFY(flagged->getSite().authHeaderName().isEmpty());
    QVERIFY(!flagged->getSite().hasAuthSecret());
}

void testWebviewEntity::site_cookiesWithDefaults() {
    QScopedPointer<Webview>          w(make(fromJson(R"({
        "url": "https://a.example/",
        "cookies": [{"name": "layout", "value": "compact"},
                    {"name": "session", "value": "s3cr3t", "domain": "example", "path": "/app", "secure": false,
                     "http_only": true, "expires": "2027-01-01T00:00:00Z"}]
    })")));
    const QList<WebViewSite::Cookie> cookies = w->getSite().cookies();
    QCOMPARE(cookies.size(), 2);
    QCOMPARE(cookies.at(0).name, QStringLiteral("layout"));
    QCOMPARE(cookies.at(0).value, QStringLiteral("compact"));
    QVERIFY(cookies.at(0).domain.isEmpty());
    QCOMPARE(cookies.at(0).path, QStringLiteral("/"));
    // secure by default for an https start page, a session cookie without expires
    QVERIFY(cookies.at(0).secure);
    QVERIFY(!cookies.at(0).httpOnly);
    QVERIFY(!cookies.at(0).expires.isValid());
    QCOMPARE(cookies.at(1).path, QStringLiteral("/app"));
    QVERIFY(!cookies.at(1).secure);
    QVERIFY(cookies.at(1).httpOnly);
    QCOMPARE(cookies.at(1).expires, QDateTime(QDate(2027, 1, 1), QTime(0, 0), Qt::UTC));
    QCOMPARE(w->getSite().requestOptions(), QStringList{QStringLiteral("cookies")});

    QScopedPointer<Webview> plain(
        make(fromJson(R"({"url": "http://a.example/", "cookies": [{"name": "a", "value": "b"}]})")));
    QVERIFY(!plain->getSite().cookies().first().secure);
}

void testWebviewEntity::site_cookieWithoutValueIsSkipped() {
    QScopedPointer<Webview> w(make(fromJson(R"({
        "url": "https://a.example/", "cookies": [{"name": "layout", "has_value": true}, {"value": "x"}]
    })")));
    QVERIFY(w->getSite().cookies().isEmpty());
}

void testWebviewEntity::site_tlsException() {
    QScopedPointer<Webview> fingerprint(make(fromJson(R"({
        "url": "https://nas.home.arpa:5001/",
        "tls": {"certificate_sha256":
                "3F:A9:12:5C:77:0B:E4:21:9D:6A:C8:0F:41:B2:93:EE:58:17:0C:D4:2A:9B:61:F0:7E:3C:88:15:A6:D2:4B:C9",
                "allow_invalid_certificate": true}
    })")));
    QCOMPARE(fingerprint->getSite().tlsCertificateSha256(),
             QStringLiteral("3fa9125c770be4219d6ac80f41b293ee58170cd42a9b61f07e3c8815a6d24bc9"));
    // the fingerprint wins over allow_invalid_certificate
    QVERIFY(!fingerprint->getSite().tlsAllowInvalidCertificate());
    QCOMPARE(fingerprint->getSite().requestOptions(), QStringList{QStringLiteral("tls")});

    QScopedPointer<Webview> any(make(fromJson(R"({
        "url": "https://nas.home.arpa/", "tls": {"allow_invalid_certificate": true}
    })")));
    QVERIFY(any->getSite().tlsAllowInvalidCertificate());
    QVERIFY(any->getSite().hasTlsException());
}

void testWebviewEntity::site_changeIsSignalledOnlyWhenItChanged() {
    QScopedPointer<Webview> w(make(fromJson(R"({"url": "https://a.example/", "zoom": 80})")));
    QSignalSpy              siteChanged(w.data(), &Webview::siteChanged);
    QSignalSpy              optionsChanged(w.data(), &Webview::optionsChanged);

    // a display option only
    w->updateOptions(fromJson(R"({"url": "https://a.example/", "zoom": 90})"));
    QCOMPARE(optionsChanged.count(), 1);
    QCOMPARE(siteChanged.count(), 0);

    // a rotated token
    w->updateOptions(
        fromJson(R"({"url": "https://a.example/", "zoom": 90, "auth": {"type": "bearer", "token": "1"}})"));
    QCOMPARE(siteChanged.count(), 1);
    w->updateOptions(
        fromJson(R"({"url": "https://a.example/", "zoom": 90, "auth": {"type": "bearer", "token": "2"}})"));
    QCOMPARE(siteChanged.count(), 2);
    w->updateOptions(
        fromJson(R"({"url": "https://a.example/", "zoom": 90, "auth": {"type": "bearer", "token": "2"}})"));
    QCOMPARE(siteChanged.count(), 2);

    // a new start page
    w->updateOptions(fromJson(R"({"url": "https://b.example/", "auth": {"type": "bearer", "token": "2"}})"));
    QCOMPARE(siteChanged.count(), 3);
}

void testWebviewEntity::secretsInOptions_tellsAnEventFromTheUisAnswer() {
    // the UI's answer: the values are there
    QVERIFY(Webview::secretsInOptions(fromJson(R"({
        "auth": {"type": "basic", "username": "u", "password": "p"}, "cookies": [{"name": "a", "value": "b"}]
    })")));
    // nothing secret configured
    QVERIFY(Webview::secretsInOptions(fromJson(R"({"url": "https://a.example/", "zoom": 70})")));
    QVERIFY(
        Webview::secretsInOptions(fromJson(R"({"auth": {"type": "basic", "username": "u", "has_password": false}})")));
    // an event: the flags without the values
    QVERIFY(
        !Webview::secretsInOptions(fromJson(R"({"auth": {"type": "basic", "username": "u", "has_password": true}})")));
    QVERIFY(!Webview::secretsInOptions(fromJson(R"({"auth": {"type": "bearer", "has_token": true}})")));
    QVERIFY(!Webview::secretsInOptions(fromJson(R"({"auth": {"type": "header", "has_header_value": true}})")));
    QVERIFY(!Webview::secretsInOptions(fromJson(R"({"cookies": [{"name": "a", "has_value": true}]})")));
}

void testWebviewEntity::login_onlyForTheStartPagesOrigin() {
    WebViewSite site;
    site.setUrl(QUrl("https://nas.home.arpa:5001/status"));
    site.setAuth(WebViewSite::Auth::Basic, QStringLiteral("viewer"), QStringLiteral("pw"));

    QVERIFY(site.loginFor(QStringLiteral("https://nas.home.arpa:5001"), true, false, false, nullptr, nullptr));
    QVERIFY(site.loginFor(QStringLiteral("https://NAS.home.arpa:5001"), true, false, false, nullptr, nullptr));
    // another port, scheme or host: never
    QVERIFY(!site.loginFor(QStringLiteral("https://nas.home.arpa"), true, false, false, nullptr, nullptr));
    QVERIFY(!site.loginFor(QStringLiteral("http://nas.home.arpa:5001"), true, false, false, nullptr, nullptr));
    QVERIFY(!site.loginFor(QStringLiteral("https://cdn.example.com"), true, false, false, nullptr, nullptr));
    QVERIFY(!site.loginFor(QString(), true, false, false, nullptr, nullptr));

    // the scheme's default port is the same origin
    WebViewSite plain;
    plain.setUrl(QUrl("http://router.local/"));
    plain.setAuth(WebViewSite::Auth::Basic, QStringLiteral("admin"), QStringLiteral("pw"));
    QVERIFY(plain.loginFor(QStringLiteral("http://router.local"), true, false, false, nullptr, nullptr));
    QVERIFY(plain.loginFor(QStringLiteral("http://router.local:80"), true, false, false, nullptr, nullptr));
}

void testWebviewEntity::login_neverForAProxyARetryOrAnotherScheme() {
    WebViewSite site;
    site.setUrl(QUrl("https://a.example/"));
    site.setAuth(WebViewSite::Auth::Basic, QStringLiteral("viewer"), QStringLiteral("pw"));
    const QString origin = QStringLiteral("https://a.example");

    QVERIFY(!site.loginFor(origin, true, true, false, nullptr, nullptr));    // proxy
    QVERIFY(!site.loginFor(origin, true, false, true, nullptr, nullptr));    // retry: the user's dialog
    QVERIFY(!site.loginFor(origin, false, false, false, nullptr, nullptr));  // NTLM, Negotiate, ...

    // without a user name there is nothing to answer
    WebViewSite noUser;
    noUser.setUrl(QUrl("https://a.example/"));
    noUser.setAuth(WebViewSite::Auth::Basic, QString(), QStringLiteral("pw"));
    QVERIFY(!noUser.loginFor(origin, true, false, false, nullptr, nullptr));
}

void testWebviewEntity::origin_defaultPortsAndCase() {
    QCOMPARE(WebViewSite::originOf(QUrl("https://Example.COM/a/b?c")), QStringLiteral("https://example.com"));
    QCOMPARE(WebViewSite::originOf(QUrl("https://example.com:443/")), QStringLiteral("https://example.com"));
    QCOMPARE(WebViewSite::originOf(QUrl("http://example.com:8080/")), QStringLiteral("http://example.com:8080"));
    QVERIFY(WebViewSite::originOf(QUrl("about:blank")).isEmpty());
    QVERIFY(WebViewSite::sameOrigin(QUrl("http://a.example:80/x"), QUrl("http://a.example/y")));
    QVERIFY(!WebViewSite::sameOrigin(QUrl("http://a.example/"), QUrl("https://a.example/")));
    QVERIFY(!WebViewSite::sameOrigin(QUrl(), QUrl()));
}

void testWebviewEntity::sha256_isNormalised() {
    const QString plain(64, 'A');
    QCOMPARE(WebViewSite::normalizedSha256(plain), QString(64, 'a'));
    QStringList pairs;
    for (int i = 0; i < 32; i++) {
        pairs.append(QStringLiteral("0f"));
    }
    QCOMPARE(WebViewSite::normalizedSha256(pairs.join(':')), pairs.join(QString()));
    QVERIFY(WebViewSite::normalizedSha256(QString(63, 'a')).isEmpty());
    QVERIFY(WebViewSite::normalizedSha256(QString(64, 'g')).isEmpty());
    QVERIFY(WebViewSite::normalizedSha256(QString()).isEmpty());
}

void testWebviewEntity::requestOptions_nameWhatTheLibraryCannotApply() {
    WebViewSite site;
    site.setUrl(QUrl("https://a.example/"));
    QVERIFY(site.requestOptions().isEmpty());

    site.setAuth(WebViewSite::Auth::Header, QString(), QStringLiteral("key"), QStringLiteral("X-API-Key"));
    site.setCookies({WebViewSite::Cookie{QStringLiteral("a"), QStringLiteral("b"), QString(), QStringLiteral("/"), true,
                                         false, QDateTime()}});
    site.setTlsException(QString(), true);
    site.setUserAgent(QStringLiteral("mobile"));
    QCOMPARE(site.requestOptions(), (QStringList{QStringLiteral("auth.header"), QStringLiteral("cookies"),
                                                 QStringLiteral("tls"), QStringLiteral("user_agent")}));
    // the names only: no value in the log line
    QVERIFY(!site.requestOptions().join(' ').contains(QStringLiteral("key")));
}

void testWebviewEntity::buttons_ofEachModel() {
    // core-api "Buttons": the common buttons, the Remote 3 has STOP, RECORD and MENU, the Remote Two the colour buttons
    const QStringList common{"BACK",        "DPAD_UP",   "DPAD_DOWN",   "DPAD_LEFT",  "DPAD_RIGHT",
                             "DPAD_MIDDLE", "VOLUME_UP", "VOLUME_DOWN", "CHANNEL_UP", "CHANNEL_DOWN",
                             "MUTE",        "PREV",      "PLAY",        "NEXT"};
    QCOMPARE(WebviewButtons::ofModel(HardwareModel::UCR3), (common + QStringList{"STOP", "RECORD", "MENU"}));
    QCOMPARE(WebviewButtons::ofModel(HardwareModel::UCR2), (common + QStringList{"RED", "GREEN", "YELLOW", "BLUE"}));
    // the desktop simulator's button window is the Remote Two's
    QCOMPARE(WebviewButtons::ofModel(HardwareModel::DEV), WebviewButtons::ofModel(HardwareModel::UCR2));
    QCOMPARE(WebviewButtons::all().size(), 21);
    // fixed functions: never a button of a mapping
    for (const char *fixed : {"HOME", "POWER", "VOICE"}) {
        QVERIFY(!WebviewButtons::all().contains(QString::fromLatin1(fixed)));
    }
}

void testWebviewEntity::buttons_defaultActions() {
    QCOMPARE(WebviewButtons::defaultAction("DPAD_UP"), fromJson(R"({"action": "key", "key": "ArrowUp"})"));
    QCOMPARE(WebviewButtons::defaultAction("DPAD_DOWN"), fromJson(R"({"action": "key", "key": "ArrowDown"})"));
    QCOMPARE(WebviewButtons::defaultAction("DPAD_LEFT"), fromJson(R"({"action": "key", "key": "ArrowLeft"})"));
    QCOMPARE(WebviewButtons::defaultAction("DPAD_RIGHT"), fromJson(R"({"action": "key", "key": "ArrowRight"})"));
    QCOMPARE(WebviewButtons::defaultAction("DPAD_MIDDLE"), fromJson(R"({"action": "key", "key": "Enter"})"));
    QCOMPARE(WebviewButtons::defaultAction("BACK"), fromJson(R"({"action": "function", "function": "close"})"));
    QCOMPARE(WebviewButtons::defaultAction("VOLUME_UP"), fromJson(R"({"action": "function", "function": "zoom_in"})"));
    QCOMPARE(WebviewButtons::defaultAction("VOLUME_DOWN"),
             fromJson(R"({"action": "function", "function": "zoom_out"})"));
    QCOMPARE(WebviewButtons::defaultAction("CHANNEL_UP"), fromJson(R"({"action": "function", "function": "page_up"})"));
    QCOMPARE(WebviewButtons::defaultAction("CHANNEL_DOWN"),
             fromJson(R"({"action": "function", "function": "page_down"})"));
    QCOMPARE(WebviewButtons::defaultAction("MENU"), fromJson(R"({"action": "key", "key": "ContextMenu"})"));
    for (const char *button : {"MUTE", "PREV", "PLAY", "NEXT", "STOP", "RECORD", "RED", "GREEN", "YELLOW", "BLUE"}) {
        QCOMPARE(WebviewButtons::defaultAction(QString::fromLatin1(button)), fromJson(R"({"action": "pass"})"));
    }
}

void testWebviewEntity::buttons_validActions_data() {
    QTest::addColumn<QByteArray>("action");
    QTest::addColumn<QByteArray>("valid");

    QTest::newRow("pass") << QByteArray(R"({"action": "pass"})") << QByteArray(R"({"action": "pass"})");
    QTest::newRow("function") << QByteArray(R"({"action": "function", "function": "back_or_close"})")
                              << QByteArray(R"({"action": "function", "function": "back_or_close"})");
    QTest::newRow("key") << QByteArray(R"({"action": "key", "key": "MediaPlayPause"})")
                         << QByteArray(R"({"action": "key", "key": "MediaPlayPause"})");
    // a field the action does not use is dropped
    QTest::newRow("pass with a key") << QByteArray(R"({"action": "pass", "key": "Enter"})")
                                     << QByteArray(R"({"action": "pass"})");
    QTest::newRow("key with a function") << QByteArray(R"({"action": "key", "key": "Tab", "function": "reload"})")
                                         << QByteArray(R"({"action": "key", "key": "Tab"})");
    // invalid: none
    QTest::newRow("unknown function") << QByteArray(R"({"action": "function", "function": "print"})")
                                      << QByteArray("{}");
    QTest::newRow("function without its name") << QByteArray(R"({"action": "function"})") << QByteArray("{}");
    QTest::newRow("unknown key") << QByteArray(R"({"action": "key", "key": "F5"})") << QByteArray("{}");
    QTest::newRow("key in the wrong case") << QByteArray(R"({"action": "key", "key": "arrowup"})") << QByteArray("{}");
    QTest::newRow("key without its name") << QByteArray(R"({"action": "key"})") << QByteArray("{}");
    QTest::newRow("unknown action") << QByteArray(R"({"action": "macro"})") << QByteArray("{}");
    QTest::newRow("no action") << QByteArray(R"({"function": "reload"})") << QByteArray("{}");
}

void testWebviewEntity::buttons_validActions() {
    QFETCH(QByteArray, action);
    QFETCH(QByteArray, valid);
    QCOMPARE(WebviewButtons::validAction(fromJson(action.constData())), fromJson(valid.constData()));
    // every function and key of the Core-API is valid
    for (const QString &function : WebviewButtons::functions()) {
        QVERIFY(!WebviewButtons::validAction(QVariantMap{{"action", "function"}, {"function", function}}).isEmpty());
    }
    for (const QString &key : WebviewButtons::keys()) {
        QVERIFY(!WebviewButtons::validAction(QVariantMap{{"action", "key"}, {"key", key}}).isEmpty());
    }
    QCOMPARE(WebviewButtons::functions().size(), 19);
    QCOMPARE(WebviewButtons::keys().size(), 36);
    QVERIFY(WebviewButtons::validAction(QVariant()).isEmpty());
}

void testWebviewEntity::buttons_parseKeepsOnlyValidEntries() {
    // unknown buttons, fixed buttons, invalid actions and null (a removed override) are left out; buttons of the other
    // model stay (a backup of another remote)
    const QVariantMap parsed = WebviewButtons::parse(fromJson(R"({
        "VOLUME_UP": {"action": "pass"}, "RED": {"action": "key", "key": "ColorF0Red"},
        "MENU": {"action": "function", "function": "reload"}, "HOME": {"action": "pass"}, "POWER": {"action": "pass"},
        "SHUFFLE": {"action": "pass"}, "BACK": {"action": "function", "function": "exit"}, "PLAY": null
    })"));
    QCOMPARE(parsed, fromJson(R"({"VOLUME_UP": {"action": "pass"}, "RED": {"action": "key", "key": "ColorF0Red"},
                                   "MENU": {"action": "function", "function": "reload"}})"));
    QVERIFY(WebviewButtons::parse(QVariant()).isEmpty());
    QVERIFY(WebviewButtons::parse(QStringLiteral("pass")).isEmpty());
}

void testWebviewEntity::buttons_withDefaultsHasEveryButton() {
    const QStringList buttons = WebviewButtons::ofModel(HardwareModel::UCR3);
    const QVariantMap actions = WebviewButtons::withDefaults(
        buttons, fromJson(R"({"VOLUME_UP": {"action": "pass"}, "BACK": {"action": "key", "key": "Nope"},
                              "RED": {"action": "pass"}})"));
    QCOMPARE(actions.keys().size(), buttons.size());
    QCOMPARE(actions.value("VOLUME_UP").toMap(), fromJson(R"({"action": "pass"})"));
    // an invalid action is the default; a button of the other model is not listed
    QCOMPARE(actions.value("BACK").toMap(), WebviewButtons::defaultAction("BACK"));
    QCOMPARE(actions.value("MENU").toMap(), WebviewButtons::defaultAction("MENU"));
    QVERIFY(!actions.contains("RED"));
}

void testWebviewEntity::buttons_heldRepeatsKeysAndScrolling() {
    // a held button repeats key events and the scroll and page functions; the other functions act once per press
    QVERIFY(WebviewButtons::repeats(fromJson(R"({"action": "key", "key": "ArrowDown"})")));
    for (const QString &function : WebviewButtons::functions()) {
        const bool scrolls = function == QLatin1String("scroll_up") || function == QLatin1String("scroll_down") ||
                             function == QLatin1String("page_up") || function == QLatin1String("page_down");
        QCOMPARE(WebviewButtons::repeats(QVariantMap{{"action", "function"}, {"function", function}}), scrolls);
    }
    QVERIFY(!WebviewButtons::repeats(fromJson(R"({"action": "pass"})")));
}

void testWebviewEntity::buttons_optionAndUserOverrides() {
    // the option of the web view, the user's override of single buttons on top, invalid entries left out
    QScopedPointer<Webview> w(make(fromJson(R"({
        "url": "https://a.example/",
        "buttons": {"CHANNEL_UP": {"action": "key", "key": "ChannelUp"}, "BACK": {"action": "function",
                    "function": "back_or_close"}, "VOICE": {"action": "pass"}, "MUTE": {"action": "key"}},
        "user": {"buttons": {"BACK": {"action": "function", "function": "close"}, "VOLUME_UP": {"action": "pass"},
                             "CHANNEL_UP": null}}
    })")));
    QCOMPARE(w->getButtons(), fromJson(R"({"CHANNEL_UP": {"action": "key", "key": "ChannelUp"},
                                           "BACK": {"action": "function", "function": "close"},
                                           "VOLUME_UP": {"action": "pass"}})"));
    QCOMPARE(w->property("buttons").toMap(), w->getButtons());

    // without buttons the global setting applies to all of them
    QSignalSpy changed(w.data(), &Webview::optionsChanged);
    w->updateOptions(fromJson(R"({"url": "https://a.example/"})"));
    QCOMPARE(changed.count(), 1);
    QVERIFY(w->getButtons().isEmpty());
}

QTEST_MAIN(testWebviewEntity)

#include "test_webview_entity.moc"
