// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "softwareUpdate.h"

#include "../logging.h"
#include "../ui/notification.h"
#include "../util.h"

namespace uc {

SoftwareUpdate *SoftwareUpdate::s_instance = nullptr;

SoftwareUpdate::SoftwareUpdate(core::Api *core, QObject *parent)
    : QObject(parent), m_core(core), m_updateDownloadState(DownloadState::Pending) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    qmlRegisterSingletonType<SoftwareUpdate>("SoftwareUpdate", 1, 0, "SoftwareUpdate", &SoftwareUpdate::qmlInstance);

    QObject::connect(m_core, &core::Api::connected, this, [=] {
        checkForUpdate(false);
        m_core->getVersion();
    });
    QObject::connect(m_core, &core::Api::softwareUpdateChanged, this, &SoftwareUpdate::onSoftwareUpdateChanged);
    QObject::connect(m_core, &core::Api::respVersion, this, &SoftwareUpdate::onRespVersion);
}

SoftwareUpdate::~SoftwareUpdate() {
    s_instance = nullptr;
}

void SoftwareUpdate::checkForUpdate(bool force, bool silent) {
    qCDebug(lcSoftwareUpdate()) << "checkForUpdate called, force:" << force << "silent:" << silent;
    int id = m_core->checkSystemUpdate(force);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respSystemUpdateInfo,
        [=](core::SystemUpdate systemUpdate) {
            // success
            qCDebug(lcSoftwareUpdate()) << "checkForUpdate response received, updateInProgress:"
                                        << systemUpdate.updateInProgress
                                        << "installedVersion:" << systemUpdate.installedVersion
                                        << "available updates:" << systemUpdate.available.size();

            if (systemUpdate.updateInProgress) {
                m_updateInProgress = true;
                emit updateInProgressChanged();
            }

            m_currentVersion = systemUpdate.installedVersion;
            emit currentVersionChanged();

            if (systemUpdate.available.size() > 0) {
                qCDebug(lcSoftwareUpdate())
                    << "Update id:" << systemUpdate.available[0].id << systemUpdate.available[0].channel;

                m_updateId = systemUpdate.available[0].id;

                m_updateAvailable = true;
                emit updateAvailableChanged();

                m_updateDownloadState =
                    static_cast<SoftwareUpdate::DownloadState>(systemUpdate.available[0].downloadState);
                emit updateDownloadStateChanged();

                qCDebug(lcSoftwareUpdate())
                    << "Update version:" << systemUpdate.available[0].version
                    << "downloadState:" << static_cast<int>(m_updateDownloadState);

                if (m_updateDownloadState == DownloadState::Error) {
                    qCDebug(lcSoftwareUpdate()) << "Download state is Error, aborting update processing";
                    m_updateAvailable = true;
                    emit updateAvailableChanged();
                    return;
                }

                // we take the first update, which should be the latest
                m_newVersion = systemUpdate.available[0].version;
                emit newVersionChanged();

                m_releaseNotesI18n = systemUpdate.available[0].description;

                m_title = systemUpdate.available[0].title;
                m_releaseNotes.clear();
                m_releaseNotes.append(m_title);
                m_releaseNotes.append("\n\n");

                m_releaseNotes.append(Util::getLanguageString(m_releaseNotesI18n, m_language));

                emit releaseNotesChanged();
            } else {
                qCDebug(lcSoftwareUpdate()) << "No updates available";
                m_updateAvailable = false;
                emit updateAvailableChanged();
            }
        },
        [=](int code, QString message) {
            // fail
            if (!silent) {
                qCWarning(lcSoftwareUpdate()) << "Error while checking for update:" << code << message;
                ui::Notification::createActionableWarningNotification(
                    tr("Update check failed"),
                    tr("There was an error while checking for new updates. Please try again later."));
            }
        });
}

void SoftwareUpdate::startUpdate() {
    qCDebug(lcSoftwareUpdate()) << "startUpdate called, updateId:" << m_updateId;
    int id = m_core->updateSystem(m_updateId);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcSoftwareUpdate()) << "startUpdate request acknowledged by core";
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcSoftwareUpdate()) << "Error while starting update:" << code << message;
            ui::Notification::createActionableWarningNotification(
                tr("Update error"), tr("Couldn't start the software update. Please try again later."));
        });
}

