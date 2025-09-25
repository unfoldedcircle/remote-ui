// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wifi.h"

#include "../logging.h"
#include "../ui/notification.h"
#include "../util.h"

namespace uc {
namespace hw {

Wifi *Wifi::s_instance = nullptr;

Wifi::Wifi(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    m_currentNetwork = new WifiNetwork(0, QString(), Security::OPEN, 0, "", "", "", 0, true, this);

    m_wowlan = qEnvironmentVariable("UC_WOWLAN").toLower() == "true";

    qRegisterMetaType<SignalStrength::Enum>("SignalStrength");
    qRegisterMetaType<Security::Enum>("Security");
    qmlRegisterUncreatableType<SignalStrength>("Wifi.SignalStrength", 1, 0, "SignalStrength", "Enum is not a type");
    qmlRegisterUncreatableType<Security>("Wifi.Security", 1, 0, "Security", "Enum is not a type");

    QObject::connect(m_core, &core::Api::connected, this, [=] {
        getWifiStatus();
        getAllWifiNetworks();
    });
    QObject::connect(m_core, &core::Api::wifiEventChanged, this, &Wifi::onWifiEventChanged);
}

Wifi::~Wifi() { s_instance = nullptr; }

QList<WifiNetwork *> Wifi::getNetworkList() {
    QList<WifiNetwork *> list;

    for (QHash<QString, WifiNetwork *>::iterator i = m_networkList.begin(); i != m_networkList.end(); i++) {
        if (!m_knownNetworkList.contains(i.value()->getSsid())) {
            list.append(i.value());
        }
    }

    return list;
}

QList<WifiNetwork *> Wifi::getKnownNetworkList() {
    QList<WifiNetwork *> list;

    for (QHash<QString, WifiNetwork *>::iterator i = m_knownNetworkList.begin(); i != m_knownNetworkList.end(); i++) {
        list.append(i.value());
    }

    return list;
}

void Wifi::turnOn() {}

void Wifi::turnOff() {}

void Wifi::connect(const QString &ssid, const QString &password, uc::hw::Security::Enum security) {
    qCDebug(lcHwWifi()) << "Connect with ssid security" << ssid << security;
    emit connecting();

    addNetwork(ssid, password, security);
    m_lastConnectedSSid     = ssid;
    m_lastConnectedPassword = password;
}

void Wifi::connectSavedNetwork(int id) {
    qCDebug(lcHwWifi()) << "Connecting to saved network with id:" << id;
    emit connecting();

    // send wifi command
    wifiNetworkCommand(id, core::WifiEnums::WifiNetworkCmd::SELECT);
}

void Wifi::enableSavedNetwork(int id, bool enable)
{
    qCDebug(lcHwWifi()) << "Enable saved network:" << id << "->" << enable;
    wifiNetworkCommand(id, enable ? core::WifiEnums::WifiNetworkCmd::ENABLE : core::WifiEnums::WifiNetworkCmd::DISABLE);
}

void Wifi::disconnect() {
    int id = m_core->wifiCommand(core::WifiEnums::WifiCmd::DISCONNECT);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error on disconnect:" << code << message;
        });
}

void Wifi::getWifiStatus() {
    int id = m_core->wifiGetStatus();

    m_core->onResponseWithErrorResult(
        id, &core::Api::wifiStatusChanged,
        [=](core::WifiStatus wifiStatus) {
            // success
            m_mac = wifiStatus.address;
            emit macAddressChanged();

            if (wifiStatus.wpaState == core::WifiEnums::WpaState::COMPLETED) {
                Security::Enum security;
                if (wifiStatus.keyManagement.isEmpty()) {
                    security = Security::Enum::OPEN;
                } else {
                    security = Util::convertStringToEnum<Security::Enum>(wifiStatus.keyManagement.replace("-", "_"));
                }

                m_currentNetwork = new WifiNetwork(wifiStatus.id, wifiStatus.ssid, security, wifiStatus.rssi, wifiStatus.keyManagement, wifiStatus.pairwiseCipher, wifiStatus.groupCipher, wifiStatus.freq, true, this);
                emit currentNetworkChanged();

                m_ipAddress = wifiStatus.ipAddress;
                emit ipAddressChanged();

                m_isConnected = true;
                emit isConnectedChanged();
            } else if (wifiStatus.wpaState == core::WifiEnums::WpaState::ERROR ||
                       wifiStatus.wpaState == core::WifiEnums::WpaState::DISCONNECTED) {
                m_isConnected = false;
                emit isConnectedChanged();
            } else {
                m_isConnected = false;
                emit isConnectedChanged();
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error getting Wifi status:" << code << message;
        });
}

void Wifi::startNetworkScan() {
    int id = m_core->wifiScanStart();

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error starting network scan:" << code << message;
        });
}

