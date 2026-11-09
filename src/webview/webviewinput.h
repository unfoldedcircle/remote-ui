// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <Qt>

#include <cstdint>

namespace uc {
namespace webview {

// Pure mappings between Qt input events and the libucwebview ABI (uc_webview.h), plus the library's rules the item
// mirrors (the URL scheme allowlist, the tap distance). They have no QQuickItem dependency so the unit tests can
// check every table entry without a window.

// XKB keysym for a Qt key event, or 0 when the key is not forwarded to the page: the event is then left to the
// QML focus chain and the remote's button navigation (BACK, HOME, the colour, media and volume buttons). text is
// QKeyEvent::text(); a printable character is forwarded as its Unicode keysym. A Qt special key (Qt::Key_Escape and
// above) outside the table is never a character, whatever its text (the button simulator sends the key's name).
uint32_t xkbKeysymForKey(int key, const QString &text);

// UC_WEBVIEW_MOD_* bits for the keyboard modifiers and the held mouse buttons of an event.
unsigned modifiersForEvent(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons = Qt::NoButton);

// UC_WEBVIEW_BUTTON_* for a Qt mouse button, 0 for a button the page does not know.
int pointerButtonForQt(Qt::MouseButton button);

// UC_WEBVIEW_TOUCH_* phase for a touch point state, -1 for a stationary point (nothing to send).
int touchPhaseForState(Qt::TouchPointState state);

// Qt::InputMethodHints for the WPE input purpose and hints (UC_WEBVIEW_IME_*) of the focused editable element.
// The Qt Virtual Keyboard picks its layout (digits, hidden text, ...) from these.
Qt::InputMethodHints inputMethodHintsForIme(unsigned hints, unsigned purpose);

// libucwebview's boolean environment switches ("1", "true", "yes", "on", case-insensitive; uc_env_value_true in
// uc_uri_policy.c), for UC_WEBVIEW_ALLOW_FILE_URLS which the item and the library must read alike.
bool envSwitchEnabled(const QByteArray &value);

// The library's URL scheme allowlist (uc_uri_policy.c, applied by uc_webview_load_uri() and to the base URL of
// uc_webview_load_html()): http, https, about and data always, file only with allowFileUrls, nothing else (other
// schemes and URLs without a scheme). Returns why url would be refused, or an empty string when it may be loaded.
QString urlRejectionReason(const QUrl &url, bool allowFileUrls);

// The distance in view pixels after which WebKit's gesture controller turns a touch sequence into a drag instead
// of a tap (WPEGestureDetector.cpp:63-69) as a library older than 0.3.0 sets it: UC_WEBVIEW_DRAG_THRESHOLD (a
// decimal 1..200, set as WPE_SETTING_DRAG_THRESHOLD) or WPE's default of 8 px (WPESettings.cpp:106). Library 0.3.0
// reports its value itself (uc_webview_global_get_drag_threshold(), 16 px by default; WebViewLibrary::dragThreshold).
double dragThresholdFromEnv(const QByteArray &value);

// The input trace's name of a key the item forwards that types no character (Return, BackSpace, the arrows, Tab,
// Escape, Delete, Insert, End, Page_Up/Down, KP_Enter, the modifiers, F1..F12), or nullptr: every other keysym is a
// character and appears in the trace as "printable" only, without keysym and scan code (docs/webview.md, "What the
// trace logs").
const char *specialKeyName(uint32_t keysym);

// The URL for a log line: never with user info (credentials); unless full (UC_WEBVIEW_LOG_FULL_URLS=1) only
// scheme, host, port and path of a URL with a host and of file: and about: URLs, a query and a fragment shown as
// "?..." and "#..." (they carry tokens); "data:<media type>,..." for a data: URL, "<scheme>:..." for other URLs
// without a host (javascript:, blob:, mailto:). At most 200 characters. The rules of the library's
// uc_uri_policy_loggable().
QString loggableUrl(const QUrl &url, bool full);

}  // namespace webview
}  // namespace uc
