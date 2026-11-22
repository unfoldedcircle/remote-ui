// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewinput.h"

#include <QChar>
#include <QRegularExpression>
#include <QStringList>

#include "uc_webview.h"

namespace uc {
namespace webview {

namespace {

struct KeyMapping {
    int         key;
    uint32_t    keysym;
    const char *name;  // for the trace (specialKeyName)
};

// Qt::Key -> XKB keysym. The values are those of WPEPlatform's WPEKeysyms.h (identical to xkbcommon-keysyms.h):
// WebKit derives key, code and text from the keysym (WebKeyboardEventWPE.cpp), the upstream Qt 6 binding passes
// Qt key codes instead, which is a known weakness. The remote's buttons that the UI handles itself are absent on
// purpose: BACK is Qt::Key_Exit, HOME is Qt::Key_Home, VOICE/MENU are F3/F4, plus the colour, channel, media, volume
// and power keys (src/ui/inputController.h). They return 0, with or without text, and stay with the QML page. On the
// test page PREV and NEXT (Key_AudioRewind/Key_AudioForward) are its history keys, VOLUME its zoom keys and
// CHANNEL its page scroll keys, which send Page_Up/Page_Down through WebViewItem::pageUp()/pageDown().
constexpr KeyMapping kKeyTable[] = {
    {Qt::Key_Return, 0xff0d, "Return"},        // WPE_KEY_Return
    {Qt::Key_Enter, 0xff8d, "KP_Enter"},       // WPE_KEY_KP_Enter
    {Qt::Key_Backspace, 0xff08, "BackSpace"},  // WPE_KEY_BackSpace
    {Qt::Key_Escape, 0xff1b, "Escape"},        // WPE_KEY_Escape (never reaches the item on the desktop model: it is BACK)
    {Qt::Key_Tab, 0xff09, "Tab"},              // WPE_KEY_Tab
    {Qt::Key_Backtab, 0xfe20, "ISO_Left_Tab"}, // WPE_KEY_ISO_Left_Tab
    {Qt::Key_Delete, 0xffff, "Delete"},        // WPE_KEY_Delete
    {Qt::Key_Insert, 0xff63, "Insert"},        // WPE_KEY_Insert
    {Qt::Key_Left, 0xff51, "Left"},            // WPE_KEY_Left
    {Qt::Key_Up, 0xff52, "Up"},                // WPE_KEY_Up
    {Qt::Key_Right, 0xff53, "Right"},          // WPE_KEY_Right
    {Qt::Key_Down, 0xff54, "Down"},            // WPE_KEY_Down
    {Qt::Key_End, 0xff57, "End"},              // WPE_KEY_End
    {Qt::Key_PageUp, 0xff55, "Page_Up"},       // WPE_KEY_Page_Up
    {Qt::Key_PageDown, 0xff56, "Page_Down"},   // WPE_KEY_Page_Down
    {Qt::Key_Shift, 0xffe1, "Shift_L"},        // WPE_KEY_Shift_L
    {Qt::Key_Control, 0xffe3, "Control_L"},    // WPE_KEY_Control_L
    {Qt::Key_Alt, 0xffe9, "Alt_L"},            // WPE_KEY_Alt_L
    {Qt::Key_Meta, 0xffe7, "Meta_L"},          // WPE_KEY_Meta_L
    {Qt::Key_CapsLock, 0xffe5, "Caps_Lock"},   // WPE_KEY_Caps_Lock
};

struct NamedPageKey {
    const char *name;  // KeyboardEvent.key
    PageKey     key;
};

// KeyboardEvent.key -> keysym, keycode (evdev + 8), checked against WebKit 2.54.2 (WebKeyboardEventWPE.cpp
// keyValueStringForWPEKeyval, windowsKeyCodeForWPEKeyval, keyCodeStringForWPEKeycode). The keysyms are those of
// xkbcommon-keysyms.h; KeyboardEvent.code is the UI Events code where one exists, else "Unidentified". ChannelUp,
// ChannelDown, Exit and MediaPlayPause arrive as these keys only with the firmware's WebKit patch (without it:
// "Unidentified", "Unidentified", "Close", "MediaPlay"); Exit and MediaPlayPause depend on the keycode for it, as
// xkeyboard-config gives KEY_EXIT and KEY_PLAYPAUSE the keysyms of KEY_CLOSE and KEY_PLAY. ContextMenu is the PC menu
// key (KEY_COMPOSE), BrowserRefresh and BrowserStop the keysyms XF86Refresh and XF86Stop, which WebKit maps.
constexpr NamedPageKey kPageKeys[] = {
    {"ArrowUp", {0xff52, 111}},                 // Up, KEY_UP 103
    {"ArrowDown", {0xff54, 116}},               // Down, KEY_DOWN 108
    {"ArrowLeft", {0xff51, 113}},               // Left, KEY_LEFT 105
    {"ArrowRight", {0xff53, 114}},              // Right, KEY_RIGHT 106
    {"Enter", {0xff0d, 36}},                    // Return, KEY_ENTER 28
    {"Escape", {0xff1b, 9}},                    // Escape, KEY_ESC 1
    {"Backspace", {0xff08, 22}},                // BackSpace, KEY_BACKSPACE 14
    {"Tab", {0xff09, 23}},                      // Tab, KEY_TAB 15
    {"PageUp", {0xff55, 112}},                  // Page_Up, KEY_PAGEUP 104
    {"PageDown", {0xff56, 117}},                // Page_Down, KEY_PAGEDOWN 109
    {"Home", {0xff50, 110}},                    // Home, KEY_HOME 102
    {"End", {0xff57, 115}},                     // End, KEY_END 107
    {"ContextMenu", {0xff67, 135}},             // Menu, KEY_COMPOSE 127
    {"BrowserBack", {0x1008ff26, 166}},         // XF86Back, KEY_BACK 158
    {"BrowserForward", {0x1008ff27, 167}},      // XF86Forward, KEY_FORWARD 159
    {"BrowserRefresh", {0x1008ff29, 181}},      // XF86Refresh, KEY_REFRESH 173
    {"BrowserStop", {0x1008ff28, 136}},         // XF86Stop, KEY_STOP 128
    {"Exit", {0x1008ff56, 182}},                // XF86Close, KEY_EXIT 174 (the keycode is required)
    {"AudioVolumeUp", {0x1008ff13, 123}},       // XF86AudioRaiseVolume, KEY_VOLUMEUP 115
    {"AudioVolumeDown", {0x1008ff11, 122}},     // XF86AudioLowerVolume, KEY_VOLUMEDOWN 114
    {"AudioVolumeMute", {0x1008ff12, 121}},     // XF86AudioMute, KEY_MUTE 113
    {"ChannelUp", {0x10081192, 410}},           // XF86ChannelUp, KEY_CHANNELUP 402
    {"ChannelDown", {0x10081193, 411}},         // XF86ChannelDown, KEY_CHANNELDOWN 403
    {"MediaPlayPause", {0x1008ff14, 172}},      // XF86AudioPlay, KEY_PLAYPAUSE 164 (the keycode is required)
    {"MediaPlay", {0x1008ff14, 215}},           // XF86AudioPlay, KEY_PLAY 207
    {"MediaPause", {0x1008ff31, 209}},          // XF86AudioPause, KEY_PAUSECD 201
    {"MediaStop", {0x1008ff15, 174}},           // XF86AudioStop, KEY_STOPCD 166
    {"MediaRecord", {0x1008ff1c, 175}},         // XF86AudioRecord, KEY_RECORD 167
    {"MediaRewind", {0x1008ff3e, 176}},         // XF86AudioRewind, KEY_REWIND 168
    {"MediaFastForward", {0x1008ff97, 216}},    // XF86AudioForward, KEY_FASTFORWARD 208
    {"MediaTrackPrevious", {0x1008ff16, 173}},  // XF86AudioPrev, KEY_PREVIOUSSONG 165
    {"MediaTrackNext", {0x1008ff17, 171}},      // XF86AudioNext, KEY_NEXTSONG 163
    {"ColorF0Red", {0x1008ffa3, 406}},          // XF86Red, KEY_RED 398
    {"ColorF1Green", {0x1008ffa4, 407}},        // XF86Green, KEY_GREEN 399
    {"ColorF2Yellow", {0x1008ffa5, 408}},       // XF86Yellow, KEY_YELLOW 400
    {"ColorF3Blue", {0x1008ffa6, 409}},         // XF86Blue, KEY_BLUE 401
};

constexpr uint32_t kKeysymF1 = 0xffbe;             // WPE_KEY_F1 .. WPE_KEY_F12 are consecutive
constexpr uint32_t kKeysymUnicodeFlag = 0x01000000;  // keysym of a Unicode code point above U+00FF

constexpr const char *kFunctionKeyNames[] = {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"};

// the longest URL text a log line carries (the library's UC_LOGGABLE_MAX)
constexpr int kLoggableUrlMax = 200;

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

    // Any other Qt special key (Qt::Key_Escape = 0x01000000 and above) is not a character, whatever text the event
    // carries. The device's evdev keymap sends the remote's buttons without text (unicode 0xffff,
    // qevdevkeyboardhandler.cpp:249), but the button simulator sends the key's name (QKeySequence(key).toString() in
    // InputController::sendKeyEvent: "Media Rewind" for PREV, "Exit" for BACK), which the fallback below would
    // forward as its first letter. Qt::Key_unknown with text is a character Qt has no key code for.
    if (key >= Qt::Key_Escape && key != Qt::Key_unknown) {
        return 0;
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

const PageKey *pageKeyForName(const QString &name) {
    for (const NamedPageKey &entry : kPageKeys) {
        if (name == QLatin1String(entry.name)) {
            return &entry.key;
        }
    }
    return nullptr;
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

bool envSwitchEnabled(const QByteArray &value) {
    const QByteArray lower = value.toLower();
    return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

QString urlRejectionReason(const QUrl &url, bool allowFileUrls) {
    // QUrl lower-cases the scheme like g_uri_peek_scheme(); "localhost:8080" has the scheme "localhost" in both
    const QString scheme = url.scheme().toLower();
    if (scheme == QLatin1String("http") || scheme == QLatin1String("https") || scheme == QLatin1String("about") ||
        scheme == QLatin1String("data")) {
        return QString();
    }
    if (scheme == QLatin1String("file")) {
        return allowFileUrls ? QString()
                             : QStringLiteral("file: URLs are disabled (UC_WEBVIEW_ALLOW_FILE_URLS=1 enables them)");
    }
    if (scheme.isEmpty()) {
        return QStringLiteral("not an absolute URL");
    }
    return QStringLiteral("scheme not allowed (http, https, about, data; file with UC_WEBVIEW_ALLOW_FILE_URLS=1)");
}

const char *specialKeyName(uint32_t keysym) {
    for (const KeyMapping &mapping : kKeyTable) {
        if (mapping.keysym == keysym) {
            return mapping.name;
        }
    }
    if (keysym >= kKeysymF1 && keysym < kKeysymF1 + 12) {
        return kFunctionKeyNames[keysym - kKeysymF1];
    }
    return nullptr;
}

QString loggableUrl(const QUrl &url, bool full) {
    // the rules of the library's uc_uri_policy_loggable(), so that the item's and the library's lines agree
    const QString scheme = url.scheme().toLower();
    QString       text;
    if (url.isEmpty()) {
        text = QStringLiteral("(empty)");
    } else if (scheme.isEmpty()) {
        text = QStringLiteral("(no scheme)");
    } else if (!url.isValid()) {
        text = scheme + QLatin1Char(':');
    } else if (full) {
        text = url.toString(QUrl::RemoveUserInfo);
    } else {
        // the origin: a path, a query and a fragment carry tokens (reset links, OAuth codes, session ids)
        text = loadErrorAddress(url);
    }
    if (text.size() > kLoggableUrlMax) {
        int cut = kLoggableUrlMax;
        if (text.at(cut - 1).isHighSurrogate()) {
            --cut;
        }
        text = text.left(cut) + QStringLiteral("...");
    }
    return text;
}

QString loadErrorAddress(const QUrl &url) {
    if (url.isEmpty()) {
        return QString();
    }
    const QString scheme = url.scheme().toLower();
    if (url.host().isEmpty()) {
        return scheme.isEmpty() ? QString() : scheme + QLatin1Char(':');
    }
    QUrl address;
    address.setScheme(scheme);
    address.setHost(url.host());
    address.setPort(url.port());
    return address.toString();
}

QString loadErrorMessage(const QString &message) {
    constexpr int kMessageMax = 300;
    // a URL with an authority, up to the next blank, quote or angle bracket (GLib puts a host name it could not
    // resolve in typographic quotes)
    static const QRegularExpression urls(
        QStringLiteral("[A-Za-z][A-Za-z0-9+.-]*://[^\\s\"'<>\u201c\u201d\u2018\u2019]*"));
    QString text;
    int     last = 0;
    for (auto it = urls.globalMatch(message); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        text += message.mid(last, match.capturedStart() - last);
        // punctuation at its end belongs to the sentence: "(see https://host/a)."
        QString url = match.captured();
        int     end = match.capturedEnd();
        while (!url.isEmpty() && QStringLiteral(").,;:]").contains(url.back())) {
            url.chop(1);
            --end;
        }
        const QString address = loadErrorAddress(QUrl(url));
        text += address.isEmpty() ? QStringLiteral("...") : address;
        last = end;
    }
    text += message.mid(last);
    for (QChar &c : text) {
        if (c.category() == QChar::Other_Control) {
            c = QLatin1Char(' ');
        }
    }
    text = text.trimmed();
    if (text.size() > kMessageMax) {
        int cut = kMessageMax;
        if (text.at(cut - 1).isHighSurrogate()) {
            --cut;
        }
        text = text.left(cut) + QStringLiteral("...");
    }
    return text;
}

QString certificateErrorNames(unsigned flags) {
    // GTlsCertificateFlags (gio/gioenums.h), in the order a user checks them
    struct Flag {
        unsigned    bit;
        const char *name;
    };
    static constexpr Flag kFlags[] = {
        {1u << 3, "expired"},             // G_TLS_CERTIFICATE_EXPIRED
        {1u << 2, "not yet valid"},       // G_TLS_CERTIFICATE_NOT_ACTIVATED
        {1u << 0, "unknown issuer"},      // G_TLS_CERTIFICATE_UNKNOWN_CA
        {1u << 1, "wrong host name"},     // G_TLS_CERTIFICATE_BAD_IDENTITY
        {1u << 4, "revoked"},             // G_TLS_CERTIFICATE_REVOKED
        {1u << 5, "insecure algorithm"},  // G_TLS_CERTIFICATE_INSECURE
        {1u << 6, "other error"},         // G_TLS_CERTIFICATE_GENERIC_ERROR
    };
    QStringList names;
    unsigned    known = 0;
    for (const Flag &flag : kFlags) {
        known |= flag.bit;
        if (flags & flag.bit) {
            names.append(QString::fromLatin1(flag.name));
        }
    }
    if (flags & ~known) {
        names.append(QStringLiteral("flags 0x%1").arg(flags & ~known, 0, 16));
    }
    return names.join(QStringLiteral(", "));
}

double dragThresholdFromEnv(const QByteArray &value) {
    constexpr double kWpeDefault = 8.0;
    bool             ok = false;
    const int        px = value.toInt(&ok);
    return ok && px >= 1 && px <= 200 ? px : kWpeDefault;
}

}  // namespace webview
}  // namespace uc
