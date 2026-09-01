// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include <limits>

#include "translation/translation.h"

using uc::ui::Translation;

class testI18n : public QObject {
    Q_OBJECT

 private slots:
    void getTimeZonesReturnsOnlyCountryZones();
    void getTimeZonesIsCaseInsensitive();
    void getTimeZonesUnknownCountry();

    void getCountriesForLanguage_data();
    void getCountriesForLanguage();

    void getLikelyCountry_data();
    void getLikelyCountry();

    void getTimeZoneInfosSingleZoneCountry();
    void getTimeZoneInfosMultiZoneCountry();
    void getAllTimeZoneInfos();
};

// regression test: the timezone list used to be widened to every world zone sharing the
// current UTC offset, turning Switzerland's single zone into 100+ entries
void testI18n::getTimeZonesReturnsOnlyCountryZones() {
    QStringList swiss = Translation::getTimeZones("CH");
    QCOMPARE(swiss, QStringList({"Europe/Zurich"}));
    QVERIFY(!swiss.contains("Europe/Berlin"));

    QStringList german = Translation::getTimeZones("DE");
    QVERIFY(german.contains("Europe/Berlin"));
    QVERIFY(!german.contains("Europe/Zurich"));
    QVERIFY(!german.contains("Africa/Algiers"));
    for (const auto& id : german) {
        QVERIFY2(id.startsWith("Europe/"), qPrintable(id));
    }
}

void testI18n::getTimeZonesIsCaseInsensitive() {
    QCOMPARE(Translation::getTimeZones("ch"), Translation::getTimeZones("CH"));
}

void testI18n::getTimeZonesUnknownCountry() {
    // an unknown code must not return the world list; the UI falls back to it deliberately
    QVERIFY(!Translation::getTimeZones("XX").contains("Europe/Zurich"));
}

void testI18n::getCountriesForLanguage_data() {
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("first");
    QTest::addColumn<QStringList>("contains");

    QTest::newRow("de_DE suggests the German speaking countries, Germany first")
        << "de_DE" << "DE" << QStringList({"DE", "AT", "CH", "BE"});
    QTest::newRow("de_CH puts Switzerland first") << "de_CH" << "CH" << QStringList({"DE", "AT", "CH"});
    QTest::newRow("da_DK suggests Denmark") << "da_DK" << "DK" << QStringList({"DK"});
    QTest::newRow("nl_NL suggests the Dutch speaking countries") << "nl_NL" << "NL" << QStringList({"NL", "BE"});
    // widely spoken languages use the curated main countries, not the ~100 unranked CLDR
    // territories
    QTest::newRow("en_US suggests the main English speaking countries")
        << "en_US" << "US" << QStringList({"US", "GB", "CA", "AU", "NZ", "IE"});
    QTest::newRow("fr_FR suggests the main French speaking countries")
        << "fr_FR" << "FR" << QStringList({"FR", "BE", "CH", "CA", "LU"});
    QTest::newRow("es_ES suggests the main Spanish speaking countries")
        << "es_ES" << "ES" << QStringList({"ES", "MX", "AR"});
    QTest::newRow("pt_PT suggests the main Portuguese speaking countries")
        << "pt_PT" << "PT" << QStringList({"PT", "BR"});
}

void testI18n::getCountriesForLanguage() {
    QFETCH(QString, language);
    QFETCH(QString, first);
    QFETCH(QStringList, contains);

    QStringList result = Translation::getCountriesForLanguage(language);

    QVERIFY(!result.isEmpty());
    QCOMPARE(result.first(), first);
    // a suggestion section longer than a screen is useless
    QVERIFY(result.length() <= 10);
    for (const auto& code : contains) {
        QVERIFY2(result.contains(code), qPrintable(language + " is expected to suggest " + code));
    }
}

void testI18n::getLikelyCountry_data() {
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("expected");

    QTest::newRow("country suffix wins: en_US") << "en_US" << "US";
    QTest::newRow("country suffix wins: de_CH") << "de_CH" << "CH";
    QTest::newRow("bare language de resolves via likely subtags") << "de" << "DE";
    QTest::newRow("bare language sv resolves via likely subtags") << "sv" << "SE";
}

void testI18n::getLikelyCountry() {
    QFETCH(QString, language);
    QFETCH(QString, expected);

    QCOMPARE(Translation::getLikelyCountry(language), expected);
}

void testI18n::getTimeZoneInfosSingleZoneCountry() {
    QVariantList swiss = Translation::getTimeZoneInfos("CH");
    QCOMPARE(swiss.length(), 1);

    QVariantMap zone = swiss.first().toMap();
    QCOMPARE(zone.value("id").toString(), QString("Europe/Zurich"));
    QCOMPARE(zone.value("city").toString(), QString("Zurich"));
    QCOMPARE(zone.value("offsetLabel").toString(), QString("GMT+01:00"));
}

void testI18n::getTimeZoneInfosMultiZoneCountry() {
    QVariantList us = Translation::getTimeZoneInfos("US");
    QVERIFY(us.length() > 1);

    QRegularExpression offsetFormat("^GMT[+-]\\d{2}:\\d{2}$");

    bool foundNewYork = false;
    int  previousOffset = std::numeric_limits<int>::max();
    for (const auto& item : us) {
        QVariantMap zone = item.toMap();

        // the IANA id tail is humanized: no underscores in the shown city
        QVERIFY(!zone.value("city").toString().contains("_"));
        QVERIFY2(offsetFormat.match(zone.value("offsetLabel").toString()).hasMatch(),
                 qPrintable(zone.value("offsetLabel").toString()));

        // sorted east to west: offsets are descending
        QVERIFY(zone.value("offset").toInt() <= previousOffset);
        previousOffset = zone.value("offset").toInt();

        if (zone.value("id").toString() == "America/New_York") {
            foundNewYork = true;
            QCOMPARE(zone.value("city").toString(), QString("New York"));
            QCOMPARE(zone.value("offsetLabel").toString(), QString("GMT-05:00"));
        }
    }
    QVERIFY(foundNewYork);
}

void testI18n::getAllTimeZoneInfos() {
    QVariantList all = Translation::getAllTimeZoneInfos();

    // far more than any single country, and sorted by id
    QVERIFY(all.length() > 300);
    for (int i = 1; i < all.length(); i++) {
        QVERIFY(all[i - 1].toMap().value("id").toString() <= all[i].toMap().value("id").toString());
    }
}

QTEST_GUILESS_MAIN(testI18n)

#include "test_translation.moc"
