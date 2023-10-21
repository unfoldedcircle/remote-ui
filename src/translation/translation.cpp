// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "translation.h"

#include "../logging.h"

namespace uc {
namespace ui {

QStringList Translation::m_translations;

Translation::Translation(QQmlEngine* engine, core::Api* core, QObject* parent)
    : QObject(parent), m_engine(engine), m_core(core) {
    m_translator = new QTranslator(this);

    if (!loadTranslations()) {
        qCWarning(lcI18n()) << "Cannot load available translations";
    }
}

Translation::~Translation() {}

void Translation::loadTranslation(const QString& countryCode) {
    if (!qGuiApp->removeTranslator(m_translator)) {
        qCWarning(lcI18n()) << "Failed to remove translation";
    }

    if (countryCode == "en_US") {
        m_engine->retranslate();
        return;
    }

    if (!m_translator->load(":/translations/" + countryCode)) {
        qCWarning(lcI18n()) << "Couldn't load transaltion:" << countryCode << getLanguageName(countryCode);
        return;
    }

    if (qGuiApp->installTranslator(m_translator)) {
        qCDebug(lcI18n()) << "Installed translation:" << countryCode << getLanguageName(countryCode);
        m_engine->retranslate();
        m_CountryCode = countryCode;
    } else {
        qCWarning(lcI18n()) << "Failed to install translation";
    }
}

bool Translation::loadTranslations() {
    m_translations.clear();

    QDirIterator it(":/translations", QDirIterator::Subdirectories);

    while (it.hasNext()) {
        auto  fileName = it.next();
        QFile file(fileName);

        m_translations.append(QFileInfo(file).fileName().section(".", 0, 0));
        qCDebug(lcI18n()) << "Country code added:" << m_translations.last();
    }

    return true;
}

QString Translation::getLanguageName(const QString& countryCode) {
    QLocale locale = QLocale(countryCode);
    return locale.languageToString(locale.language());
}

QString Translation::getNativeLanguageName(const QString& countryCode) {
    if (countryCode == "de_CH") {
        return QString("Schwitzertüütsch");
    }

    QLocale locale = QLocale(countryCode);
    QString name = locale.nativeLanguageName();
    name.replace(0, 1, name.at(0).toUpper());
    return name;
}

QString Translation::getLanguageCode(const QString& countryCode) {
    QLocale locale = QLocale(countryCode);
    return locale.bcp47Name();
}

QString Translation::getCountryName(const QString& countryCode) {
    //    QLocale locale = QLocale(countryCode);
    //    return locale.countryToString(locale.country());
    QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for (int i = 0; i < locales.count(); i++) {
        QStringList l = locales.value(i).name().split("_");
        if (l.length() > 1 && countryCode.contains(l[1])) {
            return QLocale::countryToString(locales[i].country());
        }
    }

    return QString();
}

QString Translation::getNativeCountryName(const QString& countryCode) {
    QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for (int i = 0; i < locales.count(); i++) {
        QStringList l = locales.value(i).name().split("_");
        if (l.length() > 1 && countryCode.contains(l[1])) {
            return locales[i].nativeCountryName();
        }
    }

    return QString();
}

QStringList Translation::getTimeZones(const QString& countryCode) {
    QLocale locale;

    // first we find the locale for the country code
    // unfortunately qt cannot always create a locale from a country code
    QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for (const auto &loc : locales) {
        QStringList l = loc.name().split("_");
        if (l.length() > 1 && countryCode.contains(l[1])) {
            locale = loc;
            break;
        }
    }

    QStringList list;
    auto        allTimeZones = QTimeZone::availableTimeZoneIds();
    auto        timeZones = QTimeZone::availableTimeZoneIds(locale.country());

    // then we iterate through all timezones and check if there's a match for the locale's country timezones
    // by comparing the current time offset from UTC
    for (const auto &id : allTimeZones) {
        QTimeZone timeZone(id);

        for (const auto &tzId : timeZones) {
            if (timeZone.offsetFromUtc(QDateTime::currentDateTime()) ==
                QTimeZone(tzId).offsetFromUtc(QDateTime::currentDateTime())) {
                list.append(id);
                break;
            }
        }
    }

    return list;
}

void Translation::onLanguageChanged(QString language) {
    loadTranslation(language);
}
}  // namespace ui
}  // namespace uc
