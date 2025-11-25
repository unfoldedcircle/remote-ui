// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core.h"

#include "../logging.h"
#include "../ui/notification.h"
#include "../util.h"

namespace uc {
namespace core {

Api::Api(const QString& url, QObject* parent) : QObject(parent), m_url(url) {
    m_requestTimeout = qEnvironmentVariableIntValue("UC_UI_REQUEST_TIMEOUT");
    if (m_requestTimeout == 0) {
        m_requestTimeout = 10000;
    }

    // connect websocket signals
    QObject::connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &Api::onTextMessageReceived);
    QObject::connect(&m_webSocket, static_cast<void (QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error),
                     this, &Api::onError);
    QObject::connect(&m_webSocket, &QWebSocket::stateChanged, this, &Api::onStateChanged);

    // setup keep alive timer
    //    m_keepAliveTimer = new QTimer(this);
    //    m_keepAliveTimer->setInterval(m_keepAliveInterval);
    //    QObject::connect(m_keepAliveTimer, &QTimer::timeout, this, &core::onKeepAliveTimerTimeout);
    //    QObject::connect(this, &core::connected, this, &core::startKeepAliveTimer);
    //    QObject::connect(this, &core::disconnected, this, &core::stopKeepAliveTimer);

    // setup reconnect timer
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(m_reconnectInterval);
    QObject::connect(m_reconnectTimer, &QTimer::timeout, this, &Api::onReconnectTimerTimeout);

    connect();
}

Api::~Api() {}

void Api::connect() {
    qCDebug(lcCore()) << "Connect to core:" << m_url;

    if (m_webSocket.state() != QAbstractSocket::ConnectedState) {
        if (m_webSocket.isValid()) {
            m_webSocket.close();
        }

        m_webSocket.open(QUrl(m_url));
    }
}

void Api::disconnect() {
    qCDebug(lcCore()) << "Disconnect from core";

    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        m_webSocket.close();
    }
}

int Api::getVersion() {
    return sendRequest(RequestTypes::version);
}

int Api::getSystemInfo() {
    return sendRequest(RequestTypes::system);
}

int Api::authenticate() {
    QString token;

    QFile file;
    file.setFileName(qgetenv("UC_TOKEN_PATH"));

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        token = in.readAll().trimmed();
    } else {
        qCWarning(lcCore()) << "Failed to open token file";
        return -1;
    }

    file.close();

    QVariantMap msgData;
    msgData.insert("token", token);

    return sendRequest(RequestTypes::auth, msgData);
}

int Api::systemCommand(SystemEnums::Commands command) {
    QVariantMap msgData;
    msgData.insert("command", Util::convertEnumToString(command));
    return sendRequest(RequestTypes::system_cmd, msgData);
}

int Api::switchProfile(const QString& profileId, const QString& pin) {
    QVariantMap msgData;
    msgData.insert("profile_id", profileId);
    //    if (!pin.isEmpty()) {
    msgData.insert("admin_pin", pin);
    //    }
    return sendRequest(RequestTypes::switch_profile, msgData);
}

int Api::sendRequest(RequestTypes::Enum type, const QVariantMap msgData) {
    if (!m_connected && type != RequestTypes::auth) {
        return -1;
    }

    // increment the request id
    m_requestId++;
    unsigned int id = m_requestId;

    // assembled the request
    QVariantMap map;
    map.insert("kind", "req");
    map.insert("id", id);
    map.insert("msg", Util::convertEnumToString(type));
    map.insert("msg_data", msgData);

    QJsonDocument doc = QJsonDocument::fromVariant(map);
    QString       message = doc.toJson(QJsonDocument::JsonFormat::Compact);

    qCDebug(lcCore()).noquote() << "Sending request:" << message;

    if (sendMessage(message)) {
        setupTimerForRequest(id);
        return id;
    } else {
        return -1;
    }
}

int Api::getProfiles() {
    return sendRequest(RequestTypes::get_profiles);
}

int Api::getProfile(const QString& profileId) {
    QVariantMap msgData;
    msgData.insert("profile_id", profileId);
    return sendRequest(RequestTypes::get_profile, msgData);
}

int Api::getActiveProfile() {
    return sendRequest(RequestTypes::get_active_profile);
}

int Api::addProfile(const QString& name, bool restricted) {
    QVariantMap msgData;
    msgData.insert("name", name);
    if (restricted) {
        msgData.insert("restricted", restricted);
    }
    return sendRequest(RequestTypes::add_profile, msgData);
}

