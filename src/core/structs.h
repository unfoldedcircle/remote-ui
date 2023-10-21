// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#include "enums.h"

namespace uc {
namespace core {

struct Profile {
    QString     id;
    QString     name;
    QString     icon;
    bool        restricted;
    int         pin;
    QString     description;
    QStringList pages;
};

struct PageItem {
    QString id;
    QString type;
};

struct Page {
    QString         name;
    QString         id;
    QString         profileId;
    QString         image;
    QList<PageItem> items;
    int             pos;
};

struct EntityFilter {
    QStringList integrationIds;
    QStringList entityTypes;
    QString     textSearch;
};

struct AvailableEntitiesFilter {
    QString                      integrationId;
    QStringList                  entityTypes;
    AvailableEntityEnums::Filter entities = AvailableEntityEnums::Filter::NEW;
    QString                      textSearch;
};

struct Entity {
    QString     id;
    QString     type;
    QVariantMap name;
    QString     icon;
    QString     integrationId;
    QStringList features;
    QString     area;
    QString     deviceClass;
    bool        enabled;
    QVariantMap options;
    QVariantMap attributes;
};

struct Group {
    QString     id;
    QString     profileId;
    QString     name;
    QString     icon;
    QStringList entities;
};

struct SettingsPage {
    QVariantMap  title;
    QVariantList settings;
};

struct ConfirmationPage {
    QVariantMap title;
    QVariantMap message1;
    QString     image;
    QVariantMap message2;
};

struct Integration {
    QString            id;
    QString            driverId;
    QString            deviceId;
    QVariantMap        name;
    QString            icon;
    bool               enabled;
    QVariantMap        setupData;
    DeviceStates::Enum deviceState;
};

struct DriverDeveloper {
    QString name;
    QString url;
    QString email;
};

struct IntegrationDriver {
    QString                        id;
    QVariantMap                    name;
    QString                        driverUrl;
    QString                        version;
    QString                        min_core_api;
    QString                        icon;
    bool                           enabled;
    QString                        description;
    DriverDeveloper                developer;
    QString                        homePage;
    bool                           deviceDiscovery;
    SettingsPage                   settingsPage;
    QString                        releaseDate;
    IntegrationDriverEnums::States state;

    bool external;
    bool configured;
    int  instanceCount;
};

struct IntegrationStatus {
    QString integrationId;
    QString name;
    QString icon;
    QString deviceState;
    QString driverState;
    bool    enabled;
};

struct IntegrationSetupInfo {
    QString                      id;
    IntegrationEnums::SetupState state;
    IntegrationEnums::SetupError error;
    bool                         requireUserAction;
    SettingsPage                 settingsPage;
    ConfirmationPage             confirmationPage;
};

struct cfgButton {
    int  brightness;
    bool autoBrightness;
};

struct cfgDisplay {
    int  brightness;
    bool autoBrightness;
};

struct cfgDevice {
    QString name;
};

struct cfgHaptic {
    bool enabled;
};

struct cfgLocalization {
    QString languageCode;
    QString countryCode;
    QString timezone;
    bool    timeFormat24h;
    QString measurementUnit;
};

struct cfgNetwork {
    bool    bluetoothEnabled;
    bool    wifiEnabled;
    QString bluetoothMac;
};

struct cfgPowerSaving {
    int wakeupSensitivity;
    int displayOffSec;
    int standbySec;
};

struct cfgSoftwareUpdate {
    bool                       checkForUpdates;
    bool                       autoUpdate;
    QString                    otaWindowStart;
    QString                    otaWindowEnd;
    UpdateEnums::UpdateChannel channel;
};

struct cfgSound {
    bool enabled;
    int  volume;
};

struct cfgVoiceControl {
    bool    microphoneEnabled;
    bool    enabled;
    QString voiceAsssistant;
};

struct Config {
    cfgButton         buttonCfg;
    cfgDisplay        displayCfg;
    cfgDevice         deviceCfg;
    cfgHaptic         hapticCfg;
    cfgLocalization   localizationCfg;
    cfgNetwork        networkCfg;
    cfgPowerSaving    powerSavingCfg;
    cfgSoftwareUpdate softwareUpdateCfg;
    cfgSound          soundCfg;
    cfgVoiceControl   voiceControlCfg;
};

struct ApiAccess {
    bool      enabled;
    QDateTime validTo;
};

struct AccessPointScan {
    QString bssid;
    QString frequency;
    int     signalLevel;
    QString auth;
    QString ssid;
};

struct SavedNetwork {
    int                     id;
    QString                 ssid;
    WifiEnums::NetworkState state;
    bool                    secured;
    int                     signalLevel;
};

struct WifiStatus {
    WifiEnums::WpaState wpaState;
    int                 id;
    QString             bssid;
    QString             ssid;
    int                 freq;
    QString             address;
    QString             pairwiseCipher;
    QString             groupCipher;
    QString             keyManagement;
    QString             ipAddress;
    int                 noise;
    int                 rssi;
    int                 averageRssi;
    int                 estimatedThroughput;
    int                 snr;
    int                 linkSpeed;
};

struct DockConfiguration {
    QString              id;
    QString              name;
    QString              customWsUrl;
    bool                 active;
    QString              model;
    QString              connectionType;
    QString              version;
    DockEnums::DockState state;
    bool                 learningActive;
    QString              description;
};

struct DockDiscovery {
    QString                           id;
    QString                           friendlyName;
    QString                           address;
    bool                              configured;
    QString                           model;
    QString                           version;
    DockSetupEnums::DockDiscoveryType discoveryType;
    int                               bluetoothSignal;
    int                               bluetoothLastSeenSeconds;
};

struct AvailableSystemUpdate {
    QString                    id;
    QString                    title;
    QVariantMap                description;
    QString                    version;
    UpdateEnums::UpdateChannel channel;
    QDateTime                  releaseDate;
    int                        size;
    UpdateEnums::DownloadState downloadState;
};

struct SystemUpdate {
    bool                         updateInProgress;
    QDateTime                    lastCheckDate;
    bool                         updateCheckEnabled;
    QString                      installedVersion;
    QList<AvailableSystemUpdate> available;
};

struct SystemUpdateProgress {
    UpdateEnums::UpdateProgressType state;
    QString                         udpateId;
    int                             downloadPercent;
    int                             downloadBytes;
    int                             totalSteps;
    int                             currentStep;
    int                             currentPercent;
};

}  // namespace core
}  // namespace uc
