// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "integration/integrationDrivers.h"
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
    void dropdown_hasThePreselectedValue();

    void clone_hasTheSameContent();
    void driver_deletesItsSchema();
    void driver_replacingTheSchema_deletesThePreviousOne();

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
        SetupSchema schema(
            title(),
            {setting(QStringLiteral("none"), QVariant()), setting(QStringLiteral("empty"), QVariantMap()),
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
    SetupSchema schema(title(),
                       {setting(QStringLiteral("address"), QVariantMap({{QStringLiteral("text"), QVariantMap()}})),
                        setting(QStringLiteral("missing"), QVariant()),
                        setting(QStringLiteral("port"), QVariantMap({{QStringLiteral("number"), QVariantMap()}}))},
                       QStringLiteral("en"));

    QCOMPARE(schema.getSettings().size(), 2);
}

void testSetupSchema::dropdown_hasThePreselectedValue() {
    QVariantMap dropdown;
    dropdown.insert(QStringLiteral("value"), QStringLiteral("b"));
    dropdown.insert(
        QStringLiteral("items"),
        QVariantList(
            {QVariantMap({{QStringLiteral("id"), QStringLiteral("a")},
                          {QStringLiteral("label"), QVariantMap({{QStringLiteral("en"), QStringLiteral("A")}})}}),
             QVariantMap({{QStringLiteral("id"), QStringLiteral("b")},
                          {QStringLiteral("label"), QVariantMap({{QStringLiteral("en"), QStringLiteral("B")}})}})}));

    SetupSchema schema(title(),
                       {setting(QStringLiteral("choice"), QVariantMap({{QStringLiteral("dropdown"), dropdown}}))},
                       QStringLiteral("en"));

    QCOMPARE(schema.getSettings().size(), 1);
    auto item = qobject_cast<uc::integration::SettingsItemDropdown*>(schema.getSettings().first());
    QVERIFY(item);
    // the Core-API's optional id of the preselected item; the first item was sent instead
    QCOMPARE(item->getValue(), QStringLiteral("b"));
    QCOMPARE(item->getModel().size(), 2);
}

void testSetupSchema::clone_hasTheSameContent() {
    SetupSchema schema(title(),
                       {setting(QStringLiteral("address"), QVariantMap({{QStringLiteral("text"), QVariantMap()}})),
                        setting(QStringLiteral("port"), QVariantMap({{QStringLiteral("number"), QVariantMap()}}))},
                       QStringLiteral("en"));

    QScopedPointer<SetupSchema> copy(schema.clone());
    QCOMPARE(copy->getTitle(), schema.getTitle());
    QCOMPARE(copy->getSettings().size(), 2);
    // a schema of its own: its items are not the original's
    QVERIFY(copy->getSettings().first() != schema.getSettings().first());
    QCOMPARE(copy->getSettings().first()->getId(), QStringLiteral("address"));
}

/**
 * Every driver list reload (after a connect, on the integrations settings page, for every "Add integration")
 * creates a schema per driver. The schema had no parent and the driver did not delete it: one schema with its
 * items leaked per driver and reload.
 */
void testSetupSchema::driver_deletesItsSchema() {
    QPointer<SetupSchema> schema = new SetupSchema(title(), {}, QStringLiteral("en"));

    auto driver = new uc::integration::IntegrationDriver(QStringLiteral("driver"), QVariantMap(), QString(), QString(),
                                                         QString(), true, QString(), QString(), QString(), QString(),
                                                         QString(), schema, false, 0, QStringLiteral("en"));

    QCOMPARE(schema->parent(), driver);
    delete driver;
    QVERIFY(schema.isNull());
}

void testSetupSchema::driver_replacingTheSchema_deletesThePreviousOne() {
    QPointer<SetupSchema> first = new SetupSchema(title(), {}, QStringLiteral("en"));

    uc::integration::IntegrationDriver driver(QStringLiteral("driver"), QVariantMap(), QString(), QString(), QString(),
                                              true, QString(), QString(), QString(), QString(), QString(), first, false,
                                              0, QStringLiteral("en"));

    QPointer<SetupSchema> second = new SetupSchema(title(), {}, QStringLiteral("en"));
    driver.setSetupScehma(second);
    QCOMPARE(driver.getSetupSchema(), second.data());

    // deleted on the next event loop turn
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(first.isNull());
    QVERIFY(!second.isNull());
}

QTEST_GUILESS_MAIN(testSetupSchema)

#include "test_setup_schema.moc"
