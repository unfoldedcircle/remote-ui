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
    /**
     * WiFi security types.
     *
     * OPEN, WPA_PSK, WPA2_WPA3 and WPA3_SAE can be requested when adding a network, AUTO leaves the
     * choice to the core. The remaining values are only used to classify an existing connection.
     */
    enum Enum { OPEN, WPA_PSK, WPA_EAP, WPA2_PSK, WPA2_EAP, WPA2_WPA3, WPA3_SAE, AUTO };
    Q_ENUM(Enum)

 private:
    Security() {}
};

class WifiNetwork : public QObject {
    Q_OBJECT

    Q_PROPERTY(int id READ getId NOTIFY idChanged)
    Q_PROPERTY(QString ssid READ getSsid CONSTANT)
    Q_PROPERTY(QString ssidHex READ getSsidHex CONSTANT)
    Q_PROPERTY(QString identifier READ getIdentifier CONSTANT)
    Q_PROPERTY(uc::hw::SignalStrength::Enum signalStrength READ getSignalStrength NOTIFY signalStrengthChanged)
    Q_PROPERTY(bool encrypted READ isEncrypted NOTIFY securityChanged)
    Q_PROPERTY(uc::hw::Security::Enum security READ getSecurity NOTIFY securityChanged)
    Q_PROPERTY(QString keyManagement READ getKeyManagement NOTIFY ciphersChanged)
    Q_PROPERTY(QString pairwiseCipher READ getPairwiseCipher NOTIFY ciphersChanged)
    Q_PROPERTY(QString groupCipher READ getGroupCipher NOTIFY ciphersChanged)
    Q_PROPERTY(int frequency READ getFrequency NOTIFY frequencyChanged)
    Q_PROPERTY(bool enabled READ getEnabled NOTIFY enabledChanged)

 public:
    explicit WifiNetwork(int id, const QString &ssid, const QString &ssidHex, Security::Enum security, int rssi, const QString keyManagement, const QString pairwiseCipher, const QString groupCipher, int frequency, bool enabled, QObject *parent = nullptr);
    ~WifiNetwork();

    int                  getId() const { return m_id; }
    QString              getSsid() const { return m_ssid; }
    QString              getSsidHex() const { return m_ssidHex; }

    /**
     * Identifies the network by its native SSID, see identifier().
     */
    QString getIdentifier() const { return identifier(m_ssid, m_ssidHex); }

    /**
     * Identifies a network by its native SSID, to compare networks from a scan result, a saved network and the
     * WiFi status.
     *
     * The friendly SSID name is a lossy conversion of the native SSID and is not unique: two networks can share
     * it. If the remote didn't report the native SSID it is derived from the friendly name, which is exact for
     * the valid UTF-8 names it can be derived from.
     */
    static QString identifier(const QString &ssid, const QString &ssidHex) {
        return ssidHex.isEmpty() ? QString::fromLatin1(ssid.toUtf8().toHex()) : ssidHex;
    }
    SignalStrength::Enum getSignalStrength() const { return m_signalStrenght; }
    bool                 isEncrypted() const { return m_security != Security::OPEN; }
    Security::Enum       getSecurity() const { return m_security; }
    QString              getKeyManagement() const { return m_keyManagement; }
    QString              getPairwiseCipher() const { return m_pairwiseCipher; }
    QString              getGroupCipher() const { return m_groupCipher; }
    int                  getFrequency() const { return m_frequency; }
    bool                 getEnabled() const { return m_enabled; }

    /**
     * Update the network in place from a new scan result or saved-network list, emitting only the change
     * signals of the properties that differ. Keeping the object lets the QML list keep its delegates - and the
     * keypad selection - instead of rebuilding them on every scan.
     */
    void update(int id, Security::Enum security, int rssi, const QString &keyManagement, const QString &pairwiseCipher, const QString &groupCipher, int frequency, bool enabled);

    /**
     * Ordering for the network lists shown in the UI: strongest signal first, then by name. A stable order keeps
     * the list from being reshuffled by the QHash iteration order every time it is handed to QML.
     */
    static bool lessThan(const WifiNetwork *a, const WifiNetwork *b);

