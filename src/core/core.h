// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <QFile>
#include <QJsonDocument>
#include <QMetaEnum>
#include <QObject>
#include <QTimer>
#include <QWebSocket>

#include "enums.h"
#include "structs.h"

namespace uc {
namespace core {

class Api : public QObject {
    Q_OBJECT

 public:
    explicit Api(const QString &url, QObject *parent = nullptr);
    ~Api();

    /**
     * @brief Connect to the remote-core
     */
    void connect();

    /**
     * @brief Disconnect to the remote-core
     */
    void disconnect();

    // common
    int getVersion();
    int getSystemInfo();
    int authenticate();
    int systemCommand(SystemEnums::Commands command);

    // entity handling
    int getEntity(const QString &entityId);
    int getEntities(int limit = 100, int page = 1, EntityFilter filter = EntityFilter());
    int getAvailableEntities(int limit = 100, int page = 1, bool forceReload = false,
                             AvailableEntitiesFilter filter = AvailableEntitiesFilter());
    int configureEntities(const QString &integrationId, const QStringList &entityIds);
    int updateEntity(const QString &entityId, QVariantMap name, const QString &icon);
    int deleteEntity(const QString &entityId);
    int deleteEntities(const QStringList &entityIds, const QString &integrationId = QString());

    int entityCommand(const QString &entityId, const QString &cmd, QVariantMap params);

    // profile handling
    int switchProfile(const QString &profileId, const QString &pin);
    int getProfiles();
    int getProfile(const QString &profileId);
    int getActiveProfile();
    int addProfile(const QString &name, bool restricted = false);
    int updateProfile(const QString &profileId, const QString &name = QString(), const QString &icon = "-1",
                      int pin = -1, const QStringList pages = QStringList({"-1"}));
    int deleteProfile(const QString &profileId, int pin = -1);

    // page handling
    int getPages(const QString &profileId, int pin = -1);
    int getPage(const QString pageId, int pin = 1);
    int addPage(const QString &profileId, const QString &name, int pos, int pin = -1);
    int updatePage(const QString &pageId, const QString &profileId, const QString &name = QString(),
                   const QString &image = "-1", int pos = -1, const QVariantList &items = QVariantList({"-1"}),
                   int pin = -1);
    int deletePage(const QString &pageId, int pin = -1);

    // group handling
    int getGroup(const QString &groupId);
    int addGroup(const QString &profileId, const QString &name, const QString &icon = QString(),
                 const QStringList &entities = QStringList());
    int updateGroup(const QString &groupId, const QString &profileId, const QString &name,
                    const QString &icon = QString(), const QStringList &entities = QStringList());
    int deleteGroup(const QString &groupId);
    int getGroups(const QString &profileId, int pin = -1);

    // integration handling
    int getIntegrationStatus(int limit = 100, int page = 1);
    int getIntegrations(int limit = 100, int page = 1, const QString &driverId = QString());
    int getIntegrationDrivers(int limit = 100, int page = 1);
    int getIntegrationDriver(const QString &integrationDriverId);
    int deleteIntegration(const QString &id);
    int deleteIntegrationDriver(const QString &driverId);
    int integrationDriverStart(const QString &integrationDriverId);
    int integrationDriverStop(const QString &integrationDriverId);
    int integrationConnect(const QString &integrationId);
    int integrationDisconnect(const QString &integrationId);

    int integrationStartDiscovery(int timeOut = 30, bool newDevicesOnly = true);
    int integrationStopDiscovery();
    int integrationGetDiscoveredDriverMetadata(const QString &driverId, const QString &driverUrl = QString(),
                                               const QString &token = QString(), int timeOut = 5);
    int integrationSetup(const QString &driverId, QVariantMap name, QVariantMap setupData);
    int integrationStopSetup(const QString &driverId);
    int integrationSetUserDataSettings(const QString &driverId, QVariantMap settings);
    int integrationSetUserDataConfirm(const QString &driverId);
    int integrationConfigureDiscoveredDriver(const QString &driverId, QVariantMap name,
                                             const QString &driverUrl = QString(), const QString &token = QString());

