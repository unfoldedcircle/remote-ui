// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockController.h"

#include "../logging.h"
#include "../ui/notification.h"
#include "../util.h"

namespace uc {
namespace dock {

DockController *DockController::s_instance = nullptr;

DockController::DockController(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    QObject::connect(m_core, &core::Api::connected, this, &DockController::onCoreConnected);

    QObject::connect(m_core, &core::Api::dockDiscoveryStarted, this, &DockController::onDockDiscoveryStarted);
    QObject::connect(m_core, &core::Api::dockDiscoveryStopped, this, &DockController::onDockDiscoveryStopped);
    QObject::connect(m_core, &core::Api::dockDiscovered, this, &DockController::onDockDiscovered);
    QObject::connect(m_core, &core::Api::dockSetupChanged, this, &DockController::onDockSetupChanged);

    QObject::connect(m_core, &core::Api::dockAdded, this, &DockController::onDockAdded);
    QObject::connect(m_core, &core::Api::dockChanged, this, &DockController::onDockChanged);
    QObject::connect(m_core, &core::Api::dockDeleted, this, &DockController::onDockDeleted);
    QObject::connect(m_core, &core::Api::dockStateChanged, this, &DockController::onDockStateChanged);
    QObject::connect(m_core, &core::Api::dockUpdateChanged, this, &DockController::onDockUpdateChanged);

    qmlRegisterSingletonType<DockController>("Dock.Controller", 1, 0, "DockController", &DockController::qmlInstance);
    qRegisterMetaType<ConfiguredDock::State>("DockStates");
    qmlRegisterUncreatableType<ConfiguredDock>("Dock.Controller", 1, 0, "DockStates", "Enum is not a type");
}

DockController::~DockController() {
    s_instance = nullptr;
}

QObject *DockController::getDiscoveredDock(const QString &dockId) {
    return m_discoveredDocks.get(dockId);
}

QObject *DockController::getConfiguredDock(const QString &dockId) {
    return m_configuredDocks.get(dockId);
}

void DockController::startDiscovery() {
    m_discoveredDocks.clear();

    qCDebug(lcDockController()) << "Starting discovery";

    int id = m_core->startDockDiscovery();

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcDockController()) << "Successfully started dock discovery";
            emit discoveryStarted();
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcDockController()) << "Error starting dock discovery:" << code << message;
            ui::Notification::createActionableWarningNotification(
                tr("Failed to start dock discovery"), tr("There was an error starting dock discovery: %1").arg(message),
                "uc:warning",
                [](QVariant param) {
                    DockController *dc = qvariant_cast<DockController *>(param);
                    dc->startDiscovery();
                },
                QVariant::fromValue(s_instance), tr("Try again"));
        });
}

void DockController::stopDiscovery() {
    qCDebug(lcDockController()) << "Stopping discovery";

    int id = m_core->stopDockDiscovery();

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcDockController()) << "Error stopping dock discovery:" << code << message;
            ui::Notification::createActionableWarningNotification(
                tr("Failed to stop dock discovery"), tr("There was an error stopping dock discovery: %1").arg(message),
                "uc:warning",
                [](QVariant param) {
                    DockController *dc = qvariant_cast<DockController *>(param);
                    dc->stopDiscovery();
                },
                QVariant::fromValue(s_instance), tr("Try again"));
        });
}

void DockController::selectDockToSetup(const QString &dockId) {
    m_dockToSetup = dockId;
    emit dockToSetupChanged(dockId);
}

void DockController::setupDock(const QString &dockId, const QString &friendlyName, const QString &password,
                               const QString &discoveryType, const QString &wifiSsid, const QString &wifiPassword) {
    qCDebug(lcDockController()) << "Creating dock setup for dock with id" << dockId;

    int id = m_core->createDockSetup(dockId, friendlyName,
                                     Util::convertStringToEnum<core::DockSetupEnums::DockDiscoveryType>(discoveryType));

    m_core->onResponseWithErrorResult(
        id, &core::Api::respDockSetupStatus,
        [=](QString dockId, core::DockSetupEnums::DockSetupState state, core::DockSetupEnums::DockSetupError error) {
            // success
            qCDebug(lcDockController()) << "DOCK CREATE SETUP RESPONSE"
                                        << "Dockid" << dockId << "state" << state << "error" << error;

            if (state != core::DockSetupEnums::DockSetupState::ERROR) {
                // start dock setup here after success
                startDockSetup(dockId, friendlyName, password, wifiSsid, wifiPassword);
            } else {
                emit setupFinished(false, Util::convertEnumToString(error));
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcDockController()) << "Error during dock setup:" << code << message;
            emit setupFinished(false, message);
        });
}

void DockController::stopSetup(const QString &dockId) {
    int id = m_core->stopDockSetup(dockId);

    m_core->onResult(
        id, [=]() {},
        [=](int code, QString message) {
            if (code != 404) {
                qCWarning(lcDockController()) << "Couldn't stop dock setup" << code << message;
                ui::Notification::createNotification(message, true);
            }
        });
}