int Api::updateProfile(const QString& profileId, const QString& name, const QString& icon, int pin,
                       const QStringList pages) {
    if (profileId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("profile_id", profileId);
    if (!name.isEmpty()) {
        msgData.insert("name", name);
    }
    if (icon != "-1") {
        msgData.insert("icon", icon);
    }
    if (pin != -1) {
        msgData.insert("pin", pin);
    }
    if ((pages.length() > 0 && pages[0] != "-1") || pages.isEmpty()) {
        msgData.insert("pages", pages);
    }
    return sendRequest(RequestTypes::update_profile, msgData);
}

int Api::deleteProfile(const QString& profileId, int pin) {
    if (profileId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("profile_id", profileId);
    if (pin != -1) {
        msgData.insert("pin", pin);
    }
    return sendRequest(RequestTypes::delete_profile, msgData);
}

int Api::getPages(const QString& profileId, int pin) {
    QVariantMap msgData;
    msgData.insert("profile_id", profileId);
    if (pin != -1) {
        msgData.insert("pin", QString::number(pin));
    }
    return sendRequest(RequestTypes::get_pages, msgData);
}

int Api::getPage(const QString pageId, int pin) {
    if (pageId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("page_id", pageId);
    if (pin != -1) {
        msgData.insert("pin", QString::number(pin));
    }
    return sendRequest(RequestTypes::get_page, msgData);
}

int Api::addPage(const QString& profileId, const QString& name, int pos, int pin) {
    if (profileId.isEmpty() || name.isEmpty() || pos < 0) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("profile_id", profileId);
    if (pin != -1) {
        msgData.insert("pin", QString::number(pin));
    }
    msgData.insert("name", name);
    msgData.insert("pos", pos);

    return sendRequest(RequestTypes::add_page, msgData);
}

int Api::updatePage(const QString& pageId, const QString& profileId, const QString& name, const QString& image, int pos,
                    const QVariantList& items, int pin) {
    if (pageId.isEmpty() || profileId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("page_id", pageId);
    msgData.insert("profile_id", profileId);

    if (!name.isEmpty()) {
        msgData.insert("name", name);
    }
    if (image != "-1") {
        msgData.insert("image", image);
    }
    if (pin != -1) {
        msgData.insert("pin", QString::number(pin));
    }
    if ((items.length() > 0 && items[0] != "-1") || items.isEmpty()) {
        msgData.insert("items", items);
    }

    if (pos != -1) {
        msgData.insert("pos", pos);
    }

    return sendRequest(RequestTypes::update_page, msgData);
}

int Api::deletePage(const QString& pageId, int pin) {
    if (pageId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("page_id", pageId);
    if (pin != -1) {
        msgData.insert("pin", QString::number(pin));
    }

    return sendRequest(RequestTypes::delete_page, msgData);
}

int Api::getEntity(const QString& entityId) {
    if (entityId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("entity_id", entityId);
    return sendRequest(RequestTypes::get_entity, msgData);
}

int Api::getEntities(int limit, int page, EntityFilter filter) {
    QVariantMap msgData;

    // paging
    QVariantMap paging;
    paging.insert("limit", limit);
    paging.insert("page", page);

    msgData.insert("paging", paging);

    // filter
    QVariantMap msgFilter;

    if (!filter.integrationIds.isEmpty()) {
        msgFilter.insert("integration_ids", filter.integrationIds);
    }

    if (!filter.entityTypes.isEmpty()) {
        msgFilter.insert("entity_types", filter.entityTypes);
    }

    if (!filter.textSearch.isEmpty()) {
        msgFilter.insert("text_search", filter.textSearch);
    }

    if (msgFilter.size() > 0) {
        msgData.insert("filter", msgFilter);
    }

    return sendRequest(RequestTypes::get_entities, msgData);
}

int Api::getAvailableEntities(int limit, int page, bool forceReload, AvailableEntitiesFilter filter) {
    QVariantMap msgData;

    // paging
    QVariantMap paging;
    paging.insert("limit", limit);
    paging.insert("page", page);

    msgData.insert("paging", paging);

    msgData.insert("force_reload", forceReload);

    // filter
    QVariantMap msgFilter;

    if (!filter.integrationId.isEmpty()) {
        msgFilter.insert("integration_id", filter.integrationId);
    }

    if (!filter.entityTypes.isEmpty()) {
        msgFilter.insert("entity_types", filter.entityTypes);
    }

    if (!filter.textSearch.isEmpty()) {
        msgFilter.insert("text_search", filter.textSearch);
    }

    msgFilter.insert("entities", Util::convertEnumToString(filter.entities));

    if (msgFilter.size() > 0) {
        msgData.insert("filter", msgFilter);
    }

    return sendRequest(RequestTypes::get_available_entities, msgData);
}

int Api::configureEntities(const QString& integrationId, const QStringList& entityIds) {
    if (integrationId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("integration_id", integrationId);
    msgData.insert("entity_ids", entityIds);

    return sendRequest(RequestTypes::configure_entities_from_integration, msgData);
}

int Api::updateEntity(const QString& entityId, QVariantMap name, const QString& icon) {
    if (entityId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("entity_id", entityId);

    if (!name.isEmpty()) {
        msgData.insert("name", name);
    }

    if (!icon.isEmpty()) {
        msgData.insert("icon", icon);
    }

    return sendRequest(RequestTypes::update_entity, msgData);
}

int Api::deleteEntity(const QString& entityId) {
    if (entityId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("entity_id", entityId);
    return sendRequest(RequestTypes::delete_entity, msgData);
}

int Api::deleteEntities(const QStringList& entityIds, const QString& integrationId) {
    QVariantMap msgData;

    if (!entityIds.isEmpty()) {
        msgData.insert("entity_ids", entityIds);
    }

    if (!integrationId.isEmpty()) {
        msgData.insert("integration_id", integrationId);
    }

    return sendRequest(RequestTypes::delete_entities, msgData);
}

int Api::getGroup(const QString& groupId) {
    if (groupId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("group_id", groupId);
    return sendRequest(RequestTypes::get_group, msgData);
}

int Api::addGroup(const QString& profileId, const QString& name, const QString& icon, const QStringList& entities) {
    if (profileId.isEmpty() || name.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("profile_id", profileId);
    msgData.insert("name", name);
    msgData.insert("icon", icon);
    msgData.insert("entities", entities);
    return sendRequest(RequestTypes::add_group, msgData);
}

int Api::updateGroup(const QString& groupId, const QString& profileId, const QString& name, const QString& icon,
                     const QStringList& entities) {
    if (groupId.isEmpty() || profileId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("group_id", groupId);
    msgData.insert("profile_id", profileId);

    if (!name.isEmpty()) {
        msgData.insert("name", name);
    }
    if (!icon.isEmpty()) {
        msgData.insert("icon", icon);
    }

    if (entities.length() > 0) {
        msgData.insert("entities", entities);
    }

    return sendRequest(RequestTypes::update_group, msgData);
}

int Api::deleteGroup(const QString& groupId) {
    if (groupId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("group_id", groupId);
    return sendRequest(RequestTypes::delete_group, msgData);
}

int Api::getGroups(const QString& profileId, int pin) {
    if (profileId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("profile_id", profileId);

    if (pin != -1) {
        msgData.insert("pin", QString::number(pin));
    }

    return sendRequest(RequestTypes::get_groups, msgData);
}

int Api::getIntegrationStatus(int limit, int page) {
    QVariantMap msgData;

    // paging
    QVariantMap paging;
    paging.insert("limit", limit);
    paging.insert("page", page);

    msgData.insert("paging", paging);

    return sendRequest(RequestTypes::get_integration_status, msgData);
}

int Api::getIntegrations(int limit, int page, const QString& driverId) {
    QVariantMap msgData;

    // paging
    QVariantMap paging;
    paging.insert("limit", limit);
    paging.insert("page", page);

    msgData.insert("paging", paging);

    // filter
    QVariantMap msgFilter;

    if (!driverId.isEmpty()) {
        msgFilter.insert("driver_id", driverId);
    }

    if (msgFilter.size() > 0) {
        msgData.insert("filter", msgFilter);
    }

    return sendRequest(RequestTypes::get_integrations, msgData);
}

int Api::getIntegrationDrivers(int limit, int page) {
    QVariantMap msgData;

    // paging
    QVariantMap paging;
    paging.insert("limit", limit);
    paging.insert("page", page);

    msgData.insert("paging", paging);

    return sendRequest(RequestTypes::get_integration_drivers, msgData);
}

int Api::getIntegrationDriver(const QString& integrationDriverId) {
    QVariantMap msgData;
    msgData.insert("driver_id", integrationDriverId);
    return sendRequest(RequestTypes::get_integration_driver, msgData);
}

int Api::deleteIntegration(const QString& id) {
    QVariantMap msgData;
    msgData.insert("integration_id", id);
    return sendRequest(RequestTypes::delete_integration, msgData);
}

int Api::deleteIntegrationDriver(const QString& driverId) {
    QVariantMap msgData;
    msgData.insert("driver_id", driverId);
    return sendRequest(RequestTypes::delete_integration_driver, msgData);
}

int Api::integrationDriverStart(const QString& integrationDriverId) {
    QVariantMap msgData;
    msgData.insert("cmd_id", Util::convertEnumToString(IntegrationDriverEnums::Commands::START));

    if (!integrationDriverId.isEmpty()) {
        msgData.insert("driver_id", integrationDriverId);
    }

    return sendRequest(RequestTypes::integration_driver_cmd, msgData);
}

int Api::integrationDriverStop(const QString& integrationDriverId) {
    QVariantMap msgData;
    msgData.insert("cmd_id", Util::convertEnumToString(IntegrationDriverEnums::Commands::STOP));

    if (!integrationDriverId.isEmpty()) {
        msgData.insert("driver_id", integrationDriverId);
    }

    return sendRequest(RequestTypes::integration_driver_cmd, msgData);
}

int Api::integrationConnect(const QString& integrationId) {
    QVariantMap msgData;
    msgData.insert("cmd_id", Util::convertEnumToString(IntegrationEnums::Commands::CONNECT));

    if (!integrationId.isEmpty()) {
        msgData.insert("integration_id", integrationId);
    }

    return sendRequest(RequestTypes::integration_cmd, msgData);
}

int Api::integrationDisconnect(const QString& integrationId) {
    QVariantMap msgData;
    msgData.insert("cmd_id", Util::convertEnumToString(IntegrationEnums::Commands::DISCONNECT));

    if (!integrationId.isEmpty()) {
        msgData.insert("integration_id", integrationId);
    }

    return sendRequest(RequestTypes::integration_cmd, msgData);
}

int Api::integrationStartDiscovery(int timeOut, bool newDevicesOnly) {
    QVariantMap msgData;
    msgData.insert("timeout", timeOut);
    msgData.insert("new", newDevicesOnly);
    return sendRequest(RequestTypes::start_integration_discovery, msgData);
}

int Api::integrationStopDiscovery() {
    return sendRequest(RequestTypes::stop_integration_discovery);
}

int Api::integrationGetDiscoveredDriverMetadata(const QString& driverId, const QString& driverUrl, const QString& token,
                                                int timeOut) {
    QVariantMap msgData;
    msgData.insert("driver_id", driverId);
    msgData.insert("timeout", timeOut);

    QVariantMap connection;
    if (!driverUrl.isEmpty()) {
        connection.insert("driver_url", driverUrl);
    }

    if (!token.isEmpty()) {
        connection.insert("token", token);
    }

    msgData.insert("connection", connection);

    return sendRequest(RequestTypes::get_discovered_integration_driver_metadata, msgData);
}

int Api::integrationSetup(const QString& driverId, QVariantMap name, QVariantMap setupData) {
    QVariantMap msgData;
    msgData.insert("driver_id", driverId);
    msgData.insert("name", name);
    msgData.insert("setup_data", setupData);

    return sendRequest(RequestTypes::setup_integration, msgData);
}

int Api::integrationStopSetup(const QString& driverId) {
    QVariantMap msgData;
    msgData.insert("driver_id", driverId);

    return sendRequest(RequestTypes::stop_integration_setup, msgData);
}

int Api::integrationSetUserDataSettings(const QString& driverId, QVariantMap settings) {
    QVariantMap msgData;
    QVariantMap data;
    data.insert("input_values", settings);
    msgData.insert("data", data);
    msgData.insert("driver_id", driverId);

    return sendRequest(RequestTypes::set_integration_user_data, msgData);
}

int Api::integrationSetUserDataConfirm(const QString& driverId) {
    QVariantMap msgData;
    QVariantMap data;
    data.insert("confirm", true);
    msgData.insert("data", data);
    msgData.insert("driver_id", driverId);

    return sendRequest(RequestTypes::set_integration_user_data, msgData);
}

int Api::integrationConfigureDiscoveredDriver(const QString& driverId, QVariantMap name, const QString& driverUrl,
                                              const QString& token) {
    QVariantMap msgData;
    msgData.insert("driver_id", driverId);
    msgData.insert("name", name);

    QVariantMap connection;
    if (!driverUrl.isEmpty()) {
        connection.insert("driver_url", driverUrl);
    }

    if (!token.isEmpty()) {
        connection.insert("token", token);
    }

    return sendRequest(RequestTypes::configure_discovered_integration_driver, msgData);
}

int Api::resetConfig() {
    return sendRequest(RequestTypes::reset_configuration);
}

int Api::getConfig() {
    return sendRequest(RequestTypes::get_configuration);
}

int Api::getButtonCfg() {
    return sendRequest(RequestTypes::get_button_cfg);
}

int Api::setButtonCfg(int brightness, bool autoBrightness) {
    QVariantMap msgData;
    msgData.insert("brightness", brightness);
    msgData.insert("auto_brightness", autoBrightness);
    return sendRequest(RequestTypes::set_button_cfg, msgData);
}

int Api::getDisplayCfg() {
    return sendRequest(RequestTypes::get_display_cfg);
}

int Api::setDisplayCfg(int brightness, bool autoBrightness) {
    QVariantMap msgData;
    msgData.insert("brightness", brightness);
    msgData.insert("auto_brightness", autoBrightness);
    return sendRequest(RequestTypes::set_display_cfg, msgData);
}

int Api::getDeviceCfg() {
    return sendRequest(RequestTypes::get_device_cfg);
}

int Api::setDeviceCfg(const QString& name) {
    QVariantMap msgData;
    msgData.insert("name", name);
    return sendRequest(RequestTypes::set_device_cfg, msgData);
}

int Api::getHapticCfg() {
    return sendRequest(RequestTypes::get_haptic_cfg);
}

int Api::setHapticCfg(bool enabled) {
    QVariantMap msgData;
    msgData.insert("enabled", enabled);
    return sendRequest(RequestTypes::set_haptic_cfg, msgData);
}

int Api::getLocalizationCfg() {
    return sendRequest(RequestTypes::get_localization_cfg);
}

int Api::setLocalizationCfg(const QString& languageCode, const QString& countryCode, const QString& timezone,
                            bool timeFormat24h, const QString& measurementUnit) {
    QVariantMap msgData;
    msgData.insert("language_code", languageCode);
    msgData.insert("country_code", countryCode);
    msgData.insert("time_zone", timezone);
    msgData.insert("time_format_24h", timeFormat24h);
    msgData.insert("measurement_unit", measurementUnit);
    return sendRequest(RequestTypes::set_localization_cfg, msgData);
}

int Api::getTimeZoneNames() {
    return sendRequest(RequestTypes::get_timezone_names);
}

int Api::getLocalizationCountries() {
    return sendRequest(RequestTypes::get_localization_countries);
}

int Api::getLocalizationLanguages() {
    return sendRequest(RequestTypes::get_localization_languages);
}

void Api::setLocalizationLanguages(int reqId, QString verison, QVariantList languages)
{
    QVariantMap msgData;
    msgData.insert("version", verison);
    msgData.insert("translations", languages);

    QVariantMap map;
    map.insert("kind", "resp");
    map.insert("req_id", reqId);
    map.insert("msg", "localization_languages");
    map.insert("msg_data", msgData);

    QJsonDocument doc = QJsonDocument::fromVariant(map);
    QString       message = doc.toJson(QJsonDocument::JsonFormat::Compact);
    qCDebug(lcCore()).noquote() << "Sending response:" << message;

    sendMessage(message);
}

int Api::getNetworkCfg() {
    return sendRequest(RequestTypes::get_network_cfg);
}

int Api::setNetworkCfg(bool bluetoothEnabled, bool wifiEnabled, bool wowlanEnabled, QString band, int scanIntervalSec) {
    QVariantMap msgData;
    msgData.insert("bt_enabled", bluetoothEnabled);
    msgData.insert("wifi_enabled", wifiEnabled);

    QVariantMap msgDataWifi;

    QVariantMap msgDataWowlan;
    msgDataWowlan.insert("enabled", wowlanEnabled);

    msgDataWifi.insert("wake_on_wlan", msgDataWowlan);
    if (!band.isEmpty()) {
        msgDataWifi.insert("band", band);
    }
    msgDataWifi.insert("scan_interval_sec", scanIntervalSec);

    msgData.insert("wifi", msgDataWifi);

    return sendRequest(RequestTypes::set_network_cfg, msgData);
}

int Api::getPowerSavingCfg() {
    return sendRequest(RequestTypes::get_power_saving_cfg);
}

int Api::setPowerSavingCfg(int wakeupSensitivity, int displayOffSec, int standbySec) {
    QVariantMap msgData;
    msgData.insert("wakeup_sensitivity", wakeupSensitivity);
    msgData.insert("display_off_sec", displayOffSec);
    msgData.insert("standby_sec", standbySec);
    return sendRequest(RequestTypes::set_power_saving_cfg, msgData);
}

int Api::getProfileCfg() {
    return sendRequest(RequestTypes::get_profile_cfg);
}

int Api::setProfileCfg(const QString& adminPin) {
    QVariantMap msgData;
    msgData.insert("admin_pin", adminPin);
    return sendRequest(RequestTypes::set_profile_cfg, msgData);
}

int Api::getSoftwareUpdateCfg() {
    return sendRequest(RequestTypes::get_software_update_cfg);
}

int Api::setSoftwareUpdateCfg(bool checkForUpdates, bool autoUpdate) {
    QVariantMap msgData;
    msgData.insert("check_for_updates", checkForUpdates);
    msgData.insert("auto_update", autoUpdate);
    return sendRequest(RequestTypes::set_software_update_cfg, msgData);
}

int Api::getSoundCfg() {
    return sendRequest(RequestTypes::get_sound_cfg);
}

int Api::setSoundCfg(bool enabled, int volume) {
    QVariantMap msgData;
    msgData.insert("enabled", enabled);
    msgData.insert("volume", volume);
    return sendRequest(RequestTypes::set_sound_cfg, msgData);
}

int Api::getVoiceControlCfg() {
    return sendRequest(RequestTypes::get_voice_control_cfg);
}

int Api::setVoiceControlCfg(bool microphoneEnabled, bool enabled, const QString& voiceAsssistant) {
    QVariantMap msgData;
    msgData.insert("microphone", microphoneEnabled);
    msgData.insert("enabled", enabled);
    msgData.insert("voice_assistant", voiceAsssistant);
    return sendRequest(RequestTypes::set_voice_control_cfg, msgData);
}

int Api::getVoiceAssistants() {
    return sendRequest(RequestTypes::get_voice_assistants);
}

int Api::wifiGetStatus() {
    return sendRequest(RequestTypes::get_wifi_status);
}

int Api::wifiCommand(WifiEnums::WifiCmd command) {
    QVariantMap msgData;
    msgData.insert("cmd", Util::convertEnumToString(command));
    return sendRequest(RequestTypes::wifi_command, msgData);
}

int Api::wifiScanStart() {
    return sendRequest(RequestTypes::wifi_scan_start);
}

int Api::wifiScanStop() {
    return sendRequest(RequestTypes::wifi_scan_stop);
}

int Api::wifiGetScanStatus() {
    return sendRequest(RequestTypes::get_wifi_scan_status);
}

int Api::wifiGetAllNetworks() {
    return sendRequest(RequestTypes::get_all_wifi_networks);
}

int Api::wifiAddNetwork(const QString& ssid, const QString& password) {
    QVariantMap msgData;
    msgData.insert("ssid", ssid);

    if (!password.isEmpty()) {
        msgData.insert("password", password);
    }

    return sendRequest(RequestTypes::add_wifi_network, msgData);
}

int Api::wifiDeleteAllNetworks() {
    return sendRequest(RequestTypes::del_all_wifi_networks);
}

int Api::wifiGetNetwork(int id) {
    QVariantMap msgData;
    msgData.insert("id", id);
    return sendRequest(RequestTypes::get_wifi_network, msgData);
}

int Api::wifiUpdateNetwork(int id, const QString& password) {
    QVariantMap msgData;
    msgData.insert("id", id);
    msgData.insert("password", password);
    return sendRequest(RequestTypes::update_wifi_network, msgData);
}

int Api::wifiNetworkCommand(int id, WifiEnums::WifiNetworkCmd command) {
    QVariantMap msgData;
    msgData.insert("id", id);
    msgData.insert("cmd", Util::convertEnumToString(command));
    return sendRequest(RequestTypes::wifi_network_command, msgData);
}

int Api::wifiDeleteNetwork(int id) {
    QVariantMap msgData;
    msgData.insert("id", id);
    return sendRequest(RequestTypes::del_wifi_network, msgData);
}

int Api::getDockCount() {
    return sendRequest(RequestTypes::get_dock_count);
}

int Api::getDocks(int limit, int page) {
    QVariantMap msgData;

    // paging
    QVariantMap paging;
    paging.insert("limit", limit);
    paging.insert("page", page);

    msgData.insert("paging", paging);

    return sendRequest(RequestTypes::get_docks, msgData);
}

int Api::createDock(const QString& dockId, bool active, const QString& name, const QString& customWsUrl,
                    const QString& token, const QString& model, const QString& description) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);
    msgData.insert("active", active);

    if (!name.isEmpty()) {
        msgData.insert("name", name);
    }

    if (!customWsUrl.isEmpty()) {
        msgData.insert("custom_ws_url", customWsUrl);
    }

    if (!token.isEmpty()) {
        msgData.insert("token", token);
    }

    if (!model.isEmpty()) {
        msgData.insert("model", model);
    }

    if (!description.isEmpty()) {
        msgData.insert("description", description);
    }

    return sendRequest(RequestTypes::create_dock, msgData);
}

int Api::deleteAllDocks() {
    return sendRequest(RequestTypes::delete_all_docks);
}

int Api::getDock(const QString& dockId) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);

    return sendRequest(RequestTypes::get_dock, msgData);
}

int Api::updateDock(const QString& dockId, const QString& name, const QString& customWsUrl, const QString& token,
                    bool active, const QString& description, const QString& wifiSsid, const QString& wifiPassword) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);
    msgData.insert("active", active);

    if (!name.isEmpty()) {
        msgData.insert("name", name);
    }

    if (!customWsUrl.isEmpty()) {
        msgData.insert("custom_ws_url", customWsUrl);
    }

    if (!token.isEmpty()) {
        msgData.insert("token", token);
    }

    if (!description.isEmpty()) {
        msgData.insert("description", description);
    }

    if (!wifiSsid.isEmpty()) {
        QVariantMap wifi;
        wifi.insert("ssid", wifiSsid);

        if (!wifiPassword.isEmpty()) {
            wifi.insert("password", wifiPassword);
        }

        msgData.insert("wifi", wifi);
    }

    return sendRequest(RequestTypes::update_dock, msgData);
}

int Api::connectDock(const QString& dockId) {
    QVariantMap msgData;
    if (!dockId.isEmpty()) {
        msgData.insert("dock_id", dockId);
    }
    msgData.insert("cmd", "CONNECT");

    return sendRequest(RequestTypes::dock_connection_command, msgData);
}

int Api::disconnectDock(const QString& dockId) {
    QVariantMap msgData;
    if (!dockId.isEmpty()) {
        msgData.insert("dock_id", dockId);
    }
    msgData.insert("cmd", "DISCONNECT");

    return sendRequest(RequestTypes::dock_connection_command, msgData);
}

int Api::deleteDock(const QString& dockId) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);

    return sendRequest(RequestTypes::delete_dock, msgData);
}

int Api::dockCommand(const QString& dockId, DockEnums::DockCommands command, const QString& value,
                     const QString& token) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);
    msgData.insert("command", Util::convertEnumToString(command));

    if (!value.isEmpty()) {
        msgData.insert("value", value);
    }

    if (!token.isEmpty()) {
        msgData.insert("token", token);
    }

    return sendRequest(RequestTypes::dock_command, msgData);
}

int Api::getDockDiscoveryStatus() {
    return sendRequest(RequestTypes::get_dock_discovery_status);
}

int Api::getDockDiscoveryDevice(const QString& dockId) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);

    return sendRequest(RequestTypes::get_dock_discovery_device, msgData);
}

int Api::execCommandOnDiscoveredDock(const QString& dockId, DockSetupEnums::DockCommands command, const QString& token,
                                     int timeout) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);
    msgData.insert("cmd", Util::convertEnumToString(command));
    msgData.insert("timeout", timeout);

    if (!token.isEmpty()) {
        msgData.insert("token", token);
    }

    return sendRequest(RequestTypes::exec_cmd_on_discovered_dock, msgData);
}