    // configuration handling
    int resetConfig();
    int getConfig();
    int getButtonCfg();
    int setButtonCfg(int brightness, bool autoBrightness);
    int getDisplayCfg();
    int setDisplayCfg(int brightness, bool autoBrightness);
    int getDeviceCfg();
    int setDeviceCfg(const QString &name);
    int getHapticCfg();
    int setHapticCfg(bool enabled);
    int getLocalizationCfg();
    int setLocalizationCfg(const QString &languageCode, const QString &countryCode, const QString &timezone,
                           bool timeFormat24h, const QString &measurementUnit);
    int getTimeZoneNames();
    int getLocalizationCountries();
    int getLocalizationLanguages();
    void setLocalizationLanguages(int reqId, QString version, QVariantList languages);
    int getNetworkCfg();
    int setNetworkCfg(bool bluetoothEnabled, bool wifiEnabled, bool wowlanEnabled, QString band, int scanIntervalSec);
    int getPowerSavingCfg();
    int setPowerSavingCfg(int wakeupSensitivity, int displayOffSec, int standbySec);
    int getProfileCfg();
    int setProfileCfg(const QString &adminPin);
    int getSoftwareUpdateCfg();
    int setSoftwareUpdateCfg(bool checkForUpdates, bool autoUpdate);
    int getSoundCfg();
    int setSoundCfg(bool enabled, int volume);
    int getVoiceControlCfg();
    int setVoiceControlCfg(bool microphoneEnabled, const QString& entityId, const QString& profileId, bool speechResponse);
    int getVoiceAssistants();

    // wifi handling
    int wifiGetStatus();
    int wifiCommand(WifiEnums::WifiCmd command);
    int wifiScanStart();
    int wifiScanStop();
    int wifiGetScanStatus();
    int wifiGetAllNetworks();
    int wifiAddNetwork(const QString &ssid, const QString &password = "");
    int wifiDeleteAllNetworks();
    int wifiGetNetwork(int id);
    int wifiUpdateNetwork(int id, const QString &password);
    int wifiNetworkCommand(int id, WifiEnums::WifiNetworkCmd command);
    int wifiDeleteNetwork(int id);

    // dock handling
    int getDockCount();
    int getDocks(int limit = 100, int page = 1);
    int createDock(const QString &dockId, bool active, const QString &name = QString(),
                   const QString &customWsUrl = QString(), const QString &token = QString(),
                   const QString &model = QString(), const QString &description = QString());
    int deleteAllDocks();
    int getDock(const QString &dockId);
    int updateDock(const QString &dockId, const QString &name = QString(), const QString &customWsUrl = QString(),
                   const QString &token = QString(), bool active = true, const QString &description = QString(),
                   const QString &wifiSsid = QString(), const QString &wifiPassword = QString());
    int connectDock(const QString &dockId);
    int disconnectDock(const QString &dockId);
    int deleteDock(const QString &dockId);
    int dockCommand(const QString &dockId, DockEnums::DockCommands command, const QString &value = QString(),
                    const QString &token = QString());
    int getDockDiscoveryStatus();
    int getDockDiscoveryDevice(const QString &dockId);
    int execCommandOnDiscoveredDock(const QString &dockId, DockSetupEnums::DockCommands command,
                                    const QString &token = QString(), int timeout = 30);
    int getDockSetupProcesses();
    int getDockSetupStatus(const QString &dockId);
    int startDockDiscovery(int timeOut = 30, bool bt = true, bool net = true, bool filterNew = true);
    int stopDockDiscovery();
    int createDockSetup(const QString &id, const QString &friendlyName,
                        DockSetupEnums::DockDiscoveryType discoveryType);
    int startDockSetup(const QString &id, const QString &friendlyName, const QString &password, const QString &wifiSsid,
                       const QString &wifiPassword);
    int stopDockSetup(const QString &dockId);
    int stopAllDockSetups();

    // factory reset
    int getFactoryResetToken();
    int factoryReset(const QString &token);

