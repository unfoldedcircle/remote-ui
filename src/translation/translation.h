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

    static QStringList getTimeZones(const QString& countryCode);

 public slots:
    void onLanguageChanged(QString language);

 private:
    QQmlEngine* m_engine;
    core::Api*  m_core;

    QTranslator*       m_translator;
    static QStringList m_translations;

    QString m_CountryCode;
};

}  // namespace ui
}  // namespace uc
