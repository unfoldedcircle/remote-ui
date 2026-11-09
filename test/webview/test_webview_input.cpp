// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

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
}

void testWebViewInput::remoteButtonsStayWithTheUi_data() {
    QTest::addColumn<int>("key");

    // src/ui/inputController.h: the buttons the UI handles on every screen
    QTest::newRow("BACK") << static_cast<int>(Qt::Key_Exit);
    QTest::newRow("HOME") << static_cast<int>(Qt::Key_Home);
    QTest::newRow("VOICE") << static_cast<int>(Qt::Key_F3);
    QTest::newRow("MENU") << static_cast<int>(Qt::Key_F4);
    QTest::newRow("VOLUME_UP") << static_cast<int>(Qt::Key_VolumeUp);
    QTest::newRow("MUTE") << static_cast<int>(Qt::Key_VolumeMute);
    QTest::newRow("GREEN") << static_cast<int>(Qt::Key_Green);
    QTest::newRow("CHANNEL_UP") << static_cast<int>(Qt::Key_ChannelUp);
    QTest::newRow("PLAY") << static_cast<int>(Qt::Key_MediaTogglePlayPause);
    QTest::newRow("POWER") << static_cast<int>(Qt::Key_PowerOff);
    QTest::newRow("STOP") << static_cast<int>(Qt::Key_Stop);
}

void testWebViewInput::remoteButtonsStayWithTheUi() {
    QFETCH(int, key);
    QCOMPARE(xkbKeysymForKey(key, QString()), uint32_t(0));
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

QTEST_GUILESS_MAIN(testWebViewInput)
#include "test_webview_input.moc"