    // api access
    int getApiAccess();
    int setApiAccess(bool enabled, const QString &pin = QString(), QDateTime validTo = QDateTime());

    // softare update
    int checkSystemUpdate(bool force = true);
    int updateSystem(const QString &updateId = "latest");

    // power mode
    int getPowerMode();

 public:
    /**
     * @brief This response handler is used for common response result processing, that doesn't return anything special.
     * Just status code and error message if any.
     */
    template <typename SuccessFunc, typename FailFunc>
    void onResult(int id, const SuccessFunc &functionSuccess, const FailFunc &functionFail) {
        QMetaObject::Connection connection;

        connection = QObject::connect(this, &Api::respResult, [=](int reqId, int code, QString message) {
            if (reqId == id) {
                if (code == 200 || code == 201) {
                    functionSuccess();
                } else {
                    functionFail(code, message);
                }
                QObject::disconnect(connection);
            }
        });
    }

    /**
     * @brief This response handler is used for responses which return something special, like a profile, but on error,
     * it's a common result response.
     */
    template <typename Signal, typename SuccessFunc, typename FailFunc>
    void onResponseWithErrorResult(int id, Signal signal, const SuccessFunc &functionSuccess,
                                   const FailFunc &functionFail) {
        QMetaObject::Connection connSuccess;
        QMetaObject::Connection connFail = onCommonFail(id, functionFail, connSuccess);

        connSuccess = QObject::connect(
            this, signal, [=](auto... args) { onSuccess(id, functionSuccess, connSuccess, connFail, args...); });
    }

    /**
     * @brief This response handler is used for responses which return something special, even on error
     */
    template <typename Signal, typename SuccessFunc, typename FailFunc>
    void onResponse(int id, Signal signal, const SuccessFunc &functionSuccess, const FailFunc &functionFail) {
        QMetaObject::Connection connSuccess;
        QMetaObject::Connection connFail;

        connSuccess = QObject::connect(this, signal, [=](auto... args) {
            onSuccess(id, functionSuccess, connSuccess, connFail, args...);
            onFail(id, functionFail, connSuccess, connFail, args...);
        });
    }

 private:
    /**
     * @brief helper template to process successful slot response
     */
    template <typename T1, typename T2, typename... Args, typename SuccessFunc>
    void onSuccess(int id, const SuccessFunc &functionSuccess, QMetaObject::Connection connSuccess,
                   QMetaObject::Connection connFail, T1 reqId, T2 code, Args... args) {
        if (reqId == id && (code == 200 || code == 201)) {
            functionSuccess(args...);
            QObject::disconnect(connSuccess);
            QObject::disconnect(connFail);
        }
    }

    /**
     * @brief helper template to handle unsuccessful responses as these are the same for all calls
     */
    template <typename FailFunc>
    QMetaObject::Connection onCommonFail(int id, const FailFunc &function, QMetaObject::Connection connSuccess) {
        QMetaObject::Connection connFail;
        connFail = QObject::connect(this, &Api::respResult, [=](int reqId, int code, QString message) {
            if (reqId == id && (code != 200 && code != 201)) {
                function(code, message);
                QObject::disconnect(connFail);
                QObject::disconnect(connSuccess);
            }
        });
        return connFail;
    }

    /**
     * @brief helper template to process failed slot response
     */
    template <typename T1, typename T2, typename... Args, typename FailFunc>
    void onFail(int id, const FailFunc &functionFail, QMetaObject::Connection connSuccess,
                QMetaObject::Connection connFail, T1 reqId, T2 code, Args... args) {
        if (reqId == id && (code != 200 && code != 201)) {
            functionFail(args...);
            QObject::disconnect(connSuccess);
            QObject::disconnect(connFail);
        }
    }

    // request signals
 signals:
    void reqGetLocalizationLanguages(int reqId);

    // response signals
 signals:
    void respResult(int reqId, int code, QString message);

    void respVersion(int reqId, int code, QString deviceName, QString api, QString core, QString ui, QString os,
                     QStringList integrations);
    void respSystem(int reqId, int code, QString modelName, QString modelNumber, QString serialNumber,
                    QString hwRevision);