int Api::getDockSetupProcesses() {
    return sendRequest(RequestTypes::get_dock_setup_processes);
}

int Api::getDockSetupStatus(const QString& dockId) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);
    return sendRequest(RequestTypes::get_dock_setup_status, msgData);
}

int Api::startDockDiscovery(int timeOut, bool bt, bool net, bool filterNew) {
    QVariantMap msgData;
    if (timeOut != 30) {
        msgData.insert("timeout", timeOut);
    }
    if (!bt) {
        msgData.insert("bt", bt);
    }
    if (!net) {
        msgData.insert("net", net);
    }
    if (!filterNew) {
        msgData.insert("new", filterNew);
    }
    return sendRequest(RequestTypes::start_dock_discovery, msgData);
}

int Api::stopDockDiscovery() {
    return sendRequest(RequestTypes::stop_dock_discovery);
}

int Api::createDockSetup(const QString& id, const QString& friendlyName,
                         DockSetupEnums::DockDiscoveryType discoveryType) {
    QVariantMap dockData;
    dockData.insert("id", id);
    dockData.insert("friendly_name", friendlyName);
    dockData.insert("discovery_type", Util::convertEnumToString(discoveryType));

    QVariantMap msgData;
    msgData.insert("discovery", dockData);
    return sendRequest(RequestTypes::create_dock_setup, msgData);
}

int Api::startDockSetup(const QString& id, const QString& friendlyName, const QString& password,
                        const QString& wifiSsid, const QString& wifiPassword) {
    QVariantMap msgData;
    msgData.insert("dock_id", id);
    msgData.insert("name", friendlyName);
    if (!password.isEmpty()) {
        msgData.insert("token", password);
    }
    if (!wifiSsid.isEmpty()) {
        QVariantMap wifiMap;
        wifiMap.insert("ssid", wifiSsid);
        wifiMap.insert("password", wifiPassword);
        msgData.insert("wifi", wifiMap);
    }

    return sendRequest(RequestTypes::start_dock_setup, msgData);
}

