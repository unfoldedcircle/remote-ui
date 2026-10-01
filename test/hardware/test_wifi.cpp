// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "system/wifi.h"

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
};

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

QTEST_GUILESS_MAIN(testWifi)

#include "test_wifi.moc"