    void respProfiles(int reqId, int code, QList<Profile> profiles);
    void respProfile(int reqId, int code, Profile profile);

    void respPages(int reqId, int code, QList<Page> pages);
    void respPage(int reqId, int code, Page page);

    void respAvailableEntities(int reqId, int code, QList<Entity> entities, int count, int limit, int page);
    void respEntity(int reqId, int code, Entity entity);
    void respEntities(int reqId, int code, QList<Entity> entities, int count, int limit, int page);

    void respGroup(int reqId, int code, Group group);
    void respGroups(int reqId, int code, QList<Group> groups);

    void respIntegrationStatus(int reqId, int code, QList<IntegrationStatus> integrationStatus, int count, int limit,
                               int page);
    void respIntegrations(int reqId, int code, QList<Integration> integrations, int count, int limit, int page);
    void respIntegrationDrivers(int reqId, int code, QList<IntegrationDriver> integrationDrivers, int count, int limit,
                                int page);
    void respIntegrationDriver(int reqId, int code, IntegrationDriver integrationDriver);
    void respIntegrationSetupInfo(int reqId, int code, IntegrationSetupInfo integrationSetupInfo);

    void respVoiceAssistants(int reqId, int code, QStringList voiceAssistants);
    void respTimeZoneNames(int reqId, int code, QStringList timeZoneNames);
    void respLocalizationCountries(int reqId, int code, QVariantList localizationCountries);
    void respLocalizationLanguages(int reqId, int code, QStringList localizationLanguages);

    void respFactoryResetToken(int reqId, int code, QString token);
    void respApiAccess(int reqId, int code, ApiAccess apiAccess);

    void wifiStatusChanged(int reqId, int code, WifiStatus wifiStatus);
    void wifiScanStatusChanged(int reqId, int code, bool active, QList<AccessPointScan> scan);
    void wifiNetworksChanged(int reqId, int code, QList<SavedNetwork> networks);
    void wifiNetworkChanged(int reqId, int code, SavedNetwork network);

    void respDockCount(int reqId, int code, int count);
    void respDocks(int reqId, int code, QList<DockConfiguration> docks, int count, int limit, int page);
    void respDock(int reqId, int code, DockConfiguration dock);
    void respDockSetupStatus(int reqId, int code, QString dockId, DockSetupEnums::DockSetupState state,
                             DockSetupEnums::DockSetupError error = DockSetupEnums::DockSetupError::NONE);
    void respDockSetupProcesses(int reqId, int code, QStringList sessions);

    void respSystemUpdateInfo(int reqId, int code, SystemUpdate systemUpdate);

    void respPowerMode(int reqId, int code, PowerEnums::PowerMode powerMode, int capacitiy, bool powerSupply,
                       PowerEnums::PowerStatus powerStatus);

    // event signals
 signals:
    void connected();
    void disconnected();
    void connectionProblem();

    void warning(MsgEventTypes::WarningEvent event, bool shutdown, QString message);

    void entityAdded(Entity entity);
    void entityChanged(QString entityId, Entity entity);
    void entityDeleted(QString entityId);
    void reloadEntities();

    void integrationDriverStateChanged(QString driverId, QString state);
    void integrationDeviceStateChanged(QString integrationId, QString driverId, QString state);
    void integrationSetupChange(IntegrationSetupInfo integrationSetupInfo);

    void configChanged(int reqId, int code, Config config);
    void voiceAssistantsChanged(int reqId, int code, QList<VoiceAssistant>);
    void cfgButtonChanged(cfgButton config);
    void cfgDisplayChanged(cfgDisplay config);
    void cfgDeviceChanged(cfgDevice config);
    void cfgHapticChanged(cfgHaptic config);
    void cfgLocalizationChanged(cfgLocalization config);
    void cfgNetworkChanged(cfgNetwork config);
    void cfgPowerSavingChanged(cfgPowerSaving config);
    void cfgSoftwareUpdateChanged(cfgSoftwareUpdate config);
    void cfgSoundChanged(cfgSound config);
    void cfgVoiceControlChanged(cfgVoiceControl config);

