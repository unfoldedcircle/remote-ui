// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QKeySequence>
#include <QtTest>

#include "ui/entity/webviewButtons.h"
#include "webview/uc_webview.h"
#include "webview/webviewinput.h"

using namespace uc::webview;
using uc::ui::entity::WebviewButtons;

class testWebViewInput : public QObject {
    Q_OBJECT

 private slots:
    void keysForThePage_data();
    void keysForThePage();
    void remoteButtonsStayWithTheUi_data();
    void remoteButtonsStayWithTheUi();
    void printableCharacters();
    void controlCharactersWithoutTableEntryAreDropped();
    void modifiers();
    void pointerButtons();
    void touchPhases();
    void imePurposeSelectsTheKeyboardLayout_data();
    void imePurposeSelectsTheKeyboardLayout();
    void imeHintsRefineTheLayout();
    void urlSchemeAllowlist_data();
    void urlSchemeAllowlist();
    void environmentSwitchLikeTheLibrary();
    void tapDistanceFromTheEnvironment();
    void specialKeysHaveTraceNamesCharactersDoNot();
    void loggableUrlIsTheOrigin_data();
    void loggableUrlIsTheOrigin();
    void loggableUrlIsCutAt200Characters();
    void loadErrorAddressIsSchemeHostAndPort_data();
    void loadErrorAddressIsSchemeHostAndPort();
    void loadErrorMessageShortensUrlsAndIsCut();
    void certificateErrorNames();
    void pageKeysByName_data();
    void pageKeysByName();
    void everyKeyOfTheButtonMappingHasItsKeysymAndKeycode();
};

void testWebViewInput::keysForThePage_data() {
    QTest::addColumn<int>("key");
    QTest::addColumn<uint32_t>("keysym");

    // values of WPEKeysyms.h (= xkbcommon-keysyms.h)
    QTest::newRow("Return") << static_cast<int>(Qt::Key_Return) << uint32_t(0xff0d);
    QTest::newRow("Enter") << static_cast<int>(Qt::Key_Enter) << uint32_t(0xff8d);
    QTest::newRow("Backspace") << static_cast<int>(Qt::Key_Backspace) << uint32_t(0xff08);
    QTest::newRow("Escape") << static_cast<int>(Qt::Key_Escape) << uint32_t(0xff1b);
    QTest::newRow("Tab") << static_cast<int>(Qt::Key_Tab) << uint32_t(0xff09);
    QTest::newRow("Backtab") << static_cast<int>(Qt::Key_Backtab) << uint32_t(0xfe20);
    QTest::newRow("Delete") << static_cast<int>(Qt::Key_Delete) << uint32_t(0xffff);
    QTest::newRow("Insert") << static_cast<int>(Qt::Key_Insert) << uint32_t(0xff63);
    QTest::newRow("Left") << static_cast<int>(Qt::Key_Left) << uint32_t(0xff51);
    QTest::newRow("Up") << static_cast<int>(Qt::Key_Up) << uint32_t(0xff52);
    QTest::newRow("Right") << static_cast<int>(Qt::Key_Right) << uint32_t(0xff53);
    QTest::newRow("Down") << static_cast<int>(Qt::Key_Down) << uint32_t(0xff54);
    QTest::newRow("End") << static_cast<int>(Qt::Key_End) << uint32_t(0xff57);
    QTest::newRow("PageUp") << static_cast<int>(Qt::Key_PageUp) << uint32_t(0xff55);
    QTest::newRow("PageDown") << static_cast<int>(Qt::Key_PageDown) << uint32_t(0xff56);
    QTest::newRow("Shift") << static_cast<int>(Qt::Key_Shift) << uint32_t(0xffe1);
    QTest::newRow("Control") << static_cast<int>(Qt::Key_Control) << uint32_t(0xffe3);
    QTest::newRow("Alt") << static_cast<int>(Qt::Key_Alt) << uint32_t(0xffe9);
    QTest::newRow("Meta") << static_cast<int>(Qt::Key_Meta) << uint32_t(0xffe7);
    QTest::newRow("CapsLock") << static_cast<int>(Qt::Key_CapsLock) << uint32_t(0xffe5);
    QTest::newRow("F1") << static_cast<int>(Qt::Key_F1) << uint32_t(0xffbe);
    QTest::newRow("F12") << static_cast<int>(Qt::Key_F12) << uint32_t(0xffc9);
}

