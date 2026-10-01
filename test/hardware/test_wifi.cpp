// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/core.h"
#include "system/wifi.h"
#include "ui/notification.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

using uc::hw::Security;
using uc::hw::Wifi;

/**
 * The security type of the current connection is derived from the key management string wpa_supplicant reports
 * through the core. A Remote 3 on a WPA3 network reports "SAE", which used to convert to -1.
 */
class testWifi : public QObject {
    Q_OBJECT

 private slots:
    void securityFromKeyManagement_data();
    void securityFromKeyManagement();

    void clearNetworkList_announcesTheChangeBeforeTheObjectsGo();
    void emptyScanResult_updatesTheScanState();

 private:
    static uc::core::AccessPointScan accessPoint(const QString& ssid, int signal);
};

uc::core::AccessPointScan testWifi::accessPoint(const QString& ssid, int signal) {
    uc::core::AccessPointScan ap;
    ap.ssid = ssid;
    ap.ssidHex = QString::fromLatin1(ssid.toUtf8().toHex());
    ap.signalLevel = signal;
    ap.auth = QStringLiteral("WPA2-PSK");
    return ap;
}

void testWifi::securityFromKeyManagement_data() {
    QTest::addColumn<QString>("keyManagement");
    QTest::addColumn<Security::Enum>("expected");

    QTest::newRow("empty") << "" << Security::OPEN;
    QTest::newRow("none") << "NONE" << Security::OPEN;
    QTest::newRow("wpa psk") << "WPA-PSK" << Security::WPA_PSK;
    QTest::newRow("wpa2 psk") << "WPA2-PSK" << Security::WPA2_PSK;
    QTest::newRow("wpa2 psk sha256") << "WPA2-PSK-SHA256" << Security::WPA2_PSK;
    QTest::newRow("wpa psk sha256 (wpa_supplicant spelling)") << "WPA-PSK-SHA256" << Security::WPA_PSK;
    QTest::newRow("wpa3 sae, as a Remote 3 reports it") << "SAE" << Security::WPA3_SAE;
    QTest::newRow("fast transition sae") << "FT-SAE" << Security::WPA3_SAE;
    QTest::newRow("wpa eap") << "WPA-EAP" << Security::WPA_EAP;
    QTest::newRow("wpa2 eap") << "WPA2-EAP" << Security::WPA2_EAP;
    QTest::newRow("lower case") << "wpa2-psk" << Security::WPA2_PSK;
    // never -1: unknown means encrypted
    QTest::newRow("unknown") << "OWE" << Security::WPA2_PSK;
}

void testWifi::securityFromKeyManagement() {
    QFETCH(QString, keyManagement);
    QFETCH(Security::Enum, expected);

    QCOMPARE(Wifi::securityFromKeyManagement(keyManagement), expected);
}

/**
 * The WiFi settings clear the list of available networks when a dialog opens on top of them. The objects were
 * deleted without the list being announced as changed: the rows of the settings page kept pointing at deleted
 * objects until the next scan result rebuilt them.
 */
void testWifi::clearNetworkList_announcesTheChangeBeforeTheObjectsGo() {
    uc::core::Api        api(kTestUrl);
    uc::ui::Notification notification;
    Wifi                 wifi(&api);

    wifi.updateNetworkList(false, {accessPoint("Home", -50), accessPoint("Office", -70)});
    QCOMPARE(wifi.getNetworkList().size(), 2);
    QPointer<uc::hw::WifiNetwork> row = wifi.getNetworkList().first();

    QSignalSpy listChanged(&wifi, &Wifi::networkListChanged);
    bool       rowAliveWhenAnnounced = false;
    QObject::connect(&wifi, &Wifi::networkListChanged, &wifi, [&] { rowAliveWhenAnnounced = !row.isNull(); });

    wifi.clearNetworkList();

    QCOMPARE(listChanged.count(), 1);
    QVERIFY(wifi.getNetworkList().isEmpty());
    // the views let go of the row while it still exists, it is deleted afterwards
    QVERIFY(rowAliveWhenAnnounced);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(row.isNull());

    // nothing to clear: nothing announced
    wifi.clearNetworkList();
    QCOMPARE(listChanged.count(), 1);
}

void testWifi::emptyScanResult_updatesTheScanState() {
    uc::core::Api        api(kTestUrl);
    uc::ui::Notification notification;
    Wifi                 wifi(&api);

    wifi.updateNetworkList(true, {accessPoint("Home", -50)});
    QVERIFY(wifi.getScanActive());
    QCOMPARE(wifi.getNetworkList().size(), 1);

    // the scan ended without access points: the scan state follows, the list is kept
    wifi.updateNetworkList(false, {});
    QVERIFY(!wifi.getScanActive());
    QCOMPARE(wifi.getNetworkList().size(), 1);
}

QTEST_GUILESS_MAIN(testWifi)

#include "test_wifi.moc"
