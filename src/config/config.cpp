// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config.h"

#include <algorithm>

#include "../logging.h"

namespace uc {

Config* Config::s_instance = nullptr;

Config::Config(core::Api* core, QObject* parent) : QObject(parent), m_core(core) {
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
    // The destructor runs after the event loop has ended, deleteLater() would never delete it. Deleting the
    // settings object writes the values that were changed last.
    delete m_settings;
}

void Config::setCurrentProfileId(const QString& profileId) {
    if (m_currentProfile != profileId) {
        m_currentProfile = profileId;
        // send to core
        emit currentProfileIdChanged();
    }
}

void Config::setLanguage(const QString& language) {
    if (language.isEmpty()) {
        qCWarning(lcConfig()) << "Ignoring empty language code";
        return;
    }

    if (m_language != language) {
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
                qCWarning(lcConfig()) << "Error setting language:" << code << message;
                //: Notification: the interface language could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting language: %1").arg(message), true);
            });
    }
}

void Config::setCountry(const QString& country) {
    if (country.isEmpty()) {
        qCWarning(lcConfig()) << "Ignoring empty country code";
        emit countryChanged(false);
        return;
    }

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
            qCWarning(lcConfig()) << "Error setting country:" << code << message;
            //: Notification: the country could not be changed. %1 is the core's error message
            ui::Notification::createNotification(tr("Error setting country: %1").arg(message), true);
            emit countryChanged(false);
        });
}

void Config::setTimezone(const QString& timezone) {
    if (timezone.isEmpty()) {
        qCWarning(lcConfig()) << "Ignoring empty timezone";
        emit timezoneChanged(false);
        return;
    }

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
            qCWarning(lcConfig()) << "Error setting timezone:" << code << message;
            //: Notification: the time zone could not be changed. %1 is the core's error message
            ui::Notification::createNotification(tr("Error setting timezone: %1").arg(message), true);
            emit timezoneChanged(false);
        });
}

void Config::setUnitSystem(QString value) {
    bool        ok = false;
    UnitSystems unitSystem = Util::convertStringToEnum<UnitSystems>(value, &ok);

    // an unknown value would be sent as an empty measurement_unit and rejected by core
    if (!ok) {
        qCWarning(lcConfig()) << "Ignoring unknown unit system:" << value;
        return;
    }

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
                qCWarning(lcConfig()) << "Error setting unit system:" << code << message;
                //: Notification: the unit system could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting unit system: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting clock:" << code << message;
                //: Notification: the 12/24-hour clock could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting clock: %1").arg(message), true);
            });
    }
}

void Config::setDeviceName(const QString& name) {
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
            qCWarning(lcConfig()) << "Error setting device name:" << code << message;
            //: Notification: the name of the remote could not be changed. %1 is the core's error message
            ui::Notification::createNotification(tr("Error setting device name: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error changing haptic settings:" << code << message;
                //: Notification: haptic feedback could not be switched. %1 is the core's error message
                ui::Notification::createNotification(tr("Error changing haptic settings: %1").arg(message), true);
            });
    }
}

void Config::setMicEnabled(bool enabled) {
    if (m_micEnabled != enabled) {
        int id = m_core->setVoiceControlCfg(enabled, m_voiceAssistantId, m_voiceAssistantProfileId,
                                            m_voiceAssistantSpeechResponse);

        m_core->onResult(
            id,
            [=]() {
                // success
                m_micEnabled = enabled;
                emit micEnabledChanged(m_micEnabled);
            },
            [=](int code, QString message) {
                // fail
                qCWarning(lcConfig()) << "Error setting microphone config:" << code << message;
                //: Notification: the microphone could not be switched. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting microphone config: %1").arg(message), true);
            });
    }
}

void Config::setVoiceAssistantId(const QString& entityId) {
    int id =
        m_core->setVoiceControlCfg(m_micEnabled, entityId, m_voiceAssistantProfileId, m_voiceAssistantSpeechResponse);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcConfig()) << "Error setting voice assistant config:" << code << message;
            //: Notification: the voice assistant could not be changed. %1 is the core's error message
            ui::Notification::createNotification(tr("Error setting voice assistant config: %1").arg(message), true);
        });
}

