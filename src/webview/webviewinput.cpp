// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewinput.h"

#include <QChar>

#include "uc_webview.h"

namespace uc {
namespace webview {

namespace {

struct KeyMapping {
    int      key;
    uint32_t keysym;
};

// Qt::Key -> XKB keysym. The values are those of WPEPlatform's WPEKeysyms.h (identical to xkbcommon-keysyms.h):
// WebKit derives key, code and text from the keysym (WebKeyboardEventWPE.cpp), the upstream Qt 6 binding passes
// Qt key codes instead, which is a known weakness. The remote's buttons that the UI handles itself are absent on
// purpose: BACK is Qt::Key_Exit, HOME is Qt::Key_Home, VOICE/MENU are F3/F4, plus the colour, channel, media,
// volume and power keys (src/ui/inputController.h). They return 0 and stay with the QML page.
constexpr KeyMapping kKeyTable[] = {
    {Qt::Key_Return, 0xff0d},    // WPE_KEY_Return
    {Qt::Key_Enter, 0xff8d},     // WPE_KEY_KP_Enter
    {Qt::Key_Backspace, 0xff08}, // WPE_KEY_BackSpace
    {Qt::Key_Escape, 0xff1b},    // WPE_KEY_Escape (never reaches the item on the desktop model: it is BACK there)
    {Qt::Key_Tab, 0xff09},       // WPE_KEY_Tab
    {Qt::Key_Backtab, 0xfe20},   // WPE_KEY_ISO_Left_Tab
    {Qt::Key_Delete, 0xffff},    // WPE_KEY_Delete
    {Qt::Key_Insert, 0xff63},    // WPE_KEY_Insert
    {Qt::Key_Left, 0xff51},      // WPE_KEY_Left
    {Qt::Key_Up, 0xff52},        // WPE_KEY_Up
    {Qt::Key_Right, 0xff53},     // WPE_KEY_Right
    {Qt::Key_Down, 0xff54},      // WPE_KEY_Down
    {Qt::Key_End, 0xff57},       // WPE_KEY_End
    {Qt::Key_PageUp, 0xff55},    // WPE_KEY_Page_Up
    {Qt::Key_PageDown, 0xff56},  // WPE_KEY_Page_Down
    {Qt::Key_Shift, 0xffe1},     // WPE_KEY_Shift_L
    {Qt::Key_Control, 0xffe3},   // WPE_KEY_Control_L
    {Qt::Key_Alt, 0xffe9},       // WPE_KEY_Alt_L
    {Qt::Key_Meta, 0xffe7},      // WPE_KEY_Meta_L
    {Qt::Key_CapsLock, 0xffe5},  // WPE_KEY_Caps_Lock
};

constexpr uint32_t kKeysymF1 = 0xffbe;             // WPE_KEY_F1 .. WPE_KEY_F12 are consecutive
constexpr uint32_t kKeysymUnicodeFlag = 0x01000000;  // keysym of a Unicode code point above U+00FF

}  // namespace

uint32_t xkbKeysymForKey(int key, const QString &text) {
    for (const KeyMapping &mapping : kKeyTable) {
        if (mapping.key == key) {
            return mapping.keysym;
        }
    }

    // F3 and F4 are the VOICE and MENU buttons of the remote
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12 && key != Qt::Key_F3 && key != Qt::Key_F4) {
        return kKeysymF1 + static_cast<uint32_t>(key - Qt::Key_F1);
    }

    if (text.isEmpty()) {
        return 0;
    }

