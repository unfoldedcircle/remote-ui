// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>

#include "../core/core.h"

namespace uc {

class SoftwareUpdate : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool updateAvailable READ getUpdateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(DownloadState updateDownloadState READ getUpdateDownloadState NOTIFY updateDownloadStateChanged)
    Q_PROPERTY(QString currentVersion READ getCurrentVersion NOTIFY currentVersionChanged)
    Q_PROPERTY(QString newVersion READ getNewVersion NOTIFY newVersionChanged)
    Q_PROPERTY(QString releaseNotes READ getReleaseNotes NOTIFY releaseNotesChanged)
    Q_PROPERTY(bool updateInProgress READ getUpdateInProgress NOTIFY updateInProgressChanged)
    Q_PROPERTY(int updateProgress READ getUpdateProgress NOTIFY updateProgressChanged)
    Q_PROPERTY(int currentStep READ getCurrentStep NOTIFY currentStepChanged);
    Q_PROPERTY(int totalSteps READ getTotalSteps NOTIFY totalStepsChanged);

    Q_PROPERTY(QString apiVersion READ getApiVersion NOTIFY apiVersionChanged);
    Q_PROPERTY(QString coreVersion READ getCoreVersion NOTIFY coreVersionChanged);
    Q_PROPERTY(QString uiVersion READ getUiVersion NOTIFY uiVersionChanged);

 public:
    explicit SoftwareUpdate(core::Api* core, QObject* parent = nullptr);
    ~SoftwareUpdate();

    enum DownloadState {
        Pending = core::UpdateEnums::DownloadState::PENDING,
        Downloading = core::UpdateEnums::DownloadState::DOWNLOADING,
        Downloaded = core::UpdateEnums::DownloadState::DOWNLOADED,
        Error = core::UpdateEnums::DownloadState::ERROR,
    };
    Q_ENUM(DownloadState)

    bool          getUpdateAvailable() { return m_updateAvailable; }
    DownloadState getUpdateDownloadState() { return m_updateDownloadState; }
    QString       getCurrentVersion() { return m_currentVersion; }
    QString       getNewVersion() { return m_newVersion; }
    QString       getReleaseNotes() { return m_releaseNotes; }
    bool          getUpdateInProgress() { return m_updateInProgress; }
    int           getUpdateProgress() { return m_updateProgress; }
    int           getCurrentStep() { return m_currentStep; }
    int           getTotalSteps() { return m_totalSteps; }

    QString getApiVersion() { return m_apiVersion; }
    QString getCoreVersion() { return m_coreVersion; }
    QString getUiVersion() { return APP_VERSION; }

    Q_INVOKABLE void checkForUpdate(bool force = true, bool silent = false);
    Q_INVOKABLE void startUpdate();

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void updateAvailableChanged();
    void updateDownloadStateChanged();
    void currentVersionChanged();
    void newVersionChanged();
    void releaseNotesChanged();
    void updateInProgressChanged();
    void updateProgressChanged();
    void currentStepChanged();
    void totalStepsChanged();
    void updateStarted();
    void updateSucceeded();
    void updateFailed(QString error);
    void apiVersionChanged();
    void coreVersionChanged();
    void uiVersionChanged();

 private:
    static SoftwareUpdate* s_instance;

    core::Api* m_core;
    QString    m_language = "en";

    bool          m_updateAvailable = false;
    DownloadState m_updateDownloadState;
    QString       m_currentVersion = "N/A";
    QString       m_newVersion = "N/A";
    QString       m_updateId;
    QString       m_title;
    QString       m_releaseNotes = "N/A";
    QVariantMap   m_releaseNotesI18n;
    bool          m_updateInProgress = false;
    int           m_updateProgress = 0;
    int           m_currentStep = 0;
    int           m_totalSteps = 0;

    QString m_apiVersion;
    QString m_coreVersion;
    QString m_uiVersion;

 public slots:
    void onSoftwareUpdateChanged(core::MsgEventTypes::Enum type, QString updateId, core::SystemUpdateProgress progress);
    void onLanguageChanged(QString language);
    void onRespVersion(int reqId, int code, QString deviceName, QString api, QString core, QString ui, QString os,
                       QStringList integrations);
};
}  // namespace uc
