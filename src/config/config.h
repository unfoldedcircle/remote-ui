// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCoreApplication>
#include <QHostInfo>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QRandomGenerator>
#include <QSettings>
#include <QTimeZone>

#include "../core/core.h"
#include "../translation/translation.h"
#include "../ui/notification.h"
#include "../util.h"
#include "../ui/entity/voiceAssistant.h"

namespace uc {

class Config : public QObject {
    Q_OBJECT

    Q_PROPERTY(
        QString currentProfileId READ getCurrentProfileId WRITE setCurrentProfileId NOTIFY currentProfileIdChanged)

    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString country READ getCountry WRITE setCountry NOTIFY countryChanged)
    Q_PROPERTY(QString countryName READ getCountryName NOTIFY countryNameChanged)
    Q_PROPERTY(QString timezone READ getTimezone WRITE setTimezone NOTIFY timezoneChanged)

    // TODO(#279) does this even work? READ & WRITE use a QString, but NOTIFY a UnitSystem enum!
    Q_PROPERTY(QString unitSystem READ getUnitSystem WRITE setUnitSystem NOTIFY unitSystemChanged)
    Q_PROPERTY(bool clock24h READ getClock24h WRITE setClock24h NOTIFY clock24hChanged)

    Q_PROPERTY(QString deviceName READ getDeviceName WRITE setDeviceName NOTIFY deviceNameChanged)

    Q_PROPERTY(bool hapticEnabled READ getHapticEnabled WRITE setHapticEnabled NOTIFY hapticEnabledChanged)

    Q_PROPERTY(bool micEnabled READ getMicEnabled WRITE setMicEnabled NOTIFY micEnabledChanged)

    Q_PROPERTY(QString voiceAssistantId READ getVoiceAssistantId WRITE setVoiceAssistantId NOTIFY voiceAssistantIdChanged)
    Q_PROPERTY(QString voiceAssistantProfileId READ getVoiceAssistantProfileId WRITE setVoiceAssistantProfileId NOTIFY voiceAssistantProfileIdChanged)
    Q_PROPERTY(bool voiceAssistantSpeechResponse READ getVoiceAssistantSpeechResponse WRITE setVoiceAssistantSpeechResponse NOTIFY voiceAssistantSpeechResponseChanged)

    Q_PROPERTY(bool soundEnabled READ getSoundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged)
    Q_PROPERTY(int soundVolume READ getSoundVolume WRITE setSoundVolume NOTIFY soundVolumeChanged)

    Q_PROPERTY(bool displayAutoBrightness READ getDisplayAutoBrightness WRITE setDisplayAutoBrightness NOTIFY
                   displayAutoBrightnessChanged)
    Q_PROPERTY(
        int displayBrightness READ getDisplayBrightness WRITE setDisplayBrightness NOTIFY displayBrightnessChanged)

    Q_PROPERTY(bool buttonAutoBirghtness READ getButtonAutoBirghtness WRITE setButtonAutoBirghtness NOTIFY
                   buttonAutoBirghtnessChanged)
    Q_PROPERTY(int buttonBrightness READ getButtonBrightness WRITE setButtonBrightness NOTIFY buttonBrightnessChanged)

    Q_PROPERTY(WakeupSensitivities wakeupSensitivity READ getWakeupSensitivity WRITE setWakeupSensitivity NOTIFY
                   wakeupSensitivityChanged)

    Q_PROPERTY(int sleepTimeout READ getSleepTimeout WRITE setSleepTimeout NOTIFY sleepTimeoutChanged)
    Q_PROPERTY(int displayTimeout READ getDisplayTimeout WRITE setDisplayTimeout NOTIFY displayTimeoutChanged)

    Q_PROPERTY(bool autoUpdate READ getAutoUpdate WRITE setAutoUpdate NOTIFY autoUpdateChanged)
    Q_PROPERTY(bool checkForUpdates READ getCheckForUpdates WRITE setCheckForUpdates NOTIFY checkForUpdatesChanged)
    Q_PROPERTY(QString otaWindowStart READ getOtaWindowStart NOTIFY otaWindowStartChanged)
    Q_PROPERTY(QString otaWindowEnd READ getOtaWindowEnd NOTIFY otaWindowEndChanged)
    Q_PROPERTY(QString updateChannel READ getUpdateChannel NOTIFY updateChannelChanged)