    void profileAdded(QString profileId, Profile profile);
    void profileChanged(QString profileId, Profile profile);
    void profileDeleted(QString profileId);

    void pageAdded(QString profileId, Page page);
    void pageChanged(QString profileId, Page page);
    void pageDeleted(QString profileId, QString pageId);

    void groupAdded(QString profileId, Group group);
    void groupChanged(QString profileId, Group group);
    void groupDeleted(QString profileId, QString groupId);

    void dockDiscoveryStarted();
    void dockDiscovered(DockDiscovery dock);
    void dockDiscoveryStopped();
    void dockSetupChanged(MsgEventTypes::Enum type, QString dockId, DockSetupEnums::DockSetupState state,
                          DockSetupEnums::DockSetupError error = DockSetupEnums::DockSetupError::NONE);
    void dockAdded(QString dockId, DockConfiguration dock);
    void dockChanged(QString dockId, DockConfiguration dock);
    void dockDeleted(QString dockId);
    void dockStateChanged(QString dockId, DockEnums::DockState state);
    void dockUpdateChanged(MsgEventTypes::Enum type, QString dockId, QString updateId, QString version, int progress,
                           DockSetupEnums::DockSetupState state,
                           DockSetupEnums::DockSetupError error = DockSetupEnums::DockSetupError::NONE);

    void integationDriverDiscoveryStarted();
    void integrationDriverDiscovered(IntegrationDriver integrationDriver);
    void integrationDriverDiscoveryStopped();

    void integrationDriverAdded(QString integrationDriverId, IntegrationDriver integrationDriver);
    void integrationDriverChanged(QString integrationDriverId, IntegrationDriver integrationDriver);
    void integrationDriverDeleted(QString integrationDriverId);

    void integrationAdded(QString integrationId, Integration integration);
    void integrationChanged(QString integrationId, Integration integration);
    void integrationDeleted(QString integrationId);

    void softwareUpdateChanged(MsgEventTypes::Enum type, QString updateId, SystemUpdateProgress progress);
    void powerModeChanged(PowerEnums::PowerMode powerMode);
    void batteryStatusChanged(int capacitiy, bool powerSupply, PowerEnums::PowerStatus powerStatus);
    void wifiEventChanged(WifiEvent::Enum event);

    void assistantEventReady(QString entityId, int sessionId);
    void assistantEventSttResponse(QString entityId, int sessionId, QString text);
    void assistantEventTextResponse(QString entityId, int sessionId, bool success, QString text);
    void assistantEventSpeechResponse(QString entityId, int sessionId, QString url, QString mimeType);
    void assistantEventFinished(QString entityId, int sessionId);
    void assistantEventError(QString entityId, int sessionId, AssistantErrorCodes::Enum code, QString message);

 private slots:
    void onTextMessageReceived(const QString &message);
    void onStateChanged(QAbstractSocket::SocketState state);
    void onError(QAbstractSocket::SocketError error);

    void onKeepAliveTimerTimeout();
    void startKeepAliveTimer();
    void stopKeepAliveTimer();

    void onReconnectTimerTimeout();

 private:
    bool         m_connected = false;
    QString      m_url;
    QWebSocket   m_webSocket;
    unsigned int m_requestId = 0;

    QTimer *m_keepAliveTimer;
    int     m_keepAliveInterval = 60000;

    QTimer *m_reconnectTimer;
    int     m_reconnectInterval = 2000;
    int     m_reconnectTries = 0;

 private:
    /**
     * @brief send a message to the socket
     * @param message
     * @return true if more than 0 bytes were sent, therefore successful
     */
    bool sendMessage(const QString &message);

    /**
     * @brief send a request to the api
     * @param msg
     * @param msgData
     * @return the id of the request
     */
    int sendRequest(RequestTypes::Enum type, const QVariantMap msgData = QVariantMap());

    void processEventMessage(QVariantMap map);
    void processResponseMessage(QVariantMap map);
    void processRequestMessage(QVariantMap map);