int Api::stopDockSetup(const QString& dockId) {
    if (dockId.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("dock_id", dockId);
    return sendRequest(RequestTypes::stop_dock_setup, msgData);
}

int Api::stopAllDockSetups() {
    return sendRequest(RequestTypes::stop_all_dock_setups);
}

int Api::getFactoryResetToken() {
    return sendRequest(RequestTypes::get_factory_reset_token);
}

int Api::factoryReset(const QString& token) {
    QVariantMap msgData;
    msgData.insert("token", token);
    return sendRequest(RequestTypes::factory_reset, msgData);
}

int Api::getApiAccess() {
    return sendRequest(RequestTypes::get_api_access);
}

int Api::setApiAccess(bool enabled, const QString& pin, QDateTime validTo) {
    Q_UNUSED(validTo)

    QVariantMap data;
    data.insert("enabled", enabled);

    if (!pin.isEmpty()) {
        data.insert("pin", pin);
    }

    QVariantMap msgData;
    msgData.insert("web_configurator", data);
    return sendRequest(RequestTypes::set_api_access, msgData);
}

int Api::checkSystemUpdate(bool force) {
    QVariantMap msgData;
    msgData.insert("force_update", force);
    return sendRequest(RequestTypes::check_system_update, msgData);
}

int Api::updateSystem(const QString& updateId) {
    QVariantMap msgData;
    msgData.insert("update_id", updateId);
    return sendRequest(RequestTypes::update_system, msgData);
}

int Api::getPowerMode() {
    return sendRequest(RequestTypes::get_power_mode);
}

bool Api::sendMessage(const QString& message) {
    if (m_webSocket.sendTextMessage(message) > 0) {
        return true;
    } else {
        return false;
    }
}

void Api::onTextMessageReceived(const QString& message) {
    // if not a valid socket, do nothing
    if (!m_webSocket.isValid()) {
        qCWarning(lcCore()) << "Invalid socket, dropping message";
        return;
    }

//    qCDebug(lcCore()).noquote() << message.simplified();

    // Parse message to JSON
    QJsonParseError parseerror;
    QJsonDocument   doc = QJsonDocument::fromJson(message.toUtf8(), &parseerror);
    if (parseerror.error != QJsonParseError::NoError) {
        qCCritical(lcCore()) << "JSON error:" << parseerror.errorString();
        return;
    }

    QVariantMap map = doc.toVariant().toMap();
    QString     kind = map.value("kind").toString();

    // process event message
    if (kind == "event") {
        processEventMessage(map);
    }

    // process response message
    if (kind == "resp") {
        processResponseMessage(map);
    }

    // process request message
    if (kind == "req") {
        processRequestMessage(map);
    }
}

void Api::onStateChanged(QAbstractSocket::SocketState state) {
    qCDebug(lcCore()) << "State:" << state;

    switch (state) {
        case QAbstractSocket::ConnectedState:
            if (m_reconnectTimer->isActive()) {
                m_reconnectTimer->stop();
                m_reconnectTries = 0;
                qCDebug(lcCore()) << "Reconnect timer stopped";
            }
            //            emit connected();
            break;
        case QAbstractSocket::UnconnectedState:
            emit disconnected();
            m_connected = false;
            m_reconnectTimer->start();
            break;
        default:
            break;
    }
}

void Api::onError(QAbstractSocket::SocketError error) {
    qCWarning(lcCore()) << "Error: " << error << m_webSocket.errorString();

    if (m_webSocket.isValid()) {
        m_webSocket.close();
    }

    // always try to reconnect on error
    qCDebug(lcCore()) << "Trying to reconnect";
    m_reconnectTimer->start();
}

void Api::onKeepAliveTimerTimeout() {
    qCDebug(lcCore()) << "Keepalive ping sent";
    m_webSocket.ping();
}

void Api::startKeepAliveTimer() {
    qCDebug(lcCore()) << "Starting keepalive timer";
    m_keepAliveTimer->start();
}

void Api::stopKeepAliveTimer() {
    qCDebug(lcCore()) << "Stopping keepalive timer";
    m_keepAliveTimer->stop();
}

void Api::onReconnectTimerTimeout() {
    qCDebug(lcCore()) << "Trying to reconnect";

    m_reconnectTries++;
    if (m_reconnectTries == 10) {
        emit connectionProblem();
        ui::Notification::createActionableWarningNotification(
            tr("Connection error"),
            tr("There was an error connecting to the core. If the issue persists, restart the remote."));
    }

    connect();
}

int Api::entityCommand(const QString& entityId, const QString& cmd, QVariantMap params) {
    if (entityId.isEmpty() || cmd.isEmpty()) {
        return -1;
    }

    QVariantMap msgData;
    msgData.insert("entity_id", entityId);
    msgData.insert("cmd_id", cmd);

    if (params.count() > 0) {
        msgData.insert("params", params);
    }

    return sendRequest(RequestTypes::execute_entity_command, msgData);
}

void Api::processEventMessage(QVariantMap map) {
    MsgEvent::Enum event = Util::convertStringToEnum<MsgEvent::Enum>(map.value("msg").toString());
    auto           msgData = map.value("msg_data").toMap();

    qCDebug(lcCore()) << "We got an event" << event;

    switch (event) {
        case MsgEvent::auth_required: {
            authenticate();
            break;
        }
        case MsgEvent::warning: {
            processWarning(msgData);
            break;
        }
        case MsgEvent::entity_change: {
            processEntityChange(msgData);
            break;
        }
        case MsgEvent::wifi_change: {
            processWifiChange(msgData);
            break;
        }
        case MsgEvent::integration_driver_change: {
            processIntegrationDriverChange(msgData);
            break;
        }
        case MsgEvent::integration_change: {
            processIntegrationChange(msgData);
            break;
        }
        case MsgEvent::integration_state: {
            if (msgData.contains("integration_id")) {
                emit integrationDeviceStateChanged(msgData.value("integration_id").toString(),
                                                   msgData.value("driver_id").toString(),
                                                   msgData.value("device_state").toString());
            } else {
                emit integrationDriverStateChanged(msgData.value("driver_id").toString(),
                                                   msgData.value("driver_state").toString());
            }
            break;
        }
        case MsgEvent::profile_change: {
            processProfileChange(msgData);
            break;
        }
        case MsgEvent::configuration_change: {
            processConfigChange(msgData);
            break;
        }
        case MsgEvent::ir_learning: {
            // TODO(marton): Implement me
            break;
        }
        case MsgEvent::dock_change: {
            processDockChange(msgData);
            break;
        }
        case MsgEvent::dock_state: {
            processDockStateChange(msgData);
            break;
        }
        case MsgEvent::dock_discovery: {
            processDockDiscoveryChange(msgData);
            break;
        }
        case MsgEvent::dock_setup_change: {
            processDockSetupChange(msgData);
            break;
        }
        case MsgEvent::dock_update_change: {
            processDockUpdateChange(msgData);
            break;
        }
        case MsgEvent::integration_discovery: {
            processIntegrationDiscoveryChange(msgData);
            break;
        }
        case MsgEvent::integration_setup_change: {
            processIntegrationSetupChange(msgData);
            break;
        }
        case MsgEvent::software_update: {
            processSoftwareUpdateChange(msgData);
            break;
        }
        case MsgEvent::power_mode_change: {
            processPowerModeChange(msgData);
            break;
        }
        case MsgEvent::battery_status: {
            processBatteryStatusChange(msgData);
            break;
        }
        default:
            break;
    }
}

void Api::processResponseMessage(QVariantMap map) {
    MsgResponse::Enum resp = Util::convertStringToEnum<MsgResponse::Enum>(map.value("msg").toString());

    int  reqId = map.value("req_id").toInt();
    int  code = map.value("code").toInt();
    auto msgData = map.value("msg_data");

    removeRequestTimer(reqId);

    qCDebug(lcCore()) << "We got a response"
                      << "id:" << reqId << "code:" << code << resp;

    switch (resp) {
        case MsgResponse::authentication: {
            processAuthResult(reqId, code, msgData);
            break;
        }
        case MsgResponse::pong: {
            // TODO(marton): Implement me
            break;
        }
        case MsgResponse::result:
        case MsgResponse::button_cfg:
        case MsgResponse::display_cfg:
        case MsgResponse::device_cfg:
        case MsgResponse::haptic_cfg:
        case MsgResponse::localization_cfg:
        case MsgResponse::network_cfg:
        case MsgResponse::software_update_cfg:
        case MsgResponse::power_saving_cfg:
        case MsgResponse::profile_cfg:
        case MsgResponse::sound_cfg:
        case MsgResponse::voice_control_cfg:
        case MsgResponse::dock_discovery_status:
        case MsgResponse::dock_discovery_device: {
            processResponseResult(reqId, code, msgData);
            break;
        }
        case MsgResponse::version_info: {
            processResponseVersionInfo(reqId, code, msgData);
            break;
        }
        case MsgResponse::system_info: {
            processResponseSystemInfo(reqId, code, msgData);
            break;
        }
        case MsgResponse::factory_reset_token: {
            processFactoryResetTokent(reqId, code, msgData);
            break;
        }
        case MsgResponse::api_access: {
            processApiAccess(reqId, code, msgData);
            break;
        }
        case MsgResponse::entity_types: {
            // TODO(marton): Implement me
            break;
        }
        case MsgResponse::entities: {
            processResponseEntities(reqId, code, msgData);
            break;
        }
        case MsgResponse::available_entities: {
            processResponseAvailableEntities(reqId, code, msgData);
            break;
        }
        case MsgResponse::entity_features: {
            // TODO(marton): Implement me
            break;
        }
        case MsgResponse::entity_commands: {
            // TODO(marton): Implement me
            break;
        }
        case MsgResponse::entity: {
            processResponseEntity(reqId, code, msgData);
            break;
        }
        case MsgResponse::profile: {
            processResponseProfile(reqId, code, msgData);
            break;
        }
        case MsgResponse::profiles: {
            processResponseProfiles(reqId, code, msgData);
            break;
        }
        case MsgResponse::page: {
            processResponsePage(reqId, code, msgData);
            break;
        }
        case MsgResponse::pages: {
            processResponsePages(reqId, code, msgData);
            break;
        }
        case MsgResponse::group: {
            processResponseGroup(reqId, code, msgData);
            break;
        }
        case MsgResponse::groups: {
            processResponseGroups(reqId, code, msgData);
            break;
        }

        case MsgResponse::integration_status: {
            processResponseIntegrationStatus(reqId, code, msgData);
            break;
        }
        case MsgResponse::integration_driver_count: {
            // TODO(marton): Implement me
            break;
        }
        case MsgResponse::integration_drivers: {
            processResponseIntegrationDrivers(reqId, code, msgData);
            break;
        }
        case MsgResponse::integration_driver: {
            processResponseIntegrationDriver(reqId, code, msgData);
            break;
        }
        case MsgResponse::integration_count: {
            // TODO(marton): Implement me
            break;
        }
        case MsgResponse::integrations: {
            processResponseIntegrations(reqId, code, msgData);
            break;
        }
        case MsgResponse::integration: {
            // TODO(marton): Implement me
            break;
        }
        case MsgResponse::integration_setup_info: {
            processResponseIntegrationSetupInfo(reqId, code, msgData);
            break;
        }

        case MsgResponse::configuration: {
            processResponseConfig(reqId, code, msgData);
            break;
        }

        case MsgResponse::voice_assistants: {
            // TODO(marton): implement me
            break;
        }

        case MsgResponse::timezone_names: {
            processResponseTimeZoneNames(reqId, code, msgData);
            break;
        }

        case MsgResponse::localization_countries: {
            processResponseLocalizationCountires(reqId, code, msgData);
            break;
        }

        case MsgResponse::localization_languages: {
            processResponseLocalizationLanguages(reqId, code, msgData);
            break;
        }

        case MsgResponse::wifi_status: {
            processWifiStatus(reqId, code, msgData);
            break;
        }
        case MsgResponse::wifi_scan_status: {
            processWifiScanStatus(reqId, code, msgData);
            break;
        }
        case MsgResponse::wifi_networks: {
            processWifiNetworks(reqId, code, msgData);
            break;
        }
        case MsgResponse::wifi_network: {
            processWifiNetwork(reqId, code, msgData);
            break;
        }
        case MsgResponse::dock_count: {
            processResponseDockCount(reqId, code, msgData);
            break;
        }
        case MsgResponse::docks: {
            processResponseDocks(reqId, code, msgData);
            break;
        }
        case MsgResponse::dock: {
            processResponseDock(reqId, code, msgData);
            break;
        }
        case MsgResponse::dock_system_info: {
            processResponseDockSystemInfo(reqId, code, msgData);
            break;
        }
        case MsgResponse::dock_setup_processes: {
            processResponseDockSetupProcesses(reqId, code, msgData);
            break;
        }
        case MsgResponse::dock_setup_status: {
            processResponseDockSetupStatus(reqId, code, msgData);
            break;
        }
        case MsgResponse::system_update_info: {
            processResponseSystemUpdateInfo(reqId, code, msgData);
            break;
        }
        case MsgResponse::power_mode: {
            processResponsePowerMode(reqId, code, msgData);
            break;
        }
        default:
            break;
    }
}

void Api::processRequestMessage(QVariantMap map)
{
    RequestTypes::Enum req = Util::convertStringToEnum<RequestTypes::Enum>(map.value("msg").toString());

    int  reqId = map.value("id").toInt();
    auto msgData = map.value("msg_data");

    switch (req) {
        case RequestTypes::get_localization_languages:
            processRequestGetLocalizationLanguages(reqId);
            break;
        default:
            break;
    }
}

void Api::setupTimerForRequest(int requestId) {
    QTimer* timer = new QTimer(this);
    timer->setInterval(m_requestTimeout);
    timer->setSingleShot(true);

    QObject::connect(timer, &QTimer::timeout, this, [=] {
        qCDebug(lcCore()) << "Request timeout:" << requestId;
        emit respResult(requestId, 408, "Request timed out");
        removeRequestTimer(requestId);
    });

    timer->start();

    m_timeoutTimers.insert(requestId, timer);
}

void Api::removeRequestTimer(int requestId) {
    if (m_timeoutTimers.contains(requestId)) {
        auto timer = m_timeoutTimers.value(requestId);
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
        m_timeoutTimers.remove(requestId);
        qCDebug(lcCore()) << "Request timeout timer removed:" << requestId;
    } else {
        qCDebug(lcCore()) << "Request timeout timer was already removed:" << requestId;
    }
}

void Api::processAuthResult(int reqId, int code, QVariant msgData) {
    Q_UNUSED(reqId)
    Q_UNUSED(code)

    if (code == 200) {
        qCDebug(lcCore()) << "Authentication successful";

        m_connected = true;
        emit connected();
    } else {
        QVariantMap result = msgData.toMap();

        qCCritical(lcCore()) << "Authentication failed" << result.value("message").toString();
        ui::Notification::createNotification(tr("Authentication to core failed"), true);
    }
}

void Api::processResponseResult(int reqId, int code, QVariant msgData) {
    QVariantMap result = msgData.toMap();

    emit respResult(reqId, code, result.value("message").toString());
}

void Api::processResponseVersionInfo(int reqId, int code, QVariant msgData) {
    QVariantMap result = msgData.toMap();

    QStringList integrations;
    auto        list = msgData.toMap().value("integrations").toMap();

    if (list.size() > 0) {
        for (QVariantMap::iterator i = list.begin(); i != list.end(); i++) {
            QString val;
            val.append(i.key()).append(":").append(i.value().toString());
            integrations << val;
        }
    }

    emit respVersion(reqId, code, result.value("device_name").toString(), result.value("api").toString(),
                     result.value("core").toString(), result.value("ui").toString(), result.value("os").toString(),
                     integrations);
}

void Api::processResponseSystemInfo(int reqId, int code, QVariant msgData) {
    QVariantMap result = msgData.toMap();

    emit respSystem(reqId, code, result.value("model_name").toString(), result.value("model_number").toString(),
                    result.value("serial_number").toString(), result.value("hw_revision").toString());
}

void Api::processResponseProfiles(int reqId, int code, QVariant msgData) {
    QList<Profile> list;
    QVariantList   msgDataList = msgData.toList();

    if (msgDataList.size() > 0) {
        for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
            struct Profile profile;
            QVariantMap    map = i->toMap();

            profile.id = map.value("profile_id").toString();
            profile.name = map.value("name").toString();
            if (map.contains("icon")) {
                profile.icon = map.value("icon").toString();
            }
            profile.restricted = map.value("restricted").toBool();

            if (map.contains("description")) {
                profile.description = map.value("description").toString();
            }
            if (map.contains("pages")) {
                profile.pages = map.value("description").toStringList();
            }

            list.append(profile);
        }
    }

    emit respProfiles(reqId, code, list);
}

void Api::processResponseProfile(int reqId, int code, QVariant msgData) {
    struct Profile profile;
    QVariantMap    map = msgData.toMap();

    profile.id = map.value("profile_id").toString();
    profile.name = map.value("name").toString();
    if (map.contains("icon")) {
        profile.icon = map.value("icon").toString();
    }
    profile.restricted = map.value("restricted").toBool();

    if (map.contains("description")) {
        profile.description = map.value("description").toString();
    }
    if (map.contains("pages")) {
        profile.pages = map.value("description").toStringList();
    }

    emit respProfile(reqId, code, profile);
}

void Api::processResponsePages(int reqId, int code, QVariant msgData) {
    QVariantList msgDataList = msgData.toList();
    QList<Page>  respList;

    if (msgDataList.size() > 0) {
        for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
            struct Page page;
            QVariantMap map = i->toMap();

            page.name = map.value("name").toString();
            page.id = map.value("page_id").toString();
            page.profileId = map.value("profile_id").toString();
            if (map.contains("image")) {
                page.image = map.value("image").toString();
            }
            page.pos = map.value("pos").toInt();

            QVariantList items = map.value("items").toList();

            if (items.size() > 0) {
                for (QVariantList::iterator j = items.begin(); j != items.end(); j++) {
                    PageItem pageItem;

                    QVariantMap itemMap = j->toMap();

                    if (itemMap.contains("entity_id")) {
                        pageItem.type = "Entity";
                        pageItem.id = itemMap.value("entity_id").toString();
                    }

                    if (itemMap.contains("group_id")) {
                        pageItem.type = "Group";
                        pageItem.id = itemMap.value("group_id").toString();
                    }
                    page.items.append(pageItem);
                }
            }

            //                page.items = map.value("items").toList();

            respList.append(page);
        }
    }

    emit respPages(reqId, code, respList);
}

void Api::processResponsePage(int reqId, int code, QVariant msgData) {
    struct Page page;
    QVariantMap map = msgData.toMap();

    page.id = map.value("page_id").toString();
    page.profileId = map.value("profile_id").toString();
    page.name = map.value("name").toString();
    if (map.contains("image")) {
        page.image = map.value("image").toString();
    }
    page.pos = map.value("pos").toInt();

    QVariantList items = map.value("items").toList();

    if (items.size() > 0) {
        for (QVariantList::iterator j = items.begin(); j != items.end(); j++) {
            PageItem pageItem;

            QVariantMap itemMap = j->toMap();

            if (itemMap.contains("entity_id")) {
                pageItem.type = "Entity";
                pageItem.id = itemMap.value("entity_id").toString();
            }

            if (itemMap.contains("group_id")) {
                pageItem.type = "Group";
                pageItem.id = itemMap.value("group_id").toString();
            }
            page.items.append(pageItem);
        }
    }
    //            page.items = map.value("items").toStringList();

    emit respPage(reqId, code, page);
}

void Api::processResponseAvailableEntities(int reqId, int code, QVariant msgData) {
    qCDebug(lcCore()) << "Available entities response";

    QVariantList  msgDataList = msgData.toMap().value("available_entities").toList();
    QVariantMap   paging = msgData.toMap().value("paging").toMap();
    QList<Entity> respList;

    if (msgDataList.size() > 0) {
        for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
            struct Entity entity;
            QVariantMap   map = i->toMap();

            entity.id = map.value("entity_id").toString();
            entity.type = uc::Util::FirstToUpper(map.value("entity_type").toString());
            entity.name = map.value("name").toMap();
            if (map.contains("icon")) {
                entity.icon = map.value("icon").toString();
            }
            entity.integrationId = map.value("integration_id").toString();
            if (map.contains("features")) {
                entity.features = uc::Util::FirstToUpperList(map.value("features").toStringList());
            }
            if (map.contains("area")) {
                entity.area = map.value("area").toString();
            }
            if (map.contains("device_class")) {
                entity.deviceClass = uc::Util::FirstToUpper(map.value("device_class").toString());
            }

            respList.append(entity);
        }
    }

    emit respAvailableEntities(reqId, code, respList, paging.value("count").toInt(), paging.value("limit").toInt(),
                               paging.value("page").toInt());
}

