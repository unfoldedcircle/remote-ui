// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewButtons.h"

namespace uc {
namespace ui {
namespace entity {

namespace {

const QString kAction = QStringLiteral("action");
const QString kFunction = QStringLiteral("function");
const QString kKey = QStringLiteral("key");
const QString kPass = QStringLiteral("pass");

// the buttons of both models, in the order of the settings page
const QStringList &commonButtons() {
    static const QStringList buttons{
        QStringLiteral("BACK"),         QStringLiteral("DPAD_UP"),     QStringLiteral("DPAD_DOWN"),
        QStringLiteral("DPAD_LEFT"),    QStringLiteral("DPAD_RIGHT"),  QStringLiteral("DPAD_MIDDLE"),
        QStringLiteral("VOLUME_UP"),    QStringLiteral("VOLUME_DOWN"), QStringLiteral("CHANNEL_UP"),
        QStringLiteral("CHANNEL_DOWN"), QStringLiteral("MUTE"),        QStringLiteral("PREV"),
        QStringLiteral("PLAY"),         QStringLiteral("NEXT")};
    return buttons;
}

const QStringList &remoteThreeButtons() {
    static const QStringList buttons{QStringLiteral("STOP"), QStringLiteral("RECORD"), QStringLiteral("MENU")};
    return buttons;
}

const QStringList &remoteTwoButtons() {
    static const QStringList buttons{QStringLiteral("RED"), QStringLiteral("GREEN"), QStringLiteral("YELLOW"),
                                     QStringLiteral("BLUE")};
    return buttons;
}

QVariantMap keyAction(const char *key) {
    return QVariantMap{{kAction, QStringLiteral("key")}, {kKey, QString::fromLatin1(key)}};
}

QVariantMap functionAction(const char *function) {
    return QVariantMap{{kAction, QStringLiteral("function")}, {kFunction, QString::fromLatin1(function)}};
}

}  // namespace

QStringList WebviewButtons::ofModel(hw::HardwareModel::Enum model) {
    return commonButtons() + (model == hw::HardwareModel::UCR3 ? remoteThreeButtons() : remoteTwoButtons());
}

QStringList WebviewButtons::all() {
    return commonButtons() + remoteThreeButtons() + remoteTwoButtons();
}

QStringList WebviewButtons::functions() {
    static const QStringList functions{QStringLiteral("zoom_in"),     QStringLiteral("zoom_out"),
                                       QStringLiteral("zoom_reset"),  QStringLiteral("scroll_up"),
                                       QStringLiteral("scroll_down"), QStringLiteral("page_up"),
                                       QStringLiteral("page_down"),   QStringLiteral("top"),
                                       QStringLiteral("bottom"),      QStringLiteral("back"),
                                       QStringLiteral("forward"),     QStringLiteral("back_or_close"),
                                       QStringLiteral("reload"),      QStringLiteral("stop"),
                                       QStringLiteral("close"),       QStringLiteral("toggle_navigation"),
                                       QStringLiteral("focus_next"),  QStringLiteral("focus_previous"),
                                       QStringLiteral("none")};
    return functions;
}

QStringList WebviewButtons::keys() {
    static const QStringList keys{QStringLiteral("ArrowUp"),
                                  QStringLiteral("ArrowDown"),
                                  QStringLiteral("ArrowLeft"),
                                  QStringLiteral("ArrowRight"),
                                  QStringLiteral("Enter"),
                                  QStringLiteral("Escape"),
                                  QStringLiteral("Backspace"),
                                  QStringLiteral("Tab"),
                                  QStringLiteral("PageUp"),
                                  QStringLiteral("PageDown"),
                                  QStringLiteral("Home"),
                                  QStringLiteral("End"),
                                  QStringLiteral("ContextMenu"),
                                  QStringLiteral("BrowserBack"),
                                  QStringLiteral("BrowserForward"),
                                  QStringLiteral("BrowserRefresh"),
                                  QStringLiteral("BrowserStop"),
                                  QStringLiteral("Exit"),
                                  QStringLiteral("AudioVolumeUp"),
                                  QStringLiteral("AudioVolumeDown"),
                                  QStringLiteral("AudioVolumeMute"),
                                  QStringLiteral("ChannelUp"),
                                  QStringLiteral("ChannelDown"),
                                  QStringLiteral("MediaPlayPause"),
                                  QStringLiteral("MediaPlay"),
                                  QStringLiteral("MediaPause"),
                                  QStringLiteral("MediaStop"),
                                  QStringLiteral("MediaRecord"),
                                  QStringLiteral("MediaRewind"),
                                  QStringLiteral("MediaFastForward"),
                                  QStringLiteral("MediaTrackPrevious"),
                                  QStringLiteral("MediaTrackNext"),
                                  QStringLiteral("ColorF0Red"),
                                  QStringLiteral("ColorF1Green"),
                                  QStringLiteral("ColorF2Yellow"),
                                  QStringLiteral("ColorF3Blue")};
    return keys;
}

QVariantMap WebviewButtons::defaultAction(const QString &button) {
    if (button == QLatin1String("DPAD_UP")) {
        return keyAction("ArrowUp");
    }
    if (button == QLatin1String("DPAD_DOWN")) {
        return keyAction("ArrowDown");
    }
    if (button == QLatin1String("DPAD_LEFT")) {
        return keyAction("ArrowLeft");
    }
    if (button == QLatin1String("DPAD_RIGHT")) {
        return keyAction("ArrowRight");
    }
    if (button == QLatin1String("DPAD_MIDDLE")) {
        return keyAction("Enter");
    }
    if (button == QLatin1String("BACK")) {
        return functionAction("close");
    }
    if (button == QLatin1String("VOLUME_UP")) {
        return functionAction("zoom_in");
    }
    if (button == QLatin1String("VOLUME_DOWN")) {
        return functionAction("zoom_out");
    }
    if (button == QLatin1String("CHANNEL_UP")) {
        return functionAction("page_up");
    }
    if (button == QLatin1String("CHANNEL_DOWN")) {
        return functionAction("page_down");
    }
    if (button == QLatin1String("MENU")) {
        return keyAction("ContextMenu");
    }
    return QVariantMap{{kAction, kPass}};
}

QVariantMap WebviewButtons::validAction(const QVariant &action) {
    const QVariantMap map = action.toMap();
    const QString     type = map.value(kAction).toString();
    if (type == kPass) {
        return QVariantMap{{kAction, kPass}};
    }
    if (type == kFunction) {
        const QString function = map.value(kFunction).toString();
        return functions().contains(function) ? QVariantMap{{kAction, kFunction}, {kFunction, function}}
                                              : QVariantMap();
    }
    if (type == kKey) {
        const QString key = map.value(kKey).toString();
        return keys().contains(key) ? QVariantMap{{kAction, kKey}, {kKey, key}} : QVariantMap();
    }
    return QVariantMap();
}

QVariantMap WebviewButtons::parse(const QVariant &buttons) {
    QVariantMap       result;
    const QVariantMap map = buttons.toMap();
    const QStringList known = all();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (!known.contains(it.key())) {
            continue;
        }
        const QVariantMap action = validAction(it.value());
        if (!action.isEmpty()) {
            result.insert(it.key(), action);
        }
    }
    return result;
}

QVariantMap WebviewButtons::withDefaults(const QStringList &buttons, const QVariantMap &configured) {
    QVariantMap result;
    for (const QString &button : buttons) {
        const QVariantMap action = validAction(configured.value(button));
        result.insert(button, action.isEmpty() ? defaultAction(button) : action);
    }
    return result;
}

bool WebviewButtons::repeats(const QVariantMap &action) {
    const QString type = action.value(kAction).toString();
    if (type == kKey) {
        return true;
    }
    if (type != kFunction) {
        return false;
    }
    const QString function = action.value(kFunction).toString();
    return function == QLatin1String("scroll_up") || function == QLatin1String("scroll_down") ||
           function == QLatin1String("page_up") || function == QLatin1String("page_down");
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
