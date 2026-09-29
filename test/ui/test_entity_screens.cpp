// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMetaEnum>
#include <QSet>
#include <QXmlStreamReader>
#include <QtTest>

#include "ui/entity/activity.h"
#include "ui/entity/button.h"
#include "ui/entity/climate.h"
#include "ui/entity/cover.h"
#include "ui/entity/entityScreens.h"
#include "ui/entity/light.h"
#include "ui/entity/macro.h"
#include "ui/entity/mediaPlayer.h"
#include "ui/entity/remote.h"
#include "ui/entity/select.h"
#include "ui/entity/sensor.h"
#include "ui/entity/switch.h"
#include "ui/entity/voiceAssistant.h"

using uc::ui::EntityScreens;
using Type = uc::ui::entity::Base::Type;

Q_DECLARE_METATYPE(QMetaEnum)

class testEntityScreens : public QObject {
    Q_OBJECT

 private slots:
    void typesWithoutScreen_resolveToEmptyUrl_data();
    void typesWithoutScreen_resolveToEmptyUrl();
    void everyTypeWithScreenHasDeviceClassEnum();

    void everyDeviceClass_resolves_data();
    void everyDeviceClass_resolves();

    void unknownDeviceClass_usesDefaultScreen();
    void coverAliases();
    void screenName_isStableForNormalisedDeviceClass();

    void everyScreen_isRegisteredInQrc();
    void everyDeviceClassScreenFile_isReachable();

 private:
    // entity type -> its device class enum; a type with a screen must be listed here
    static QHash<int, QMetaEnum> deviceClassEnums() {
        using namespace uc::ui::entity;
        return {
            {Type::Activity, QMetaEnum::fromType<ActivityDeviceClass::Enum>()},
            {Type::Button, QMetaEnum::fromType<ButtonDeviceClass::Enum>()},
            {Type::Climate, QMetaEnum::fromType<ClimateDeviceClass::Enum>()},
            {Type::Cover, QMetaEnum::fromType<CoverDeviceClass::Enum>()},
            {Type::Light, QMetaEnum::fromType<LightDeviceClass::Enum>()},
            {Type::Macro, QMetaEnum::fromType<MacroDeviceClass::Enum>()},
            {Type::Media_player, QMetaEnum::fromType<MediaPlayerDeviceClass::Enum>()},
            {Type::Remote, QMetaEnum::fromType<RemoteDeviceClass::Enum>()},
            {Type::Select, QMetaEnum::fromType<SelectDeviceClass::Enum>()},
            {Type::Sensor, QMetaEnum::fromType<SensorDeviceClass::Enum>()},
            {Type::Switch, QMetaEnum::fromType<SwitchDeviceClass::Enum>()},
        };
    }

    static bool expectsScreen(Type type) { return type != Type::Voice_assistant && type != Type::Unsupported; }

    // alias -> file of every entry in main.qrc, the alias is the path the app loads the file with
    static QHash<QString, QString> qrcEntries() {
        QHash<QString, QString> entries;
        QFile                   file(QStringLiteral(MAIN_QRC));
        if (!file.open(QIODevice::ReadOnly)) {
            return entries;
        }

        QXmlStreamReader xml(&file);
        QString          prefix;
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) {
                continue;
            }
            if (xml.name() == QLatin1String("qresource")) {
                prefix = xml.attributes().value(QLatin1String("prefix")).toString();
                if (!prefix.endsWith('/')) {
                    prefix.append('/');
                }
            } else if (xml.name() == QLatin1String("file")) {
                const QString alias = xml.attributes().value(QLatin1String("alias")).toString();
                const QString path = xml.readElementText().trimmed();
                entries.insert(prefix + (alias.isEmpty() ? path : alias), path);
            }
        }
        if (xml.hasError()) {
            qWarning() << "Cannot parse" << MAIN_QRC << xml.errorString();
            entries.clear();
        }
        return entries;
    }
};

void testEntityScreens::typesWithoutScreen_resolveToEmptyUrl_data() {
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("screen");

    const QMetaEnum types = QMetaEnum::fromType<Type>();
    for (int i = 0; i < types.keyCount(); i++) {
        QTest::newRow(types.key(i)) << types.value(i) << expectsScreen(static_cast<Type>(types.value(i)));
    }
    // an entity type the UI does not know does not show a detail screen
    QTest::newRow("unknown type") << 4711 << false;
}

void testEntityScreens::typesWithoutScreen_resolveToEmptyUrl() {
    QFETCH(int, type);
    QFETCH(bool, screen);

    const auto t = static_cast<Type>(type);
    QCOMPARE(EntityScreens::hasScreen(t), screen);
    QCOMPARE(EntityScreens::screenUrl(t, QString()).isEmpty(), !screen);
    QCOMPARE(EntityScreens::screenUrl(t, QStringLiteral("NoSuchDeviceClass")).isEmpty(), !screen);
    QCOMPARE(EntityScreens::screenName(t, QString()).isEmpty(), !screen);
    if (!screen) {
        QVERIFY(EntityScreens::deviceClasses(t).isEmpty());
    }
}

void testEntityScreens::everyTypeWithScreenHasDeviceClassEnum() {
    const QMetaEnum types = QMetaEnum::fromType<Type>();
    const auto      enums = deviceClassEnums();
    for (int i = 0; i < types.keyCount(); i++) {
        const auto type = static_cast<Type>(types.value(i));
        if (EntityScreens::hasScreen(type)) {
            QVERIFY2(enums.contains(type), types.key(i));
        }
    }
}

