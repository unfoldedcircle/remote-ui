// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "entity.h"

namespace uc {
namespace ui {

/**
 * @brief The one place that knows which QML detail screen shows an entity.
 *
 * A detail screen is a file `qrc:/components/entities/<type directory>/deviceclass/<screen>.qml`. The registry maps an
 * entity type to its screen directory and its default screen, and a device class to the screen that shows it. Device
 * classes that look the same from the UI point of view share a screen (e.g. a shade cover uses the blind screen). An
 * unknown device class of a known type gets the type's default screen. An entity type without a detail screen
 * (voice assistant, unsupported or unknown types) gets an empty URL and does not show a detail screen.
 *
 * A new entity type or device class screen is registered in the table in entityScreens.cpp, and the QML file must be
 * added to resources/qrc/main.qrc. The testEntityScreens unit test checks both.
 */
class EntityScreens {
 public:
    /**
     * @brief The screen a device class is shown with.
     * @return the name of the screen file without the `.qml` suffix, the type's default screen for an unknown or empty
     * device class, or an empty string if the entity type has no detail screen.
     */
    static QString screenName(entity::Base::Type type, const QString& deviceClass);

    /**
     * @brief The qrc URL of the detail screen for an entity type and device class.
     * @return the URL, or an empty URL if the entity type has no detail screen.
     */
    static QUrl screenUrl(entity::Base::Type type, const QString& deviceClass);

    /// Whether the entity type has a detail screen at all.
    static bool hasScreen(entity::Base::Type type);

    /// The device classes registered for an entity type, empty if the type has no detail screen.
    static QStringList deviceClasses(entity::Base::Type type);

    /// Every URL the registry can return, each one once.
    static QList<QUrl> allScreenUrls();
};

}  // namespace ui
}  // namespace uc
