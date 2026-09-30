// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#include "enums.h"

namespace uc {
namespace core {

struct Paging {
    int limit = 10;
    int page = 1;
};

struct Pagination {
    int count = 0;
    int limit = 0;
    int page = 0;
};

struct Profile {
    QString     id;
    QString     name;
    QString     icon;
    bool        restricted = false;
    int         pin = -1;
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
    int             pos = 0;
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
    bool        featuresProvided = false;
    QString     area;
    QString     deviceClass;
    // TODO(zehnm): the enabled flag of an entity is not used yet, see Base::Base(). What it means for an entity that is
    // still delivered to the UI has to be checked in the core first.
    bool        enabled = true;
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
    QString     id;
    QString     driverId;
    QString     deviceId;
    QVariantMap name;
    QString     icon;
    bool        enabled = false;
    QVariantMap setupData;
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
    bool                           enabled = false;
    QString                        description;
    DriverDeveloper                developer;
    QString                        homePage;
    bool                           deviceDiscovery = false;
    SettingsPage                   settingsPage;
    QString                        releaseDate;
    IntegrationDriverEnums::States state = {};

    bool external = false;
    bool configured = false;
    int  instanceCount = 0;
};

struct IntegrationStatus {
    QString integrationId;
    QString name;
    QString icon;
    QString deviceState;
    QString driverState;
    bool    enabled = false;
};

struct IntegrationSetupInfo {
    QString                      id;
    IntegrationEnums::SetupState state = {};
    IntegrationEnums::SetupError error = {};
    // optional driver provided error description, language text map
    QVariantMap errorMessage;
    bool        requireUserAction = false;
    // raw require_user_action object: an event repeating the current page (e.g. a battery change) carries the
    // same object as the event which introduced the page
    QVariantMap      userAction;
    SettingsPage     settingsPage;
    ConfirmationPage confirmationPage;
    // keep-alive lease of the session, 0 if not reported (session without lease)
    int keepaliveTimeoutSec = 0;
    // battery budget: only while the device runs on battery
    bool                               setupLimitActive = false;
    int                                setupExpiresInSec = 0;
    int                                setupLimitTotalSec = 0;
    IntegrationEnums::SetupLimitReason setupLimitReason = IntegrationEnums::SetupLimitReason::NO_LIMIT;
};

struct cfgButton {
    int  brightness = 0;
    bool autoBrightness = false;
};

struct cfgDisplay {
    int  brightness = 0;
    bool autoBrightness = false;
};

struct cfgDevice {
    QString name;
};

struct cfgHaptic {
    bool enabled = false;
};

struct cfgLocalization {
    QString languageCode;
    QString countryCode;
    QString timezone;
    bool    timeFormat24h = false;
    QString measurementUnit;
};

struct cfgWifi {
    bool wowlan = false;
    QStringList bands;
    QString band;
    QString ipv4Type;
    int scanIntervalSec = 0;
};

struct cfgNetwork {
    bool    bluetoothEnabled = false;
    bool    wifiEnabled = false;
    QString bluetoothMac;
    cfgWifi wifi;
};

struct cfgPowerSaving {
    int wakeupSensitivity = 0;
    int displayOffSec = 0;
    int standbySec = 0;
};

struct cfgSoftwareUpdate {
    bool                       checkForUpdates = false;
    bool                       autoUpdate = false;
    QString                    otaWindowStart;
    QString                    otaWindowEnd;
    UpdateEnums::UpdateChannel channel = {};
};

struct cfgSound {
    bool enabled = false;
    int  volume = 0;
};

struct VoiceAssistantProfile {
    QString     id;
    QString     name;
    QString     language;
    QStringList features;
};

struct VoiceAssistant {
    QString                         entity_id;
    QVariantMap                     name;
    QString                         icon;
    QString                         state;
    QStringList                     features;
    QList<VoiceAssistantProfile>    profiles;
    QString                         preferredProfile;
};

struct cfgVoiceAssistant {
    VoiceAssistant      active;
    QString             profile_id;
    bool                speechResponse = false;
};

struct cfgVoiceControl {
    bool                microphoneEnabled = false;
    cfgVoiceAssistant   voiceAsssistant;
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
    bool      enabled = false;
    QDateTime validTo;
};

struct AccessPointScan {
    QString                 bssid;
    int                     frequency = 0;
    int                     signalLevel = 0;
    QString                 auth;
    WifiEnums::WifiSecurity security = {};
    QString                 ssid;
    QString                 ssidHex;
};

struct SavedNetwork {
    int                     id = 0;
    QString                 ssid;
    QString                 ssidHex;
    WifiEnums::NetworkState state = {};
    bool                    secured = false;
    WifiEnums::WifiSecurity security = {};
    int                     signalLevel = 0;
};

struct WifiStatus {
    WifiEnums::WpaState wpaState = {};
    int                 id = 0;
    QString             bssid;
    QString             ssid;
    QString             ssidHex;
    int                 freq = 0;
    QString             address;
    QString             pairwiseCipher;
    QString             groupCipher;
    QString             keyManagement;
    QString             ipAddress;
    int                 noise = 0;
    int                 rssi = 0;
    int                 averageRssi = 0;
    int                 estimatedThroughput = 0;
    int                 snr = 0;
    int                 linkSpeed = 0;
};

struct DockConfiguration {
    QString              id;
    QString              name;
    QString              customWsUrl;
    bool                 active = false;
    QString              model;
    QString              revision;
    QString              serial;
    QString              connectionType;
    QString              version;
    DockEnums::DockState state = {};
    bool                 learningActive = false;
    QString              description;
    int                  ledBrightness = -1;
    int                  ethLedBrightness = -1;
};

struct DockDiscovery {
    QString                           id;
    QString                           friendlyName;
    QString                           address;
    bool                              configured = false;
    QString                           model;
    QString                           revision;
    QString                           serial;
    QString                           version;
    DockSetupEnums::DockDiscoveryType discoveryType = {};
    int                               bluetoothSignal = 0;
    int                               bluetoothLastSeenSeconds = 0;
};

struct AvailableSystemUpdate {
    QString                    id;
    QString                    title;
    QVariantMap                description;
    QString                    version;
    UpdateEnums::UpdateChannel channel = {};
    QDateTime                  releaseDate;
    int                        size = 0;
    UpdateEnums::DownloadState downloadState = {};
};

struct SystemUpdate {
    bool                         updateInProgress = false;
    QDateTime                    lastCheckDate;
    bool                         updateCheckEnabled = false;
    QString                      installedVersion;
    QList<AvailableSystemUpdate> available;
};

struct SystemUpdateProgress {
    UpdateEnums::UpdateProgressType state = {};
    QString                         udpateId;
    int                             downloadPercent = 0;
    int                             downloadBytes = 0;
    int                             totalSteps = 0;
    int                             currentStep = 0;
    int                             currentPercent = 0;
};

struct BrowseMediaItem {
    QString                mediaId;
    QString                title;
    QString                subtitle;          // optional
    QString                artist;
    QString                album;
    QString                mediaClass;
    QString                mediaType;
    bool                   canBrowse  = false;
    bool                   canPlay    = false;
    bool                   canSearch  = false;
    QString                thumbnail;
    int                    duration   = 0;
    QStringList            playMediaActions;  // optional, e.g. ["PLAY_NOW","PLAY_NEXT"]
    QList<BrowseMediaItem> items;
};

using SearchMediaItem = BrowseMediaItem;

struct MediaSearchFilter {
    QStringList mediaClasses;
    QString     artist;
    QString     album;
};

// --- sequence readiness (get_sequence_readiness / sequence_readiness) ---

struct ReadinessReason {
    QString code;             // ReadinessReasonCode as string; unknown values must be tolerated
    QString message;          // English diagnostic — never shown to the user
    QString groupId;          // opaque cause key: equal for steps blocked by the same cause
    QString integrationId;    // optional
    QString integrationName;  // optional, localized
    QString state;            // optional
    QString emitterId;        // optional
    QString portId;           // optional (REMOTE_IR_OUTPUT_INVALID)
    QString emitterName;      // optional
    QString deviceType;       // optional: DOCK, EXTERNAL, INTERNAL
    QString dockId;           // optional
    QString dockName;         // optional
};

struct ReadinessEntityRef {
    QString entityId;
    QString name;
};

struct ReadinessStep {
    int                       index = 0;
    int                       authoredIndex = -1;    // optional: -1 = absent
    int                       switchFromIndex = -1;  // optional
    int                       nestedIndex = -1;      // optional
    QString                   part;                  // "transition" | "sequence"
    QString                   type;                  // "command" | "delay"
    QString                   entityId;              // optional (delay steps)
    QString                   entityType;            // optional
    QString                   name;                  // optional, localized
    QString                   cmdId;                 // optional
    QVariantMap               params;                // optional
    int                       delay = 0;             // optional
    QList<ReadinessEntityRef> parents;               // optional
    bool                      ready = false;
    bool                      skipped = false;    // wire: only present when true
    bool                      abortsRun = false;  // wire: absent for ready and skipped steps
    bool                      hasReason = false;
    ReadinessReason           reason;  // valid only when hasReason
};

struct ReadinessOmittedStep {
    int             authoredIndex = 0;
    QString         type;      // "command" | "delay"
    QString         entityId;  // optional
    QString         entityType;
    QString         name;
    QString         cmdId;
    QVariantMap     params;
    int             delay = 0;
    ReadinessReason reason;  // required, always ALREADY_IN_STATE
};

struct ReadinessDependencies {
    QStringList integrationIds;
    QStringList emitterIds;
    QStringList dockIds;
    bool        bt = false;
};

struct ReadinessGroupMember {
    QString entityId;
    QString name;
    QString state;  // ON, OFF, RUNNING, ERROR, TIMEOUT, STOPPED
};

struct ReadinessActivityGroup {
    QString                     groupId;
    QString                     name;
    QString                     turnOffUnusedEntities;  // always | in_off_sequence | run_off_sequence | never
    QList<ReadinessGroupMember> members;
};

struct ReadinessSwitchFrom {
    QString entityId;
    QString name;
    QString state;  // ON, RUNNING, ERROR, TIMEOUT, STOPPED (never OFF)
};

struct SequenceReadiness {
    QString                     entityId;
    QString                     entityType;  // "activity" | "macro"
    QString                     name;        // localized title of the checked entity
    QString                     cmdId;
    QString                     lang;
    QString                     errorPolicy;
    QDateTime                   timestamp;  // ISO 8601 with ms
    bool                        ready = false;
    int                         totalSteps = 0;
    int                         transitionSteps = 0;
    int                         blockedSteps = 0;
    int                         skippedSteps = 0;
    int                         abortingSteps = 0;
    int                         omittedSteps = 0;
    ReadinessDependencies       dependsOn;
    bool                        hasActivityGroup = false;
    ReadinessActivityGroup      activityGroup;  // valid only when hasActivityGroup
    bool                        hasSwitchFrom = false;
    ReadinessSwitchFrom         switchFrom;  // valid only when hasSwitchFrom
    QList<ReadinessStep>        steps;
    QList<ReadinessOmittedStep> omitted;
};

}  // namespace core
}  // namespace uc