void Config::setVoiceAssistantProfileId(const QString& profileId) {
    int id = m_core->setVoiceControlCfg(m_micEnabled, m_voiceAssistantId, profileId, m_voiceAssistantSpeechResponse);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcConfig()) << "Error setting voice assistant profile config:" << code << message;
            //: Notification: a voice assistant option could not be changed. %1 is the core's error message
            ui::Notification::createNotification(tr("Error setting voice assistant profile config: %1").arg(message),
                                                 true);
        });
}

void Config::setVoiceAssistantSpeechResponse(bool value) {
    if (m_voiceAssistantSpeechResponse != value) {
        int id = m_core->setVoiceControlCfg(m_micEnabled, m_voiceAssistantId, m_voiceAssistantProfileId, value);

        m_core->onResult(
            id,
            [=]() {
                // success
            },
            [=](int code, QString message) {
                // fail
                qCWarning(lcConfig()) << "Error setting voice assistant profile config:" << code << message;
                //: Notification: a voice assistant option could not be changed. %1 is the core's error message
                ui::Notification::createNotification(
                    tr("Error setting voice assistant profile config: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting sound config:" << code << message;
                //: Notification: sound effects could not be switched. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting sound config: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting sound volume:" << code << message;
                //: Notification: the sound effect volume could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting sound volume: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting display config:" << code << message;
                //: Notification: the display brightness could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting display config: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting display config:" << code << message;
                //: Notification: the display brightness could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting display config: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting button backlight:" << code << message;
                //: Notification: the button backlight could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting button backlight: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting button backlight:" << code << message;
                //: Notification: the button backlight could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting button backlight: %1").arg(message), true);
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

bool Config::getShowBatteryPercentage() {
    return m_settings->value("ui/batteryPercent", false).toBool();
}

void Config::setShowBatteryPercentage(bool value) {
    m_settings->setValue("ui/batteryPercent", value);
    emit showBatteryPercentageChanged();
}

bool Config::getShowBatteryEveryWhere() {
    return m_settings->value("ui/batteryEveryWhere", false).toBool();
}

void Config::setShowBatteryEveryWhere(bool value) {
    m_settings->setValue("ui/batteryEveryWhere", value);
    emit showBatteryEveryWhereChanged();
}

bool Config::getEnableActivityBar() {
    return m_settings->value("ui/activityBar", true).toBool();
}

void Config::setEnableActivityBar(bool value) {
    m_settings->setValue("ui/activityBar", value);
    emit enableActivityBarChanged();
}

bool Config::getOpenActivityOnApiStart() {
    return m_settings->value("ui/openActivityOnApiStart", false).toBool();
}

void Config::setOpenActivityOnApiStart(bool value) {
    m_settings->setValue("ui/openActivityOnApiStart", value);
    emit openActivityOnApiStartChanged();
}

bool Config::getFillMediaArtwork() {
    return m_settings->value("ui/fillMediaArtwork", false).toBool();
}

void Config::setFillMediaArtwork(bool value) {
    m_settings->setValue("ui/fillMediaArtwork", value);
    emit fillMediaArtworkChanged();
}

bool Config::getMediaCoverflowDefault() {
    return m_settings->value("ui/mediaCoverflowDefault", false).toBool();
}

void Config::setMediaCoverflowDefault(bool value) {
    m_settings->setValue("ui/mediaCoverflowDefault", value);
    emit mediaCoverflowDefaultChanged();
}

int Config::getResumeTimeoutWindowSec() {
    return m_settings->value("ui/resumeTimeoutWindow", 2).toInt();
}

void Config::setResumeTimeoutWindowSec(int value) {
    m_settings->setValue("ui/resumeTimeoutWindow", value);
    emit resumeTimeoutWindowSecChanged(value);
}

bool Config::getTouchSliderEnabled() {
    return m_settings->value("touchslider/enabled", true).toBool();
}

void Config::setTouchSliderEnabled(bool value) {
    m_settings->setValue("touchslider/enabled", value);
    emit touchSliderEnabledChanged();
}

double Config::getTouchSliderGainVolume() {
    return m_settings->value("touchslider/gainVolume", 0.4).toDouble();
}

void Config::setTouchSliderGainVolume(double value) {
    m_settings->setValue("touchslider/gainVolume", value);
    emit touchSliderGainVolumeChanged();
}

double Config::getTouchSliderGainBrightness() {
    return m_settings->value("touchslider/gainBrightness", 1.2).toDouble();
}

void Config::setTouchSliderGainBrightness(double value) {
    m_settings->setValue("touchslider/gainBrightness", value);
    emit touchSliderGainBrightnessChanged();
}

double Config::getTouchSliderGainPosition() {
    return m_settings->value("touchslider/gainPosition", 1.2).toDouble();
}

void Config::setTouchSliderGainPosition(double value) {
    m_settings->setValue("touchslider/gainPosition", value);
    emit touchSliderGainPositionChanged();
}

double Config::getTouchSliderGainSeek() {
    return m_settings->value("touchslider/gainSeek", 1.0).toDouble();
}

void Config::setTouchSliderGainSeek(double value) {
    m_settings->setValue("touchslider/gainSeek", value);
    emit touchSliderGainSeekChanged();
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
                qCWarning(lcConfig()) << "Error setting wakeup sensitivity:" << code << message;
                //: Notification: the wakeup sensitivity could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting wakeup sensitivity: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting sleep timeout:" << code << message;
                //: Notification: the sleep timeout could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting sleep timeout: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting display sleep timeout:" << code << message;
                //: Notification: the display timeout could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting display sleep timeout: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting update config:" << code << message;
                //: Notification: a software update setting could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting update config: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting update config:" << code << message;
                //: Notification: a software update setting could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting update config: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting Bluetooth:" << code << message;
                //: Notification: Bluetooth could not be switched on or off. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting Bluetooth: %1").arg(message), true);
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
                qCWarning(lcConfig()) << "Error setting WiFi:" << code << message;
                //: Notification: WiFi could not be switched on or off. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting WiFi: %1").arg(message), true);
            });
    }
}

void Config::setWowlanEnabled(bool enabled) {
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
                qCWarning(lcConfig()) << "Error setting Wowlan:" << code << message;
                //: Notification: "Keep WiFi connected in standby" could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting WiFi in standby: %1").arg(message), true);
            });
    }
}