    Q_PROPERTY(bool bluetoothEnabled READ getBluetoothEnabled WRITE setBluetoothEnabled NOTIFY bluetoothEnabledChanged)

    Q_PROPERTY(bool wifiEnabled READ getWifiEnabled WRITE setWifiEnabled NOTIFY wifiEnabledChanged)
    Q_PROPERTY(bool wowlanEnabled READ getWowlanEnabled WRITE setWowlanEnabled NOTIFY wowlanChanged)
    Q_PROPERTY(QStringList wifiBands READ getWifiBands NOTIFY wifiBandsChanged)
    Q_PROPERTY(QString wifiBand READ getWifiBand WRITE setWifiBand NOTIFY wifiBandChanged)
    Q_PROPERTY(int scanIntervalSec READ getScanIntervalSec WRITE setScanIntervalSec NOTIFY scanIntervalSecChanged)
    Q_PROPERTY(QString bluetoothMac READ getBluetoothMac CONSTANT)

    Q_PROPERTY(QString legalPath READ getLegalPath CONSTANT)

    Q_PROPERTY(bool webConfiguratorEnabled READ getWebConfiguratorEnabled WRITE setWebConfiguratorEnabled NOTIFY
                   webConfiguratorEnabledChanged)
    Q_PROPERTY(QString webConfiguratorAddress READ getWebConfiguratorAddress CONSTANT)
    Q_PROPERTY(QString webConfiguratorPin READ getWebConfiguratorPin NOTIFY webConfiguratorPinChanged)

    Q_PROPERTY(bool entityButtonFuncInverted READ getEntityButtonFuncInverted WRITE setEntityButtonFuncInverted NOTIFY
                   entityButtonFuncInvertedChanged)

    Q_PROPERTY(bool showBatteryPercentage READ getShowBatteryPercentage WRITE setShowBatteryPercentage NOTIFY showBatteryPercentageChanged)

    Q_PROPERTY(bool enableActivityBar READ getEnableActivityBar WRITE setEnableActivityBar NOTIFY enableActivityBarChanged)
    Q_PROPERTY(bool fillMediaArtwork READ getFillMediaArtwork WRITE setFillMediaArtwork NOTIFY fillMediaArtworkChanged)

    Q_PROPERTY(int resumeTimeoutWindowSec READ getResumeTimeoutWindowSec WRITE setResumeTimeoutWindowSec NOTIFY resumeTimeoutWindowSecChanged)


 public:
    explicit Config(core::Api* core, QObject* parent = nullptr);
    ~Config();

    enum UnitSystems { Metric, Us, Uk };  // is this the reason why the UI shows `Us` & `Uk` and not `US` & `UK`?
    Q_ENUM(UnitSystems)

    // Q_PROPERTY methods
    QString getCurrentProfileId() { return m_currentProfile; }
    void    setCurrentProfileId(const QString& profileId);

    QString getLanguage() { return m_language; }
    void    setLanguage(const QString& language);
    QString getCountry() { return m_country; }
    void    setCountry(const QString& country);
    QString getCountryName() { return m_countryName; }
    QString getTimezone() { return m_timezone; }
    void    setTimezone(const QString& timezone);
    // TODO(#279) why use a String when there's a UnitSystems enum? Because of QML?
    QString     getUnitSystem() { return Util::convertEnumToString<UnitSystems>(m_unitSystem); }
    UnitSystems getUnitSystemEnum() { return m_unitSystem; }
    void        setUnitSystem(QString value);

    bool getClock24h() { return m_clock24h; }
    void setClock24h(bool value);

    QString getDeviceName() { return m_deviceName; }
    void    setDeviceName(const QString& name);

    bool getHapticEnabled() { return m_hapticEnabled; }
    void setHapticEnabled(bool enabled);

    bool getMicEnabled() { return m_micEnabled; }
    void setMicEnabled(bool enabled);

    QString getVoiceAssistantId() { return m_voiceAssistantId; }
    void setVoiceAssistantId(const QString& entityId);

    QString getVoiceAssistantProfileId() { return m_voiceAssistantProfileId; }
    void setVoiceAssistantProfileId(const QString& profileId);

    bool getVoiceAssistantSpeechResponse() { return m_voiceAssistantSpeechResponse; }
    void setVoiceAssistantSpeechResponse(bool value);