void testWebViewInput::keysForThePage() {
    QFETCH(int, key);
    QFETCH(uint32_t, keysym);
    // the text of a special key (Qt sends "\r" for Return) must not turn it into a character
    QCOMPARE(xkbKeysymForKey(key, QString()), keysym);
    QCOMPARE(xkbKeysymForKey(key, QStringLiteral("\r")), keysym);
    // and the trace names it (a character key would only be "printable")
    QVERIFY(specialKeyName(keysym) != nullptr);
}

void testWebViewInput::remoteButtonsStayWithTheUi_data() {
    QTest::addColumn<int>("key");

    // src/ui/inputController.h: the buttons the UI handles itself, every one of InputController::Buttons that is not
    // a d-pad key
    QTest::newRow("BACK") << static_cast<int>(Qt::Key_Exit);
    QTest::newRow("HOME") << static_cast<int>(Qt::Key_Home);
    QTest::newRow("VOICE") << static_cast<int>(Qt::Key_F3);
    QTest::newRow("MENU") << static_cast<int>(Qt::Key_F4);
    QTest::newRow("VOLUME_UP") << static_cast<int>(Qt::Key_VolumeUp);
    QTest::newRow("VOLUME_DOWN") << static_cast<int>(Qt::Key_VolumeDown);
    QTest::newRow("MUTE") << static_cast<int>(Qt::Key_VolumeMute);
    QTest::newRow("GREEN") << static_cast<int>(Qt::Key_Green);
    QTest::newRow("YELLOW") << static_cast<int>(Qt::Key_Yellow);
    QTest::newRow("RED") << static_cast<int>(Qt::Key_Red);
    QTest::newRow("BLUE") << static_cast<int>(Qt::Key_Blue);
    QTest::newRow("CHANNEL_UP") << static_cast<int>(Qt::Key_ChannelUp);
    QTest::newRow("CHANNEL_DOWN") << static_cast<int>(Qt::Key_ChannelDown);
    QTest::newRow("PREV") << static_cast<int>(Qt::Key_AudioRewind);
    QTest::newRow("PLAY") << static_cast<int>(Qt::Key_MediaTogglePlayPause);
    QTest::newRow("NEXT") << static_cast<int>(Qt::Key_AudioForward);
    QTest::newRow("POWER") << static_cast<int>(Qt::Key_PowerOff);
    QTest::newRow("STOP") << static_cast<int>(Qt::Key_Stop);
    QTest::newRow("RECORD") << static_cast<int>(Qt::Key_MediaRecord);
}

void testWebViewInput::remoteButtonsStayWithTheUi() {
    QFETCH(int, key);
    // the device: the evdev keymap gives the remote's buttons no text
    QCOMPARE(xkbKeysymForKey(key, QString()), uint32_t(0));
    // the button simulator: InputController::sendKeyEvent() sends the key's name as text ("Media Rewind" for PREV),
    // which must not reach the page as its first letter
    const QString name = QKeySequence(key).toString();
    QVERIFY(!name.isEmpty());
    QCOMPARE(xkbKeysymForKey(key, name), static_cast<uint32_t>(0));
}

void testWebViewInput::printableCharacters() {
    // Latin-1: the keysym is the code point
    QCOMPARE(xkbKeysymForKey(Qt::Key_A, QStringLiteral("a")), uint32_t(0x61));
    QCOMPARE(xkbKeysymForKey(Qt::Key_A, QStringLiteral("A")), uint32_t(0x41));
    QCOMPARE(xkbKeysymForKey(Qt::Key_Space, QStringLiteral(" ")), uint32_t(0x20));
    QCOMPARE(xkbKeysymForKey(Qt::Key_Eacute, QString(QChar(0xe9))), uint32_t(0xe9));
    // above U+00FF: the Unicode keysym range
    QCOMPARE(xkbKeysymForKey(0, QString(QChar(0x20ac))), uint32_t(0x01000000 | 0x20ac));
    // a surrogate pair is one code point
    QCOMPARE(xkbKeysymForKey(0, QString::fromUcs4(reinterpret_cast<const uint *>(U"\U0001F600"), 1)),
             uint32_t(0x01000000 | 0x1F600));
}