void Wifi::getWifiScanStatus() {
    int id = m_core->wifiGetScanStatus();

    m_core->onResponseWithErrorResult(
        id, &core::Api::wifiScanStatusChanged,
        [=](bool active, QList<core::AccessPointScan> scan) {
            // success
            if (scan.size() > 0) {
                m_networkList.clear();

                if (m_scanActive != active) {
                    m_scanActive = active;
                    emit scanActiveChanged();
                }

                for (QList<core::AccessPointScan>::iterator i = scan.begin(); i != scan.end(); i++) {
                    if (!i->ssid.isEmpty()) {
                        Security::Enum security;
                        if (i->auth.isEmpty()) {
                            security = Security::Enum::OPEN;
                        } else {
                            // TODO(marton): properly convert security
                            qCDebug(lcHwWifi()) << "Access point" << i->ssid << i->auth;
                            security = Security::Enum::WPA2_PSK;
                        }

                        m_networkList.insert(i->ssid, new WifiNetwork(0, i->ssid, security, i->signalLevel, i->auth, "", "", i->frequency, true, this));
                        emit networkListChanged();
                    }
                }
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error on getting scan status:" << code << message;
        });
}

void Wifi::stopNetworkScan() {
    int id = m_core->wifiScanStop();

    m_core->onResponseWithErrorResult(
        id, &core::Api::wifiScanStatusChanged,
        [=](bool active, QList<core::AccessPointScan> scan) {
            // success
            if (scan.size() > 0) {
                m_networkList.clear();

                if (m_scanActive != active) {
                    m_scanActive = active;
                    emit scanActiveChanged();
                }

                for (QList<core::AccessPointScan>::iterator i = scan.begin(); i != scan.end(); i++) {
                    if (!i->ssid.isEmpty()) {
                        Security::Enum security;
                        if (i->auth.isEmpty()) {
                            security = Security::Enum::OPEN;
                        } else {
                            security = Security::Enum::WPA2_PSK;
                        }

                        m_networkList.insert(i->ssid, new WifiNetwork(0, i->ssid, security, i->signalLevel, i->auth, "", "", i->frequency, true, this));
                        emit networkListChanged();
                    }
                }
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error while stopping network scan:" << code << message;
        });
}

void Wifi::clearNetworkList() { m_networkList.clear(); }

void Wifi::getAllWifiNetworks() {
    m_knownNetworkList.clear();

    int id = m_core->wifiGetAllNetworks();

    m_core->onResponseWithErrorResult(
        id, &core::Api::wifiNetworksChanged,
        [=](QList<core::SavedNetwork> networks) {
            // success
            if (networks.size() > 0) {
                for (QList<core::SavedNetwork>::iterator i = networks.begin(); i != networks.end(); i++) {
                    qCDebug(lcHwWifi()) << "Saved network:" << i->id << i->ssid << i->state;

                    m_knownNetworkList.insert(
                        i->ssid,
                        new WifiNetwork(i->id, i->ssid, i->secured ? Security::Enum::WPA2_PSK : Security::Enum::OPEN,
                                        i->signalLevel, "", "", "", 0, i->state == uc::core::WifiEnums::NetworkState::DISABLED ? false : true,  this));
                    emit knownNetworkListChanged();
                }
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error getting all wifi networks:" << code << message;
        });
}

void Wifi::deleteSavedNetwork(const QString &networkId) {
    auto network = m_knownNetworkList.value(networkId);

    if (!network) {
        ui::Notification::createNotification(tr("Failed to delete network. Wifi network does not exist."), true);
        return;
    }

    int id = m_core->wifiDeleteNetwork(network->getId());

    m_core->onResult(
        id,
        [=]() {
            // success
            m_knownNetworkList.remove(networkId);
            emit knownNetworkListChanged();
            QTimer::singleShot(1500, [=] { getAllWifiNetworks(); });
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error deleting a saved network with id:" << networkId << code << message;
            ui::Notification::createNotification(message, true);
        });
}

void Wifi::deleteAllNetworks() {
    int id = m_core->wifiDeleteAllNetworks();

    m_core->onResult(
        id,
        [=]() {
            // success
            m_knownNetworkList.clear();
            emit knownNetworkListChanged();
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error deleting all wifi networks" << code << message;
            ui::Notification::createNotification(message, true);
        });
}

void Wifi::addNetwork(const QString &ssid, const QString &password, Security::Enum security) {
    Q_UNUSED(security)

    int id = m_core->wifiAddNetwork(ssid, password);

    m_core->onResponseWithErrorResult(
        id, &core::Api::wifiNetworkChanged,
        [=](core::SavedNetwork network) {
            // success
            qCDebug(lcHwWifi()) << "Network added successfully with id:" << network.id;

            wifiNetworkCommand(network.id, core::WifiEnums::WifiNetworkCmd::ENABLE);

            QTimer::singleShot(500, [=] { getAllWifiNetworks(); });
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error adding network:" << code << message;
            emit connected(false);
            ui::Notification::createNotification("Error adding network: " + message, true);
        });
}

void Wifi::wifiNetworkCommand(int networkId, core::WifiEnums::WifiNetworkCmd command) {
    int id = m_core->wifiNetworkCommand(networkId, command);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcHwWifi()) << "Successfully sent network command";
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error executing network command:" << code << message;
            ui::Notification::createNotification("Error executing network command: " + message, true);
        });
}

