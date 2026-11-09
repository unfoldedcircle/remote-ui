// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QKeySequence>
#include <QtTest>

#include "webview/uc_webview.h"
#include "webview/webviewinput.h"

using namespace uc::webview;

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
    void loggableUrlDropsCredentialsQueryAndPayload_data();
    void loggableUrlDropsCredentialsQueryAndPayload();
    void loggableUrlIsCutAt200Characters();
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

void testWebViewInput::loggableUrlDropsCredentialsQueryAndPayload_data() {
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("full");
    QTest::addColumn<QString>("expected");

    // the cases of the library's own uc_uri_policy_loggable() tests
    QTest::newRow("credentials and query") << "https://user:secret@example.com/path?q=1" << false
                                           << "https://example.com/path?...";
    QTest::newRow("full keeps the query, not the credentials")
        << "https://user:secret@example.com/path?q=1" << true << "https://example.com/path?q=1";
    QTest::newRow("oauth callback") << "https://example.com/cb?code=4/0AY0e-g7&state=xyz#access_token=ya29.a0Af"
                                    << false << "https://example.com/cb?...#...";
    QTest::newRow("spa route") << "http://localhost/configurator/#/login" << false
                               << "http://localhost/configurator/#...";
    QTest::newRow("port") << "http://localhost:8080" << false << "http://localhost:8080";
    QTest::newRow("file") << "file:///app/data/emoji.html" << false << "file:///app/data/emoji.html";
    QTest::newRow("about") << "about:blank" << false << "about:blank";
    QTest::newRow("data payload") << "data:text/html,<input value=secret>" << false << "data:text/html,...";
    QTest::newRow("data base64") << "data:text/html;base64,PGlucHV0Pg==" << false << "data:text/html;base64,...";
    QTest::newRow("data full") << "data:text/html,hi" << true << "data:text/html,hi";
    QTest::newRow("javascript") << "javascript:send('secret')" << false << "javascript:...";
    QTest::newRow("mailto") << "mailto:someone@example.com" << false << "mailto:...";
    QTest::newRow("no scheme") << "/app/data/emoji.html" << false << "(no scheme):...";
    QTest::newRow("empty") << "" << false << "(empty)";
}

void testWebViewInput::loggableUrlDropsCredentialsQueryAndPayload() {
    QFETCH(QString, url);
    QFETCH(bool, full);
    QFETCH(QString, expected);
    QCOMPARE(loggableUrl(QUrl(url), full), expected);
}

void testWebViewInput::loggableUrlIsCutAt200Characters() {
    const QUrl    url(QStringLiteral("https://example.com/") + QString(300, QLatin1Char('a')) +
                   QStringLiteral("?token=secret"));
    const QString text = loggableUrl(url, true);
    QCOMPARE(text.size(), 203);
    QVERIFY(text.endsWith(QStringLiteral("...")));
    QVERIFY(!loggableUrl(url, false).contains(QStringLiteral("secret")));
}

QTEST_GUILESS_MAIN(testWebViewInput)
#include "test_webview_input.moc"
