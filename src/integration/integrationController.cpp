// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "integrationController.h"

#include "../logging.h"

namespace uc {
namespace integration {

IntegrationController *IntegrationController::s_instance = nullptr;

IntegrationController::IntegrationController(core::Api *core, const QString &language, QObject *parent)
    : QObject(parent), m_core(core), m_language(language), m_integrationDriverToSetup(nullptr) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    QObject::connect(m_core, &core::Api::connected, this, &IntegrationController::getAllIntegrations);
    QObject::connect(m_core, &core::Api::connected, this, &IntegrationController::getAllIntegrationDrivers);
    QObject::connect(m_core, &core::Api::integrationDriverStateChanged, this,
                     &IntegrationController::onIntegrationDriverStateChanged);
    QObject::connect(m_core, &core::Api::integrationDeviceStateChanged, this,
                     &IntegrationController::onIntegrationDeviceStateChanged);

    QObject::connect(m_core, &core::Api::integationDriverDiscoveryStarted, this,
                     &IntegrationController::onDriverDiscoveryStarted);
    QObject::connect(m_core, &core::Api::integrationDriverDiscoveryStopped, this,
                     &IntegrationController::onDriverDiscoveryStopped);

    QObject::connect(m_core, &core::Api::integrationDriverDiscovered, this, &IntegrationController::onDriverDiscovered);

    QObject::connect(m_core, &core::Api::integrationSetupChange, this,
                     &IntegrationController::onIntegrationSetupChange);

    QObject::connect(m_core, &core::Api::integrationDriverAdded, this, &IntegrationController::onDriverAdded);
    QObject::connect(m_core, &core::Api::integrationDriverChanged, this, &IntegrationController::onDriverChanged);
    QObject::connect(m_core, &core::Api::integrationDriverDeleted, this, &IntegrationController::onDriverDeleted);

    QObject::connect(m_core, &core::Api::integrationAdded, this, &IntegrationController::onIntegrationAdded);
    QObject::connect(m_core, &core::Api::integrationChanged, this, &IntegrationController::onIntegrationChanged);
    QObject::connect(m_core, &core::Api::integrationDeleted, this, &IntegrationController::onIntegrationDeleted);

    QObject::connect(this, &IntegrationController::integrationStatusLoaded, this,
                     &IntegrationController::onIntegrationStatusLoaded);
    QObject::connect(this, &IntegrationController::integrationDriverLoaded, this,
                     &IntegrationController::onIntegrationDriverLoaded);
    QObject::connect(this, &IntegrationController::integrationDriversLoaded, this,
                     &IntegrationController::onIntegrationDriversLoaded);
    QObject::connect(this, &IntegrationController::integrationsLoaded, this,
                     &IntegrationController::onIntegrationsLoaded);

    m_integrationDriversFilter.setSourceModel(&m_integrationDrivers);
    m_integrationDriversFilter.setDynamicSortFilter(true);
    m_integrationDriversFilter.setFilterCaseSensitivity(Qt::CaseInsensitive);

    qmlRegisterSingletonType<IntegrationController>("Integration.Controller", 1, 0, "IntegrationController",
                                                    &IntegrationController::qmlInstance);

    qmlRegisterUncreatableType<IntegrationController>("Integration.Controller", 1, 0, "IntegrationControllerEnums",
                                                      "Enum is not a type");
    qRegisterMetaType<IntegrationController::IntegrationSetupType>("Setup Step Type");
    qRegisterMetaType<IntegrationController::SetupState>("Setup State");
}

IntegrationController::~IntegrationController() {
    s_instance = nullptr;
}

void IntegrationController::getAllIntegrationStatus() {
    getIntegrationStatus();
}

void IntegrationController::getAllIntegrationDrivers() {
    m_integrationDrivers.clear();
    m_integrationDriversLoaded = 0;
    getIntegrationDrivers();
}

void IntegrationController::getAllIntegrations() {
    m_integrations.clear();
    getIntegrations();
}

void IntegrationController::getIntegrationStatus(int limit, int page) {
    int id = m_core->getIntegrationStatus(limit, page);

    qCDebug(lcIntegrationController()) << "Call get integration status";

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrationStatus,
        [=](QList<core::IntegrationStatus> integrationStatus, int count, int limit, int page) {
            // success

            qCDebug(lcIntegrationController())
                << "Integration status:" << count << "page:" << page << "limit:" << limit;

            if (count > 0) {
                m_integrationStatusTotalItems = count;
                if (m_integrationStatusLimit == 0) {
                    m_integrationStatusLimit = limit;
                    m_integrationStatusTotalPages = qCeil(static_cast<float>(count) / static_cast<float>(limit));
                }
                m_integrationStatusLastPageLoaded = page;

                for (QList<core::IntegrationStatus>::iterator i = integrationStatus.begin();
                     i != integrationStatus.end(); i++) {
                    if (m_integrations.contains(i->integrationId)) {
                        auto integration = m_integrations.get(i->integrationId);

                        if (!integration) {
                            continue;
                        }

                        m_integrations.setState(i->integrationId, i->deviceState.toLower());

                        if (m_integrationDrivers.contains(integration->getDriverId())) {
                            auto driver = m_integrationDrivers.get(integration->getDriverId());

                            if (driver) {
                                m_integrationDrivers.setState(driver->getId(), i->driverState.toLower());
                            }
                        }
                    }
                }
            }
            emit integrationStatusLoaded();
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot get integration drivers" << code << message;
        });
}