void testWebViewInput::controlCharactersWithoutTableEntryAreDropped() {
    QCOMPARE(xkbKeysymForKey(Qt::Key_unknown, QString()), uint32_t(0));
    QCOMPARE(xkbKeysymForKey(Qt::Key_unknown, QString(QChar(0x1b))), uint32_t(0));
    QCOMPARE(xkbKeysymForKey(Qt::Key_unknown, QString(QChar(0x7f))), uint32_t(0));
}

void testWebViewInput::modifiers() {
    QCOMPARE(modifiersForEvent(Qt::NoModifier), 0u);
    QCOMPARE(modifiersForEvent(Qt::ShiftModifier), unsigned(UC_WEBVIEW_MOD_SHIFT));
    QCOMPARE(modifiersForEvent(Qt::ControlModifier | Qt::AltModifier),
             unsigned(UC_WEBVIEW_MOD_CONTROL | UC_WEBVIEW_MOD_ALT));
    QCOMPARE(modifiersForEvent(Qt::MetaModifier), unsigned(UC_WEBVIEW_MOD_META));
    QCOMPARE(modifiersForEvent(Qt::NoModifier, Qt::LeftButton), unsigned(UC_WEBVIEW_MOD_BUTTON1));
    QCOMPARE(modifiersForEvent(Qt::ShiftModifier, Qt::RightButton | Qt::MiddleButton),
             unsigned(UC_WEBVIEW_MOD_SHIFT | UC_WEBVIEW_MOD_BUTTON2 | UC_WEBVIEW_MOD_BUTTON3));
}

void testWebViewInput::pointerButtons() {
    QCOMPARE(pointerButtonForQt(Qt::LeftButton), UC_WEBVIEW_BUTTON_PRIMARY);
    QCOMPARE(pointerButtonForQt(Qt::MiddleButton), UC_WEBVIEW_BUTTON_MIDDLE);
    QCOMPARE(pointerButtonForQt(Qt::RightButton), UC_WEBVIEW_BUTTON_SECONDARY);
    QCOMPARE(pointerButtonForQt(Qt::BackButton), 0);
}

void testWebViewInput::touchPhases() {
    QCOMPARE(touchPhaseForState(Qt::TouchPointPressed), UC_WEBVIEW_TOUCH_DOWN);
    QCOMPARE(touchPhaseForState(Qt::TouchPointMoved), UC_WEBVIEW_TOUCH_MOVE);
    QCOMPARE(touchPhaseForState(Qt::TouchPointReleased), UC_WEBVIEW_TOUCH_UP);
    QCOMPARE(touchPhaseForState(Qt::TouchPointStationary), -1);
}

