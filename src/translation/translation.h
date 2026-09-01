// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDirIterator>
#include <QGuiApplication>
#include <QLocale>
#include <QObject>
#include <QQmlEngine>
#include <QTimeZone>
#include <QTranslator>
#include <QVariant>

#include "../core/core.h"

namespace uc {
namespace ui {

class Translation : public QObject {
    Q_OBJECT

 public:
    explicit Translation(QQmlEngine* engine, core::Api* core, QObject* parent = nullptr);
    virtual ~Translation();

    /**
     * @brief load a translation and translate the UI
     * @param countryCode: en_US for example, available from m_translations
     */
    void loadTranslation(const QString& countryCode);

    /**
     * @brief reads the qml resource and stores the avaialble translations
     * @return
     */
    bool loadTranslations();

    /**
     * @brief returns a list of available translations
     * @return list of available languages, eg.: en_US
     */
    static QStringList getTranslations() { return m_translations; }

    /**
     * @brief gets the language name in English
     * @param countryCode: en_US for example, available from m_translations
     * @return language name in English
     */
    static QString getLanguageName(const QString& countryCode);

    /**
     * @brief gets the language name in the native language
     * @param countryCode: en_US for example, available from m_translations
     * @return language name in native language
     */
    static QString getNativeLanguageName(const QString& countryCode);

    /**
     * @brief gets the two character language code
     * @param countryCode: en_US for example, available from m_translations
     * @return bcp47 language code
     */
    static QString getLanguageCode(const QString& countryCode);

    /**
     * @brief gets the country name in English
     * @param countryCode: en_US for example, available from m_translations
     * @return country name in English
     */
    static QString getCountryName(const QString& countryCode);

    /**
     * @brief gets the countr name in the native language
     * @param countryCode: en_US for example, available from m_translations
     * @return country name in native language
     */
    static QString getNativeCountryName(const QString& countryCode);

    /**
     * @brief gets the IANA timezone ids of a country
     * @param countryCode: ISO 3166-1 alpha-2 country code, e.g. CH
     * @return IANA timezone ids of the country, e.g. Europe/Zurich
     */
    static QStringList getTimeZones(const QString& countryCode);

    /**
     * @brief gets the countries where a given language is spoken, from the CLDR data bundled with Qt
     * @param languageCode: language code with optional country suffix, e.g. de or de_DE
     * @return ISO 3166-1 alpha-2 country codes, most likely country first
     */
    static QStringList getCountriesForLanguage(const QString& languageCode);

    /**
     * @brief gets the most likely country for a language (CLDR likely subtags)
     * @param languageCode: language code with optional country suffix, e.g. de or de_DE
     * @return ISO 3166-1 alpha-2 country code, e.g. DE, or an empty string
     */
    static QString getLikelyCountry(const QString& languageCode);

    /**
     * @brief gets display information for the timezones of a country, sorted east to west
     * @param countryCode: ISO 3166-1 alpha-2 country code, e.g. US
     * @return list of maps { id, city, offset, offsetLabel, zoneName }, offsetLabel is the
     *         standard-time offset (a factory-new device has no synchronized clock, so
     *         DST-dependent values would be unreliable)
     */
    static QVariantList getTimeZoneInfos(const QString& countryCode);

    /**
     * @brief gets display information for all available timezones, sorted by id
     * @return list of maps { id, city, offset, offsetLabel, zoneName }
     */
    static QVariantList getAllTimeZoneInfos();

 public slots:
    void onLanguageChanged(QString language);
    void onReqGetLocalizationLanguages(int reqId);

 private:
    static QLocale::Country countryFromCode(const QString& countryCode);
    static QVariantList     toTimeZoneInfos(const QList<QByteArray>& ids);

    QQmlEngine* m_engine;
    core::Api*  m_core;

    QTranslator*       m_translator;
    static QStringList m_translations;

    QString m_CountryCode;
};

}  // namespace ui
}  // namespace uc