void IntegrationController::getIntegrationDrivers(int limit, int page) {
    int id = m_core->getIntegrationDrivers(limit, page);

    qCDebug(lcIntegrationController()) << "Call get integration drivers";

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrationDrivers,
        [=](QList<core::IntegrationDriver> integrationDrivers, int count, int limit, int page) {
            // success

            qCDebug(lcIntegrationController()) << "Integrations:" << count << "page:" << page << "limit:" << limit;

            if (count > 0) {
                m_integrationDrivers.totalItems = count;
                if (m_integrationDrivers.limit == 0) {
                    m_integrationDrivers.limit = limit;
                    m_integrationDrivers.totalPages = qCeil(static_cast<float>(count) / static_cast<float>(limit));
                }
                m_integrationDrivers.lastPageLoaded = page;

                if (integrationDrivers.size() > 0) {
                    for (QList<core::IntegrationDriver>::iterator i = integrationDrivers.begin();
                         i != integrationDrivers.end(); i++) {
                        // get detailed driver info
                        getIntegrationDriver(i->id);
                    }
                }
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot get integration drivers" << code << message;
        });
}

void IntegrationController::getIntegrations(int limit, int page) {
    int id = m_core->getIntegrations(limit, page);

    qCDebug(lcIntegrationController()) << "Call get integrations";

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrations,
        [=](QList<core::Integration> integrations, int count, int limit, int page) {
            // success
            qCDebug(lcIntegrationController()) << "Integrations:" << count << "page:" << page << "limit:" << limit;

            if (count > 0) {
                m_integrations.totalItems = count;
                if (m_integrations.limit == 0) {
                    m_integrations.limit = limit;
                    m_integrations.totalPages = qCeil(static_cast<float>(count) / static_cast<float>(limit));
                }
                m_integrations.lastPageLoaded = page;

                if (integrations.size() > 0) {
                    for (QList<core::Integration>::iterator i = integrations.begin(); i != integrations.end(); i++) {
                        qCDebug(lcIntegrationController()) << i->name << m_integrations.contains(i->id);

                        // add integration to model
                        if (!m_integrations.contains(i->id)) {
                            m_integrations.append(new Integration(i->id, i->driverId, i->deviceId, i->name, i->icon,
                                                                  i->enabled, i->setupData, m_language, false, this));
                            qCDebug(lcIntegrationController()) << "Integration created:" << i->name << i->id;
                        }
                    }
                }
            }

            emit integrationsLoaded();
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot get integrations" << code << message;
        });
}

QObject *IntegrationController::getModelItem(const QString &id) {
    return m_integrations.get(id);
}

QObject *IntegrationController::getDriversModelItem(const QString &id) {
    return m_integrationDrivers.get(id);
}

void IntegrationController::deleteIntegration(const QString &integrationId) {
    int id = m_core->deleteIntegration(integrationId);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot delete integrations" << code << message;
            ui::Notification::createNotification(tr("Error while deleting integration"), true);
        });
}

void IntegrationController::deleteIntegrationDriver(const QString &driverId) {
    int id = m_core->deleteIntegrationDriver(driverId);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot delete integration driver" << code << message;
            ui::Notification::createNotification(tr("Error while deleting integration driver"), true);
        });
}