void testEntityScreens::everyDeviceClass_resolves_data() {
    QTest::addColumn<int>("type");
    QTest::addColumn<QMetaEnum>("deviceClasses");

    const QMetaEnum types = QMetaEnum::fromType<Type>();
    const auto      enums = deviceClassEnums();
    for (auto it = enums.constBegin(); it != enums.constEnd(); ++it) {
        QTest::newRow(types.valueToKey(it.key())) << it.key() << it.value();
    }
}

void testEntityScreens::everyDeviceClass_resolves() {
    QFETCH(int, type);
    QFETCH(QMetaEnum, deviceClasses);

    const auto t = static_cast<Type>(type);
    QVERIFY(deviceClasses.isValid());

    QSet<QString> enumKeys;
    for (int i = 0; i < deviceClasses.keyCount(); i++) {
        const QString deviceClass = QString::fromLatin1(deviceClasses.key(i));
        enumKeys.insert(deviceClass);

        const QUrl url = EntityScreens::screenUrl(t, deviceClass);
        QVERIFY2(url.isValid() && !url.isEmpty(), qPrintable(deviceClass));
        QCOMPARE(url.scheme(), QStringLiteral("qrc"));
    }

    // registered explicitly, not only reached through the default screen, and no stale device classes
    const QStringList registered = EntityScreens::deviceClasses(t);
    QCOMPARE(QSet<QString>(registered.begin(), registered.end()), enumKeys);
}

void testEntityScreens::unknownDeviceClass_usesDefaultScreen() {
    QCOMPARE(EntityScreens::screenUrl(Type::Cover, QStringLiteral("Awning")),
             QUrl(QStringLiteral("qrc:/components/entities/cover/deviceclass/Blind.qml")));
    QCOMPARE(EntityScreens::screenUrl(Type::Media_player, QString()),
             QUrl(QStringLiteral("qrc:/components/entities/media_player/deviceclass/Speaker.qml")));
    QCOMPARE(EntityScreens::screenUrl(Type::Sensor, QStringLiteral("Pressure")),
             QUrl(QStringLiteral("qrc:/components/entities/sensor/deviceclass/Custom.qml")));
    QCOMPARE(EntityScreens::screenUrl(Type::Switch, QStringLiteral("switch")),
             QUrl(QStringLiteral("qrc:/components/entities/switch/deviceclass/Switch.qml")));
}

void testEntityScreens::coverAliases() {
    QCOMPARE(EntityScreens::screenName(Type::Cover, QStringLiteral("Shade")), QStringLiteral("Blind"));
    QCOMPARE(EntityScreens::screenName(Type::Cover, QStringLiteral("Door")), QStringLiteral("Window"));
    QCOMPARE(EntityScreens::screenName(Type::Cover, QStringLiteral("Gate")), QStringLiteral("Window"));
    QCOMPARE(EntityScreens::screenName(Type::Cover, QStringLiteral("Window")), QStringLiteral("Window"));
    QCOMPARE(EntityScreens::screenName(Type::Cover, QStringLiteral("Garage")), QStringLiteral("Garage"));
    QCOMPARE(EntityScreens::screenUrl(Type::Cover, QStringLiteral("Gate")),
             QUrl(QStringLiteral("qrc:/components/entities/cover/deviceclass/Window.qml")));
}

void testEntityScreens::screenName_isStableForNormalisedDeviceClass() {
    // an entity may store the screen name as its device class (the cover does): resolving it again must not move it
    const auto enums = deviceClassEnums();
    for (auto it = enums.constBegin(); it != enums.constEnd(); ++it) {
        const auto t = static_cast<Type>(it.key());
        for (int i = 0; i < it.value().keyCount(); i++) {
            const QString name = EntityScreens::screenName(t, QString::fromLatin1(it.value().key(i)));
            QCOMPARE(EntityScreens::screenName(t, name), name);
        }
    }
}

void testEntityScreens::everyScreen_isRegisteredInQrc() {
    const auto entries = qrcEntries();
    QVERIFY2(!entries.isEmpty(), "main.qrc could not be read");

    const QDir        qrcDir = QFileInfo(QStringLiteral(MAIN_QRC)).absoluteDir();
    const QList<QUrl> urls = EntityScreens::allScreenUrls();
    QVERIFY(!urls.isEmpty());

    for (const QUrl& url : urls) {
        const QString path = url.path();
        QVERIFY2(entries.contains(path), qPrintable(url.toString() + " is not registered in resources/qrc/main.qrc"));
        QVERIFY2(QFileInfo::exists(qrcDir.filePath(entries.value(path))),
                 qPrintable(entries.value(path) + " does not exist"));
    }
}

void testEntityScreens::everyDeviceClassScreenFile_isReachable() {
    const QList<QUrl> urls = EntityScreens::allScreenUrls();
    const QDir        entitiesDir(QStringLiteral(ENTITIES_QML_DIR));
    QVERIFY(entitiesDir.exists());

    int          found = 0;
    QDirIterator it(entitiesDir.absolutePath(), {QStringLiteral("*.qml")}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = it.next();
        const QString relative = entitiesDir.relativeFilePath(file);
        // <type>/deviceclass/<screen>.qml
        const QStringList parts = relative.split('/');
        if (parts.size() != 3 || parts.at(1) != QLatin1String("deviceclass")) {
            continue;
        }
        found++;
        const QUrl url(QStringLiteral("qrc:/components/entities/") + relative);
        QVERIFY2(urls.contains(url), qPrintable(relative + " is not reachable from EntityScreens"));
    }
    QCOMPARE(found, urls.size());
}

QTEST_GUILESS_MAIN(testEntityScreens)

#include "test_entity_screens.moc"
