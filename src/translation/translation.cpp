// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "translation.h"

#include <QHash>
#include <algorithm>

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

    QObject::connect(m_core, &core::Api::reqGetLocalizationLanguages, this,
                     &Translation::onReqGetLocalizationLanguages);
}

Translation::~Translation() {}

void Translation::loadTranslation(const QString& countryCode) {
    // removeTranslator() only fails if the translator isn't installed, which is the case before the first load
    qGuiApp->removeTranslator(m_translator);

    if (countryCode == "en_US") {
        m_engine->retranslate();
        return;
    }

    if (!m_translator->load(":/translations/" + countryCode)) {
        qCWarning(lcI18n()) << "Couldn't load translation:" << countryCode << getLanguageName(countryCode);
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
    QLocale::Country country = countryFromCode(countryCode);
    if (country == QLocale::AnyCountry) {
        return QString();
    }

    return QLocale::countryToString(country);
}

QString Translation::getNativeCountryName(const QString& countryCode) {
    QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for (int i = 0; i < locales.count(); i++) {
        QStringList l = locales.value(i).name().split("_");
        if (l.length() > 1 && l[1].compare(countryCode, Qt::CaseInsensitive) == 0) {
            return locales[i].nativeCountryName();
        }
    }

    return QString();
}

QStringList Translation::getTimeZones(const QString& countryCode) {
    QStringList list;

    const auto timeZones = QTimeZone::availableTimeZoneIds(countryFromCode(countryCode));
    for (const auto& id : timeZones) {
        list.append(QString::fromUtf8(id));
    }

    return list;
}

QStringList Translation::getCountriesForLanguage(const QString& languageCode) {
    QStringList list;

    QLocale::Language language = QLocale(languageCode).language();
    if (language == QLocale::C || language == QLocale::AnyLanguage) {
        return list;
    }

    QString likely = getLikelyCountry(languageCode);
    if (!likely.isEmpty()) {
        list.append(likely);
    }

    // Qt's CLDR data lists every territory where a language has official or de-facto status,
    // but without any ranking. For widely spoken languages that is a screenful of mostly small
    // territories in no meaningful order (English: ~100, starting with Anguilla and American
    // Samoa), so those get a curated list of the main countries instead. Languages with a
    // naturally small footprint (German, Dutch, Danish, ...) use the CLDR list as-is.
    // Extend the table when adding a translation for another widely spoken language.
    static const QHash<QString, QStringList> s_curated = {
        {"en", {"US", "GB", "CA", "AU", "NZ", "IE"}},
        {"fr", {"FR", "BE", "CH", "CA", "LU", "MC"}},
        {"es", {"ES", "MX", "AR", "CO", "CL", "PE", "US"}},
        {"pt", {"PT", "BR", "AO", "MZ", "CV"}},
    };

    QString lang = languageCode.split("_").value(0).toLower();
    if (s_curated.contains(lang)) {
        for (const auto& code : s_curated.value(lang)) {
            if (!list.contains(code)) {
                list.append(code);
            }
        }
        return list;
    }

    const QList<QLocale> locales = QLocale::matchingLocales(language, QLocale::AnyScript, QLocale::AnyCountry);
    for (const auto& loc : locales) {
        QStringList l = loc.name().split("_");
        if (l.length() > 1 && !list.contains(l[1])) {
            list.append(l[1]);
        }
    }

    // safety net for a widely spoken language missing from the curated table: better only the
    // likely country than a screenful of unranked territories
    if (list.length() > 10) {
        return QStringList(list.first());
    }

    return list;
}

QString Translation::getLikelyCountry(const QString& languageCode) {
    // the app's language codes carry a country suffix (de_CH): that is the best guess
    QStringList parts = languageCode.split("_");
    if (parts.length() > 1 && parts[1].length() == 2) {
        return parts[1].toUpper();
    }

    // bare language code: CLDR likely subtags fill in the most likely country
    QLocale locale(QLocale(languageCode).language());
    parts = locale.name().split("_");
    return parts.length() > 1 ? parts[1] : QString();
}

QVariantList Translation::getTimeZoneInfos(const QString& countryCode) {
    QVariantList list = toTimeZoneInfos(QTimeZone::availableTimeZoneIds(countryFromCode(countryCode)));

    // east to west within a country, cities alphabetical within the same offset
    std::sort(list.begin(), list.end(), [](const QVariant& a, const QVariant& b) {
        QVariantMap mapA = a.toMap();
        QVariantMap mapB = b.toMap();
        int         offsetA = mapA.value("offset").toInt();
        int         offsetB = mapB.value("offset").toInt();
        if (offsetA != offsetB) {
            return offsetA > offsetB;
        }
        return mapA.value("city").toString() < mapB.value("city").toString();
    });

    return list;
}

QVariantList Translation::getAllTimeZoneInfos() {
    QVariantList list = toTimeZoneInfos(QTimeZone::availableTimeZoneIds());

    // sorted by id: the IANA area prefix (Africa/, America/, ...) groups the world list by region
    std::sort(list.begin(), list.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("id").toString() < b.toMap().value("id").toString();
    });

    return list;
}

QLocale::Country Translation::countryFromCode(const QString& countryCode) {
    // unfortunately qt cannot always create a locale from a country code
    const QList<QLocale> locales =
        QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for (const auto& loc : locales) {
        QStringList l = loc.name().split("_");
        if (l.length() > 1 && l[1].compare(countryCode, Qt::CaseInsensitive) == 0) {
            return loc.country();
        }
    }

    return QLocale::AnyCountry;
}

QVariantList Translation::toTimeZoneInfos(const QList<QByteArray>& ids) {
    QVariantList list;

    const QDateTime atTime = QDateTime::currentDateTimeUtc();

    for (const auto& id : ids) {
        QTimeZone timeZone(id);
        if (!timeZone.isValid()) {
            continue;
        }

        QString zoneId = QString::fromUtf8(id);
        QString city = zoneId.section('/', -1);
        city.replace('_', ' ');

        // standard-time offset on purpose: a factory-new device has no synchronized clock,
        // so anything depending on the current date (DST state, wall time) may be wrong
        int     offset = timeZone.standardTimeOffset(atTime);
        int     absOffset = qAbs(offset);
        QString offsetLabel = QString("GMT%1%2:%3")
                                  .arg(offset < 0 ? "-" : "+")
                                  .arg(absOffset / 3600, 2, 10, QChar('0'))
                                  .arg((absOffset % 3600) / 60, 2, 10, QChar('0'));

        // without ICU/CLDR data the display name is just another offset string: no added value
        QString zoneName = timeZone.displayName(QTimeZone::GenericTime, QTimeZone::LongName);
        if (zoneName.startsWith("UTC") || zoneName.startsWith("GMT") || zoneName == zoneId) {
            zoneName.clear();
        }

        QVariantMap map;
        map.insert("id", zoneId);
        map.insert("city", city);
        map.insert("offset", offset);
        map.insert("offsetLabel", offsetLabel);
        map.insert("zoneName", zoneName);
        list.append(map);
    }

    return list;
}

void Translation::onLanguageChanged(QString language) {
    loadTranslation(language);
}

void Translation::onReqGetLocalizationLanguages(int reqId) {
    qCDebug(lcI18n()) << "Request from core for localization languages" << reqId;

    QVariantList list;

    for (const auto& item : m_translations) {
        QVariantMap map;
        map.insert("name", getNativeLanguageName(item));
        map.insert("code", item);
        list.append(map);
    }

    m_core->setLocalizationLanguages(reqId, APP_VERSION, list);
}
}  // namespace ui
}  // namespace uc
