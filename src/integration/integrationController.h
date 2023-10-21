// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QtMath>

#include "../core/core.h"
#include "../ui/notification.h"
#include "../util.h"
#include "confirmationPage.h"
#include "integrationDrivers.h"
#include "integrations.h"

namespace uc {
namespace integration {

class IntegrationController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractListModel* integrationsModel READ getIntegrationsModel CONSTANT)
    Q_PROPERTY(QSortFilterProxyModel* driversModel READ getDriversModel CONSTANT)
    Q_PROPERTY(QStringList driversError READ getDriversError NOTIFY driversErrorChanged)
    Q_PROPERTY(QAbstractListModel* discoveredIntegrationDrivers READ getDiscoveredIntegrationDrivers CONSTANT)
    Q_PROPERTY(
        QObject* integrationDriverTosetup READ getIntegrationDriverTosetup NOTIFY integrationDriverToSetupChanged)
    Q_PROPERTY(QList<QObject*> configPages READ getConfigPages NOTIFY configPagesChanged)

 public:
    explicit IntegrationController(core::Api* core, const QString& language, QObject* parent = nullptr);
    ~IntegrationController();

    enum IntegrationSetupType {
        Configured,
        Notconfigured,
    };
    Q_ENUM(IntegrationSetupType)

    enum SetupState {
        Setup,
        Wait_user_action,
        Ok,
        Error,
    };
    Q_ENUM(SetupState)

 public:
    // Q_PROPERTY methods
    QAbstractListModel*    getIntegrationsModel() { return &m_integrations; }
    QSortFilterProxyModel* getDriversModel() { return &m_integrationDriversFilter; }
    QStringList            getDriversError() { return m_integrationDriversError; }
    QAbstractListModel*    getDiscoveredIntegrationDrivers() { return &m_discoveredIntegrationDrivers; }
    QObject*               getIntegrationDriverTosetup() { return m_integrationDriverToSetup; }
    QList<QObject*>        getConfigPages() { return m_configPages; }

 public:
    // QML accessible methods

    Q_INVOKABLE void     getAllIntegrationStatus();
    Q_INVOKABLE void     getAllIntegrationDrivers();
    Q_INVOKABLE void     getAllIntegrations();
    Q_INVOKABLE QObject* getModelItem(const QString& id);
    Q_INVOKABLE QObject* getDriversModelItem(const QString& id);

    Q_INVOKABLE void deleteIntegration(const QString& integrationId);
    Q_INVOKABLE void deleteIntegrationDriver(const QString& driverId);

    Q_INVOKABLE void startDriverDiscovery();
    Q_INVOKABLE void stopDriverDiscovery();
    Q_INVOKABLE void getDiscoveredDriverMetadata(const QString& driverId, const QString& driverUrl = QString(),
                                                 const QString& token = QString());
    Q_INVOKABLE void integrationDriverStart(const QString& integrationDriverId = QString());
    Q_INVOKABLE void integrationDriverStop(const QString& integrationDriverId = QString());
    Q_INVOKABLE void integrationConnect(const QString& integrationId = QString());
    Q_INVOKABLE void integrationDisconnect(const QString& integrationId = QString());

    Q_INVOKABLE void selectIntegrationToSetup(const QString& integrationDriverId);
    Q_INVOKABLE void setupIntegration(const QString& integrationDriverId, QVariantMap setupData);
    Q_INVOKABLE void configureDiscoveredIntegrationDriver(const QString& integrationDriverId, QVariantMap setupData);
    Q_INVOKABLE void stopIntegrationSetup(const QString& integrationDriverId);
    Q_INVOKABLE void integrationSetUserDataSettings(const QString& integrationDriverId, QVariantMap setupData);
    Q_INVOKABLE void integrationSetUserDataConfirm(const QString& integrationDriverId);

    Q_INVOKABLE void clearConfigPages();

    void getIntegrationStatus(int limit = 100, int page = 1);
    void getIntegrationDrivers(int limit = 100, int page = 1);
    void getIntegrations(int limit = 100, int page = 1);

    void getIntegrationDriver(const QString& driverId);

 public:
    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void driversErrorChanged();
    void integrationDriverToSetupChanged();
    void configPagesChanged();

    void integrationIsConnecting(bool value);
    void integrationError(QString name, QString id);

    void configureDiscoveredIntegrationDriverError(QString message);

    void integrationStatusLoaded();
    void integrationsLoaded();
    void integrationDriverLoaded(QString driverId);
    void integrationDriversLoaded();
    void integrationSetupStopped();
    void integrationSetupChange(QString driverId, SetupState state, QString error, bool requireUserAction);
    void integrationUserDataError(QString labelId, QString error);

    void integrationAdded(QString integrationId);
    void integrationDeleted(QString integrationId);

    void driverDiscoveryStarted();
    void driverDiscoveryStopped();

 public slots:
    void onIntegrationDriverStateChanged(QString driverId, QString state);
    void onIntegrationDeviceStateChanged(QString integrationId, QString driverId, QString state);
    void onLanguageChanged(QString language);
    void onIntegrationSetupChange(core::IntegrationSetupInfo integrationSetupInfo);

 private:
    static IntegrationController* s_instance;
    core::Api*                    m_core;

    QString m_language;

    Integrations m_integrations;

    IntegrationDrivers    m_integrationDrivers;
    QSortFilterProxyModel m_integrationDriversFilter;
    QStringList           m_integrationDriversError;
    int                   m_integrationDriversLoaded = 0;

    IntegrationDrivers m_discoveredIntegrationDrivers;
    IntegrationDriver* m_integrationDriverToSetup;
    QString            m_integrationDriverSetupId;
    QList<QObject*>    m_configPages;

    int m_integrationStatusLimit = 0;
    int m_integrationStatusTotalItems = 0;
    int m_integrationStatusTotalPages = 0;
    int m_integrationStatusLastPageLoaded = 0;

 private:
    bool checkConnections();

 private slots:
    void onIntegrationStatusLoaded();
    void onIntegrationDriverLoaded(QString driverId);
    void onIntegrationDriversLoaded();
    void onIntegrationsLoaded();

    void onDriverDiscoveryStarted();
    void onDriverDiscoveryStopped();
    void onDriverDiscovered(core::IntegrationDriver integrationDriver);

    void onDriverAdded(QString driverId, core::IntegrationDriver integrationDriver);
    void onDriverChanged(QString driverId, core::IntegrationDriver integrationDriver);
    void onDriverDeleted(QString driverId);

    void onIntegrationAdded(QString integrationId, core::Integration integration);
    void onIntegrationChanged(QString integrationId, core::Integration integration);
    void onIntegrationDeleted(QString integrationId);
};

}  // namespace integration
}  // namespace uc