void Api::processResponseEntity(int reqId, int code, QVariant msgData) {
    struct Entity entity;

    QVariantMap map = msgData.toMap();
    if (map.isEmpty()) {
        QStringList entities = msgData.toStringList();
        emit        respResult(reqId, code, QString());
        return;
    }

    entity.id = map.value("entity_id").toString();
    entity.type = uc::Util::FirstToUpper(map.value("entity_type").toString());
    entity.name = map.value("name").toMap();
    if (map.contains("icon")) {
        entity.icon = map.value("icon").toString();
    }
    entity.integrationId = map.value("integration_id").toString();
    if (map.contains("features")) {
        entity.features = uc::Util::FirstToUpperList(map.value("features").toStringList());
    }
    if (map.contains("area")) {
        entity.area = map.value("area").toString();
    }
    if (map.contains("enabled")) {
        entity.enabled = map.value("enabled").toBool();
    }
    if (map.contains("device_class")) {
        entity.deviceClass = uc::Util::FirstToUpper(map.value("device_class").toString());
    }
    if (map.contains("options")) {
        entity.options = map.value("options").toMap();
    }
    if (map.contains("attributes")) {
        entity.attributes = map.value("attributes").toMap();
    }

    emit respEntity(reqId, code, entity);
}

void Api::processResponseEntities(int reqId, int code, QVariant msgData) {
    qCDebug(lcCore()) << "Configured entities response";

    QVariantList  msgDataList = msgData.toMap().value("entities").toList();
    QVariantMap   paging = msgData.toMap().value("paging").toMap();
    QList<Entity> respList;

    if (msgDataList.size()) {
        for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
            struct Entity entity;
            QVariantMap   map = i->toMap();

            entity.id = map.value("entity_id").toString();
            entity.type = uc::Util::FirstToUpper(map.value("entity_type").toString());
            entity.name = map.value("name").toMap();
            if (map.contains("icon")) {
                entity.icon = map.value("icon").toString();
            }
            entity.integrationId = map.value("integration_id").toString();
            if (map.contains("features")) {
                entity.features = uc::Util::FirstToUpperList(map.value("features").toStringList());
            }
            if (map.contains("area")) {
                entity.area = map.value("area").toString();
            }
            if (map.contains("enabled")) {
                entity.enabled = map.value("enabled").toBool();
            }
            if (map.contains("device_class")) {
                entity.deviceClass = uc::Util::FirstToUpper(map.value("device_class").toString());
            }
            if (map.contains("options")) {
                entity.options = map.value("options").toMap();
            }
            if (map.contains("attributes")) {
                entity.attributes = map.value("attributes").toMap();
            }

            respList.append(entity);
        }
    }

    emit respEntities(reqId, code, respList, paging.value("count").toInt(), paging.value("limit").toInt(),
                      paging.value("page").toInt());
}

void Api::processEntityChange(QVariant msgData) {
    QVariantMap msgDataMap = msgData.toMap();
    QVariantMap newState = msgDataMap.value("new_state").toMap();

    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());
    struct Entity entity;

    if (!newState.isEmpty()) {
        entity.id = msgDataMap.value("entity_id").toString();
        entity.type = uc::Util::FirstToUpper(msgDataMap.value("entity_type").toString());
        entity.name = newState.value("name").toMap();
        if (newState.contains("icon")) {
            entity.icon = newState.value("icon").toString();
        }
        entity.integrationId = newState.value("integration_id").toString();

        if (newState.contains("features")) {
            entity.features = uc::Util::FirstToUpperList(newState.value("features").toStringList());
        }
        if (newState.contains("area")) {
            entity.area = newState.value("area").toString();
        }
        if (newState.contains("enabled")) {
            entity.enabled = newState.value("enabled").toBool();
        }
        if (newState.contains("device_class")) {
            entity.deviceClass = uc::Util::FirstToUpper(newState.value("device_class").toString());
        }
        if (newState.contains("options")) {
            entity.options = newState.value("options").toMap();
        }
        if (newState.contains("attributes")) {
            entity.attributes = newState.value("attributes").toMap();
        }
    }

    switch (eventType) {
        case MsgEventTypes::NEW:
            if (entity.id.isEmpty()) {
                emit reloadEntities();
            } else {
                emit entityAdded(entity);
            }
            break;
        case MsgEventTypes::CHANGE:
            qCDebug(lcCore()) << "ENTITY CHANGE" << entity.id;
            if (entity.id.isEmpty()) {
                emit reloadEntities();
            } else {
                emit entityChanged(entity.id, entity);
            }
            break;
        case MsgEventTypes::DELETE:
            if (!msgDataMap.contains("entity_id")) {
                emit reloadEntities();
            } else {
                emit entityDeleted(msgDataMap.value("entity_id").toString());
            }
            break;
        default:
            break;
    }
}

void Api::processWifiChange(QVariant msgData) {
    QVariantMap     msgDataMap = msgData.toMap();
    WifiEvent::Enum wifiEvent = Util::convertStringToEnum<WifiEvent::Enum>(msgDataMap.value("event").toString());

    emit wifiEventChanged(wifiEvent);
}

void Api::processResponseIntegrationStatus(int reqId, int code, QVariant msgData) {
    qCDebug(lcCore()) << "Integration status response";

    QVariantList             msgDataList = msgData.toMap().value("status").toList();
    QVariantMap              paging = msgData.toMap().value("paging").toMap();
    QList<IntegrationStatus> respList;

    if (msgDataList.size() > 0) {
        for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
            struct IntegrationStatus integrationStatus;
            QVariantMap              map = i->toMap();

            integrationStatus.integrationId = map.value("integration_id").toString();
            integrationStatus.name = map.value("name").toMap().value("en").toString();
            if (map.contains("icon")) {
                integrationStatus.icon = map.value("icon").toString();
            }
            integrationStatus.deviceState = map.value("device_state").toString();
            integrationStatus.driverState = map.value("driver_state").toString();
            integrationStatus.enabled = map.value("enabled").toBool();

            respList.append(integrationStatus);
        }
    }

    emit respIntegrationStatus(reqId, code, respList, paging.value("count").toInt(), paging.value("limit").toInt(),
                               paging.value("page").toInt());
}

void Api::processResponseIntegrations(int reqId, int code, QVariant msgData) {
    qCDebug(lcCore()) << "Integrations response";

    QVariantList       msgDataList = msgData.toMap().value("integrations").toList();
    QVariantMap        paging = msgData.toMap().value("paging").toMap();
    QList<Integration> respList;

    if (msgDataList.size() > 0) {
        for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
            struct Integration integration;
            QVariantMap        map = i->toMap();

            integration.id = map.value("integration_id").toString();
            integration.driverId = map.value("driver_id").toString();
            if (map.contains("device_id")) {
                integration.deviceId = map.value("device_id").toString();
            }
            integration.name = map.value("name").toMap();
            if (map.contains("icon")) {
                integration.icon = map.value("icon").toString();
            }
            integration.enabled = map.value("enabled").toBool();
            if (map.contains("setup_data")) {
                integration.setupData = map.value("setup_data").toMap();
            }

            respList.append(integration);
        }
    }

    emit respIntegrations(reqId, code, respList, paging.value("count").toInt(), paging.value("limit").toInt(),
                          paging.value("page").toInt());
}

void Api::processResponseIntegrationDrivers(int reqId, int code, QVariant msgData) {
    qCDebug(lcCore()) << "Integration drivers response";

    QVariantList             msgDataList = msgData.toMap().value("drivers").toList();
    QVariantMap              paging = msgData.toMap().value("paging").toMap();
    QList<IntegrationDriver> respList;

    for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
        struct IntegrationDriver integrationDriver;
        QVariantMap              map = i->toMap();

        integrationDriver.id = map.value("driver_id").toString();
        integrationDriver.name = map.value("name").toMap();
        integrationDriver.driverUrl = map.value("driver_url").toString();
        integrationDriver.version = map.value("version").toString();
        integrationDriver.min_core_api = map.value("min_core_api").toString();
        integrationDriver.icon = map.value("icon").toString();
        integrationDriver.enabled = map.value("enabled").toBool();
        integrationDriver.description = map.value("description").toString();

        struct DriverDeveloper driverDeveloper;
        QVariantMap            developer = map.value("developer").toMap();
        driverDeveloper.name = developer.value("name").toString();
        driverDeveloper.url = developer.value("url").toString();
        driverDeveloper.email = developer.value("email").toString();

        integrationDriver.developer = driverDeveloper;
        integrationDriver.homePage = map.value("home_page").toString();
        integrationDriver.deviceDiscovery = map.value("device_discovery").toBool();

        QVariantMap         settingsPageMap = map.value("setup_data_schema").toMap();
        struct SettingsPage settingsPage;
        settingsPage.title = settingsPageMap.value("title").toMap();
        settingsPage.settings = settingsPageMap.value("settings").toList();

        integrationDriver.settingsPage = settingsPage;
        integrationDriver.releaseDate = map.value("release_date").toString();
        integrationDriver.state =
            Util::convertStringToEnum<IntegrationDriverEnums::States>(map.value("driver_state").toString());

        integrationDriver.external = map.value("driver_type").toString().contains("EXTERNAL") ? true : false;
        integrationDriver.instanceCount = map.value("instance_count").toInt();

        respList.append(integrationDriver);
    }

    emit respIntegrationDrivers(reqId, code, respList, paging.value("count").toInt(), paging.value("limit").toInt(),
                                paging.value("page").toInt());
}

void Api::processResponseIntegrationDriver(int reqId, int code, QVariant msgData) {
    QVariantMap              msgDataMap = msgData.toMap();
    struct IntegrationDriver integrationDriver;

    integrationDriver.id = msgDataMap.value("driver_id").toString();
    integrationDriver.name = msgDataMap.value("name").toMap();
    integrationDriver.driverUrl = msgDataMap.value("driver_url").toString();
    integrationDriver.version = msgDataMap.value("version").toString();
    integrationDriver.min_core_api = msgDataMap.value("min_core_api").toString();
    integrationDriver.icon = msgDataMap.value("icon").toString();
    integrationDriver.enabled = msgDataMap.value("enabled").toBool();
    integrationDriver.description = msgDataMap.value("description").toString();

    struct DriverDeveloper driverDeveloper;
    QVariantMap            developer = msgDataMap.value("developer").toMap();
    driverDeveloper.name = developer.value("name").toString();
    driverDeveloper.url = developer.value("url").toString();
    driverDeveloper.email = developer.value("email").toString();

    integrationDriver.developer = driverDeveloper;
    integrationDriver.homePage = msgDataMap.value("home_page").toString();
    integrationDriver.deviceDiscovery = msgDataMap.value("device_discovery").toBool();

    QVariantMap         settingsPageMap = msgDataMap.value("setup_data_schema").toMap();
    struct SettingsPage settingsPage;
    settingsPage.title = settingsPageMap.value("title").toMap();
    settingsPage.settings = settingsPageMap.value("settings").toList();

    integrationDriver.settingsPage = settingsPage;
    integrationDriver.releaseDate = msgDataMap.value("release_date").toString();
    integrationDriver.state =
        Util::convertStringToEnum<IntegrationDriverEnums::States>(msgDataMap.value("driver_state").toString());

    integrationDriver.external = msgDataMap.value("driver_type").toString().contains("EXTERNAL") ? true : false;
    integrationDriver.instanceCount = msgDataMap.value("instance_count").toInt();

    emit respIntegrationDriver(reqId, code, integrationDriver);
}

void Api::processResponseIntegrationSetupInfo(int reqId, int code, QVariant msgData) {
    QVariantMap                 msgDataMap = msgData.toMap();
    struct IntegrationSetupInfo integrationSetupInfo;

    integrationSetupInfo.id = msgDataMap.value("id").toString();
    integrationSetupInfo.state =
        Util::convertStringToEnum<IntegrationEnums::SetupState>(msgDataMap.value("state").toString());
    integrationSetupInfo.error =
        Util::convertStringToEnum<IntegrationEnums::SetupError>(msgDataMap.value("error").toString());
    integrationSetupInfo.requireUserAction = msgDataMap.contains("require_user_action");

    if (integrationSetupInfo.requireUserAction) {
        QVariantMap reqUserAction = msgDataMap.value("require_user_action").toMap();

        if (reqUserAction.contains("input")) {
            struct SettingsPage settingsPage;
            settingsPage.title = reqUserAction.value("input").toMap().value("title").toMap();
            settingsPage.settings = reqUserAction.value("input").toMap().value("settings").toList();
            integrationSetupInfo.settingsPage = settingsPage;
        } else if (reqUserAction.contains("confirmation")) {
            struct ConfirmationPage confirmationPage;
            confirmationPage.title = reqUserAction.value("confirmation").toMap().value("title").toMap();
            confirmationPage.message1 = reqUserAction.value("confirmation").toMap().value("message1").toMap();
            confirmationPage.image = reqUserAction.value("confirmation").toMap().value("image").toString();
            confirmationPage.message2 = reqUserAction.value("confirmation").toMap().value("message2").toMap();
            integrationSetupInfo.confirmationPage = confirmationPage;
        }
    }

    emit respIntegrationSetupInfo(reqId, code, integrationSetupInfo);
}