    bool getSoundEnabled() { return m_soundEnabled; }
    void setSoundEnabled(bool enabled);
    int  getSoundVolume() { return m_soundVolume; }
    void setSoundVolume(int volume);

    bool getDisplayAutoBrightness() { return m_displayAutoBrightness; }
    void setDisplayAutoBrightness(bool enabled);
    int  getDisplayBrightness() { return m_displayBrightness; }
    void setDisplayBrightness(int brightness);

    bool getButtonAutoBirghtness() { return m_buttonAutoBrightness; }
    void setButtonAutoBirghtness(bool enabled);
    int  getButtonBrightness() { return m_buttonBrightness; }
    void setButtonBrightness(int brightness);

    bool getEntityButtonFuncInverted();
    void setEntityButtonFuncInverted(bool value);

    bool getShowBatteryPercentage();
    void setShowBatteryPercentage(bool value);

    bool getEnableActivityBar();
    void setEnableActivityBar(bool value);

    bool getFillMediaArtwork();
    void setFillMediaArtwork(bool value);

    int getResumeTimeoutWindowSec();
    void setResumeTimeoutWindowSec(int value);

    enum WakeupSensitivities { off = 0, low = 1, medium = 2, high = 3 };
    Q_ENUM(WakeupSensitivities)

    WakeupSensitivities getWakeupSensitivity() { return m_wakeupSensitivity; }
    void                setWakeupSensitivity(WakeupSensitivities sensitivity);

    int  getSleepTimeout() { return m_sleepTimeout; }
    void setSleepTimeout(int timeout);
    int  getDisplayTimeout() { return m_displayTimeout; }
    void setDisplayTimeout(int timeout);

    bool    getAutoUpdate() { return m_autoUpdate; }
    void    setAutoUpdate(bool enabled);
    bool    getCheckForUpdates() { return m_checkForUpdates; }
    void    setCheckForUpdates(bool enabled);
    QString getOtaWindowStart() { return m_otaWindowStart; }
    QString getOtaWindowEnd() { return m_otaWindowEnd; }
    QString getUpdateChannel() { return m_updateChannel; }

    bool    getBluetoothEnabled() { return m_bluetoothEnabled; }
    void    setBluetoothEnabled(bool enabled);
    bool    getWifiEnabled() { return m_wifiEnabled; }
    void    setWifiEnabled(bool enabled);
    bool    getWowlanEnabled() { return m_wowlanEnabled; }
    void    setWowlanEnabled(bool enabled);

    void    setWifiBand(QString value);
    void    setScanIntervalSec(int value);

    QStringList getWifiBands() { return m_bands; }
    QString     getWifiBand() { return m_band; }
    int         getScanIntervalSec() { return m_scanIntervalSec; }

    QString getBluetoothMac() { return m_bluetoothMac; }

    Q_INVOKABLE QString     getLanguageAsNative(const QString language);
    Q_INVOKABLE QString     getLanguageAsNative();
    Q_INVOKABLE QStringList getTranslations();
    Q_INVOKABLE QString     getLanguageCodeFromCountry(const QString& country);

    Q_INVOKABLE QString getCountry(const QString country);
    Q_INVOKABLE QString getCountryAsNative();
    Q_INVOKABLE QString getCountryAsNative(const QString country);

    Q_INVOKABLE void getCountryList();
    Q_INVOKABLE void getTimeZones();
    Q_INVOKABLE void getTimeZones(const QString country);

    Q_INVOKABLE void generateNewWebConfigPin();

    Q_INVOKABLE void setAdminPin(const QString& pin);

    QString getLegalPath() { return QCoreApplication::applicationDirPath() + "/legal"; }

    Q_INVOKABLE void    getConfig();

    void    getApiAccess();
    void    getActiveProfile();
    bool    getWebConfiguratorEnabled() { return m_webConfiguratorEnabled; }
    void    setWebConfiguratorEnabled(bool value);
    QString getWebConfiguratorAddress() { return QHostInfo::localHostName(); }
    QString getWebConfiguratorPin() { return m_webConfiguratorPin; }

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void currentProfileIdChanged();
    void noCurrentProfileFound();

    void languageChanged(QString language);
    void countryChanged(bool success);
    void countryNameChanged(QString countryName);
    void timezoneChanged(bool success);
    void unitSystemChanged(UnitSystems unitSystem);
    void clock24hChanged(bool value);
    void timeZoneListChanged(QStringList list);
    void countryListChanged(QVariantList list);

