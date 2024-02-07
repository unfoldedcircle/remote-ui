// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>

#include "../core/core.h"
#include "../ui/notification.h"
#include "configuredDocks.h"
#include "discoveredDocks.h"

namespace uc {
namespace dock {

class DockController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractListModel* discoveredDocks READ getDiscoveredDocks CONSTANT)
    Q_PROPERTY(QString dockToSetup READ getDockToSetup NOTIFY dockToSetupChanged)
    Q_PROPERTY(QAbstractListModel* configuredDocks READ getConfiguredDocks CONSTANT)

 public:
    explicit DockController(core::Api* core, QObject* parent = nullptr);
    ~DockController();

    QAbstractListModel* getDiscoveredDocks() { return &m_discoveredDocks; }
    QString             getDockToSetup() { return m_dockToSetup; }
    QAbstractListModel* getConfiguredDocks() { return &m_configuredDocks; }

    Q_INVOKABLE QObject* getDiscoveredDock(const QString& dockId);
    Q_INVOKABLE QObject* getConfiguredDock(const QString& dockId);
    Q_INVOKABLE void     getConfiguredDockFromCore(const QString &dockId);

    Q_INVOKABLE void startDiscovery();
    Q_INVOKABLE void stopDiscovery();
    Q_INVOKABLE void selectDockToSetup(const QString& dockId);
    Q_INVOKABLE void setupDock(const QString& dockId, const QString& friendlyName, const QString& password,
                               const QString& discoveryType, const QString& wifiSsid = QString(),
                               const QString& wifiPassword = QString());
    Q_INVOKABLE void stopSetup(const QString& dockId);
    Q_INVOKABLE void identify(const QString& dockId);
    Q_INVOKABLE void connect(const QString& dockId);
    Q_INVOKABLE void deleteDock(const QString& dockId);
    Q_INVOKABLE void factoryReset(const QString& dockId);
    Q_INVOKABLE void updateDockName(const QString& dockId, const QString& name);
    Q_INVOKABLE void updateDockPassword(const QString& dockId, const QString& password);
    Q_INVOKABLE void setDockLedBrightness(const QString& dockId, int brightness);

    void getDocks(int limit = 100, int page = 1);

 public:
    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void discoveryStarted();
    void discoveryStopped();
    void dockToSetupChanged(QString dockId);
    void setupStarted();
    void setupFinished(bool success, QString message = QString());
    void setupChanged(QString message);

    void docksLoaded();
    void dockAdded(QString dockId);
    void dockChanged(QString dockId);
    void dockDeleted(QString dockId);

    void dockNameChanged(QString dockId);
    void dockPasswordChanged(QString dockId);
    void error(QString messsage);

    void gotDock(bool success, QString id);

 private:
    static DockController* s_instance;
    core::Api*             m_core;

    DiscoveredDocks m_discoveredDocks;
    QString         m_dockToSetup;

    ConfiguredDocks m_configuredDocks;

    void startDockSetup(const QString& dockId, const QString& friendlyName, const QString& password,
                        const QString& wifiSsid, const QString& wifiPassword);

 private slots:
    void onCoreConnected();

    void onDockDiscoveryStarted();
    void onDockDiscoveryStopped();
    void onDockDiscovered(core::DockDiscovery dock);
    void onDockSetupChanged(core::MsgEventTypes::Enum type, QString dockId, core::DockSetupEnums::DockSetupState state,
                            core::DockSetupEnums::DockSetupError error);

    void onDockAdded(QString dockId, core::DockConfiguration dock);
    void onDockChanged(QString dockId, core::DockConfiguration dock);
    void onDockDeleted(QString dockId);
    void onDockStateChanged(QString dockId, core::DockEnums::DockState state);
    void onDockUpdateChanged(core::MsgEventTypes::Enum type, QString dockId, QString updateId, QString version,
                             int progress, core::DockSetupEnums::DockSetupState state,
                             core::DockSetupEnums::DockSetupError error);
};
}  // namespace dock
}  // namespace uc