void Api::processResponseGroup(int reqId, int code, QVariant msgData) {
    struct Group group;
    QVariantMap  map = msgData.toMap();

    group.id = map.value("group_id").toString();
    group.profileId = map.value("profile_id").toString();
    group.name = map.value("name").toString();
    if (map.contains("icon")) {
        group.icon = map.value("icon").toString();
    }
    group.entities = map.value("entities").toStringList();

    emit respGroup(reqId, code, group);
}

void Api::processResponseGroups(int reqId, int code, QVariant msgData) {
    QVariantList msgDataList = msgData.toList();
    QList<Group> respList;

    if (msgDataList.size() > 0) {
        for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
            struct Group group;
            QVariantMap  map = i->toMap();

            group.id = map.value("group_id").toString();
            group.profileId = map.value("profile_id").toString();
            group.name = map.value("name").toString();
            if (map.contains("icon")) {
                group.icon = map.value("icon").toString();
            }
            group.entities = map.value("entities").toStringList();

            respList.append(group);
        }
    }

    emit respGroups(reqId, code, respList);
}

void Api::processResponseConfig(int reqId, int code, QVariant msgData) {
    qCDebug(lcCore()) << "Config response";

    struct Config config;

    struct cfgButton button;
    button.brightness = msgData.toMap().value("button").toMap().value("brightness").toInt();
    button.autoBrightness = msgData.toMap().value("button").toMap().value("auto_brightness").toBool();
    config.buttonCfg = button;

    struct cfgDisplay display;
    display.brightness = msgData.toMap().value("display").toMap().value("brightness").toInt();
    display.autoBrightness = msgData.toMap().value("display").toMap().value("auto_brightness").toBool();
    config.displayCfg = display;

    struct cfgDevice device;
    device.name = msgData.toMap().value("device").toMap().value("name").toString();
    config.deviceCfg = device;

    struct cfgHaptic haptic;
    haptic.enabled = msgData.toMap().value("haptic").toMap().value("enabled").toBool();
    config.hapticCfg = haptic;

    struct cfgLocalization localization;
    localization.countryCode = msgData.toMap().value("localization").toMap().value("country_code").toString();
    localization.languageCode = msgData.toMap().value("localization").toMap().value("language_code").toString();
    localization.timeFormat24h = msgData.toMap().value("localization").toMap().value("time_format_24h").toBool();
    localization.timezone = msgData.toMap().value("localization").toMap().value("time_zone").toString();
    localization.measurementUnit = msgData.toMap().value("localization").toMap().value("measurement_unit").toString();
    config.localizationCfg = localization;

    struct cfgWifi wifi;
    wifi.wowlan = msgData.toMap().value("network").toMap().value("wifi").toMap().value("wake_on_wlan").toMap().value("enabled").toBool();
    if (msgData.toMap().value("network").toMap().value("wifi").toMap().contains("band")) {
        wifi.bands = msgData.toMap().value("network").toMap().value("wifi").toMap().value("bands").toStringList();
        wifi.band = msgData.toMap().value("network").toMap().value("wifi").toMap().value("band").toString();
    }
    wifi.ipv4Type = msgData.toMap().value("network").toMap().value("wifi").toMap().value("ipv4_type").toString();
    wifi.scanIntervalSec = msgData.toMap().value("network").toMap().value("wifi").toMap().value("scan_interval_sec").toInt();

    struct cfgNetwork network;
    network.bluetoothEnabled = msgData.toMap().value("network").toMap().value("bt_enabled").toBool();
    network.wifiEnabled = msgData.toMap().value("network").toMap().value("wifi_enabled").toBool();
    network.bluetoothMac = msgData.toMap().value("network").toMap().value("bt").toMap().value("address").toString();
    network.wifi = wifi;
    config.networkCfg = network;

    struct cfgPowerSaving powerSaving;
    powerSaving.displayOffSec = msgData.toMap().value("power_saving").toMap().value("display_off_sec").toInt();
    powerSaving.standbySec = msgData.toMap().value("power_saving").toMap().value("standby_sec").toInt();
    powerSaving.wakeupSensitivity = msgData.toMap().value("power_saving").toMap().value("wakeup_sensitivity").toInt();
    config.powerSavingCfg = powerSaving;

    struct cfgSoftwareUpdate softwareUpdate;
    softwareUpdate.autoUpdate = msgData.toMap().value("software_update").toMap().value("auto_update").toBool();
    softwareUpdate.checkForUpdates =
        msgData.toMap().value("software_update").toMap().value("check_for_updates").toBool();
    softwareUpdate.otaWindowStart =
        msgData.toMap().value("software_update").toMap().value("ota_window_start").toString();
    softwareUpdate.otaWindowEnd = msgData.toMap().value("software_update").toMap().value("ota_window_end").toString();
    softwareUpdate.channel = Util::convertStringToEnum<UpdateEnums::UpdateChannel>(
        msgData.toMap().value("software_update").toMap().value("channel").toString());
    config.softwareUpdateCfg = softwareUpdate;

    struct cfgSound sound;
    sound.enabled = msgData.toMap().value("sound").toMap().value("enabled").toBool();
    sound.volume = msgData.toMap().value("sound").toMap().value("volume").toInt();
    config.soundCfg = sound;

    struct cfgVoiceControl voiceControl;
    voiceControl.enabled = msgData.toMap().value("voice_control").toMap().value("enabled").toBool();
    voiceControl.microphoneEnabled = msgData.toMap().value("voice_control").toMap().value("microphone").toBool();
    voiceControl.voiceAsssistant = msgData.toMap().value("voice_control").toMap().value("voice_assistant").toString();
    config.voiceControlCfg = voiceControl;

    emit configChanged(reqId, code, config);
}

void Api::processConfigChange(QVariant msgData) {
    auto newState = msgData.toMap().value("new_state").toMap();

    if (newState.contains("button")) {
        auto data = newState.value("button").toMap();

        struct cfgButton button;
        button.autoBrightness = data.value("auto_brightness").toBool();
        button.brightness = data.value("brightness").toInt();

        emit cfgButtonChanged(button);
    }

    if (newState.contains("display")) {
        auto data = newState.value("display").toMap();

        struct cfgDisplay display;
        display.autoBrightness = data.value("auto_brightness").toBool();
        display.brightness = data.value("brightness").toInt();

        emit cfgDisplayChanged(display);
    }

    if (newState.contains("device")) {
        auto data = newState.value("device").toMap();

        struct cfgDevice device;
        device.name = data.value("name").toString();

        emit cfgDeviceChanged(device);
    }

    if (newState.contains("haptic")) {
        auto data = newState.value("haptic").toMap();

        struct cfgHaptic haptic;
        haptic.enabled = data.value("enabled").toBool();

        emit cfgHapticChanged(haptic);
    }

    if (newState.contains("localization")) {
        auto data = newState.value("localization").toMap();

        struct cfgLocalization localization;
        localization.countryCode = data.value("country_code").toString();
        localization.languageCode = data.value("language_code").toString();
        localization.timeFormat24h = data.value("time_format_24h").toBool();
        localization.timezone = data.value("time_zone").toString();
        localization.measurementUnit = data.value("measurement_unit").toString();

        emit cfgLocalizationChanged(localization);
    }

    if (newState.contains("network")) {
        auto data = newState.value("network").toMap();

        struct cfgWifi wifi;
        wifi.wowlan = data.value("wifi").toMap().value("wake_on_wlan").toMap().value("enabled").toBool();
        if (data.value("wifi").toMap().contains("band")) {
            wifi.bands = data.value("wifi").toMap().value("bands").toStringList();
            wifi.band = data.value("wifi").toMap().value("band").toString();
        }
        wifi.ipv4Type = data.value("wifi").toMap().value("ipv4_type").toString();
        wifi.scanIntervalSec = data.value("wifi").toMap().value("scan_interval_sec").toInt();

        struct cfgNetwork network;
        network.bluetoothEnabled = data.value("bt_enabled").toBool();
        network.wifiEnabled = data.value("wifi_enabled").toBool();
        network.bluetoothMac = data.value("bt").toMap().value("address").toString();
        network.wifi = wifi;

        emit cfgNetworkChanged(network);
    }

    if (newState.contains("software_update")) {
        auto data = newState.value("software_update").toMap();

        struct cfgSoftwareUpdate softwareUpdate;
        softwareUpdate.autoUpdate = data.value("auto_update").toBool();
        softwareUpdate.checkForUpdates = data.value("check_for_updates").toBool();
        softwareUpdate.otaWindowStart = data.value("ota_window_start").toString();
        softwareUpdate.otaWindowEnd = data.value("ota_window_end").toString();
        softwareUpdate.channel =
            Util::convertStringToEnum<UpdateEnums::UpdateChannel>(data.value("channel").toString());

        emit cfgSoftwareUpdateChanged(softwareUpdate);
    }

    if (newState.contains("power_saving")) {
        auto data = newState.value("power_saving").toMap();

        struct cfgPowerSaving powerSaving;
        powerSaving.displayOffSec = data.value("display_off_sec").toInt();
        powerSaving.standbySec = data.value("standby_sec").toInt();
        powerSaving.wakeupSensitivity = data.value("wakeup_sensitivity").toInt();

        emit cfgPowerSavingChanged(powerSaving);
    }

    if (newState.contains("sound")) {
        auto data = newState.value("sound").toMap();

        struct cfgSound sound;
        sound.enabled = data.value("enabled").toBool();
        sound.volume = data.value("volume").toInt();

        emit cfgSoundChanged(sound);
    }

    if (newState.contains("voice_control")) {
        auto data = newState.value("voice_control").toMap();

        struct cfgVoiceControl voiceControl;
        voiceControl.enabled = data.value("enabled").toBool();
        voiceControl.microphoneEnabled = data.value("microphone").toBool();
        voiceControl.voiceAsssistant = data.value("voice_assistant").toString();

        emit cfgVoiceControlChanged(voiceControl);
    }
}

void Api::processResponseTimeZoneNames(int reqId, int code, QVariant msgData) {
    QStringList list = msgData.toStringList();
    emit        respTimeZoneNames(reqId, code, list);
}

void Api::processResponseLocalizationCountires(int reqId, int code, QVariant msgData) {
    QVariantList list = msgData.toList();
    emit         respLocalizationCountries(reqId, code, list);
}

void Api::processResponseLocalizationLanguages(int reqId, int code, QVariant msgData) {
    QStringList list = msgData.toStringList();
    emit        respLocalizationLanguages(reqId, code, list);
}

void Api::processFactoryResetTokent(int reqId, int code, QVariant msgData) {
    emit respFactoryResetToken(reqId, code, msgData.toMap().value("token").toString());
}

void Api::processApiAccess(int reqId, int code, QVariant msgData) {
    QVariantMap      webConfigurator = msgData.toMap().value("web_configurator").toMap();
    struct ApiAccess apiAcces;

    apiAcces.enabled = webConfigurator.value("enabled").toBool();
    if (webConfigurator.contains("valid_to")) {
        apiAcces.validTo = webConfigurator.value("valid_to").toDateTime();
    }

    emit respApiAccess(reqId, code, apiAcces);
}

