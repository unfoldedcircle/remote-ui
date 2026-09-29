// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "entityScreens.h"

#include <QHash>

namespace uc {
namespace ui {

namespace {

struct TypeScreens {
    // directory under qrc:/components/entities/
    QString directory;
    // screen for an empty or unknown device class
    QString defaultScreen;
    // device class, as the entity reports it in getDeviceClass(), -> screen
    QHash<QString, QString> screens;
};

using Type = entity::Base::Type;

// Device class keys are the names of the <Type>DeviceClass::Enum values of the entity classes. A type missing here has
// no detail screen.
const QHash<int, TypeScreens>& registry() {
    static const QHash<int, TypeScreens> table = {
        {Type::Activity, {"activity", "Activity", {{"Activity", "Activity"}}}},
        {Type::Button, {"button", "Button", {{"Button", "Button"}}}},
        {Type::Climate, {"climate", "Climate", {{"Climate", "Climate"}}}},
        // some cover device classes look the same from the UI point of view
        {Type::Cover,
         {"cover",
          "Blind",
          {{"Blind", "Blind"},
           {"Curtain", "Curtain"},
           {"Garage", "Garage"},
           {"Shade", "Blind"},
           {"Door", "Window"},
           {"Gate", "Window"},
           {"Window", "Window"}}}},
        {Type::Light, {"light", "Light", {{"Light", "Light"}}}},
        {Type::Macro, {"macro", "Macro", {{"Macro", "Macro"}}}},
        {Type::Media_player,
         {"media_player",
          "Speaker",
          {{"Receiver", "Receiver"},
           {"Set_top_box", "Set_top_box"},
           {"Speaker", "Speaker"},
           {"Streaming_box", "Streaming_box"},
           {"Tv", "Tv"}}}},
        {Type::Remote, {"remote", "Remote", {{"Remote", "Remote"}}}},
        {Type::Select, {"select", "Select", {{"Select", "Select"}}}},
        {Type::Sensor,
         {"sensor",
          "Custom",
          {{"Custom", "Custom"},
           {"Battery", "Battery"},
           {"Current", "Current"},
           {"Energy", "Energy"},
           {"Humidity", "Humidity"},
           {"Power", "Power"},
           {"Temperature", "Temperature"},
           {"Voltage", "Voltage"},
           {"Binary", "Binary"}}}},
        {Type::Switch, {"switch", "Switch", {{"Switch", "Switch"}, {"Outlet", "Outlet"}}}},
    };
    return table;
}

QUrl urlFor(const TypeScreens& screens, const QString& screen) {
    return QUrl(QStringLiteral("qrc:/components/entities/%1/deviceclass/%2.qml").arg(screens.directory, screen));
}

}  // namespace

QString EntityScreens::screenName(entity::Base::Type type, const QString& deviceClass) {
    auto it = registry().constFind(type);
    if (it == registry().constEnd()) {
        return QString();
    }
    return it->screens.value(deviceClass, it->defaultScreen);
}

QUrl EntityScreens::screenUrl(entity::Base::Type type, const QString& deviceClass) {
    auto it = registry().constFind(type);
    if (it == registry().constEnd()) {
        return QUrl();
    }
    return urlFor(*it, it->screens.value(deviceClass, it->defaultScreen));
}

bool EntityScreens::hasScreen(entity::Base::Type type) {
    return registry().contains(type);
}

QStringList EntityScreens::deviceClasses(entity::Base::Type type) {
    return registry().value(type).screens.keys();
}

QList<QUrl> EntityScreens::allScreenUrls() {
    QList<QUrl> urls;
    for (const TypeScreens& screens : registry()) {
        QList<QString> names = screens.screens.values();
        names.append(screens.defaultScreen);
        for (const QString& name : names) {
            const QUrl url = urlFor(screens, name);
            if (!urls.contains(url)) {
                urls.append(url);
            }
        }
    }
    return urls;
}

}  // namespace ui
}  // namespace uc
