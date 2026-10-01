// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/core.h"
#include "softwareupdate/softwareUpdate.h"
#include "ui/notification.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

using uc::SoftwareUpdate;
using uc::core::MsgEventTypes;
using uc::core::SystemUpdateProgress;
using uc::core::UpdateEnums;

/**
 * The core's software update events, as remote-core sends them (actors/ota/system): an installation starts
 * with START, reports PROGRESS (RUN, PROGRESS, SUCCESS, DONE, or FAILURE) and reboots the remote on success.
 * STOP is only sent when the update client cannot be started, always with a FAILURE state. A download runs
 * without START and reports PROGRESS DOWNLOAD, or PROGRESS FAILURE when it fails.
 */
class testSoftwareUpdate : public QObject {
    Q_OBJECT

 private slots:
    void downloadFailure_isNotAnInstallationFailure();
    void installationFailure_isReported();
    void stopAfterAFailedStart_isReported();

 private:
    static SystemUpdateProgress progress(UpdateEnums::UpdateProgressType state);
};

SystemUpdateProgress testSoftwareUpdate::progress(UpdateEnums::UpdateProgressType state) {
    SystemUpdateProgress p;
    p.state = state;
    return p;
}

void testSoftwareUpdate::downloadFailure_isNotAnInstallationFailure() {
    uc::core::Api        api(kTestUrl);
    uc::ui::Notification notification;
    SoftwareUpdate       softwareUpdate(&api);

    QSignalSpy failed(&softwareUpdate, &SoftwareUpdate::updateFailed);

    SystemUpdateProgress downloading = progress(UpdateEnums::UpdateProgressType::DOWNLOAD);
    downloading.downloadPercent = 40;
    emit api.softwareUpdateChanged(MsgEventTypes::PROGRESS, QStringLiteral("update-1"), downloading);
    QCOMPARE(softwareUpdate.getUpdateDownloadState(), SoftwareUpdate::Downloading);

    emit api.softwareUpdateChanged(MsgEventTypes::PROGRESS, QStringLiteral("update-1"),
                                   progress(UpdateEnums::UpdateProgressType::FAILURE));

    // the download failed: shown on the software update page, the installation screen is not involved
    QCOMPARE(failed.count(), 0);
    QCOMPARE(softwareUpdate.getUpdateDownloadState(), SoftwareUpdate::Error);
    QVERIFY(!softwareUpdate.getUpdateInProgress());
}

void testSoftwareUpdate::installationFailure_isReported() {
    uc::core::Api        api(kTestUrl);
    uc::ui::Notification notification;
    SoftwareUpdate       softwareUpdate(&api);

    QSignalSpy started(&softwareUpdate, &SoftwareUpdate::updateStarted);
    QSignalSpy failed(&softwareUpdate, &SoftwareUpdate::updateFailed);

    emit api.softwareUpdateChanged(MsgEventTypes::START, QStringLiteral("update-1"), SystemUpdateProgress());
    QCOMPARE(started.count(), 1);
    QVERIFY(softwareUpdate.getUpdateInProgress());

    emit api.softwareUpdateChanged(MsgEventTypes::PROGRESS, QStringLiteral("update-1"),
                                   progress(UpdateEnums::UpdateProgressType::RUN));
    emit api.softwareUpdateChanged(MsgEventTypes::PROGRESS, QStringLiteral("update-1"),
                                   progress(UpdateEnums::UpdateProgressType::FAILURE));

    QCOMPARE(failed.count(), 1);
    QVERIFY(!softwareUpdate.getUpdateInProgress());
}

void testSoftwareUpdate::stopAfterAFailedStart_isReported() {
    uc::core::Api        api(kTestUrl);
    uc::ui::Notification notification;
    SoftwareUpdate       softwareUpdate(&api);

    QSignalSpy failed(&softwareUpdate, &SoftwareUpdate::updateFailed);
    QSignalSpy succeeded(&softwareUpdate, &SoftwareUpdate::updateSucceeded);

    emit api.softwareUpdateChanged(MsgEventTypes::START, QStringLiteral("update-1"), SystemUpdateProgress());
    emit api.softwareUpdateChanged(MsgEventTypes::STOP, QStringLiteral("update-1"),
                                   progress(UpdateEnums::UpdateProgressType::FAILURE));

    QCOMPARE(failed.count(), 1);
    QCOMPARE(succeeded.count(), 0);
    QVERIFY(!softwareUpdate.getUpdateInProgress());
}

QTEST_GUILESS_MAIN(testSoftwareUpdate)

#include "test_software_update.moc"