void Wifi::wifiCommand(core::WifiEnums::WifiCmd command) {
    int id = m_core->wifiCommand(command);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcHwWifi()) << "Successfully sent wifi command";
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcHwWifi()) << "Error executing command:" << code << message;
        });
}

QObject *Wifi::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void Wifi::onWifiEventChanged(core::WifiEvent::Enum wifiEvent) {
    switch (wifiEvent) {
        case core::WifiEvent::Enum::CONNECTED:
            m_isConnected = true;
            emit isConnectedChanged();
            emit connected(true);
            getWifiStatus();
            break;
        case core::WifiEvent::Enum::DISCONNECTED:
            m_isConnected = false;
            emit isConnectedChanged();
            emit connected(false);
            break;
        case core::WifiEvent::Enum::SCAN_STARTED:
            m_scanActive = true;
            emit scanActiveChanged();
            break;
        case core::WifiEvent::Enum::SCAN_COMPLETED:
            m_scanActive = false;
            emit scanActiveChanged();
            break;
        case core::WifiEvent::Enum::SCAN_FAILED:
            m_scanActive = false;
            emit scanActiveChanged();
            qCWarning(lcHwWifi()) << "Wifi scan failed";
            emit scanFailed();
            break;
        case core::WifiEvent::Enum::NETWORK_NOT_FOUND:
            qCWarning(lcHwWifi()) << "Network not found";
            emit networkNotFound();
            break;
        case core::WifiEvent::Enum::NETWORK_ADDED:
            // TODO(marton): implement me
            break;
        case core::WifiEvent::Enum::NETWORK_REMOVED:
            // TODO(marton): implement me
            break;
        case core::WifiEvent::Enum::WRONG_KEY:
            qCWarning(lcHwWifi()) << "Wrong network key";
            emit wrongKey();
            ui::Notification::createNotification(tr("Wrong network key"), true);
            break;
    }
}

WifiNetwork::WifiNetwork(int id, const QString &ssid, Security::Enum security, int rssi, const QString keyManagement, const QString pairwiseCipher, const QString groupCipher, int frequency, bool enabled, QObject *parent)
    : QObject(parent), m_id(id), m_ssid(ssid), m_security(security), m_signalStrenght(SignalStrength::fromRssi(rssi)), m_keyManagement(keyManagement), m_pairwiseCipher(pairwiseCipher), m_groupCipher(groupCipher), m_frequency(frequency), m_enabled(enabled) {}

WifiNetwork::~WifiNetwork() {}

}  // namespace hw
}  // namespace uc