void IntegrationController::startDriverDiscovery() {
    qCDebug(lcIntegrationController()) << "START DISCOVERY";
    m_discoveredIntegrationDrivers.clear();

    // first we get all the integration drivers again
    QObject *scope = new QObject(this);
    QObject::connect(this, &IntegrationController::integrationDriversLoaded, scope, [=]() {
        scope->deleteLater();
        int id = m_core->integrationStartDiscovery();

        m_core->onResult(
            id,
            [=]() {
                // success
                qCDebug(lcIntegrationController()) << "Integration discovery successfully started";
            },
            [=](int code, QString message) {
                // fail
                qCWarning(lcIntegrationController()) << "Integration discovery failed to start" << code << message;
                ui::Notification::createNotification(tr("Integration discovery failed to start"), true);
            });
    });

    getAllIntegrationDrivers();
}

void IntegrationController::stopDriverDiscovery() {
    qCDebug(lcIntegrationController()) << "STOP DISCOVERY";

    int id = m_core->integrationStopDiscovery();

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcIntegrationController()) << "Integration discovery successfully stopped";
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Integration discovery failed to stop" << code << message;
            ui::Notification::createNotification(tr("Integration discovery failed to stop"), true);
        });
}

void IntegrationController::getDiscoveredDriverMetadata(const QString &driverId, const QString &driverUrl,
                                                        const QString &token) {
    int id = m_core->integrationGetDiscoveredDriverMetadata(driverId, driverUrl, token);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrationDriver,
        [=](core::IntegrationDriver integrationDriver) {
            // success
            qCDebug(lcIntegrationController()) << "Integration metadata" << integrationDriver.id;
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Error getting integration driver metadata" << code << message;
            ui::Notification::createNotification(tr("Error getting integration driver metadata"), true);
        });
}

void IntegrationController::getIntegrationDriver(const QString &driverId) {
    int id = m_core->getIntegrationDriver(driverId);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrationDriver,
        [=](core::IntegrationDriver integrationDriver) {
            // success

            qCDebug(lcIntegrationController()) << "Integration driver" << integrationDriver.id;
            if (!m_integrationDrivers.contains(integrationDriver.id)) {
                m_integrationDrivers.append(new IntegrationDriver(
                    integrationDriver.id, integrationDriver.name, integrationDriver.driverUrl,
                    integrationDriver.version, integrationDriver.icon, integrationDriver.enabled, "",
                    integrationDriver.description, integrationDriver.developer.name, integrationDriver.homePage,
                    integrationDriver.releaseDate,
                    new SetupSchema(integrationDriver.settingsPage.title, integrationDriver.settingsPage.settings,
                                    m_language),
                    false, integrationDriver.instanceCount, m_language, false, integrationDriver.external, this));
            }

            emit integrationDriverLoaded(driverId);
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Error getting integration driver" << code << message;
            ui::Notification::createNotification(tr("Error getting integration driver"), true);
        });
}

void IntegrationController::integrationDriverStart(const QString &integrationDriverId) {
    int id = m_core->integrationDriverStart(integrationDriverId);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcIntegrationController()) << "Integration driver successfully started" << integrationDriverId;
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot start integration driver" << code << message;
            ui::Notification::createNotification(tr("Error while starting integration driver"), true);
        });
}

void IntegrationController::integrationDriverStop(const QString &integrationDriverId) {
    int id = m_core->integrationDriverStop(integrationDriverId);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcIntegrationController()) << "Integration driver successfully started" << integrationDriverId;
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot start integration driver" << code << message;
            ui::Notification::createNotification(tr("Error while starting integration driver"), true);
        });
}

void IntegrationController::integrationConnect(const QString &integrationId) {
    int id = m_core->integrationConnect(integrationId);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcIntegrationController()) << "Integration successfully connected" << integrationId;
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot connect to the integration" << code << message;
            ui::Notification::createNotification(tr("Error while connecting to the integration"), true);
        });
}

void IntegrationController::integrationDisconnect(const QString &integrationId) {
    int id = m_core->integrationDisconnect(integrationId);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcIntegrationController()) << "Integration successfully disconnected" << integrationId;
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot disconnect to the integration" << code << message;
            ui::Notification::createNotification(tr("Error while disconnecting to the integration"), true);
        });
}

void IntegrationController::selectIntegrationToSetup(const QString &integrationDriverId) {
    auto obj = m_discoveredIntegrationDrivers.get(integrationDriverId);

    if (obj) {
        m_integrationDriverToSetup = obj;
        emit integrationDriverToSetupChanged();

        SetupSchema *schema = qobject_cast<SetupSchema *>(obj->getSetupSchema());

        if (!schema->getTitle().isEmpty()) {
            m_configPages.append(obj->getSetupSchema());
            emit configPagesChanged();
        }

        qCDebug(lcIntegrationController()) << "Integration selected for setup:" << obj->getId();
    }
}

