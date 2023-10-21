// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>

#include "../core/core.h"

namespace uc {
namespace hw {

class SignalStrength {
    Q_GADGET

 public:
    enum Enum { NONE, WEAK, OK, GOOD, EXCELLENT };
    Q_ENUM(Enum)

    static Enum fromRssi(int rssi) {
        if (rssi == 0)
            return NONE;
        else if (rssi >= -60)
            return EXCELLENT;
        else if (rssi >= -68)
            return GOOD;
        else if (rssi >= -76)
            return OK;
        else if (rssi >= -84)
            return WEAK;
        else
            return NONE;
    }

 private:
    SignalStrength() {}
};

class Security {
    Q_GADGET

 public:
    enum Enum { OPEN, WPA_PSK, WPA_EAP, WPA2_PSK, WPA2_EAP };
    Q_ENUM(Enum)

 private:
    Security() {}
};

class WifiNetwork : public QObject {
    Q_OBJECT

    Q_PROPERTY(int id READ getId CONSTANT)
    Q_PROPERTY(QString ssid READ getSsid CONSTANT)
    Q_PROPERTY(uc::hw::SignalStrength::Enum signalStrength READ getSignalStrength CONSTANT)
    Q_PROPERTY(bool encrypted READ isEncrypted CONSTANT)
    Q_PROPERTY(uc::hw::Security::Enum security READ getSecurity CONSTANT)

 public:
    explicit WifiNetwork(int id, const QString &ssid, Security::Enum security, int rssi, QObject *parent = nullptr);
    ~WifiNetwork();

    int                  getId() const { return m_id; }
    QString              getSsid() const { return m_ssid; }
    SignalStrength::Enum getSignalStrength() const { return m_signalStrenght; }
    bool                 isEncrypted() const { return m_security != Security::OPEN; }
    Security::Enum       getSecurity() const { return m_security; }

 private:
    int                  m_id;
    QString              m_ssid;
    Security::Enum       m_security;
    SignalStrength::Enum m_signalStrenght;
};

class Wifi : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isConnected READ getIsConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(QObject *currentNetwork READ getCurrentNetwork NOTIFY currentNetworkChanged)
    Q_PROPERTY(QString ipAddress READ getIpAddress NOTIFY ipAddressChanged)
    Q_PROPERTY(QString macAddress READ getMac NOTIFY macAddressChanged)
    Q_PROPERTY(QList<WifiNetwork *> networkList READ getNetworkList NOTIFY networkListChanged)
    Q_PROPERTY(QList<WifiNetwork *> unfilteredNetworkList READ getUnfilteredNetworkList NOTIFY networkListChanged)
    Q_PROPERTY(QList<WifiNetwork *> knownNetworkList READ getKnownNetworkList NOTIFY knownNetworkListChanged)
    Q_PROPERTY(bool scanActive READ getScanActive NOTIFY scanActiveChanged);

 public:
    explicit Wifi(core::Api *core, QObject *parent = nullptr);
    ~Wifi();

    // Q_PROPERTY methods
    bool                 getIsConnected() { return m_isConnected; }
    QObject             *getCurrentNetwork() { return m_currentNetwork; }
    QString              getIpAddress() { return m_ipAddress; }
    QString              getMac() { return m_mac; }
    QList<WifiNetwork *> getNetworkList();
    QList<WifiNetwork *> getUnfilteredNetworkList();
    QList<WifiNetwork *> getKnownNetworkList();
    bool                 getScanActive() { return m_scanActive; }

    Q_INVOKABLE void turnOn();
    Q_INVOKABLE void turnOff();
    Q_INVOKABLE void connect(const QString &ssid, const QString &password, uc::hw::Security::Enum security);
    Q_INVOKABLE void connectSavedNetwork(int id);
    Q_INVOKABLE void disconnect();

    Q_INVOKABLE void getWifiStatus();
    Q_INVOKABLE void startNetworkScan();
    Q_INVOKABLE void getWifiScanStatus();
    Q_INVOKABLE void stopNetworkScan();
    Q_INVOKABLE void clearNetworkList();

    Q_INVOKABLE void getAllWifiNetworks();
    Q_INVOKABLE void deleteSavedNetwork(const QString &networkId);
    Q_INVOKABLE void deleteAllNetworks();

    Q_INVOKABLE QString getLastConnectedSsid() { return m_lastConnectedSSid; }
    Q_INVOKABLE QString getLastConnectedPassword() { return m_lastConnectedPassword; }

    void addNetwork(const QString &ssid, const QString &password, uc::hw::Security::Enum security);
    void wifiNetworkCommand(int networkId, core::WifiEnums::WifiNetworkCmd command);
    void wifiCommand(core::WifiEnums::WifiCmd command);

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void isConnectedChanged();
    void ipAddressChanged();
    void macAddressChanged();
    void currentNetworkChanged();
    void networkListChanged();
    void knownNetworkListChanged();
    void scanActiveChanged();
    void scanFailed();
    void connecting();
    void connected(bool success);
    void networkNotFound();
    void wrongKey();

 private:
    static Wifi *s_instance;
    core::Api   *m_core;

    bool         m_isConnected = true;
    WifiNetwork *m_currentNetwork = nullptr;
    QString      m_ipAddress;
    QString      m_mac;

    QHash<QString, WifiNetwork *> m_networkList;
    QHash<QString, WifiNetwork *> m_knownNetworkList;
    bool                          m_scanActive = true;

 private:
    QString m_lastConnectedSSid;
    QString m_lastConnectedPassword;

 private slots:
    void onWifiEventChanged(core::WifiEvent::Enum wifiEvent);
};

}  // namespace hw
}  // namespace uc