QObject *SoftwareUpdate::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void SoftwareUpdate::onSoftwareUpdateChanged(core::MsgEventTypes::Enum type, QString updateId,
                                             core::SystemUpdateProgress progress) {
    qCDebug(lcSoftwareUpdate()) << "Software update changed" << type << updateId;

    switch (type) {
        case core::MsgEventTypes::Enum::START: {
            qCDebug(lcSoftwareUpdate()) << "Update START event, updateId:" << updateId;
            emit updateStarted();

            m_updateInProgress = true;
            emit updateInProgressChanged();
            break;
        }
        case core::MsgEventTypes::Enum::PROGRESS: {
            qCDebug(lcSoftwareUpdate()) << "Update PROGRESS event, step:" << progress.currentStep
                                        << "/" << progress.totalSteps
                                        << "percent:" << progress.currentPercent
                                        << "state:" << progress.state;

            if (m_totalSteps != progress.totalSteps) {
                m_totalSteps = progress.totalSteps;
                emit totalStepsChanged();
            }

            if (m_currentStep != progress.currentStep) {
                m_currentStep = progress.currentStep;
                emit currentStepChanged();
            }

            m_updateProgress = progress.currentPercent;
            emit updateProgressChanged();

            switch (progress.state) {
                case core::UpdateEnums::UpdateProgressType::SUCCESS:
                    qCDebug(lcSoftwareUpdate()) << "Update succeeded";
                    emit updateSucceeded();
                    break;
                case core::UpdateEnums::UpdateProgressType::DONE:
                    qCDebug(lcSoftwareUpdate()) << "Update done";
                    break;
                case core::UpdateEnums::UpdateProgressType::FAILURE: {
                    m_updateInProgress = false;
                    emit updateInProgressChanged();
                    emit updateFailed(tr("Software update has failed."));

                    m_totalSteps = 0;
                    emit totalStepsChanged();
                    m_currentStep = 0;
                    emit currentStepChanged();

                    break;
                }
                default:
                    break;
            }

            break;
        }
        case core::MsgEventTypes::Enum::STOP: {
            qCDebug(lcSoftwareUpdate()) << "Update STOP event, updateId:" << updateId
                                        << "state:" << progress.state
                                        << "step:" << progress.currentStep
                                        << "/" << progress.totalSteps
                                        << "percent:" << progress.currentPercent;

            m_updateInProgress = false;
            emit updateInProgressChanged();

            if (progress.state == core::UpdateEnums::UpdateProgressType::FAILURE) {
                qCWarning(lcSoftwareUpdate()) << "Update STOP with FAILURE state";
                emit updateFailed(tr("Software update has failed."));
            } else {
                qCDebug(lcSoftwareUpdate()) << "Update STOP with non-failure state, emitting updateSucceeded";
                emit updateSucceeded();
            }

            m_totalSteps = 0;
            emit totalStepsChanged();
            m_currentStep = 0;
            emit currentStepChanged();

            break;
        }
        default:
            break;
    }
}

void SoftwareUpdate::onLanguageChanged(QString language) {
    m_language = language.split("_")[0];

    m_releaseNotes.clear();
    m_releaseNotes.append(m_title);
    m_releaseNotes.append("\n\n");

    m_releaseNotes.append(Util::getLanguageString(m_releaseNotesI18n, m_language));

    emit releaseNotesChanged();
}

void SoftwareUpdate::onRespVersion(int reqId, int code, QString deviceName, QString api, QString core, QString ui,
                                   QString os, QStringList integrations) {
    Q_UNUSED(reqId)
    Q_UNUSED(code)
    Q_UNUSED(deviceName)
    Q_UNUSED(os)
    Q_UNUSED(integrations)

    m_apiVersion = api;
    emit apiVersionChanged();

    m_coreVersion = core;
    emit coreVersionChanged();

    m_uiVersion = ui;
    emit uiVersionChanged();

    qCDebug(lcSoftwareUpdate()) << "Api" << api << "Core" << core << "Ui" << ui;
}

}  // namespace uc