void IntegrationController::setupIntegration(const QString &integrationDriverId, QVariantMap setupData) {
    auto obj = m_discoveredIntegrationDrivers.get(integrationDriverId);

    qCDebug(lcIntegrationController()) << "SETUP INTEGRATION WITH SETUP DATA:" << setupData;

    if (!obj) {
        ui::Notification::createNotification(tr("Integration setup error. Aborting setup"), true);
        stopIntegrationSetup(integrationDriverId);
        return;
    }

    int id = m_core->integrationSetup(integrationDriverId, obj->getNameI18n(), setupData);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrationSetupInfo,
        [=](core::IntegrationSetupInfo integrationSetupInfo) {
            // success
            qCDebug(lcIntegrationController())
                << "Integration setup info" << integrationSetupInfo.id << integrationSetupInfo.state;

            if (integrationSetupInfo.state == core::IntegrationEnums::SetupState::ERROR) {
                ui::Notification::createNotification(tr("Integration setup error. Aborting setup"), true);
                stopIntegrationSetup(integrationDriverId);
            }
        },
        [=](int code, QString message) {
            // fail
            switch (code) {
                // invalid data
                case 400: {
                    QRegularExpression rx(R"**((?<!\\)([\"'])(.+?)(?<!\\)\1)**");
                    QStringList        list;

                    QRegularExpressionMatchIterator i = rx.globalMatch(&message);
                    while (i.hasNext()) {
                        QRegularExpressionMatch expMatch = i.next();
                        if (expMatch.hasMatch()) {
                            list.append(expMatch.captured(0).replace("'", ""));
                        }
                    }

                    if (list.length() > 0) {
                        qCDebug(lcIntegrationController()) << "Error in:" << list[0];
                        emit integrationUserDataError(list[0], message);
                    } else {
                        ui::Notification::createNotification(tr("Invalid data"), true);
                    }
                    break;
                }
                case 404:
                    ui::Notification::createNotification(tr("The integration driver id does not exist."), true);
                    break;
                case 409: {
                    QVariantMap param;
                    param.insert("id", integrationDriverId);
                    param.insert("obj", QVariant::fromValue(s_instance));

                    ui::Notification::createActionableWarningNotification(
                        tr("Failed to start setup"),
                        tr("There is already a running setup for this integration. Would you like to stop "
                           "that?"),
                        "uc:triangle-exclamation",
                        [](QVariant param) {
                            IntegrationController *ic =
                                qvariant_cast<IntegrationController *>(param.toMap().value("obj"));
                            ic->stopIntegrationSetup(param.toMap().value("id").toString());
                        },
                        param, tr("Stop"));
                    break;
                }
                case 422:
                    ui::Notification::createNotification(
                        tr("The integration is already configured or doesn't allow to be set up again."), true);
                    break;
                default:
                    ui::Notification::createNotification(tr("Cannot start integration setup"), true);
                    break;
            }

            emit integrationSetupStopped();
            qCWarning(lcIntegrationController()) << code << message;
        });
}

void IntegrationController::configureDiscoveredIntegrationDriver(const QString &integrationDriverId,
                                                                 QVariantMap    setupData) {
    auto obj = m_discoveredIntegrationDrivers.get(integrationDriverId);

    if (!obj) {
        ui::Notification::createNotification(tr("Integration setup error. Aborting setup"), true);
        stopIntegrationSetup(integrationDriverId);
        return;
    }

    if (m_integrationDrivers.contains(integrationDriverId)) {
        qCDebug(lcIntegrationController()) << "Integration is already configured" << integrationDriverId;
        return;
    }

    int id = m_core->integrationConfigureDiscoveredDriver(integrationDriverId, obj->getNameI18n(),
                                                          setupData.value("driver_url").toString(),
                                                          setupData.value("token").toString());

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrationDriver,
        [=](core::IntegrationDriver integrationDriver) {
            // success

            qCDebug(lcIntegrationController()) << "Integration setup info" << integrationDriver.id;

            obj->setSetupScehma(new SetupSchema(integrationDriver.settingsPage.title,
                                                integrationDriver.settingsPage.settings, m_language));

            m_configPages.append(obj->getSetupSchema());
            emit configPagesChanged();
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController())
                << "Error while configuring discovered integration driver" << code << message;
            emit configureDiscoveredIntegrationDriverError(message);
        });
}

void IntegrationController::stopIntegrationSetup(const QString &integrationDriverId) {
    int id = m_core->integrationStopSetup(integrationDriverId);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcIntegrationController()) << "Integration setup stopped successfully" << integrationDriverId;
            emit integrationSetupStopped();
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcIntegrationController()) << "Cannot stop the integration setup" << code << message;
            ui::Notification::createNotification(tr("Cannot stop the integration setup"), true);
        });
}

