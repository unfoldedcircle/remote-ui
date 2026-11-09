// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <Qt>

#include <cstdint>

namespace uc {
namespace webview {

// Pure mappings between Qt input events and the libucwebview ABI (uc_webview.h). They have no QQuickItem
// dependency so the unit tests can check every table entry without a window.

// XKB keysym for a Qt key event, or 0 when the key is not forwarded to the page: the event is then left to the
// QML focus chain and the remote's button navigation (BACK, HOME, the colour, media and volume buttons). text is
// QKeyEvent::text(); a printable character is forwarded as its Unicode keysym.
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

}  // namespace webview
}  // namespace uc