void DockController::identify(const QString &dockId) {
    int id = m_core->dockCommand(dockId, core::DockEnums::DockCommands::IDENTIFY);

    m_core->onResult(
        id, [=]() {},
        [=](int code, QString message) {
            qCWarning(lcDockController()) << "Identify command failed" << code << message;
            ui::Notification::createNotification(message, true);
        });
}

void DockController::connect(const QString &dockId) {
    int id = m_core->connectDock(dockId);

    m_core->onResult(
        id, [=]() {},
        [=](int code, QString message) {
            qCWarning(lcDockController()) << "Connect command failed" << code << message;
            ui::Notification::createNotification(message, true);
        });
}

void DockController::deleteDock(const QString &dockId) {
    int id = m_core->deleteDock(dockId);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcDockController) << "Failed to delete dock" << code << message;
            ui::Notification::createNotification(message, true);
        });
}

void DockController::factoryReset(const QString &dockId) {
    int id = m_core->dockCommand(dockId, core::DockEnums::DockCommands::RESET);

    m_core->onResult(
        id, [=]() {},
        [=](int code, QString message) {
            qCWarning(lcDockController()) << "Reset command failed" << code << message;
            ui::Notification::createNotification(message, true);
        });
}

void DockController::updateDockName(const QString &dockId, const QString &name) {
    int id = m_core->updateDock(dockId, name);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respDock,
        [=](core::DockConfiguration dock) {
            Q_UNUSED(dock)
            emit dockNameChanged(dockId);
        },
        [=](int code, QString message) {
            qCWarning(lcDockController()) << "Failed to update dock name" << code << message;
            emit error(message);
        });
}

void DockController::updateDockPassword(const QString &dockId, const QString &password) {
    int id = m_core->updateDock(dockId, QString(), QString(), password);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respDock,
        [=](core::DockConfiguration dock) {
            Q_UNUSED(dock)
            emit dockPasswordChanged(dockId);
        },
        [=](int code, QString message) {
            qCWarning(lcDockController()) << "Failed to update dock password" << code << message;
            emit error(message);
        });
}

void DockController::setDockLedBrightness(const QString &dockId, int brightness) {
    int id =
        m_core->dockCommand(dockId, core::DockEnums::DockCommands::SET_LED_BRIGHTNESS, QString::number(brightness));

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcDockController) << "Failed to set led brightness" << code << message;
            ui::Notification::createNotification(message, true);
        });
}

void DockController::getDocks(int limit, int page) {
    int id = m_core->getDocks(limit, page);

    qCDebug(lcDockController()) << "Get docks, limit:" << limit << "page:" << page;

    m_core->onResponseWithErrorResult(
        id, &core::Api::respDocks,
        [=](QList<core::DockConfiguration> docks, int count, int limit, int page) {
            qCDebug(lcIntegrationController()) << "Docks:" << count << "page:" << page << "limit:" << limit;

            if (count > 0) {
                m_configuredDocks.totalItems = count;
                if (m_configuredDocks.limit == 0) {
                    m_configuredDocks.limit = limit;
                    m_configuredDocks.totalPages = qCeil(static_cast<float>(count) / static_cast<float>(limit));
                }
                m_configuredDocks.lastPageLoaded = page;

                if (docks.size() > 0) {
                    for (QList<core::DockConfiguration>::iterator i = docks.begin(); i != docks.end(); i++) {
                        qCDebug(lcDockController()) << i->name << m_configuredDocks.contains(i->id);

                        if (!m_configuredDocks.contains(i->id)) {
                            m_configuredDocks.append(new ConfiguredDock(
                                i->id, i->name, i->customWsUrl, i->active, i->model, i->connectionType, i->version,
                                static_cast<ConfiguredDock::State>(i->state), i->learningActive, i->description, this));
                            qCDebug(lcDockController()) << "Dock created:" << i->name << i->id;
                        }
                    }
                }
            }

            emit docksLoaded();
        },
        [=](int code, QString message) { qCWarning(lcDockController()) << "Cannot get docks" << code << message; });
}

QObject *DockController::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void DockController::startDockSetup(const QString &dockId, const QString &friendlyName, const QString &password,
                                    const QString &wifiSsid, const QString &wifiPassword) {
    qCDebug(lcDockController()) << "Start dock setup for" << dockId;

    int id = m_core->startDockSetup(dockId, friendlyName, password, wifiSsid, wifiPassword);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respDockSetupStatus,
        [=](QString dockId, core::DockSetupEnums::DockSetupState state, core::DockSetupEnums::DockSetupError error) {
            // success
            qCDebug(lcDockController()) << "DOCK SETUP START RESPONSE"
                                        << "Dockid" << dockId << "state" << state << "error" << error;

            if (state == core::DockSetupEnums::DockSetupState::ERROR) {
                emit setupFinished(false, Util::convertEnumToString(error));
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcDockController()) << "Error starting dock setup:" << code << message;
        });
}