void IntegrationController::integrationSetUserDataSettings(const QString &integrationDriverId, QVariantMap setupData) {
    int id = m_core->integrationSetUserDataSettings(integrationDriverId, setupData);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrationSetupInfo,
        [=](core::IntegrationSetupInfo integrationSetupInfo) {
            // success
            qCDebug(lcIntegrationController())
                << "Integration setup info" << integrationSetupInfo.id << integrationSetupInfo.state;

            if (integrationSetupInfo.state == core::IntegrationEnums::SetupState::ERROR) {
                ui::Notification::createNotification(tr("Integration setup error. Aborting setup"), true);
                stopIntegrationSetup(integrationDriverId);
            }
        },
        [=](int code, QString message) {
            // fail
            switch (code) {
                // invalid data
                case 400: {
                    QRegularExpression rx(R"**((?<!\\)([\"'])(.+?)(?<!\\)\1)**");
                    QStringList        list;

                    QRegularExpressionMatchIterator i = rx.globalMatch(&message);
                    while (i.hasNext()) {
                        QRegularExpressionMatch expMatch = i.next();
                        if (expMatch.hasMatch()) {
                            list.append(expMatch.captured(0).replace("'", ""));
                        }
                    }

                    if (list.length() > 0) {
                        qCDebug(lcIntegrationController()) << "Error in:" << list[0];
                        emit integrationUserDataError(list[0], message);
                    } else {
                        ui::Notification::createNotification(tr("Invalid data"), true);
                    }
                    break;
                }
                case 404:
                    ui::Notification::createNotification(tr("The integration driver id does not exist."), true);
                    break;
                case 409: {
                    QVariantMap param;
                    param.insert("id", integrationDriverId);
                    param.insert("obj", QVariant::fromValue(s_instance));

                    ui::Notification::createActionableWarningNotification(
                        tr("Failed to start setup"),
                        tr("There is already a running setup for this integration. Would you like to stop "
                           "that?"),
                        "uc:triangle-exclamation",
                        [](QVariant param) {
                            IntegrationController *ic =
                                qvariant_cast<IntegrationController *>(param.toMap().value("obj"));
                            ic->stopIntegrationSetup(param.toMap().value("id").toString());
                        },
                        param, tr("Stop"));
                    break;
                }
                case 422:
                    ui::Notification::createNotification(
                        tr("The integration is already configured or doesn't allow to be set up again."), true);
                    break;
                default:
                    ui::Notification::createNotification(tr("Cannot start integration setup"), true);
                    break;
            }

            emit integrationSetupStopped();
            qCWarning(lcIntegrationController()) << code << message;
        });
}

void IntegrationController::integrationSetUserDataConfirm(const QString &integrationDriverId) {
    int id = m_core->integrationSetUserDataConfirm(integrationDriverId);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respIntegrationSetupInfo,
        [=](core::IntegrationSetupInfo integrationSetupInfo) {
            // success
            qCDebug(lcIntegrationController())
                << "Integration setup info" << integrationSetupInfo.id << integrationSetupInfo.state;

            if (integrationSetupInfo.state == core::IntegrationEnums::SetupState::ERROR) {
                ui::Notification::createNotification(tr("Integration setup error. Aborting setup"), true);
                stopIntegrationSetup(integrationDriverId);
            }
        },
        [=](int code, QString message) {
            // fail
            switch (code) {
                // invalid data
                case 400:
                    ui::Notification::createNotification(tr("Invalid data"), true);
                    break;
                case 404:
                    ui::Notification::createNotification(tr("The integration driver id does not exist."), true);
                    break;
                case 409: {
                    QVariantMap param;
                    param.insert("id", integrationDriverId);
                    param.insert("obj", QVariant::fromValue(s_instance));

                    ui::Notification::createActionableWarningNotification(
                        tr("Failed to start setup"),
                        tr("There is already a running setup for this integration. Would you like to stop "
                           "that?"),
                        "uc:triangle-exclamation",
                        [](QVariant param) {
                            IntegrationController *ic =
                                qvariant_cast<IntegrationController *>(param.toMap().value("obj"));
                            ic->stopIntegrationSetup(param.toMap().value("id").toString());
                        },
                        param, tr("Stop"));
                    break;
                }
                case 422:
                    ui::Notification::createNotification(
                        tr("The integration is already configured or doesn't allow to be set up again."), true);
                    break;
                default:
                    ui::Notification::createNotification(tr("Cannot start integration setup"), true);
                    break;
            }

            emit integrationSetupStopped();
            qCWarning(lcIntegrationController()) << code << message;
        });
}

