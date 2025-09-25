#include <QtTest>

#include "util.h"

class testCommon : public QObject {
    Q_OBJECT

 private slots:
    void getDefaultCountryLocale_data();
    void getDefaultCountryLocale();

    void getLanguageStringWithFallback();

    void getLanguageString_data();
    void getLanguageString();
};

void testCommon::getDefaultCountryLocale_data() {
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("expected");

    QTest::newRow("returns the full locale for short language key de") << "de" << "de_DE";
    QTest::newRow("returns the full locale for short language key fr") << "fr" << "fr_FR";
    QTest::newRow("returns the full locale for short language key it") << "it" << "it_IT";

    QTest::newRow("returns the same locale for a default country locale de_DE") << "de_DE" << "de_DE";
    QTest::newRow("returns the same locale for a default country locale fr_FR") << "fr_FR" << "fr_FR";
    QTest::newRow("returns the same locale for a default country locale it_IT") << "it_IT" << "it_IT";

    QTest::newRow("returns the default country locale for a country locale de_CH") << "de_CH" << "de_DE";
    QTest::newRow("returns the default country locale for a country locale fr_CA") << "fr_CA" << "fr_FR";
    QTest::newRow("returns the default country locale for a country locale it_CH") << "it_CH" << "it_IT";
}

void testCommon::getDefaultCountryLocale() {
    QFETCH(QString, locale);
    QFETCH(QString, expected);

    QString result = uc::Util::getDefaultCountryLocale(locale);

    QCOMPARE(result, expected);
}

void testCommon::getLanguageStringWithFallback() {
    QString result = uc::Util::getLanguageString(QVariantMap(), "en", "foobar");

    QCOMPARE(result, "foobar");

    result = uc::Util::getLanguageString(QVariantMap(), "en");
    QCOMPARE(result, "");
}