void Config::setWifiBand(QString value) {
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
            qCWarning(lcConfig()) << "Error setting Wifi band:" << code << message;
            //: Notification: the WiFi band could not be changed. %1 is the core's error message
            ui::Notification::createNotification(tr("Error setting WiFi band: %1").arg(message), true);
        });
}

void Config::setScanIntervalSec(int value) {
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
                qCWarning(lcConfig()) << "Error setting Wifi scan interval:" << code << message;
                //: Notification: the WiFi scan interval could not be changed. %1 is the core's error message
                ui::Notification::createNotification(tr("Error setting WiFi scan interval: %1").arg(message), true);
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
            qCWarning(lcConfig()) << "Error getting country list:" << code << message;
            //: Notification: the list of countries could not be loaded. %1 is the core's error message
            ui::Notification::createNotification(tr("Error getting country list: %1").arg(message), true);
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
            qCWarning(lcConfig()) << "Error getting timezones:" << code << message;
            //: Notification: the list of time zones could not be loaded. %1 is the core's error message
            ui::Notification::createNotification(tr("Error getting timezones: %1").arg(message), true);
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

void Config::setAdminPin(const QString& pin) {
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
            qCWarning(lcConfig()) << "Error while setting admin pin:" << code << message;
            //: Notification: the administrator PIN could not be set. %1 is the core's error message
            ui::Notification::createNotification(tr("Could not set the admin PIN: %1").arg(message), true);
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
            qCWarning(lcConfig()) << "Error enabling the web configurator";
            //: Notification: the state of the web configurator could not be read
            ui::Notification::createNotification(tr("Error enabling the web configurator"), true);
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
            qCWarning(lcUi()) << "Error getting active profile:" << code << message;
            emit noCurrentProfileFound();
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
                qCWarning(lcConfig()) << "Error enabling the web configurator:" << code << message;
                //: Notification: the web configurator could not be switched on or off. %1 is the core's error message
                ui::Notification::createNotification(tr("Error enabling the web configurator: %1").arg(message), true);
            });
    }
}