void IntegrationController::clearConfigPages() {
    m_configPages.clear();
    emit configPagesChanged();
}

QObject *IntegrationController::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void IntegrationController::onIntegrationDeviceStateChanged(QString integrationId, QString driverId, QString state) {
    qCDebug(lcIntegrationController()) << "Integration device state changed" << integrationId << driverId << state;
    if (m_integrations.contains(integrationId)) {
        m_integrations.setState(integrationId, state.toLower());
    }
}

void IntegrationController::onLanguageChanged(QString language) {
    m_language = language;

    m_integrations.setLanguage(m_language);
    m_integrationDrivers.setLanguage(m_language);
    m_discoveredIntegrationDrivers.setLanguage(m_language);

    if (m_integrationDriverToSetup) {
        m_integrationDriverToSetup->updateLanguage(m_language);
    }
}

void IntegrationController::onIntegrationSetupChange(core::IntegrationSetupInfo integrationSetupInfo) {
    qCDebug(lcIntegrationController()) << "INTEGRATION SETUP CHANGE" << integrationSetupInfo.id
                                       << integrationSetupInfo.state << integrationSetupInfo.requireUserAction;

    switch (integrationSetupInfo.state) {
        case core::IntegrationEnums::SetupState::SETUP:
            if (!integrationSetupInfo.settingsPage.settings.isEmpty()) {
                m_configPages.append(new SetupSchema(integrationSetupInfo.settingsPage.title,
                                                     integrationSetupInfo.settingsPage.settings, m_language, this));
                emit configPagesChanged();
            }

            emit integrationSetupChange(integrationSetupInfo.id, SetupState::Setup, QString(),
                                        integrationSetupInfo.requireUserAction);
            break;
        case core::IntegrationEnums::SetupState::WAIT_USER_ACTION:

            if (!integrationSetupInfo.settingsPage.settings.isEmpty()) {
                m_configPages.append(new SetupSchema(integrationSetupInfo.settingsPage.title,
                                                     integrationSetupInfo.settingsPage.settings, m_language, this));
                emit configPagesChanged();

                emit integrationSetupChange(integrationSetupInfo.id, SetupState::Setup, QString(),
                                            integrationSetupInfo.requireUserAction);
            } else if (!integrationSetupInfo.confirmationPage.title.isEmpty()) {
                m_configPages.append(new ConfirmationPage(
                    integrationSetupInfo.confirmationPage.title, integrationSetupInfo.confirmationPage.message1,
                    integrationSetupInfo.confirmationPage.image, integrationSetupInfo.confirmationPage.message2,
                    m_language, this));
                emit configPagesChanged();

                emit integrationSetupChange(integrationSetupInfo.id, SetupState::Wait_user_action, QString(),
                                            integrationSetupInfo.requireUserAction);
            }
            break;
        case core::IntegrationEnums::SetupState::OK:
            clearConfigPages();
            emit integrationSetupChange(integrationSetupInfo.id, SetupState::Ok, QString(),
                                        integrationSetupInfo.requireUserAction);
            break;
        case core::IntegrationEnums::SetupState::ERROR:
            clearConfigPages();
            QString errorString;

            switch (integrationSetupInfo.error) {
                case core::IntegrationEnums::SetupError::AUTHORIZATION_ERROR:
                    errorString = tr("Authorization error");
                    break;
                case core::IntegrationEnums::SetupError::CONNECTION_REFUSED:
                    errorString = tr("Connection refused");
                    break;
                case core::IntegrationEnums::SetupError::NONE:
                    errorString = tr("Unknown error");
                    break;
                case core::IntegrationEnums::SetupError::NOT_FOUND:
                    errorString = tr("Not found");
                    break;
                case core::IntegrationEnums::SetupError::OTHER:
                    errorString = tr("Unknown error");
                    break;
                case core::IntegrationEnums::SetupError::TIMEOUT:
                    errorString = tr("Timeout");
                    break;
            }

            emit integrationSetupChange(integrationSetupInfo.id, SetupState::Error, errorString,
                                        integrationSetupInfo.requireUserAction);
            break;
    }
}

bool IntegrationController::checkConnections() {
    bool connecting = false;

    for (int i = 0; i < m_integrationDrivers.count(); i++) {
        qCDebug(lcIntegrationController())
            << m_integrationDrivers.get(i)->getId() << m_integrationDrivers.get(i)->getState();

        if (m_integrationDrivers.get(i)->getState().contains("connecting")) {
            connecting = true;
        }
    }

    return connecting;
}