    void                 setupTimerForRequest(int requestId);
    void                 removeRequestTimer(int requestId);
    int                  m_requestTimeout;
    QHash<int, QTimer *> m_timeoutTimers;

    // processing response
 private:
    void processAuthResult(int reqId, int code, QVariant msgData);

    void processResponseResult(int reqId, int code, QVariant msgData);

    void processResponseVersionInfo(int reqId, int code, QVariant msgData);
    void processResponseSystemInfo(int reqId, int code, QVariant msgData);

    void processResponseProfiles(int reqId, int code, QVariant msgData);
    void processResponseProfile(int reqId, int code, QVariant msgData);

    void processResponsePages(int reqId, int code, QVariant msgData);
    void processResponsePage(int reqId, int code, QVariant msgData);

    void processResponseAvailableEntities(int reqId, int code, QVariant msgData);
    void processResponseEntity(int reqId, int code, QVariant msgData);
    void processResponseEntities(int reqId, int code, QVariant msgData);

    void processResponseIntegrationStatus(int reqId, int code, QVariant msgData);
    void processResponseIntegrations(int reqId, int code, QVariant msgData);
    void processResponseIntegrationDrivers(int reqId, int code, QVariant msgData);
    void processResponseIntegrationDriver(int reqId, int code, QVariant msgData);
    void processResponseIntegrationSetupInfo(int reqId, int code, QVariant msgData);

    void processResponseGroup(int reqId, int code, QVariant msgData);
    void processResponseGroups(int reqId, int code, QVariant msgData);

    void processResponseConfig(int reqId, int code, QVariant msgData);
    void processResponseVoiceAssistants(int reqId, int code, QVariant msgData);
    void processResponseTimeZoneNames(int reqId, int code, QVariant msgData);
    void processResponseLocalizationCountires(int reqId, int code, QVariant msgData);
    void processResponseLocalizationLanguages(int reqId, int code, QVariant msgData);

    void processFactoryResetTokent(int reqId, int code, QVariant msgData);

    void processApiAccess(int reqId, int code, QVariant msgData);

    void processWifiStatus(int reqId, int code, QVariant msgData);
    void processWifiScanStatus(int reqId, int code, QVariant msgData);
    void processWifiNetworks(int reqId, int code, QVariant msgData);
    void processWifiNetwork(int reqId, int code, QVariant msgData);

    void processResponseDockCount(int reqId, int code, QVariant msgData);
    void processResponseDocks(int reqId, int code, QVariant msgData);
    void processResponseDock(int reqId, int code, QVariant msgData);
    void processResponseDockSystemInfo(int reqId, int code, QVariant msgData);
    void processResponseDockSetupProcesses(int reqId, int code, QVariant msgData);
    void processResponseDockSetupStatus(int reqId, int code, QVariant msgData);

    void processResponseSystemUpdateInfo(int reqId, int code, QVariant msgData);

    void processResponsePowerMode(int reqId, int code, QVariant msgData);

    // processing events
 private:
    void processWarning(QVariant msgData);
    void processEntityChange(QVariant msgData);
    void processWifiChange(QVariant msgData);
    void processConfigChange(QVariant msgData);

    void processProfileChange(QVariant msgData);

    void processDockChange(QVariant msgData);
    void processDockStateChange(QVariant msgData);
    void processDockDiscoveryChange(QVariant msgData);
    void processDockSetupChange(QVariant msgData);
    void processDockUpdateChange(QVariant msgData);

    void processIntegrationDriverChange(QVariant msgData);
    void processIntegrationChange(QVariant msgData);
    void processIntegrationDiscoveryChange(QVariant msgData);
    void processIntegrationSetupChange(QVariant msgData);

    void processSoftwareUpdateChange(QVariant msgData);
    void processPowerModeChange(QVariant msgData);
    void processBatteryStatusChange(QVariant msgData);

    void processAssistantEvent(QVariant msgData);

    // processing requests
 private:
    void processRequestGetLocalizationLanguages(int reqId);
};

}  // namespace core
}  // namespace uc