    void deviceNameChanged(bool success);

    void hapticEnabledChanged(bool value);

    void micEnabledChanged(bool value);
    void voiceAssistantIdChanged(QString voiceAssistantId);
    void voiceAssistantProfileIdChanged(QString profileId);
    void voiceAssistantSpeechResponseChanged(bool speechResponse);

    void soundEnabledChanged(bool value);
    void soundVolumeChanged(int volume);

    void displayAutoBrightnessChanged(bool value);
    void displayBrightnessChanged(int brightness);

    void buttonAutoBirghtnessChanged(bool value);
    void buttonBrightnessChanged(int brightness);

    void wakeupSensitivityChanged(WakeupSensitivities wakeupSensitivity);

    void sleepTimeoutChanged(int timeout);
    void displayTimeoutChanged(int timeout);

    void autoUpdateChanged(bool value);
    void checkForUpdatesChanged(bool value);
    void otaWindowStartChanged(QString value);
    void otaWindowEndChanged(QString value);
    void updateChannelChanged(QString value);

    void webConfiguratorEnabledChanged(bool value);
    void webConfiguratorPinChanged(QString pin);

    void bluetoothEnabledChanged(bool value);
    void wifiEnabledChanged(bool value);
    void wowlanChanged(bool value);
    void wifiBandsChanged(QStringList value);
    void wifiBandChanged(QString value);
    void scanIntervalSecChanged(int value);

    void adminPinSet(bool success);

    void entityButtonFuncInvertedChanged();
    void showBatteryPercentageChanged();
    void enableActivityBarChanged();
    void fillMediaArtworkChanged();
    void resumeTimeoutWindowSecChanged(int value);

 public slots:
    void onCoreConnected();
    void onConfigChanged(int reqId, int code, core::Config config);
    void onButtonCfgChanged(core::cfgButton cfgButton);
    void onDisplayCfgChanged(core::cfgDisplay cfgDisplay);
    void onDeviceCfgChanged(core::cfgDevice cfgDevice);
    void onHapticCfgChanged(core::cfgHaptic cfgHaptic);
    void onLocalizationCfgChanged(core::cfgLocalization cfgLocalization);
    void onNetworkCfgChanged(core::cfgNetwork cfgNetwork);
    void onPowerSavingCfgChanged(core::cfgPowerSaving cfgPowerSaving);
    void onSoftwareUpdateCfgChanged(core::cfgSoftwareUpdate cfgSoftwareUpdate);
    void onSoundCfgChanged(core::cfgSound cfgSound);
    void onVoiceControlCfgChanged(core::cfgVoiceControl cfgVoiceControl);

 private:
    static Config* s_instance;

    core::Api* m_core;

    QSettings* m_settings;

    QString m_currentProfile;
    int     m_currentProfileLoadTries = 0;

    QString      m_language;
    QString      m_country;
    QString      m_countryName;
    QVariantList m_countryList;
    QString      m_timezone;
    UnitSystems  m_unitSystem;
    bool         m_clock24h = false;

    QString m_deviceName;

    bool m_hapticEnabled;

    bool    m_micEnabled;
    QString m_voiceAssistantId;
    QString m_voiceAssistantProfileId;
    bool    m_voiceAssistantSpeechResponse;

    bool m_soundEnabled;
    int  m_soundVolume;

    bool m_displayAutoBrightness;
    int  m_displayBrightness;

    bool m_buttonAutoBrightness;
    int  m_buttonBrightness;

    WakeupSensitivities m_wakeupSensitivity;
    int                 m_sleepTimeout;
    int                 m_displayTimeout;

    bool    m_autoUpdate;
    bool    m_checkForUpdates;
    QString m_otaWindowStart;
    QString m_otaWindowEnd;
    QString m_updateChannel = "DEFAULT";

    bool    m_bluetoothEnabled;
    bool    m_wifiEnabled;
    bool    m_wowlanEnabled;

    QStringList m_bands;
    QString     m_band;
    int         m_scanIntervalSec;

    QString m_bluetoothMac;

    bool    m_webConfiguratorEnabled = false;
    QString m_webConfiguratorAddress = "http://192.168.100.35:8080/configurator";
    QString m_webConfiguratorPin = "••••";

    QString generateRandomPin();

    void setCountryNameAsSelectedLanguage();
};

}  // namespace uc