void IntegrationController::onIntegrationStatusLoaded() {
    if (m_integrationStatusTotalPages != m_integrationStatusLastPageLoaded) {
        qCDebug(lcIntegrationController())
            << "More integration status to load" << m_integrationStatusLastPageLoaded + 1;
        getIntegrationStatus(100, m_integrationStatusLastPageLoaded + 1);
    } else {
        qCDebug(lcIntegrationController()) << "Integration status all loaded";
    }
}

void IntegrationController::onIntegrationDriverLoaded(QString driverId) {
    Q_UNUSED(driverId)

    m_integrationDriversLoaded++;
    if (m_integrationDriversLoaded == m_integrationDrivers.totalItems) {
        emit integrationDriversLoaded();
    }
}

void IntegrationController::onIntegrationDriversLoaded() {
    if (m_integrationDrivers.totalPages != m_integrationDrivers.lastPageLoaded) {
        qCDebug(lcIntegrationController())
            << "More integration drivers to load" << m_integrationDrivers.lastPageLoaded + 1;
        getIntegrationDrivers(100, m_integrationDrivers.lastPageLoaded + 1);
    } else {
        qCDebug(lcIntegrationController()) << "Integration drivers all loaded";
        getAllIntegrationStatus();
    }
}

void IntegrationController::onIntegrationsLoaded() {
    qCDebug(lcIntegrationController()) << "totalpages:" << m_integrations.totalPages
                                       << "lastpageloaded:" << m_integrations.lastPageLoaded;

    if (m_integrations.totalPages != m_integrations.lastPageLoaded) {
        qCDebug(lcIntegrationController()) << "More integrations to load" << m_integrations.lastPageLoaded + 1;
        getIntegrations(100, m_integrations.lastPageLoaded + 1);
    } else {
        qCDebug(lcIntegrationController()) << "Integrations all loaded";
    }
}

void IntegrationController::onDriverDiscoveryStarted() {
    qCDebug(lcIntegrationController()) << "Integration driver discovery started";

    if (m_integrationDrivers.count() > 0) {
        for (int i = 0; i < m_integrationDrivers.count(); i++) {
            auto driver = m_integrationDrivers.get(i);

            qCDebug(lcIntegrationController())
                << "Configured driver instance count:" << driver->getId() << driver->getInstanceCount();

            if (driver->getInstanceCount() == 0) {
                m_discoveredIntegrationDrivers.append(new IntegrationDriver(
                    driver->getId(), driver->getNameI18n(), driver->getDriverUrl(), driver->getVersion(),
                    driver->getIcon(), false, QString(), driver->getDescription(), driver->getDeveloperName(),
                    driver->getHomePage(), driver->getReleaseDate(),
                    qobject_cast<SetupSchema *>(driver->getSetupSchema()), false, driver->getInstanceCount(),
                    m_language, false, driver->getExternal(), this));
            }
        }
    }

    emit driverDiscoveryStarted();
}

void IntegrationController::onDriverDiscoveryStopped() {
    qCDebug(lcIntegrationController()) << "Integration driver discovery stopped";
    emit driverDiscoveryStopped();
}

void IntegrationController::onDriverDiscovered(core::IntegrationDriver integrationDriver) {
    qCDebug(lcIntegrationController()) << "Integration discovered" << integrationDriver.id << integrationDriver.name
                                       << integrationDriver.developer.name;

    m_discoveredIntegrationDrivers.append(new IntegrationDriver(
        integrationDriver.id, integrationDriver.name, integrationDriver.driverUrl, integrationDriver.version,
        integrationDriver.icon, false, QString(), integrationDriver.description, integrationDriver.developer.name,
        integrationDriver.homePage, integrationDriver.releaseDate,
        new SetupSchema(integrationDriver.settingsPage.title, integrationDriver.settingsPage.settings, m_language),
        true, 0, m_language, false, integrationDriver.external, this));
}

void IntegrationController::onDriverAdded(QString driverId, core::IntegrationDriver integrationDriver) {
    if (!m_integrationDrivers.contains(integrationDriver.id)) {
        m_integrationDrivers.append(new IntegrationDriver(
            integrationDriver.id, integrationDriver.name, integrationDriver.driverUrl, integrationDriver.version,
            integrationDriver.icon, integrationDriver.enabled, "", integrationDriver.description,
            integrationDriver.developer.name, integrationDriver.homePage, integrationDriver.releaseDate,
            new SetupSchema(integrationDriver.settingsPage.title, integrationDriver.settingsPage.settings, m_language),
            false, integrationDriver.instanceCount, m_language, false, integrationDriver.external, this));

        qCDebug(lcIntegrationController()) << "Added integration driver:" << driverId;
    }
}