void testCommon::getLanguageString_data() {
    QTest::addColumn<QVariantMap>("input");
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("expectedresult");

    QVariantMap map = {{"en", "English fallback"}, {"de", "German fallback"}, {"fr", "French fallback"},
                       {"de_DE", "German"},        {"de_CH", "Swiss German"}, {"en_UK", "UK English"},
                       {"en_US", "US English"},    {"fr_FR", "French"},       {"fr_CH", "Swiss French"},
                       // empty language text needs to be skipped
                       {"it_IT", ""}};

    QTest::newRow("Short direct match for en") << map << "en" << "English fallback";
    QTest::newRow("Short direct match for de") << map << "de" << "German fallback";

    QTest::newRow("Direct match for en_US") << map << "en_US" << "US English";
    QTest::newRow("Direct match for en_UK") << map << "en_UK" << "UK English";

    QTest::newRow("Direct match for de_DE") << map << "de_DE" << "German";
    QTest::newRow("Direct match for de_CH") << map << "de_CH" << "Swiss German";

    QTest::newRow("German fallback for missing de_AT") << map << "de_AT" << "German fallback";

    QTest::newRow("Direct match for fr_FR") << map << "fr_FR" << "French";
    QTest::newRow("Direct match for fr_CH") << map << "fr_CH" << "Swiss French";
    QTest::newRow("French fallback for missing fr_CA") << map << "fr_CA" << "French fallback";

    QTest::newRow("English fallback for missing it") << map << "it" << "English fallback";
    QTest::newRow("English fallback for missing it_IT") << map << "it_IT" << "English fallback";

    // Special logic for Swiss German de_CH
    QVariantMap mapCH = {{"en", "English fallback"}, {"de_DE", "German"}, {"de_CH", "Swiss German"}};

    QTest::newRow("CH: Short direct match for en") << mapCH << "en" << "English fallback";
    QTest::newRow("CH: Direct match for de_DE") << mapCH << "de_DE" << "German";
    QTest::newRow("CH: Direct match for de_CH") << mapCH << "de_CH" << "Swiss German";

    // there is no `de` -> use first country specific match, EXCEPT de_CH
    QTest::newRow("CH: German match for missing de") << mapCH << "de" << "German";

    QTest::newRow("CH: German match for missing de_AT") << mapCH << "de_AT" << "German";

    QVariantMap mapNoFallback = {{"de_DE", "German"}, {"de_CH", "Swiss German"},
                           {"en_UK", "UK English"}, {"en_US", "US English"}, {"en_AU", "AU English"},
                           {"fr_FR", "French"}, {"fr_CH", "Swiss French"}};

    QTest::newRow("First sorted en_## country text for missing en") << mapNoFallback << "en" << "AU English";
    QTest::newRow("de_DE text for de without fallback") << mapNoFallback << "de" << "German";

    QVariantMap mapNoEn = {{"de_DE", "German"}, {"de_CH", "Swiss German"},
                           {"fr_FR", "French"}, {"fr_CH", "Swiss French"}};

    // Attention: this test case result is different than in web-configurator.
    // Qt sorts the language key (result: de_DE German), TS uses object key positions (de_CH Swiss German)!
    QTest::newRow("Fallback to first entry if no matches") << mapNoEn << "xx" << "Swiss German";

    // if a language text is empty, the next fallback must be returned
    QVariantMap mapEmptyCountry = {{"en", "English fallback"}, {"de", "German fallback"}, {"fr", "French fallback"},
                                      {"de_DE", ""}, {"de_CH", ""}, {"fr_FR", ""}, {"fr_CH", ""},
                                      {"en_UK", ""}, {"en_US", ""}};

    QTest::newRow("returns language fallback for empty country text de_DE") << mapEmptyCountry << "de_DE" << "German fallback";
    QTest::newRow("returns language fallback for empty country text de_DE") << mapEmptyCountry << "de_CH" << "German fallback";
    QTest::newRow("returns language fallback for empty country text en_UK") << mapEmptyCountry << "en_UK" << "English fallback";
    QTest::newRow("returns language fallback for empty country text en_US") << mapEmptyCountry << "en_US" << "English fallback";
    QTest::newRow("returns language fallback for empty country text fr_FR") << mapEmptyCountry << "fr_FR" << "French fallback";
    QTest::newRow("returns language fallback for empty country text fr_CH") << mapEmptyCountry << "fr_CH" << "French fallback";

    // if a language fallback text is empty, try a country specific fallback  (alphabetically ordered)
    QVariantMap mapCountryFallback = {{"en", "English fallback"}, {"de", ""}, {"fr", ""},
                         {"de_DE", "German"}, {"de_CH", ""}, {"fr_CH", "French"},
                         {"it_IT", ""}};

    QTest::newRow("language fallback for empty country text de") << mapCountryFallback << "de" << "German";
    QTest::newRow("language fallback for empty country text de_CH") << mapCountryFallback << "de_CH" << "German";
    QTest::newRow("direct match for empty country text de_DE") << mapCountryFallback << "de_DE" << "German";
    QTest::newRow("language fallback for empty country text fr") << mapCountryFallback << "fr" << "French";
    QTest::newRow("direct match for empty country text") << mapCountryFallback << "fr_FR" << "French";
    QTest::newRow("direct match for empty country text") << mapCountryFallback << "fr_CH" << "French";
    QTest::newRow("Country fallback for missing fr_BE returns default French country") << mapCountryFallback << "fr_BE" << "French";
    QTest::newRow("language fallback for empty country text it_IT") << mapCountryFallback << "it_IT" << "English fallback";

    // multiple entries with empty texts: first non-empty text needs to be returned in fallback logic
    QVariantMap mapFF = {{"en", ""}, {"de", ""}, {"fr", ""},
                         {"de_DE", "German"}, {"de_CH", ""}, {"fr_CH", ""},
                         {"it_IT", ""}};

    QTest::newRow("Final fallback for empty en returns first non-empty match") << mapFF << "en" << "German";
    QTest::newRow("Final fallback for empty it_IT returns first non-empty match") << mapFF << "it_IT" << "German";
    QTest::newRow("Final fallback for missing it returns first non-empty match") << mapFF << "it" << "German";
    QTest::newRow("Final fallback for empty fr_CH returns first non-empty match") << mapFF << "fr_CH" << "German";
    QTest::newRow("Final fallback for empty fr returns first non-empty match") << mapFF << "fr" << "German";
    QTest::newRow("Final fallback for missing sk returns first non-empty match") << mapFF << "sk" << "German";
}

void testCommon::getLanguageString() {
    QFETCH(QVariantMap, input);
    QFETCH(QString, locale);
    QFETCH(QString, expectedresult);

    QString result = uc::Util::getLanguageString(input, locale);

    QCOMPARE(result, expectedresult);
}

QTEST_APPLESS_MAIN(testCommon)

#include "test_util.moc"