void Api::processProfileChange(QVariant msgData) {
    QVariantMap msgDataMap = msgData.toMap();
    QVariantMap newState = msgDataMap.value("new_state").toMap();

    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());
    QString        profileId = msgDataMap.value("profile_id").toString();
    QString        pageId = msgDataMap.contains("page_id") ? msgDataMap.value("page_id").toString() : QString();
    QString        groupId = msgDataMap.contains("group_id") ? msgDataMap.value("group_id").toString() : QString();
    struct Page    page;
    struct Group   group;
    struct Profile profile;

    switch (eventType) {
        case MsgEventTypes::NEW:
            if (!pageId.isEmpty()) {
                QVariantMap newPage = newState.value("page").toMap();

                page.id = pageId;
                page.profileId = profileId;
                page.name = newPage.value("name").toString();
                if (newPage.contains("image")) {
                    page.image = newPage.value("image").toString();
                }
                page.pos = newPage.value("post").toInt();

                emit pageAdded(profileId, page);
            } else if (!groupId.isEmpty()) {
                QVariantMap newGroup = newState.value("group").toMap();

                group.id = groupId;
                group.profileId = newGroup.value("profile_id").toString();
                group.name = newGroup.value("name").toString();
                if (newGroup.contains("icon")) {
                    group.icon = newGroup.value("icon").toString();
                }
                group.entities = newGroup.value("entities").toStringList();

                emit groupAdded(profileId, group);
            } else {
                QVariantMap newProfile = newState.value("profile").toMap();

                profile.id = profileId;
                profile.name = newProfile.value("name").toString();
                if (newProfile.contains("icon")) {
                    profile.icon = newProfile.value("icon").toString();
                }
                profile.restricted = newState.value("restricted").toBool();
                if (newProfile.contains("description")) {
                    profile.description = newProfile.value("description").toString();
                }
                if (newProfile.contains("pages")) {
                    profile.pages = newProfile.value("description").toStringList();
                }

                emit profileAdded(profileId, profile);
            }
            break;
        case MsgEventTypes::CHANGE:
            if (!pageId.isEmpty()) {
                QVariantMap newPage = newState.value("page").toMap();

                page.id = pageId;
                page.profileId = profileId;
                page.name = newPage.value("name").toString();
                if (newPage.contains("image")) {
                    page.image = newPage.value("image").toString();
                }
                page.pos = newPage.value("post").toInt();

                QVariantList items = newPage.value("items").toList();

                if (items.size() > 0) {
                    for (QVariantList::iterator j = items.begin(); j != items.end(); j++) {
                        PageItem pageItem;

                        QVariantMap itemMap = j->toMap();

                        if (itemMap.contains("entity_id")) {
                            pageItem.type = "Entity";
                            pageItem.id = itemMap.value("entity_id").toString();
                        }

                        if (itemMap.contains("group_id")) {
                            pageItem.type = "Group";
                            pageItem.id = itemMap.value("group_id").toString();
                        }
                        page.items.append(pageItem);
                    }
                }

                emit pageChanged(profileId, page);
            } else if (!groupId.isEmpty()) {
                QVariantMap newGroup = newState.value("group").toMap();

                group.id = groupId;
                group.profileId = newGroup.value("profile_id").toString();
                group.name = newGroup.value("name").toString();
                if (newGroup.contains("icon")) {
                    group.icon = newGroup.value("icon").toString();
                }
                group.entities = newGroup.value("entities").toStringList();

                emit groupChanged(profileId, group);
            } else {
                QVariantMap newProfile = newState.value("profile").toMap();

                profile.id = profileId;
                profile.name = newProfile.value("name").toString();
                if (newProfile.contains("icon")) {
                    profile.icon = newProfile.value("icon").toString();
                }
                profile.restricted = newProfile.value("restricted").toBool();
                if (newProfile.contains("description")) {
                    profile.description = newProfile.value("description").toString();
                }
                if (newProfile.contains("pages")) {
                    profile.pages = newProfile.value("description").toStringList();
                }

                emit profileChanged(profileId, profile);
            }
            break;
        case MsgEventTypes::DELETE:
            if (!pageId.isEmpty()) {
                emit pageDeleted(profileId, pageId);
            } else if (!groupId.isEmpty()) {
                emit groupDeleted(profileId, groupId);
            } else {
                emit profileDeleted(profileId);
            }
            break;
        default:
            break;
    }
}

void Api::processWifiStatus(int reqId, int code, QVariant msgData) {
    struct WifiStatus wifiStatus;
    QVariantMap       map = msgData.toMap();

    wifiStatus.wpaState = Util::convertStringToEnum<WifiEnums::WpaState>(map.value("wpa_state").toString());
    wifiStatus.id = map.value("id").toInt();
    wifiStatus.bssid = map.value("bssid").toString();
    wifiStatus.ssid = map.value("ssid").toString();
    wifiStatus.freq = map.value("freq").toInt();
    wifiStatus.address = map.value("address").toString();
    wifiStatus.pairwiseCipher = map.value("pairwise_cipher").toString();
    wifiStatus.groupCipher = map.value("group_chipher").toString();
    wifiStatus.keyManagement = map.value("key_mgmt").toString();
    wifiStatus.ipAddress = map.value("ip_address").toString();
    wifiStatus.noise = map.value("noise").toInt();
    wifiStatus.rssi = map.value("rssi").toInt();
    wifiStatus.averageRssi = map.value("avg_rssi").toInt();
    wifiStatus.estimatedThroughput = map.value("est_throughput").toInt();
    wifiStatus.snr = map.value("snr").toInt();
    wifiStatus.linkSpeed = map.value("linkspeed").toInt();

    emit wifiStatusChanged(reqId, code, wifiStatus);
}

void Api::processWifiScanStatus(int reqId, int code, QVariant msgData) {
    QVariantMap            map = msgData.toMap();
    bool                   active = map.value("active").toBool();
    QVariantList           list = map.value("scan").toList();
    QList<AccessPointScan> resp;

    if (list.size() > 0) {
        for (QVariantList::iterator i = list.begin(); i != list.end(); i++) {
            struct AccessPointScan accessPointScan;
            QVariantMap            listMap = i->toMap();

            accessPointScan.bssid = listMap.value("bssid").toString();
            accessPointScan.frequency = listMap.value("frequency").toInt();
            accessPointScan.signalLevel = listMap.value("signal_level").toInt();
            accessPointScan.auth = listMap.value("auth").toString();
            accessPointScan.ssid = listMap.value("ssid").toString();

            resp.append(accessPointScan);
        }
    }

    emit wifiScanStatusChanged(reqId, code, active, resp);
}

void Api::processWifiNetworks(int reqId, int code, QVariant msgData) {
    QVariantList        list = msgData.toList();
    QList<SavedNetwork> networks;

    if (list.size() > 0) {
        for (QVariantList::iterator i = list.begin(); i != list.end(); i++) {
            struct SavedNetwork savedNetwork;
            QVariantMap         listMap = i->toMap();

            savedNetwork.id = listMap.value("id").toInt();
            savedNetwork.ssid = listMap.value("ssid").toString();
            savedNetwork.state = Util::convertStringToEnum<WifiEnums::NetworkState>(listMap.value("state").toString());
            savedNetwork.secured = listMap.value("secured").toBool();
            savedNetwork.signalLevel = listMap.value("signal_level").toBool();

            networks.append(savedNetwork);
        }
    }

    emit wifiNetworksChanged(reqId, code, networks);
}

void Api::processWifiNetwork(int reqId, int code, QVariant msgData) {
    struct SavedNetwork savedNetwork;
    QVariantMap         map = msgData.toMap();

    savedNetwork.id = map.value("id").toInt();
    savedNetwork.ssid = map.value("ssid").toString();
    savedNetwork.state = Util::convertStringToEnum<WifiEnums::NetworkState>(map.value("state").toString());
    savedNetwork.secured = map.value("secured").toBool();
    savedNetwork.signalLevel = map.value("signal_level").toBool();

    emit wifiNetworkChanged(reqId, code, savedNetwork);
}

void Api::processResponseDockCount(int reqId, int code, QVariant msgData) {
    QVariantMap map = msgData.toMap();
    int         count = map.value("count").toInt();
    emit        respDockCount(reqId, code, count);
}

void Api::processResponseDocks(int reqId, int code, QVariant msgData) {
    QVariantList             msgDataList = msgData.toMap().value("docks").toList();
    QVariantMap              paging = msgData.toMap().value("paging").toMap();
    QList<DockConfiguration> respList;

    if (msgDataList.size() > 0) {
        for (QVariantList::iterator i = msgDataList.begin(); i != msgDataList.end(); i++) {
            struct DockConfiguration dock;
            QVariantMap              map = i->toMap();

            dock.id = map.value("dock_id").toString();
            dock.name = map.value("name").toString();
            dock.customWsUrl = map.value("custom_ws_url").toString();
            dock.active = map.value("active").toBool();
            dock.model = map.value("model").toString();
            dock.revision = map.value("revision").toString();
            dock.serial = map.value("serial").toString();
            dock.connectionType = map.value("connection_type").toString();
            dock.version = map.value("version").toString();
            dock.state = Util::convertStringToEnum<DockEnums::DockState>(map.value("state").toString());
            dock.learningActive = map.value("learning_active").toBool();
            dock.description = map.value("descriptions").toString();
            if (map.contains("led_brightness")) {
                dock.ledBrightness = map.value("led_brightness").toInt();
            }
            if (map.contains("eth_led_brightness")) {
                dock.ethLedBrightness = map.value("eth_led_brightness").toInt();
            }

            respList.append(dock);
        }
    }

    emit respDocks(reqId, code, respList, paging.value("count").toInt(), paging.value("limit").toInt(),
                   paging.value("page").toInt());
}

void Api::processResponseDock(int reqId, int code, QVariant msgData) {
    QVariantMap              map = msgData.toMap();
    struct DockConfiguration dock;

    dock.id = map.value("dock_id").toString();
    dock.name = map.value("name").toString();
    dock.customWsUrl = map.value("custom_ws_url").toString();
    dock.active = map.value("active").toBool();
    dock.model = map.value("model").toString();
    dock.revision = map.value("revision").toString();
    dock.serial = map.value("serial").toString();
    dock.connectionType = map.value("connection_type").toString();
    dock.version = map.value("version").toString();
    dock.state = Util::convertStringToEnum<DockEnums::DockState>(map.value("state").toString());
    dock.learningActive = map.value("learning_active").toBool();
    dock.description = map.value("descriptions").toString();
    if (map.contains("led_brightness")) {
        dock.ledBrightness = map.value("led_brightness").toInt();
    }
    if (map.contains("eth_led_brightness")) {
        dock.ethLedBrightness = map.value("eth_led_brightness").toInt();
    }

    emit respDock(reqId, code, dock);
}

void Api::processResponseDockSystemInfo(int reqId, int code, QVariant msgData) {
    // QVariantMap map = msgData.toMap();
}

void Api::processResponseDockSetupProcesses(int reqId, int code, QVariant msgData) {
    QVariantMap map = msgData.toMap();
    QStringList sessions = map.value("sessions").toStringList();

    emit respDockSetupProcesses(reqId, code, sessions);
}

void Api::processResponseDockSetupStatus(int reqId, int code, QVariant msgData) {
    QVariantMap                    msgDataMap = msgData.toMap();
    QString                        dockId = msgDataMap.value("id").toString();
    DockSetupEnums::DockSetupState state =
        Util::convertStringToEnum<DockSetupEnums::DockSetupState>(msgDataMap.value("state").toString());
    DockSetupEnums::DockSetupError error =
        Util::convertStringToEnum<DockSetupEnums::DockSetupError>(msgDataMap.value("error").toString());

    emit respDockSetupStatus(reqId, code, dockId, state, error);
}

void Api::processResponseSystemUpdateInfo(int reqId, int code, QVariant msgData) {
    struct SystemUpdate          systemUpdate;
    QList<AvailableSystemUpdate> availableUpdates;
    QVariantMap                  map = msgData.toMap();

    systemUpdate.updateInProgress = map.value("update_in_progress").toBool();
    systemUpdate.lastCheckDate = map.value("last_check_date").toDateTime();
    systemUpdate.updateCheckEnabled = map.value("update_check_enabled").toBool();
    systemUpdate.installedVersion = map.value("installed_version").toString();

    QVariantList list = map.value("available").toList();

    if (list.size() > 0) {
        for (QVariantList::iterator i = list.begin(); i != list.end(); i++) {
            struct AvailableSystemUpdate availableUpdate;
            QVariantMap                  listMap = i->toMap();

            availableUpdate.id = listMap.value("id").toString();
            availableUpdate.title = listMap.value("title").toString();
            availableUpdate.description = listMap.value("description").toMap();
            availableUpdate.version = listMap.value("version").toString();
            availableUpdate.channel =
                Util::convertStringToEnum<UpdateEnums::UpdateChannel>(listMap.value("channel").toString());
            availableUpdate.releaseDate = listMap.value("release_date").toDateTime();
            availableUpdate.size = listMap.value("size").toInt();
            availableUpdate.downloadState =
                Util::convertStringToEnum<UpdateEnums::DownloadState>(listMap.value("download").toString());

            availableUpdates.append(availableUpdate);
        }
    }

    systemUpdate.available = availableUpdates;

    emit respSystemUpdateInfo(reqId, code, systemUpdate);
}

void Api::processResponsePowerMode(int reqId, int code, QVariant msgData) {
    QVariantMap           map = msgData.toMap();
    PowerEnums::PowerMode powerMode = Util::convertStringToEnum<PowerEnums::PowerMode>(map.value("mode").toString());

    QVariantMap             battery = map.value("battery").toMap();
    int                     capacity = battery.value("capacity").toInt();
    bool                    powerSupply = map.value("power_supply").toBool();
    PowerEnums::PowerStatus powerStatus =
        Util::convertStringToEnum<PowerEnums::PowerStatus>(battery.value("status").toString());

    emit respPowerMode(reqId, code, powerMode, capacity, powerSupply, powerStatus);
}

void Api::processWarning(QVariant msgData) {
    QVariantMap map = msgData.toMap();

    MsgEventTypes::WarningEvent event =
        Util::convertStringToEnum<MsgEventTypes::WarningEvent>(map.value("event").toString());
    bool    shutdown = map.value("shutdown").toBool();
    QString message = map.value("message").toString();

    emit warning(event, shutdown, message);
}

