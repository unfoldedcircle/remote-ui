// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "integration/setupSchema.h"

using uc::integration::SettingsEnum;
using uc::integration::SetupSchema;

/**
 * The setup schema comes from an integration driver, which may be a third party one. A setting whose `field` is
 * missing or names no field type the UI knows is skipped; its type used to be read without ever being assigned.
 */
class testSetupSchema : public QObject {
    Q_OBJECT

 private slots:
    void settingWithoutField_isSkipped();
    void settingWithUnknownField_isSkipped();
    void knownFields_areCreated();

 private:
    static QVariantMap setting(const QString& id, const QVariant& field);
    static QVariantMap title();
};

QVariantMap testSetupSchema::setting(const QString& id, const QVariant& field) {
    QVariantMap map;
    map.insert(QStringLiteral("id"), id);
    map.insert(QStringLiteral("label"), QVariantMap({{QStringLiteral("en"), id}}));
    if (field.isValid()) {
        map.insert(QStringLiteral("field"), field);
    }
    return map;
}

QVariantMap testSetupSchema::title() {
    return QVariantMap({{QStringLiteral("en"), QStringLiteral("Settings")}});
}

void testSetupSchema::settingWithoutField_isSkipped() {
    // several rounds: an indeterminate value does not have to be wrong the first time
    for (int i = 0; i < 20; i++) {
        SetupSchema schema(title(),
                           {setting(QStringLiteral("none"), QVariant()),
                            setting(QStringLiteral("empty"), QVariantMap()),
                            setting(QStringLiteral("no_object"), QStringLiteral("text"))},
                           QStringLiteral("en"));

        QCOMPARE(schema.getSettings().size(), 0);
    }
}

void testSetupSchema::settingWithUnknownField_isSkipped() {
    SetupSchema schema(title(),
                       {setting(QStringLiteral("future"), QVariantMap({{QStringLiteral("hologram"), QVariantMap()}}))},
                       QStringLiteral("en"));

    QCOMPARE(schema.getSettings().size(), 0);
}

void testSetupSchema::knownFields_areCreated() {
    SetupSchema schema(
        title(),
        {setting(QStringLiteral("address"), QVariantMap({{QStringLiteral("text"), QVariantMap()}})),
         setting(QStringLiteral("missing"), QVariant()),
         setting(QStringLiteral("port"), QVariantMap({{QStringLiteral("number"), QVariantMap()}}))},
        QStringLiteral("en"));

    QCOMPARE(schema.getSettings().size(), 2);
}

QTEST_GUILESS_MAIN(testSetupSchema)

#include "test_setup_schema.moc"
