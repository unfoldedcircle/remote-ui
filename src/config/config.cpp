// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config.h"

#include "../logging.h"

namespace uc {

Config *Config::s_instance = nullptr;

Config::Config(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    qmlRegisterSingletonType<Config>("Config", 1, 0, "Config", &Config::qmlInstance);

            // after connected to the api, get the config
    QObject::connect(m_core, &uc::core::Api::connected, this, &uc::Config::onCoreConnected);
    QObject::connect(m_core, &uc::core::Api::configChanged, this, &uc::Config::onConfigChanged);
    QObject::connect(m_core, &uc::core::Api::cfgButtonChanged, this, &uc::Config::onButtonCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgDisplayChanged, this, &uc::Config::onDisplayCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgDeviceChanged, this, &uc::Config::onDeviceCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgHapticChanged, this, &uc::Config::onHapticCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgLocalizationChanged, this, &uc::Config::onLocalizationCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgNetworkChanged, this, &uc::Config::onNetworkCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgPowerSavingChanged, this, &uc::Config::onPowerSavingCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgSoftwareUpdateChanged, this, &uc::Config::onSoftwareUpdateCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgSoundChanged, this, &uc::Config::onSoundCfgChanged);
    QObject::connect(m_core, &uc::core::Api::cfgVoiceControlChanged, this, &uc::Config::onVoiceControlCfgChanged);

    const QString configPath = qgetenv("UC_CONFIG_HOME");
    m_settings = new QSettings(configPath + "/config.ini", QSettings::IniFormat);
}

Config::~Config() {
    s_instance = nullptr;
    m_settings->deleteLater();
}

void Config::setCurrentProfileId(const QString &profileId) {
    if (!m_currentProfile.contains(profileId)) {
        m_currentProfile = profileId;
        // send to core
        emit currentProfileIdChanged();
    }
}

void Config::setLanguage(const QString &language) {
    if (!m_language.contains(language)) {
        int id =
            m_core->setLocalizationCfg(language, getCountry(), getTimezone(), getClock24h(), getUnitSystem().toUpper());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_language = language;
                emit languageChanged(m_language);

                setCountryNameAsSelectedLanguage();
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting language: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setCountry(const QString &country) {
    ui::Translation::getTimeZones(country);

    int id =
        m_core->setLocalizationCfg(getLanguage(), country, getTimezone(), getClock24h(), getUnitSystem().toUpper());

    m_core->onResult(
        id,
        [=]() {
            // success
            m_country = country;
            emit countryChanged(true);

            setCountryNameAsSelectedLanguage();
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error setting country: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
            emit countryChanged(false);
        });
}

void Config::setTimezone(const QString &timezone) {
    int id =
        m_core->setLocalizationCfg(getLanguage(), getCountry(), timezone, getClock24h(), getUnitSystem().toUpper());

    m_core->onResult(
        id,
        [=]() {
            // success
            m_timezone = timezone;
            emit timezoneChanged(true);
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error setting timezone: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
            emit timezoneChanged(false);
        });
}

void Config::setUnitSystem(QString value) {
    UnitSystems unitSystem = Util::convertStringToEnum<UnitSystems>(value);

    if (m_unitSystem != unitSystem) {
        int id = m_core->setLocalizationCfg(getLanguage(), getCountry(), getTimezone(), getClock24h(),
                                            Util::convertEnumToString<UnitSystems>(unitSystem).toUpper());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_unitSystem = unitSystem;
                emit unitSystemChanged(m_unitSystem);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting unit system: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setClock24h(bool value) {
    if (m_clock24h != value) {
        int id =
            m_core->setLocalizationCfg(getLanguage(), getCountry(), getTimezone(), value, getUnitSystem().toUpper());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_clock24h = value;
                emit clock24hChanged(m_clock24h);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting clock: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setDeviceName(const QString &name) {
    int id = m_core->setDeviceCfg(name);

    m_core->onResult(
        id,
        [=]() {
            // success
            m_deviceName = name;
            emit deviceNameChanged(true);
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error setting device name: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
            emit deviceNameChanged(false);
        });
}

void Config::setHapticEnabled(bool enabled) {
    if (m_hapticEnabled != enabled) {
        int id = m_core->setHapticCfg(enabled);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_hapticEnabled = enabled;
                emit hapticEnabledChanged(m_hapticEnabled);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error changing haptic settings: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setMicEnabled(bool enabled) {
    if (m_micEnabled != enabled) {
        int id = m_core->setVoiceControlCfg(enabled, m_voiceAssistantId, m_voiceAssistantProfileId, m_voiceAssistantSpeechResponse);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_micEnabled = enabled;
                emit micEnabledChanged(m_micEnabled);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting microphone config: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setVoiceAssistantId(const QString &entityId)
{
    int id = m_core->setVoiceControlCfg(m_micEnabled, entityId, m_voiceAssistantProfileId, m_voiceAssistantSpeechResponse);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error setting voice assistant config: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
        });
}

void Config::setVoiceAssistantProfileId(const QString &profileId)
{
    int id = m_core->setVoiceControlCfg(m_micEnabled, m_voiceAssistantId, profileId, m_voiceAssistantSpeechResponse);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error setting voice assistant profile config: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
        });
}

void Config::setVoiceAssistantSpeechResponse(bool value)
{
    if (m_voiceAssistantSpeechResponse != value) {
        int id = m_core->setVoiceControlCfg(m_micEnabled, m_voiceAssistantId, m_voiceAssistantProfileId, value);

        m_core->onResult(
            id,
            [=]() {
                // success
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting voice assistant profile config: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setSoundEnabled(bool enabled) {
    if (m_soundEnabled != enabled) {
        int id = m_core->setSoundCfg(enabled, getSoundVolume());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_soundEnabled = enabled;
                emit soundEnabledChanged(m_soundEnabled);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting sound config: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setSoundVolume(int volume) {
    if (m_soundVolume != volume) {
        int id = m_core->setSoundCfg(getSoundEnabled(), volume);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_soundVolume = volume;
                emit soundVolumeChanged(m_soundVolume);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting sound volume: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setDisplayAutoBrightness(bool enabled) {
    if (m_displayAutoBrightness != enabled) {
        int id = m_core->setDisplayCfg(getDisplayBrightness(), enabled);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_displayAutoBrightness = enabled;
                emit displayAutoBrightnessChanged(m_displayAutoBrightness);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting display config: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setDisplayBrightness(int brightness) {
    if (m_displayBrightness != brightness) {
        int id = m_core->setDisplayCfg(brightness, getDisplayAutoBrightness());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_displayBrightness = brightness;
                emit displayBrightnessChanged(m_displayBrightness);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting display config: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setButtonAutoBirghtness(bool enabled) {
    if (m_buttonAutoBrightness != enabled) {
        int id = m_core->setButtonCfg(getButtonBrightness(), enabled);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_buttonAutoBrightness = enabled;
                emit buttonAutoBirghtnessChanged(m_buttonAutoBrightness);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting button backlight: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setButtonBrightness(int brightness) {
    if (m_buttonBrightness != brightness) {
        int id = m_core->setButtonCfg(brightness, getButtonAutoBirghtness());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_buttonBrightness = brightness;
                emit buttonBrightnessChanged(m_buttonBrightness);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting button backlight: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

bool Config::getEntityButtonFuncInverted() {
    return m_settings->value("ui/buttonFunc", false).toBool();
}

void Config::setEntityButtonFuncInverted(bool value) {
    m_settings->setValue("ui/buttonFunc", value);
    emit entityButtonFuncInvertedChanged();
}

bool Config::getShowBatteryPercentage()
{
    return m_settings->value("ui/batteryPercent", false).toBool();
}

void Config::setShowBatteryPercentage(bool value)
{
    m_settings->setValue("ui/batteryPercent", value);
    emit showBatteryPercentageChanged();
}

bool Config::getEnableActivityBar()
{
    return m_settings->value("ui/activityBar", true).toBool();
}

void Config::setEnableActivityBar(bool value)
{
    m_settings->setValue("ui/activityBar", value);
    emit enableActivityBarChanged();
}

bool Config::getFillMediaArtwork()
{
    return m_settings->value("ui/fillMediaArtwork", false).toBool();
}

void Config::setFillMediaArtwork(bool value)
{
    m_settings->setValue("ui/fillMediaArtwork", value);
    emit fillMediaArtworkChanged();
}

int Config::getResumeTimeoutWindowSec()
{
    return m_settings->value("ui/resumeTimeoutWindow", 2).toInt();
}

void Config::setResumeTimeoutWindowSec(int value)
{
    m_settings->setValue("ui/resumeTimeoutWindow", value);
    emit resumeTimeoutWindowSecChanged(value);
}

void Config::setWakeupSensitivity(Config::WakeupSensitivities sensitivity) {
    if (m_wakeupSensitivity != sensitivity) {
        int id = m_core->setPowerSavingCfg(sensitivity, getDisplayTimeout(), getSleepTimeout());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_wakeupSensitivity = sensitivity;
                emit wakeupSensitivityChanged(m_wakeupSensitivity);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting wakeup sensitivity: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setSleepTimeout(int timeout) {
    if (m_sleepTimeout != timeout) {
        int id = m_core->setPowerSavingCfg(getWakeupSensitivity(), getDisplayTimeout(), timeout);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_sleepTimeout = timeout;
                emit sleepTimeoutChanged(m_sleepTimeout);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting sleep timeout: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setDisplayTimeout(int timeout) {
    if (m_displayTimeout != timeout) {
        int id = m_core->setPowerSavingCfg(getWakeupSensitivity(), timeout, getSleepTimeout());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_displayTimeout = timeout;
                emit displayTimeoutChanged(m_displayTimeout);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting display sleep timeout: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setAutoUpdate(bool enabled) {
    if (m_autoUpdate != enabled) {
        int id = m_core->setSoftwareUpdateCfg(getCheckForUpdates(), enabled);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_autoUpdate = enabled;
                emit autoUpdateChanged(m_autoUpdate);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting update config: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setCheckForUpdates(bool enabled) {
    if (m_checkForUpdates != enabled) {
        int id = m_core->setSoftwareUpdateCfg(enabled, getAutoUpdate());

        m_core->onResult(
            id,
            [=]() {
                // success
                m_checkForUpdates = enabled;
                emit checkForUpdatesChanged(m_checkForUpdates);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting update config: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setBluetoothEnabled(bool enabled) {
    if (m_bluetoothEnabled != enabled) {
        int id = m_core->setNetworkCfg(enabled, m_wifiEnabled, m_wowlanEnabled, m_band, m_scanIntervalSec);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_bluetoothEnabled = enabled;
                emit bluetoothEnabledChanged(m_bluetoothEnabled);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting Bluetooth: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setWifiEnabled(bool enabled) {
    if (m_wifiEnabled != enabled) {
        int id = m_core->setNetworkCfg(m_bluetoothEnabled, enabled, m_wowlanEnabled, m_band, m_scanIntervalSec);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_wifiEnabled = enabled;
                emit wifiEnabledChanged(m_wifiEnabled);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting WiFi: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setWowlanEnabled(bool enabled)
{
    if (m_wowlanEnabled != enabled) {
        int id = m_core->setNetworkCfg(m_bluetoothEnabled, m_wifiEnabled, enabled, m_band, m_scanIntervalSec);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_wowlanEnabled = enabled;
                emit wowlanChanged(m_wowlanEnabled);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting Wowlan: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

void Config::setWifiBand(QString value)
{
    int id = m_core->setNetworkCfg(m_bluetoothEnabled, m_wifiEnabled, m_wowlanEnabled, value, m_scanIntervalSec);

    m_core->onResult(
        id,
        [=]() {
            // success
            m_band = value;
            emit wifiBandChanged(m_band);
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error setting Wifi band: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
        });
}

void Config::setScanIntervalSec(int value)
{
    if (m_scanIntervalSec != value) {
        int id = m_core->setNetworkCfg(m_bluetoothEnabled, m_wifiEnabled, m_wowlanEnabled, m_band, value);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_scanIntervalSec = value;
                emit scanIntervalSecChanged(m_scanIntervalSec);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error setting Wifi scan interval: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

QString Config::getLanguageAsNative(const QString language) {
    return ui::Translation::getNativeLanguageName(language);
}

QString Config::getLanguageAsNative() {
    return ui::Translation::getNativeLanguageName(m_language);
}

QString Config::getCountryAsNative() {
    return ui::Translation::getNativeCountryName(m_country);
}

QString Config::getCountryAsNative(const QString country) {
    return ui::Translation::getNativeCountryName(country);
}

void Config::getCountryList() {
    int id = m_core->getLocalizationCountries();

    m_core->onResponseWithErrorResult(
        id, &core::Api::respLocalizationCountries,
        [=](QVariantList list) {
            // success
            m_countryList = list;
            emit countryListChanged(list);

            setCountryNameAsSelectedLanguage();
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error getting country list: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
        });
}

void Config::getTimeZones() {
    int id = m_core->getTimeZoneNames();

    m_core->onResponseWithErrorResult(
        id, &core::Api::respTimeZoneNames,
        [=](QStringList list) {
            // success
            emit timeZoneListChanged(list);
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error getting timezones: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
        });
}

void Config::getTimeZones(const QString country) {
    QStringList list = ui::Translation::getTimeZones(country);
    emit        timeZoneListChanged(list);
}

void Config::generateNewWebConfigPin() {
    QString webConfiguratorPin = generateRandomPin();

    int id = m_core->setApiAccess(true, webConfiguratorPin);

    m_core->onResult(
        id,
        [=]() {
            // success
            m_webConfiguratorPin = webConfiguratorPin;
            emit webConfiguratorPinChanged(m_webConfiguratorPin);
            m_webConfiguratorEnabled = true;
            emit webConfiguratorEnabledChanged(m_webConfiguratorEnabled);
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcConfig()) << "Error generating new web config pin:" << code << message;
        });
}

void Config::setAdminPin(const QString &pin) {
    int id = m_core->setProfileCfg(pin);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcConfig()) << "Successfully set admin pin";
            emit adminPinSet(true);
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error white setting admin pin: " + message;
            qCWarning(lcConfig()) << code << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
            emit adminPinSet(false);
        });
}

void Config::getApiAccess() {
    int id = m_core->getApiAccess();

    m_core->onResponse(
        id, &core::Api::respApiAccess,
        [=](core::ApiAccess apiAccess) {
            // success
            m_webConfiguratorEnabled = apiAccess.enabled;
            emit webConfiguratorEnabledChanged(m_webConfiguratorEnabled);
        },
        [=](core::ApiAccess apiAccess) {
            // fail
            Q_UNUSED(apiAccess)
            QString errorMsg = "Error enabling the web configurator";
            qCWarning(lcConfig()) << errorMsg;
            ui::Notification::createNotification(errorMsg, true);
        });
}

void Config::getActiveProfile() {
    qCDebug(lcConfig()) << "Get active profile";
    int id = m_core->getActiveProfile();

    m_core->onResponseWithErrorResult(
        id, &core::Api::respProfile,
        [=](core::Profile profile) {
            // success
            m_currentProfile = profile.id;
            emit currentProfileIdChanged();
        },
        [=](int code, QString message) {
            // fail
            if (m_currentProfileLoadTries == 2) {
                m_currentProfileLoadTries = 0;
                emit noCurrentProfileFound();
            } else {
                m_currentProfileLoadTries++;
                QTimer::singleShot(500 * m_currentProfileLoadTries, [=] { getActiveProfile(); });
                qCWarning(lcUi()) << "Error getting active profile:" << code << message;
            }
        });
}

void Config::setWebConfiguratorEnabled(bool value) {
    if (m_webConfiguratorEnabled != value) {
        int     id;
        QString webConfiguratorPin = generateRandomPin();

        if (value) {
            id = m_core->setApiAccess(value, webConfiguratorPin);

        } else {
            id = m_core->setApiAccess(value);
        }

        m_core->onResult(
            id,
            [=]() {
                // success
                m_webConfiguratorPin = webConfiguratorPin;
                emit webConfiguratorPinChanged(m_webConfiguratorPin);

                m_webConfiguratorEnabled = value;
                emit webConfiguratorEnabledChanged(m_webConfiguratorEnabled);
            },
            [=](int code, QString message) {
                // fail
                QString errorMsg = "Error enabling the web configurator: " + message;
                qCWarning(lcConfig()) << code << errorMsg;
                ui::Notification::createNotification(errorMsg, true);
            });
    }
}

QStringList Config::getTranslations() {
    return ui::Translation::getTranslations();
}

QString Config::getLanguageCodeFromCountry(const QString &country) {
    return ui::Translation::getLanguageCode(country);
}

QString Config::getCountry(const QString country) {
    return ui::Translation::getCountryName(country);
}

QObject *Config::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void Config::getConfig() {
    m_core->getConfig();
}

void Config::onCoreConnected() {
    getConfig();
    getApiAccess();
    getActiveProfile();
}

void Config::onConfigChanged(int reqId, int code, core::Config config) {
    Q_UNUSED(reqId)

    if (code != 200 && code != 201) {
        ui::Notification::createNotification(tr("Error while loading configuration. Trying again."), true);
        QTimer::singleShot(2000, [=] { getConfig(); });
        return;
    }

    onButtonCfgChanged(config.buttonCfg);
    onDisplayCfgChanged(config.displayCfg);
    onDeviceCfgChanged(config.deviceCfg);
    onHapticCfgChanged(config.hapticCfg);
    onLocalizationCfgChanged(config.localizationCfg);
    onNetworkCfgChanged(config.networkCfg);
    onPowerSavingCfgChanged(config.powerSavingCfg);
    onSoftwareUpdateCfgChanged(config.softwareUpdateCfg);
    onSoundCfgChanged(config.soundCfg);
    onVoiceControlCfgChanged(config.voiceControlCfg);

    qCDebug(lcConfig()) << "Config loaded";
}

void Config::onButtonCfgChanged(core::cfgButton cfgButton) {
    m_buttonBrightness = cfgButton.brightness;
    emit buttonBrightnessChanged(m_buttonBrightness);

    m_buttonAutoBrightness = cfgButton.autoBrightness;
    emit buttonAutoBirghtnessChanged(m_buttonAutoBrightness);
}

void Config::onDisplayCfgChanged(core::cfgDisplay cfgDisplay) {
    m_displayAutoBrightness = cfgDisplay.autoBrightness;
    emit displayAutoBrightnessChanged(m_displayAutoBrightness);

    m_displayBrightness = cfgDisplay.brightness;
    emit displayBrightnessChanged(m_displayBrightness);
}

void Config::onDeviceCfgChanged(core::cfgDevice cfgDevice) {
    m_deviceName = cfgDevice.name;
    emit deviceNameChanged(true);
}

void Config::onHapticCfgChanged(core::cfgHaptic cfgHaptic) {
    m_hapticEnabled = cfgHaptic.enabled;
    emit hapticEnabledChanged(m_hapticEnabled);
}

void Config::onLocalizationCfgChanged(core::cfgLocalization cfgLocalization) {
    if (!m_language.contains(cfgLocalization.languageCode)) {
        m_language = cfgLocalization.languageCode;
        emit languageChanged(m_language);

        setCountryNameAsSelectedLanguage();
    }

    if (!m_country.contains(cfgLocalization.countryCode)) {
        m_country = cfgLocalization.countryCode;
        emit countryChanged(true);

        setCountryNameAsSelectedLanguage();
    }

    if (!m_timezone.contains(cfgLocalization.timezone)) {
        m_timezone = cfgLocalization.timezone;
        emit timezoneChanged(true);
    }

    auto newUnitSystem = Util::convertStringToEnum<UnitSystems>(Util::FirstToUpper(cfgLocalization.measurementUnit));

    if (m_unitSystem != newUnitSystem) {
        m_unitSystem = newUnitSystem;
        emit unitSystemChanged(m_unitSystem);
    }

    if (m_clock24h != cfgLocalization.timeFormat24h) {
        m_clock24h = cfgLocalization.timeFormat24h;
        emit clock24hChanged(m_clock24h);
    }
}

void Config::onNetworkCfgChanged(core::cfgNetwork cfgNetwork) {
    m_bluetoothEnabled = cfgNetwork.bluetoothEnabled;
    emit bluetoothEnabledChanged(m_bluetoothEnabled);

    m_wifiEnabled = cfgNetwork.wifiEnabled;
    emit wifiEnabledChanged(m_wifiEnabled);

    m_wowlanEnabled = cfgNetwork.wifi.wowlan;
    emit wowlanChanged(m_wowlanEnabled);

    m_bands = cfgNetwork.wifi.bands;
    emit wifiBandsChanged(m_bands);

    m_band = cfgNetwork.wifi.band;
    emit wifiBandChanged(m_band);

    m_scanIntervalSec = cfgNetwork.wifi.scanIntervalSec;
    emit scanIntervalSecChanged(m_scanIntervalSec);

    m_bluetoothMac = cfgNetwork.bluetoothMac;
}

void Config::onPowerSavingCfgChanged(core::cfgPowerSaving cfgPowerSaving) {
    m_sleepTimeout = cfgPowerSaving.standbySec;
    emit sleepTimeoutChanged(m_sleepTimeout);

    m_displayTimeout = cfgPowerSaving.displayOffSec;
    emit displayTimeoutChanged(m_displayTimeout);

    m_wakeupSensitivity = static_cast<WakeupSensitivities>(cfgPowerSaving.wakeupSensitivity);
    emit wakeupSensitivityChanged(m_wakeupSensitivity);
}

void Config::onSoftwareUpdateCfgChanged(core::cfgSoftwareUpdate cfgSoftwareUpdate) {
    m_autoUpdate = cfgSoftwareUpdate.autoUpdate;
    emit autoUpdateChanged(m_autoUpdate);

    m_checkForUpdates = cfgSoftwareUpdate.checkForUpdates;
    emit checkForUpdatesChanged(m_checkForUpdates);

    if (!cfgSoftwareUpdate.otaWindowStart.isEmpty()) {
        m_otaWindowStart = cfgSoftwareUpdate.otaWindowStart;
        emit otaWindowStartChanged(m_otaWindowStart);
    }

    if (!cfgSoftwareUpdate.otaWindowEnd.isEmpty()) {
        m_otaWindowEnd = cfgSoftwareUpdate.otaWindowEnd;
        emit otaWindowEndChanged(m_otaWindowEnd);
    }

    m_updateChannel = Util::convertEnumToString(cfgSoftwareUpdate.channel);
    emit updateChannelChanged(m_updateChannel);
}

void Config::onSoundCfgChanged(core::cfgSound cfgSound) {
    m_soundEnabled = cfgSound.enabled;
    emit soundEnabledChanged(m_soundEnabled);

    m_soundVolume = cfgSound.volume;
    emit soundVolumeChanged(m_soundVolume);
}

void Config::onVoiceControlCfgChanged(core::cfgVoiceControl cfgVoiceControl) {
    m_micEnabled = cfgVoiceControl.microphoneEnabled;
    emit micEnabledChanged(m_micEnabled);

    m_voiceAssistantId = cfgVoiceControl.voiceAsssistant.active.entity_id;
    emit voiceAssistantIdChanged(m_voiceAssistantId);

    m_voiceAssistantProfileId = cfgVoiceControl.voiceAsssistant.profile_id;
    emit voiceAssistantProfileIdChanged(m_voiceAssistantProfileId);

    m_voiceAssistantSpeechResponse = cfgVoiceControl.voiceAsssistant.speechResponse;
    emit voiceAssistantSpeechResponseChanged(m_voiceAssistantSpeechResponse);
}

QString Config::generateRandomPin() {
    int     pinNumber = QRandomGenerator::global()->bounded(0, 9999);
    QString pin = QString::number(pinNumber);

    while (3 - pin.length() == 0) {
        pin.prepend("0");
    }

    return pin;
}

void Config::setCountryNameAsSelectedLanguage() {
    if (m_countryList.isEmpty()) {
        m_countryName = getCountryAsNative();
        emit countryNameChanged(m_countryName);
        return;
    }

    QStringList tmp = m_language.split("_");
    QString     language;
    if (tmp.length() > 0) {
        language = tmp[0];
    }

    for (const auto &item : qAsConst(m_countryList)) {
        QVariantMap country = item.toMap();

        if (country.value("code").toString() == m_country) {
            if (country.contains("name_" + language)) {
                m_countryName = country.value("name_" + language).toString();
            } else {
                m_countryName = country.value("name_en").toString();
            }

            qCDebug(lcConfig()) << "Country name as selected language:" << m_countryName;
            emit countryNameChanged(m_countryName);
        }
    }
}

}  // namespace uc