void DockController::onCoreConnected() {
    getDocks();
}

void DockController::onDockDiscoveryStarted() {
    qCDebug(lcDockController()) << "Dock discovery started";
    emit discoveryStarted();
}

void DockController::onDockDiscoveryStopped() {
    qCDebug(lcDockController()) << "Dock discovery stopped";
    emit discoveryStopped();
}

void DockController::onDockDiscovered(core::DockDiscovery dock) {
    qCDebug(lcDockController()) << "Dock discovered" << dock.friendlyName;

    if (!dock.configured && !m_discoveredDocks.contains(dock.id)) {
        m_discoveredDocks.append(new DiscoveredDock(
            dock.id, dock.configured, dock.friendlyName, dock.address, dock.model, dock.version,
            Util::convertEnumToString(dock.discoveryType), dock.bluetoothSignal, dock.bluetoothLastSeenSeconds, this));
    }
}

void DockController::onDockSetupChanged(core::MsgEventTypes::Enum type, QString dockId,
                                        core::DockSetupEnums::DockSetupState state,
                                        core::DockSetupEnums::DockSetupError error) {
    qCDebug(lcDockController()) << "--------------------------------------------------------------------------------";
    qCDebug(lcDockController()) << "DOCK SETUP CHANGED:" << type << "Dockid" << dockId << "state " << state << "error "
                                << error;
    qCDebug(lcDockController()) << "--------------------------------------------------------------------------------";

    switch (type) {
        case core::MsgEventTypes::Enum::START:
            if (state != core::DockSetupEnums::DockSetupState::ERROR) {
                emit setupStarted();
            }
            break;

        case core::MsgEventTypes::Enum::STOP:
            if (state == core::DockSetupEnums::DockSetupState::OK) {
                emit setupFinished(true);
            }
            break;
        default:
            break;
    }

    if (state == core::DockSetupEnums::DockSetupState::ERROR) {
        emit setupFinished(false, Util::convertEnumToString(error));
    }

    QString stateText;

    switch (state) {
        case core::DockSetupEnums::DockSetupState::CONFIGURING:
            stateText = tr("Configuring");
            break;
        case core::DockSetupEnums::DockSetupState::RESTARTING:
            stateText = tr("Restarting");
            break;
        case core::DockSetupEnums::DockSetupState::UPLOADING:
            stateText = tr("Uploading");
            break;
        default:
            break;
    }

    emit setupChanged(stateText);
}

void DockController::onDockAdded(QString dockId, core::DockConfiguration dock) {
    if (!m_configuredDocks.contains(dockId)) {
        m_configuredDocks.append(new ConfiguredDock(
            dockId, dock.name, dock.customWsUrl, dock.active, dock.model, dock.connectionType, dock.version,
            static_cast<ConfiguredDock::State>(dock.state), dock.learningActive, dock.description, this));

        qCDebug(lcDockController()) << "Dock added:" << dockId;
        emit dockAdded(dockId);
    }
}

void DockController::onDockChanged(QString dockId, core::DockConfiguration dock) {
    if (m_configuredDocks.contains(dockId)) {
        m_configuredDocks.updateActive(dockId, dock.active);
        m_configuredDocks.updateLearningActive(dockId, dock.learningActive);

        if (!dock.name.isEmpty()) {
            m_configuredDocks.updateName(dockId, dock.name);
        }
        if (!dock.customWsUrl.isEmpty()) {
            m_configuredDocks.updateCustomWsUrl(dockId, dock.customWsUrl);
        }
        if (!dock.connectionType.isEmpty()) {
            m_configuredDocks.updateConnectionType(dockId, dock.connectionType);
        }
        if (!dock.version.isEmpty()) {
            m_configuredDocks.updateVersion(dockId, dock.version);
        }
        if (!dock.description.isEmpty()) {
            m_configuredDocks.updateDescription(dockId, dock.description);
        }

        qCDebug(lcDockController()) << "Dock updated:" << dockId;
        emit dockChanged(dockId);
    }
}

void DockController::onDockDeleted(QString dockId) {
    if (m_configuredDocks.contains(dockId)) {
        m_configuredDocks.removeItem(dockId);

        qCDebug(lcDockController()) << "Dock deleted:" << dockId;
        emit dockDeleted(dockId);
    }
}

void DockController::onDockStateChanged(QString dockId, core::DockEnums::DockState state) {
    if (m_configuredDocks.contains(dockId)) {
        m_configuredDocks.updateState(dockId, static_cast<ConfiguredDock::State>(state));

        qCDebug(lcDockController()) << "Dock state updated" << dockId << state;
    }
}

void DockController::onDockUpdateChanged(core::MsgEventTypes::Enum type, QString dockId, QString updateId,
                                         QString version, int progress, core::DockSetupEnums::DockSetupState state,
                                         core::DockSetupEnums::DockSetupError error) {}

}  // namespace dock
}  // namespace uc