void IntegrationController::onDriverChanged(QString driverId, core::IntegrationDriver integrationDriver) {
    if (!m_integrationDrivers.contains(integrationDriver.id)) {
        auto obj = m_integrationDrivers.get(driverId);

        if (obj) {
            obj->setNameI18n(integrationDriver.name);
            obj->setDriverUrl(integrationDriver.driverUrl);
            obj->setVersion(integrationDriver.version);
            obj->setIcon(integrationDriver.icon);
            obj->setEnabled(integrationDriver.enabled);
            obj->setState(Util::convertEnumToString(integrationDriver.state));
            obj->setDescription(integrationDriver.description);
            obj->setDeveloperName(integrationDriver.developer.name);
            obj->setHomePage(integrationDriver.homePage);
            obj->setReleaseDate(integrationDriver.releaseDate);
            obj->setDiscovered(integrationDriver.deviceDiscovery);
            obj->setSetupScehma(new SetupSchema(integrationDriver.settingsPage.title,
                                                integrationDriver.settingsPage.settings, m_language));
            obj->setInstanceCount(integrationDriver.instanceCount);
            obj->updateLanguage(m_language);
        }
        qCDebug(lcIntegrationController()) << "Changed integration driver:" << driverId;
    }
}

void IntegrationController::onDriverDeleted(QString driverId) {
    if (m_integrationDrivers.contains(driverId)) {
        m_integrationDrivers.get(driverId)->deleteLater();
        m_integrationDrivers.removeItem(driverId);

        qCDebug(lcIntegrationController()) << "Deleted integration driver:" << driverId;
    }

    QString integrationId = m_integrations.getIntegrationIdFromDriverId(driverId);
    if (!integrationId.isEmpty()) {
        onIntegrationDeleted(integrationId);
    }
}

void IntegrationController::onIntegrationAdded(QString integrationId, core::Integration integration) {
    if (!m_integrations.contains(integrationId)) {
        m_integrations.append(new Integration(integration.id, integration.driverId, integration.deviceId,
                                              integration.name, integration.icon, integration.enabled,
                                              integration.setupData, m_language, false, this));

        qCDebug(lcIntegrationController()) << "Added integration:" << integrationId;
        emit integrationAdded(integrationId);
    }
}

void IntegrationController::onIntegrationChanged(QString integrationId, core::Integration integration) {
    if (m_integrations.contains(integrationId)) {
        auto obj = m_integrations.get(integrationId);

        if (obj) {
            obj->setNameI18n(integration.name);
            obj->setIcon(integration.icon);
            obj->setEnabled(integration.enabled);
            obj->setState(Util::convertEnumToString(integration.deviceState));
            obj->setSetupData(integration.setupData);
            obj->updateLanguage(m_language);
        }

        qCDebug(lcIntegrationController()) << "Changed integration:" << integrationId;
    }
}

void IntegrationController::onIntegrationDeleted(QString integrationId) {
    if (m_integrations.contains(integrationId)) {
        m_integrations.get(integrationId)->deleteLater();
        m_integrations.removeItem(integrationId);

        emit integrationDeleted(integrationId);

        qCDebug(lcIntegrationController()) << "Deleted integration:" << integrationId;
    }
}

void IntegrationController::onIntegrationDriverStateChanged(QString driverId, QString state) {
    if (!m_integrationDrivers.get(driverId)) {
        return;
    }

    if (!m_integrationDrivers.get(driverId)->getState().contains(state.toLower())) {
        qCDebug(lcIntegrationController()) << "Integration driver state changed" << driverId << state;

        m_integrationDrivers.setState(driverId, state.toLower());

        if (state.contains("error", Qt::CaseInsensitive)) {
            emit integrationError(m_integrationDrivers.get(driverId)->getName(),
                                  m_integrationDrivers.get(driverId)->getId());
        }

        if (!state.contains("active", Qt::CaseInsensitive)) {
            if (!m_integrationDriversError.contains(driverId)) {
                m_integrationDriversError.append(driverId);
            }
        } else {
            m_integrationDriversError.removeOne(driverId);
        }
        emit driversErrorChanged();
    }

    emit integrationIsConnecting(checkConnections());
}

}  // namespace integration
}  // namespace uc