void Api::processDockChange(QVariant msgData) {
    QVariantMap         msgDataMap = msgData.toMap();
    QVariantMap         newState = msgDataMap.value("new_state").toMap();
    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());
    QString                  dockId = msgDataMap.value("dock_id").toString();
    struct DockConfiguration dock;

    dock.id = newState.value("dock_id").toString();
    dock.name = newState.value("name").toString();
    dock.active = newState.value("active").toBool();
    dock.model = newState.value("model").toString();
    dock.revision = newState.value("revision").toString();
    dock.serial = newState.value("serial").toString();
    dock.connectionType = newState.value("connection_type").toString();
    dock.version = newState.value("version").toString();
    dock.state = Util::convertStringToEnum<DockEnums::DockState>(newState.value("state").toString());
    dock.learningActive = newState.value("learning_active").toBool();
    dock.description = newState.value("description").toString();
    if (newState.contains("led_brightness")) {
        dock.ledBrightness = newState.value("led_brightness").toInt();
    }
    if (newState.contains("eth_led_brightness")) {
        dock.ethLedBrightness = newState.value("eth_led_brightness").toInt();
    }

    switch (eventType) {
        case MsgEventTypes::NEW:
            emit dockAdded(dockId, dock);
            break;
        case MsgEventTypes::CHANGE:
            emit dockChanged(dockId, dock);
            break;

        case MsgEventTypes::DELETE:
            emit dockDeleted(dockId);
            break;
        default:
            break;
    }
}

void Api::processDockStateChange(QVariant msgData) {
    QVariantMap msgDataMap = msgData.toMap();

    QString              dockId = msgDataMap.value("dock_id").toString();
    DockEnums::DockState state = Util::convertStringToEnum<DockEnums::DockState>(msgDataMap.value("state").toString());

    emit dockStateChanged(dockId, state);
}

void Api::processDockDiscoveryChange(QVariant msgData) {
    QVariantMap         msgDataMap = msgData.toMap();
    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());

    switch (eventType) {
        case MsgEventTypes::START:
            emit dockDiscoveryStarted();
            break;
        case MsgEventTypes::STOP:
            emit dockDiscoveryStopped();
            break;
        case MsgEventTypes::DISCOVER: {
            QVariantMap          dockMap = msgDataMap.value("dock").toMap();
            struct DockDiscovery dock;
            dock.id = dockMap.value("id").toString();
            dock.configured = dockMap.value("configured").toBool();
            dock.friendlyName = dockMap.value("friendly_name").toString();
            dock.address = dockMap.value("address").toString();
            dock.model = dockMap.value("model").toString();
            dock.version = dockMap.value("version").toString();
            dock.discoveryType = Util::convertStringToEnum<DockSetupEnums::DockDiscoveryType>(
                dockMap.value("discovery_type").toString());

            if (dockMap.contains("bt")) {
                dock.bluetoothSignal = dockMap.value("bt").toMap().value("signal").toInt();
                dock.bluetoothLastSeenSeconds = dockMap.value("bt").toMap().value("last_seen_sec").toInt();
            }

            emit dockDiscovered(dock);
            break;
        }
        default:
            break;
    }
}

void Api::processDockSetupChange(QVariant msgData) {
    QVariantMap         msgDataMap = msgData.toMap();
    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());
    QString                        dockId = msgDataMap.value("dock_id").toString();
    DockSetupEnums::DockSetupState state =
        Util::convertStringToEnum<DockSetupEnums::DockSetupState>(msgDataMap.value("state").toString());
    DockSetupEnums::DockSetupError error =
        Util::convertStringToEnum<DockSetupEnums::DockSetupError>(msgDataMap.value("error").toString());

    emit dockSetupChanged(eventType, dockId, state, error);
}

void Api::processDockUpdateChange(QVariant msgData) {
    QVariantMap         msgDataMap = msgData.toMap();
    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());
    QString                        dockId = msgDataMap.value("dock_id").toString();
    QString                        updateId = msgDataMap.value("update_id").toString();
    QString                        version = msgDataMap.value("version").toString();
    int                            progress = msgDataMap.value("progress").toInt();
    DockSetupEnums::DockSetupState state =
        Util::convertStringToEnum<DockSetupEnums::DockSetupState>(msgDataMap.value("state").toString());
    DockSetupEnums::DockSetupError error =
        Util::convertStringToEnum<DockSetupEnums::DockSetupError>(msgDataMap.value("error").toString());

    emit dockUpdateChanged(eventType, dockId, updateId, version, progress, state, error);
}

void Api::processIntegrationDriverChange(QVariant msgData) {
    QVariantMap         msgDataMap = msgData.toMap();
    QVariantMap         newState = msgDataMap.value("new_state").toMap();
    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());
    QString                  integrationDriverId = msgDataMap.value("driver_id").toString();
    struct IntegrationDriver integrationDriver;

    integrationDriver.id = newState.value("driver_id").toString();
    integrationDriver.name = newState.value("name").toMap();
    integrationDriver.driverUrl = newState.value("driver_url").toString();
    integrationDriver.version = newState.value("version").toString();
    integrationDriver.min_core_api = newState.value("min_core_api").toString();
    integrationDriver.icon = newState.value("icon").toString();
    integrationDriver.enabled = newState.value("enabled").toBool();
    integrationDriver.description = newState.value("description").toString();

    struct DriverDeveloper driverDeveloper;
    QVariantMap            developer = newState.value("developer").toMap();
    driverDeveloper.name = developer.value("name").toString();
    driverDeveloper.url = developer.value("url").toString();
    driverDeveloper.email = developer.value("email").toString();

    integrationDriver.developer = driverDeveloper;
    integrationDriver.homePage = newState.value("home_page").toString();
    integrationDriver.deviceDiscovery = newState.value("device_discovery").toBool();

    QVariantMap         settingsPageMap = newState.value("setup_data_schema").toMap();
    struct SettingsPage settingsPage;
    settingsPage.title = settingsPageMap.value("title").toMap();
    settingsPage.settings = settingsPageMap.value("settings").toList();

    integrationDriver.settingsPage = settingsPage;
    integrationDriver.releaseDate = newState.value("release_date").toString();
    integrationDriver.state =
        Util::convertStringToEnum<IntegrationDriverEnums::States>(newState.value("driver_state").toString());

    integrationDriver.external = newState.value("driver_type").toString().contains("EXTERNAL") ? true : false;
    integrationDriver.instanceCount = newState.value("instance_count").toInt();

    switch (eventType) {
        case MsgEventTypes::NEW:
            emit integrationDriverAdded(integrationDriverId, integrationDriver);
            break;
        case MsgEventTypes::CHANGE:
            emit integrationDriverChanged(integrationDriverId, integrationDriver);
            break;
        case MsgEventTypes::DELETE:
            emit integrationDriverDeleted(integrationDriverId);
            break;
        default:
            break;
    }
}

void Api::processIntegrationChange(QVariant msgData) {
    QVariantMap         msgDataMap = msgData.toMap();
    QVariantMap         newState = msgDataMap.value("new_state").toMap();
    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());
    QString            integrationId = msgDataMap.value("integration_id").toString();
    struct Integration integration;

    integration.id = newState.value("integration_id").toString();
    integration.driverId = newState.value("driver_id").toString();
    if (newState.contains("device_id")) {
        integration.deviceId = newState.value("device_id").toString();
    }
    integration.name = newState.value("name").toMap();
    if (newState.contains("icon")) {
        integration.icon = newState.value("icon").toString();
    }
    integration.enabled = newState.value("enabled").toBool();
    if (newState.contains("setup_data")) {
        integration.setupData = newState.value("setup_data").toMap();
    }

    switch (eventType) {
        case MsgEventTypes::NEW:
            emit integrationAdded(integrationId, integration);
            break;
        case MsgEventTypes::CHANGE:
            emit integrationChanged(integrationId, integration);
            break;
        case MsgEventTypes::DELETE:
            emit integrationDeleted(integrationId);
            break;
        default:
            break;
    }
}

void Api::processIntegrationDiscoveryChange(QVariant msgData) {
    QVariantMap         msgDataMap = msgData.toMap();
    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());

    switch (eventType) {
        case MsgEventTypes::START:
            emit integationDriverDiscoveryStarted();
            break;
        case MsgEventTypes::STOP:
            emit integrationDriverDiscoveryStopped();
            break;
        case MsgEventTypes::DISCOVER: {
            QVariantMap              integrationDriverMap = msgDataMap.value("integration").toMap();
            struct IntegrationDriver integrationDriver;

            integrationDriver.id = integrationDriverMap.value("id").toString();
            integrationDriver.configured = integrationDriverMap.value("configured").toBool();

            QVariantMap name;
            name.insert("en", integrationDriverMap.value("name").toString());
            integrationDriver.name = name;
            integrationDriver.icon = integrationDriverMap.value("icon").toString();
            integrationDriver.developer.name = integrationDriverMap.value("developer_name").toString();
            integrationDriver.driverUrl = integrationDriverMap.value("driver_url").toString();
            integrationDriver.version = integrationDriverMap.value("version").toString();
            integrationDriver.external = true;
            integrationDriver.instanceCount = 0;

            emit integrationDriverDiscovered(integrationDriver);
            break;
        }
        default:
            break;
    }
}

void Api::processIntegrationSetupChange(QVariant msgData) {
    QVariantMap         msgDataMap = msgData.toMap();
    MsgEventTypes::Enum eventType =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());

    QString driverId = msgDataMap.value("driver_id").toString();

    switch (eventType) {
        case MsgEventTypes::START:
        case MsgEventTypes::SETUP:
        case MsgEventTypes::STOP: {
            struct IntegrationSetupInfo integrationSetupInfo;

            integrationSetupInfo.id = driverId;
            integrationSetupInfo.state =
                Util::convertStringToEnum<IntegrationEnums::SetupState>(msgDataMap.value("state").toString());
            integrationSetupInfo.error =
                Util::convertStringToEnum<IntegrationEnums::SetupError>(msgDataMap.value("error").toString());
            integrationSetupInfo.requireUserAction = msgDataMap.contains("require_user_action");

            if (integrationSetupInfo.requireUserAction) {
                QVariantMap reqUserAction = msgDataMap.value("require_user_action").toMap();

                if (reqUserAction.contains("input")) {
                    struct SettingsPage settingsPage;
                    settingsPage.title = reqUserAction.value("input").toMap().value("title").toMap();
                    settingsPage.settings = reqUserAction.value("input").toMap().value("settings").toList();
                    integrationSetupInfo.settingsPage = settingsPage;
                } else if (reqUserAction.contains("confirmation")) {
                    struct ConfirmationPage confirmationPage;
                    confirmationPage.title = reqUserAction.value("confirmation").toMap().value("title").toMap();
                    confirmationPage.message1 = reqUserAction.value("confirmation").toMap().value("message1").toMap();
                    confirmationPage.image = reqUserAction.value("confirmation").toMap().value("image").toString();
                    confirmationPage.message2 = reqUserAction.value("confirmation").toMap().value("message2").toMap();
                    integrationSetupInfo.confirmationPage = confirmationPage;
                }
            }

            emit integrationSetupChange(integrationSetupInfo);
            break;
        }
        default:
            break;
    }
}

void Api::processSoftwareUpdateChange(QVariant msgData) {
    QVariantMap msgDataMap = msgData.toMap();
    QVariantMap progressMap = msgDataMap.value("progress").toMap();

    MsgEventTypes::Enum type =
        Util::convertStringToEnum<MsgEventTypes::Enum>(msgDataMap.value("event_type").toString());
    QString updateId = msgDataMap.value("update_id").toString();

    struct SystemUpdateProgress progress;

    progress.state = Util::convertStringToEnum<UpdateEnums::UpdateProgressType>(progressMap.value("state").toString());
    progress.udpateId = progressMap.value("update_id").toString();
    progress.downloadPercent = progressMap.value("download_percent").toInt();
    progress.downloadBytes = progressMap.value("download_bytes").toInt();
    progress.totalSteps = progressMap.value("total_steps").toInt();
    progress.currentStep = progressMap.value("current_step").toInt();
    progress.currentPercent = progressMap.value("current_percent").toInt();

    emit softwareUpdateChanged(type, updateId, progress);
}

void Api::processPowerModeChange(QVariant msgData) {
    QVariantMap           msgDataMap = msgData.toMap();
    PowerEnums::PowerMode powerMode =
        Util::convertStringToEnum<PowerEnums::PowerMode>(msgDataMap.value("mode").toString());

    emit powerModeChanged(powerMode);
}

void Api::processBatteryStatusChange(QVariant msgData) {
    QVariantMap             msgDataMap = msgData.toMap();
    int                     capacity = msgDataMap.value("capacity").toInt();
    bool                    powerSupply = msgDataMap.value("power_supply").toBool();
    PowerEnums::PowerStatus powerStatus =
        Util::convertStringToEnum<PowerEnums::PowerStatus>(msgDataMap.value("status").toString());

    emit batteryStatusChanged(capacity, powerSupply, powerStatus);
}

void Api::processRequestGetLocalizationLanguages(int reqId)
{
    emit reqGetLocalizationLanguages(reqId);
}

}  // namespace core
}  // namespace uc