void testWebViewInput::imePurposeSelectsTheKeyboardLayout_data() {
    QTest::addColumn<unsigned>("purpose");
    QTest::addColumn<int>("hints");

    QTest::newRow("free form") << unsigned(UC_WEBVIEW_IME_PURPOSE_FREE_FORM) << static_cast<int>(Qt::ImhNone);
    QTest::newRow("digits (inputmode=numeric)")
        << unsigned(UC_WEBVIEW_IME_PURPOSE_DIGITS) << static_cast<int>(Qt::ImhDigitsOnly);
    QTest::newRow("number") << unsigned(UC_WEBVIEW_IME_PURPOSE_NUMBER) << static_cast<int>(Qt::ImhFormattedNumbersOnly);
    QTest::newRow("phone") << unsigned(UC_WEBVIEW_IME_PURPOSE_PHONE) << static_cast<int>(Qt::ImhDialableCharactersOnly);
    QTest::newRow("url") << unsigned(UC_WEBVIEW_IME_PURPOSE_URL)
                         << static_cast<int>(Qt::ImhUrlCharactersOnly | Qt::ImhNoAutoUppercase |
                                             Qt::ImhNoPredictiveText);
    QTest::newRow("email") << unsigned(UC_WEBVIEW_IME_PURPOSE_EMAIL)
                           << static_cast<int>(Qt::ImhEmailCharactersOnly | Qt::ImhNoAutoUppercase |
                                               Qt::ImhNoPredictiveText);
    QTest::newRow("password (type=password)")
        << unsigned(UC_WEBVIEW_IME_PURPOSE_PASSWORD)
        << static_cast<int>(Qt::ImhHiddenText | Qt::ImhSensitiveData | Qt::ImhNoAutoUppercase |
                            Qt::ImhNoPredictiveText);
    QTest::newRow("pin") << unsigned(UC_WEBVIEW_IME_PURPOSE_PIN)
                         << static_cast<int>(Qt::ImhDigitsOnly | Qt::ImhHiddenText | Qt::ImhSensitiveData);
    QTest::newRow("terminal") << unsigned(UC_WEBVIEW_IME_PURPOSE_TERMINAL)
                              << static_cast<int>(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    QTest::newRow("search") << unsigned(UC_WEBVIEW_IME_PURPOSE_SEARCH) << static_cast<int>(Qt::ImhNone);
}

void testWebViewInput::imePurposeSelectsTheKeyboardLayout() {
    QFETCH(unsigned, purpose);
    QFETCH(int, hints);
    QCOMPARE(static_cast<int>(inputMethodHintsForIme(UC_WEBVIEW_IME_HINT_NONE, purpose)), hints);
}

void testWebViewInput::imeHintsRefineTheLayout() {
    QCOMPARE(inputMethodHintsForIme(UC_WEBVIEW_IME_HINT_LOWERCASE, UC_WEBVIEW_IME_PURPOSE_FREE_FORM),
             Qt::ImhPreferLowercase | Qt::ImhNoAutoUppercase);
    QCOMPARE(inputMethodHintsForIme(UC_WEBVIEW_IME_HINT_UPPERCASE_CHARS, UC_WEBVIEW_IME_PURPOSE_FREE_FORM),
             Qt::InputMethodHints(Qt::ImhPreferUppercase));
    QCOMPARE(inputMethodHintsForIme(UC_WEBVIEW_IME_HINT_NO_SPELLCHECK, UC_WEBVIEW_IME_PURPOSE_FREE_FORM),
             Qt::InputMethodHints(Qt::ImhNoPredictiveText));
    QCOMPARE(inputMethodHintsForIme(UC_WEBVIEW_IME_HINT_PRIVATE, UC_WEBVIEW_IME_PURPOSE_FREE_FORM),
             Qt::ImhSensitiveData | Qt::ImhNoPredictiveText);
    // the sentence/word capitalisation hints leave the keyboard's default behaviour alone
    QCOMPARE(inputMethodHintsForIme(UC_WEBVIEW_IME_HINT_UPPERCASE_SENTENCES | UC_WEBVIEW_IME_HINT_WORD_COMPLETION,
                                    UC_WEBVIEW_IME_PURPOSE_FREE_FORM),
             Qt::InputMethodHints(Qt::ImhNone));
    // hints add to the purpose
    QCOMPARE(inputMethodHintsForIme(UC_WEBVIEW_IME_HINT_LOWERCASE, UC_WEBVIEW_IME_PURPOSE_DIGITS),
             Qt::ImhDigitsOnly | Qt::ImhPreferLowercase | Qt::ImhNoAutoUppercase);
}

void testWebViewInput::urlSchemeAllowlist_data() {
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("withoutFile");  // allowed while file: URLs are blocked (the production default)
    QTest::addColumn<bool>("withFile");     // allowed with UC_WEBVIEW_ALLOW_FILE_URLS=1

    // the rows of the library's allowlist (uc_uri_policy.c, tests/test-uri-policy.c)
    QTest::newRow("http") << "http://localhost/" << true << true;
    QTest::newRow("https upper case") << "HTTPS://EXAMPLE.COM/" << true << true;
    QTest::newRow("about") << "about:blank" << true << true;
    QTest::newRow("data") << "data:text/html,<p>hi</p>" << true << true;
    QTest::newRow("file") << "file:///app/data/emoji.html" << false << true;
    QTest::newRow("javascript") << "javascript:alert(1)" << false << false;
    QTest::newRow("blob") << "blob:http://x/1" << false << false;
    QTest::newRow("ftp") << "ftp://host/file" << false << false;
    QTest::newRow("host:port parses as a scheme") << "localhost:8080/" << false << false;
    QTest::newRow("bare path") << "/app/data/emoji.html" << false << false;
    QTest::newRow("host without scheme") << "example.com" << false << false;
}

void testWebViewInput::urlSchemeAllowlist() {
    QFETCH(QString, url);
    QFETCH(bool, withoutFile);
    QFETCH(bool, withFile);
    QCOMPARE(urlRejectionReason(QUrl(url), false).isEmpty(), withoutFile);
    QCOMPARE(urlRejectionReason(QUrl(url), true).isEmpty(), withFile);
}

void testWebViewInput::environmentSwitchLikeTheLibrary() {
    for (const char *on : {"1", "true", "YES", "On"}) {
        QVERIFY2(envSwitchEnabled(on), on);
    }
    for (const char *off : {"", "0", "2", "false", "off", "1 "}) {
        QVERIFY2(!envSwitchEnabled(off), off);
    }
    QVERIFY(!envSwitchEnabled(QByteArray()));
    QVERIFY(urlRejectionReason(QUrl("file:///x"), false).contains("UC_WEBVIEW_ALLOW_FILE_URLS=1"));
}

void testWebViewInput::tapDistanceFromTheEnvironment() {
    // the library's UC_WEBVIEW_DRAG_THRESHOLD rule (uc_events_drag_threshold), WPE's 8 px otherwise
    QCOMPARE(dragThresholdFromEnv(QByteArray()), 8.0);
    QCOMPARE(dragThresholdFromEnv("12"), 12.0);
    QCOMPARE(dragThresholdFromEnv("1"), 1.0);
    QCOMPARE(dragThresholdFromEnv("200"), 200.0);
    QCOMPARE(dragThresholdFromEnv("0"), 8.0);
    QCOMPARE(dragThresholdFromEnv("201"), 8.0);
    QCOMPARE(dragThresholdFromEnv("12px"), 8.0);
}

void testWebViewInput::specialKeysHaveTraceNamesCharactersDoNot() {
    QCOMPARE(QString::fromLatin1(specialKeyName(0xff0d)), QStringLiteral("Return"));
    QCOMPARE(QString::fromLatin1(specialKeyName(0xff08)), QStringLiteral("BackSpace"));
    QCOMPARE(QString::fromLatin1(specialKeyName(0xff52)), QStringLiteral("Up"));
    QCOMPARE(QString::fromLatin1(specialKeyName(0xffbe)), QStringLiteral("F1"));
    QCOMPARE(QString::fromLatin1(specialKeyName(0xffc9)), QStringLiteral("F12"));
    // every character the item forwards, Latin-1 and Unicode, has no name: the trace shows it as "printable"
    for (uint codePoint = 0x20; codePoint <= 0xff; ++codePoint) {
        if (codePoint >= 0x7f && codePoint < 0xa0) {
            continue;
        }
        const uint32_t keysym = xkbKeysymForKey(Qt::Key_unknown, QString(QChar(codePoint)));
        QVERIFY2(keysym != 0 && specialKeyName(keysym) == nullptr, qPrintable(QString::number(codePoint, 16)));
    }
    QVERIFY(specialKeyName(xkbKeysymForKey(Qt::Key_unknown, QStringLiteral("\u20ac"))) == nullptr);
    QVERIFY(specialKeyName(0xffb7) == nullptr);  // KP_7: a keypad digit is a character
    QVERIFY(specialKeyName(0xfe51) == nullptr);  // dead_acute
    QVERIFY(specialKeyName(0) == nullptr);
}

void testWebViewInput::loggableUrlIsTheOrigin_data() {
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("full");
    QTest::addColumn<QString>("expected");

    // the cases of the library's own uc_uri_policy_loggable() tests: by default the origin only
    QTest::newRow("credentials and query") << "https://user:secret@example.com/path?q=1" << false
                                           << "https://example.com";
    QTest::newRow("path, query and fragment") << "https://nonexistent-host.invalid/secret/path?token=1#frag" << false
                                              << "https://nonexistent-host.invalid";
    QTest::newRow("oauth callback") << "https://example.com/cb?code=4/0AY0e-g7&state=xyz#access_token=ya29.a0Af"
                                    << false << "https://example.com";
    QTest::newRow("reset link") << "https://example.com/reset/0123456789abcdef" << false << "https://example.com";
    QTest::newRow("spa route") << "http://localhost/configurator/#/login" << false << "http://localhost";
    QTest::newRow("port") << "http://localhost:8080" << false << "http://localhost:8080";
    QTest::newRow("port and path") << "https://host:8443/a/b?c" << false << "https://host:8443";
    QTest::newRow("default port named") << "http://example.com:80/" << false << "http://example.com:80";
    QTest::newRow("IPv6") << "http://[::1]:80/a" << false << "http://[::1]:80";
    QTest::newRow("IPv6 without a port") << "http://[2001:db8::1]/a?b" << false << "http://[2001:db8::1]";
    QTest::newRow("file") << "file:///app/data/emoji.html" << false << "file:";
    QTest::newRow("about") << "about:blank" << false << "about:";
    QTest::newRow("data payload") << "data:text/html,<input value=secret>" << false << "data:";
    QTest::newRow("data base64") << "data:text/html;base64,PGlucHV0Pg==" << false << "data:";
    QTest::newRow("javascript") << "javascript:send('secret')" << false << "javascript:";
    QTest::newRow("blob") << "blob:http://x/1" << false << "blob:";
    QTest::newRow("mailto") << "mailto:someone@example.com" << false << "mailto:";
    QTest::newRow("no scheme") << "/app/data/emoji.html" << false << "(no scheme)";
    QTest::newRow("no scheme full") << "/app/data/emoji.html" << true << "(no scheme)";
    QTest::newRow("empty") << "" << false << "(empty)";
    // UC_WEBVIEW_LOG_FULL_URLS=1: everything but the credentials
    QTest::newRow("full keeps path and query, not the credentials")
        << "https://user:secret@example.com/path?q=1" << true << "https://example.com/path?q=1";
    QTest::newRow("full keeps the fragment") << "https://example.com/cb?code=4#access_token=ya29" << true
                                             << "https://example.com/cb?code=4#access_token=ya29";
    QTest::newRow("data full") << "data:text/html,hi" << true << "data:text/html,hi";
    QTest::newRow("about full") << "about:blank" << true << "about:blank";
}

void testWebViewInput::loggableUrlIsTheOrigin() {
    QFETCH(QString, url);
    QFETCH(bool, full);
    QFETCH(QString, expected);
    QCOMPARE(loggableUrl(QUrl(url), full), expected);
}

void testWebViewInput::loadErrorAddressIsSchemeHostAndPort_data() {
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expected");

    QTest::newRow("path, query, fragment") << "https://example.com/a/b?token=1#x" << "https://example.com";
    QTest::newRow("credentials and port") << "http://user:secret@nas.local:8080/" << "http://nas.local:8080";
    QTest::newRow("scheme in capitals") << "HTTPS://Example.COM/" << "https://example.com";
    QTest::newRow("IPv4") << "http://192.168.1.20:8123/lovelace" << "http://192.168.1.20:8123";
    QTest::newRow("IPv6") << "http://[fe80::1]:8080/x" << "http://[fe80::1]:8080";
    QTest::newRow("without a host") << "javascript:alert(1)" << "javascript:";
    QTest::newRow("data") << "data:text/html,<p>secret" << "data:";
    QTest::newRow("file") << "file:///etc/passwd" << "file:";
    QTest::newRow("no scheme") << "/app/x.html" << "";
    QTest::newRow("empty") << "" << "";
}

void testWebViewInput::loadErrorAddressIsSchemeHostAndPort() {
    QFETCH(QString, url);
    QFETCH(QString, expected);
    QCOMPARE(loadErrorAddress(QUrl(url)), expected);
}

void testWebViewInput::loadErrorMessageShortensUrlsAndIsCut() {
    QCOMPARE(loadErrorMessage(QStringLiteral("Cannot load https://u:p@a.example/x?t=1, try http://b.example:81/y.")),
             QStringLiteral("Cannot load https://a.example, try http://b.example:81."));
    QCOMPARE(loadErrorMessage(QString::fromUtf8("Error resolving \u201cnas.local\u201d: Name or service not known")),
             QString::fromUtf8("Error resolving \u201cnas.local\u201d: Name or service not known"));
    QCOMPARE(loadErrorMessage(QStringLiteral(" line\none\tend\r\n")), QStringLiteral("line one end"));
    QCOMPARE(loadErrorMessage(QString()), QString());
    const QString cut = loadErrorMessage(QString(400, QLatin1Char('e')));
    QCOMPARE(cut.size(), 303);
    QVERIFY(cut.endsWith(QStringLiteral("...")));
}

void testWebViewInput::certificateErrorNames() {
    QCOMPARE(uc::webview::certificateErrorNames(0), QString());
    // GTlsCertificateFlags: UNKNOWN_CA 1, BAD_IDENTITY 2, NOT_ACTIVATED 4, EXPIRED 8, REVOKED 16, INSECURE 32,
    // GENERIC_ERROR 64
    QCOMPARE(uc::webview::certificateErrorNames(1 | 2 | 8), QStringLiteral("expired, unknown issuer, wrong host name"));
    QCOMPARE(uc::webview::certificateErrorNames(4 | 16 | 32 | 64),
             QStringLiteral("not yet valid, revoked, insecure algorithm, other error"));
    QCOMPARE(uc::webview::certificateErrorNames(1 | 0x300), QStringLiteral("unknown issuer, flags 0x300"));
}

void testWebViewInput::loggableUrlIsCutAt200Characters() {
    const QUrl    url(QStringLiteral("https://example.com/") + QString(300, QLatin1Char('a')) +
                   QStringLiteral("?token=secret"));
    const QString text = loggableUrl(url, true);
    QCOMPARE(text.size(), 203);
    QVERIFY(text.endsWith(QStringLiteral("...")));
    QCOMPARE(loggableUrl(url, false), QStringLiteral("https://example.com"));
    // a long host name is cut as well
    const QUrl longHost(QStringLiteral("https://") + QStringLiteral("abcdefghi.").repeated(25) +
                        QStringLiteral("example/x"));
    QCOMPARE(loggableUrl(longHost, false).size(), 203);
}

void testWebViewInput::pageKeysByName_data() {
    QTest::addColumn<QString>("name");
    QTest::addColumn<uint32_t>("keysym");
    QTest::addColumn<uint32_t>("keycode");

    // the keysym WebKit 2.54.2 takes KeyboardEvent.key and keyCode from, the XKB keycode (evdev + 8) it takes
    // KeyboardEvent.code from
    QTest::newRow("ArrowUp") << QStringLiteral("ArrowUp") << 0xff52u << 111u;
    QTest::newRow("ArrowDown") << QStringLiteral("ArrowDown") << 0xff54u << 116u;
    QTest::newRow("ArrowLeft") << QStringLiteral("ArrowLeft") << 0xff51u << 113u;
    QTest::newRow("ArrowRight") << QStringLiteral("ArrowRight") << 0xff53u << 114u;
    QTest::newRow("Enter") << QStringLiteral("Enter") << 0xff0du << 36u;
    QTest::newRow("Escape") << QStringLiteral("Escape") << 0xff1bu << 9u;
    QTest::newRow("Backspace") << QStringLiteral("Backspace") << 0xff08u << 22u;
    QTest::newRow("Tab") << QStringLiteral("Tab") << 0xff09u << 23u;
    QTest::newRow("PageUp") << QStringLiteral("PageUp") << 0xff55u << 112u;
    QTest::newRow("PageDown") << QStringLiteral("PageDown") << 0xff56u << 117u;
    QTest::newRow("Home") << QStringLiteral("Home") << 0xff50u << 110u;
    QTest::newRow("End") << QStringLiteral("End") << 0xff57u << 115u;
    // the PC menu key: KEY_COMPOSE, not KEY_MENU, whose keysym WebKit does not map
    QTest::newRow("ContextMenu") << QStringLiteral("ContextMenu") << 0xff67u << 135u;
    QTest::newRow("BrowserBack") << QStringLiteral("BrowserBack") << 0x1008ff26u << 166u;
    QTest::newRow("BrowserForward") << QStringLiteral("BrowserForward") << 0x1008ff27u << 167u;
    QTest::newRow("BrowserRefresh") << QStringLiteral("BrowserRefresh") << 0x1008ff29u << 181u;
    QTest::newRow("BrowserStop") << QStringLiteral("BrowserStop") << 0x1008ff28u << 136u;
    // XF86Close is also KEY_CLOSE's keysym: Exit needs KEY_EXIT's keycode
    QTest::newRow("Exit") << QStringLiteral("Exit") << 0x1008ff56u << 182u;
    QTest::newRow("AudioVolumeUp") << QStringLiteral("AudioVolumeUp") << 0x1008ff13u << 123u;
    QTest::newRow("AudioVolumeDown") << QStringLiteral("AudioVolumeDown") << 0x1008ff11u << 122u;
    QTest::newRow("AudioVolumeMute") << QStringLiteral("AudioVolumeMute") << 0x1008ff12u << 121u;
    QTest::newRow("ChannelUp") << QStringLiteral("ChannelUp") << 0x10081192u << 410u;
    QTest::newRow("ChannelDown") << QStringLiteral("ChannelDown") << 0x10081193u << 411u;
    // one keysym for both: MediaPlayPause is told from MediaPlay by KEY_PLAYPAUSE's keycode
    QTest::newRow("MediaPlayPause") << QStringLiteral("MediaPlayPause") << 0x1008ff14u << 172u;
    QTest::newRow("MediaPlay") << QStringLiteral("MediaPlay") << 0x1008ff14u << 215u;
    QTest::newRow("MediaPause") << QStringLiteral("MediaPause") << 0x1008ff31u << 209u;
    QTest::newRow("MediaStop") << QStringLiteral("MediaStop") << 0x1008ff15u << 174u;
    QTest::newRow("MediaRecord") << QStringLiteral("MediaRecord") << 0x1008ff1cu << 175u;
    QTest::newRow("MediaRewind") << QStringLiteral("MediaRewind") << 0x1008ff3eu << 176u;
    QTest::newRow("MediaFastForward") << QStringLiteral("MediaFastForward") << 0x1008ff97u << 216u;
    QTest::newRow("MediaTrackPrevious") << QStringLiteral("MediaTrackPrevious") << 0x1008ff16u << 173u;
    QTest::newRow("MediaTrackNext") << QStringLiteral("MediaTrackNext") << 0x1008ff17u << 171u;
    QTest::newRow("ColorF0Red") << QStringLiteral("ColorF0Red") << 0x1008ffa3u << 406u;
    QTest::newRow("ColorF1Green") << QStringLiteral("ColorF1Green") << 0x1008ffa4u << 407u;
    QTest::newRow("ColorF2Yellow") << QStringLiteral("ColorF2Yellow") << 0x1008ffa5u << 408u;
    QTest::newRow("ColorF3Blue") << QStringLiteral("ColorF3Blue") << 0x1008ffa6u << 409u;
}

void testWebViewInput::pageKeysByName() {
    QFETCH(QString, name);
    QFETCH(uint32_t, keysym);
    QFETCH(uint32_t, keycode);
    const PageKey *key = pageKeyForName(name);
    QVERIFY(key);
    QCOMPARE(key->keysym, keysym);
    QCOMPARE(key->keycode, keycode);
}

void testWebViewInput::everyKeyOfTheButtonMappingHasItsKeysymAndKeycode() {
    // every KeyboardEvent.key of core-api WebviewButtonKey, and nothing else
    for (const QString &name : WebviewButtons::keys()) {
        QVERIFY2(pageKeyForName(name) != nullptr, qPrintable(name));
    }
    QVERIFY(pageKeyForName(QStringLiteral("arrowup")) == nullptr);
    QVERIFY(pageKeyForName(QStringLiteral("F5")) == nullptr);
    QVERIFY(pageKeyForName(QString()) == nullptr);
    QVERIFY(pageKeyForName(QStringLiteral("Close")) == nullptr);
}

QTEST_GUILESS_MAIN(testWebViewInput)
#include "test_webview_input.moc"