 signals:
    void idChanged();
    void signalStrengthChanged();
    void securityChanged();
    void ciphersChanged();
    void frequencyChanged();
    void enabledChanged();

 private:
    int                  m_id;
    QString              m_ssid;
    QString              m_ssidHex;
    Security::Enum       m_security;
    SignalStrength::Enum m_signalStrenght;
    QString              m_keyManagement;
    QString              m_pairwiseCipher;
    QString              m_groupCipher;
    int                  m_frequency;
    bool                 m_enabled;
};

class Wifi : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isConnected READ getIsConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(QObject *currentNetwork READ getCurrentNetwork NOTIFY currentNetworkChanged)
    Q_PROPERTY(QString ipAddress READ getIpAddress NOTIFY ipAddressChanged)
    Q_PROPERTY(QString macAddress READ getMac NOTIFY macAddressChanged)
    Q_PROPERTY(QList<WifiNetwork *> networkList READ getNetworkList NOTIFY networkListChanged)
    Q_PROPERTY(QList<WifiNetwork *> knownNetworkList READ getKnownNetworkList NOTIFY knownNetworkListChanged)
    Q_PROPERTY(bool scanActive READ getScanActive NOTIFY scanActiveChanged);
    Q_PROPERTY(bool wowlanEnabled READ isWowlan CONSTANT)

 public:
    explicit Wifi(core::Api *core, QObject *parent = nullptr);
    ~Wifi();

    // Q_PROPERTY methods
    bool                 getIsConnected() { return m_isConnected; }
    QObject             *getCurrentNetwork() { return m_currentNetwork; }
    QString              getIpAddress() { return m_ipAddress; }
    QString              getMac() { return m_mac; }
    QList<WifiNetwork *> getNetworkList();
    QList<WifiNetwork *> getKnownNetworkList();
    bool                 getScanActive() { return m_scanActive; }
    bool                 isWowlan() const { return m_wowlan; }

    Q_INVOKABLE void turnOn();
    Q_INVOKABLE void turnOff();
    Q_INVOKABLE void connect(const QString &ssid, const QString &ssidHex, const QString &password,
                             uc::hw::Security::Enum security = uc::hw::Security::AUTO, bool hidden = false);
    Q_INVOKABLE void connectSavedNetwork(int id);
    Q_INVOKABLE void enableSavedNetwork(int id, bool enable);
    Q_INVOKABLE void disconnect();

    Q_INVOKABLE void getWifiStatus();
    Q_INVOKABLE void startNetworkScan();
    Q_INVOKABLE void getWifiScanStatus();
    Q_INVOKABLE void stopNetworkScan();
    Q_INVOKABLE void clearNetworkList();

    Q_INVOKABLE void getAllWifiNetworks();
    Q_INVOKABLE void deleteSavedNetwork(const QString &identifier);
    Q_INVOKABLE void deleteAllNetworks();

    Q_INVOKABLE QString getLastConnectedSsid() { return m_lastConnectedSSid; }
    Q_INVOKABLE QString getLastConnectedPassword() { return m_lastConnectedPassword; }

    void addNetwork(const QString &ssid, const QString &ssidHex, const QString &password,
                    uc::hw::Security::Enum security = uc::hw::Security::AUTO, bool hidden = false);

    static core::WifiEnums::WifiSecurity toApiSecurity(uc::hw::Security::Enum security);
    static uc::hw::Security::Enum        fromApiSecurity(core::WifiEnums::WifiSecurity security);
    void updateNetworkList(bool scanActive, const QList<core::AccessPointScan> &scan);
    void wifiNetworkCommand(int networkId, core::WifiEnums::WifiNetworkCmd command);
    void wifiCommand(core::WifiEnums::WifiCmd command);
    void clearKnownNetworkList();

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
    bool                          m_wowlan = false;

 private:
    QString m_lastConnectedSSid;
    QString m_lastConnectedPassword;

 private slots:
    void onWifiEventChanged(core::WifiEvent::Enum wifiEvent);
};

}  // namespace hw
}  // namespace uc