    // A printable character: the keysym of a Unicode code point is the code point for U+0020..U+00FF and
    // 0x01000000 | code point above that (xkbcommon's xkb_utf32_to_keysym()).
    uint codePoint = text.at(0).unicode();
    if (text.at(0).isHighSurrogate() && text.size() > 1) {
        codePoint = QChar::surrogateToUcs4(text.at(0), text.at(1));
    }
    if (codePoint < 0x20 || (codePoint >= 0x7f && codePoint < 0xa0)) {
        return 0;  // control character: the key itself was not in the table
    }
    if (codePoint <= 0xff) {
        return codePoint;
    }
    return kKeysymUnicodeFlag | codePoint;
}

unsigned modifiersForEvent(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons) {
    unsigned result = 0;
    if (modifiers & Qt::ShiftModifier) {
        result |= UC_WEBVIEW_MOD_SHIFT;
    }
    if (modifiers & Qt::ControlModifier) {
        result |= UC_WEBVIEW_MOD_CONTROL;
    }
    if (modifiers & Qt::AltModifier) {
        result |= UC_WEBVIEW_MOD_ALT;
    }
    if (modifiers & Qt::MetaModifier) {
        result |= UC_WEBVIEW_MOD_META;
    }
    if (buttons & Qt::LeftButton) {
        result |= UC_WEBVIEW_MOD_BUTTON1;
    }
    if (buttons & Qt::MiddleButton) {
        result |= UC_WEBVIEW_MOD_BUTTON2;
    }
    if (buttons & Qt::RightButton) {
        result |= UC_WEBVIEW_MOD_BUTTON3;
    }
    return result;
}

int pointerButtonForQt(Qt::MouseButton button) {
    switch (button) {
        case Qt::LeftButton:
            return UC_WEBVIEW_BUTTON_PRIMARY;
        case Qt::MiddleButton:
            return UC_WEBVIEW_BUTTON_MIDDLE;
        case Qt::RightButton:
            return UC_WEBVIEW_BUTTON_SECONDARY;
        default:
            return 0;
    }
}

int touchPhaseForState(Qt::TouchPointState state) {
    switch (state) {
        case Qt::TouchPointPressed:
            return UC_WEBVIEW_TOUCH_DOWN;
        case Qt::TouchPointMoved:
            return UC_WEBVIEW_TOUCH_MOVE;
        case Qt::TouchPointReleased:
            return UC_WEBVIEW_TOUCH_UP;
        default:
            return -1;
    }
}

Qt::InputMethodHints inputMethodHintsForIme(unsigned hints, unsigned purpose) {
    Qt::InputMethodHints result = Qt::ImhNone;

    switch (purpose) {
        case UC_WEBVIEW_IME_PURPOSE_DIGITS:
            result |= Qt::ImhDigitsOnly;
            break;
        case UC_WEBVIEW_IME_PURPOSE_NUMBER:
            result |= Qt::ImhFormattedNumbersOnly;
            break;
        case UC_WEBVIEW_IME_PURPOSE_PHONE:
            result |= Qt::ImhDialableCharactersOnly;
            break;
        case UC_WEBVIEW_IME_PURPOSE_URL:
            result |= Qt::ImhUrlCharactersOnly | Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText;
            break;
        case UC_WEBVIEW_IME_PURPOSE_EMAIL:
            result |= Qt::ImhEmailCharactersOnly | Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText;
            break;
        case UC_WEBVIEW_IME_PURPOSE_PASSWORD:
            result |= Qt::ImhHiddenText | Qt::ImhSensitiveData | Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText;
            break;
        case UC_WEBVIEW_IME_PURPOSE_PIN:
            result |= Qt::ImhDigitsOnly | Qt::ImhHiddenText | Qt::ImhSensitiveData;
            break;
        case UC_WEBVIEW_IME_PURPOSE_TERMINAL:
            result |= Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText;
            break;
        default:
            // FREE_FORM, ALPHA, NAME, SEARCH: the keyboard's default text layout
            break;
    }

    if (hints & UC_WEBVIEW_IME_HINT_LOWERCASE) {
        result |= Qt::ImhPreferLowercase | Qt::ImhNoAutoUppercase;
    }
    if (hints & UC_WEBVIEW_IME_HINT_UPPERCASE_CHARS) {
        result |= Qt::ImhPreferUppercase;
    }
    if (hints & UC_WEBVIEW_IME_HINT_NO_SPELLCHECK) {
        result |= Qt::ImhNoPredictiveText;
    }
    if (hints & UC_WEBVIEW_IME_HINT_PRIVATE) {
        result |= Qt::ImhSensitiveData | Qt::ImhNoPredictiveText;
    }

    return result;
}

}  // namespace webview
}  // namespace uc
