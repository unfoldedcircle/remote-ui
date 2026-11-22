// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStringList>
#include <QVariantMap>

#include "../../hardware/hardwareModel.h"

namespace uc {
namespace ui {
namespace entity {

/**
 * The physical buttons of the fullscreen web view (core-api entity_webview.md "Buttons", "Button settings"): which
 * buttons a web view can change on a remote model, what they do by default, and the actions of a `buttons` object.
 *
 * An action is a map as the Core-API writes it: {"action": "pass"}, {"action": "function", "function": <name>} or
 * {"action": "key", "key": <KeyboardEvent.key name>}. HOME, POWER and VOICE have a fixed function in the web view and
 * are no button of a mapping.
 *
 * The action of a button in a fullscreen web view is the first of: the web view's (its user override, else its option,
 * entity::Webview::getButtons()), the global setting (WebViewController::buttons(), every button of the model with its
 * configured or default action).
 */
class WebviewButtons {
 public:
    // the buttons of the model that a mapping changes, in the order of the settings page: the Remote Two has the colour
    // buttons, the Remote 3 STOP, RECORD and MENU. The desktop simulator has the Remote Two's (its button window).
    static QStringList ofModel(hw::HardwareModel::Enum model);
    // every button identifier of the Core-API (WebviewButton), of both models
    static QStringList all();
    // the web view functions (WebviewButtonFunction) and the keys (WebviewButtonKey), in the order of the Core-API
    static QStringList functions();
    static QStringList keys();

    // the action of the button when neither the global setting nor the web view changes it
    static QVariantMap defaultAction(const QString &button);
    // the action in its Core-API form, or an empty map when it is none: an unknown action, function or key, or a
    // missing `function` or `key`. A field the action does not use is dropped.
    static QVariantMap validAction(const QVariant &action);
    // the valid actions of a `buttons` object: an unknown button, one with a fixed function, an invalid action and null
    // (a removed override) are left out (ADR 0018)
    static QVariantMap parse(const QVariant &buttons);
    // every one of the buttons with its action in configured, else its default action
    static QVariantMap withDefaults(const QStringList &buttons, const QVariantMap &configured);
    // whether a held button repeats its action: a key event and the scroll and page functions do, the other functions
    // act once per press, and `pass` repeats as the layer below does
    static bool repeats(const QVariantMap &action);
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