QStringList Config::getTranslations() {
    return ui::Translation::getTranslations();
}

QVariantList Config::getLocalizedCountryList() {
    QVariantList list;

    QString language = m_language.split("_").value(0);

    for (const auto& item : qAsConst(m_countryList)) {
        QVariantMap country = item.toMap();

        QString name = country.value("name_" + language).toString();
        if (name.isEmpty()) {
            name = country.value("name_en").toString();
        }

        QVariantMap map;
        map.insert("code", country.value("code").toString());
        map.insert("name", name);
        list.append(map);
    }

    std::sort(list.begin(), list.end(), [](const QVariant& a, const QVariant& b) {
        return QString::localeAwareCompare(a.toMap().value("name").toString(), b.toMap().value("name").toString()) < 0;
    });

    return list;
}

QStringList Config::getSuggestedCountries() {
    return ui::Translation::getCountriesForLanguage(m_language);
}

QString Config::getLikelyCountry() {
    return ui::Translation::getLikelyCountry(m_language);
}

QVariantList Config::getTimeZoneInfos(const QString& country) {
    return ui::Translation::getTimeZoneInfos(country);
}

QVariantList Config::getAllTimeZoneInfos() {
    return ui::Translation::getAllTimeZoneInfos();
}

QString Config::getCountry(const QString country) {
    return ui::Translation::getCountryName(country);
}

QObject* Config::qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject* obj = s_instance;
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
    // deviceNameChanged(true) also tells the onboarding that the name it sent was accepted: a configuration that
    // is merely loaded again, e.g. after a reconnect, must not look like that
    if (m_deviceName == cfgDevice.name) {
        return;
    }
    m_deviceName = cfgDevice.name;
    emit deviceNameChanged(true);
}

void Config::onHapticCfgChanged(core::cfgHaptic cfgHaptic) {
    m_hapticEnabled = cfgHaptic.enabled;
    emit hapticEnabledChanged(m_hapticEnabled);
}

void Config::onLocalizationCfgChanged(core::cfgLocalization cfgLocalization) {
    // Every localization setter sends the complete localization configuration, so a field that is
    // missing here must not clear the cached value: the next setter would send it back as empty
    // and core would reject the whole request.
    if (cfgLocalization.languageCode.isEmpty()) {
        qCWarning(lcConfig()) << "Localization configuration without language code";
    } else if (m_language != cfgLocalization.languageCode) {
        m_language = cfgLocalization.languageCode;
        emit languageChanged(m_language);

        setCountryNameAsSelectedLanguage();
    }

    if (cfgLocalization.countryCode.isEmpty()) {
        qCWarning(lcConfig()) << "Localization configuration without country code";
    } else if (m_country != cfgLocalization.countryCode) {
        m_country = cfgLocalization.countryCode;
        emit countryChanged(true);

        setCountryNameAsSelectedLanguage();
    }

    if (cfgLocalization.timezone.isEmpty()) {
        qCWarning(lcConfig()) << "Localization configuration without timezone";
    } else if (m_timezone != cfgLocalization.timezone) {
        m_timezone = cfgLocalization.timezone;
        emit timezoneChanged(true);
    }

    bool ok = false;
    auto newUnitSystem =
        Util::convertStringToEnum<UnitSystems>(Util::FirstToUpper(cfgLocalization.measurementUnit), &ok);

    if (!ok) {
        qCWarning(lcConfig()) << "Localization configuration with unknown unit system:"
                              << cfgLocalization.measurementUnit;
    } else if (m_unitSystem != newUnitSystem) {
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
    return QStringLiteral("%1").arg(QRandomGenerator::global()->bounded(10000), 4, 10, QChar('0'));
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

    for (const auto& item : qAsConst(m_countryList)) {
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
