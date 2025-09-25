// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVariant>

namespace uc {
namespace dock {

class DiscoveredDock : public QObject {
    Q_OBJECT

 public:
    explicit DiscoveredDock(const QString& id, bool configured, const QString& friendlyName, const QString& address,
                            const QString& model, const QString& revision, const QString& serial, const QString& version, const QString& discoveryType,
                            int bluetoothSignal, int bluetoothLastSeenSeconds, QObject* parent = nullptr);
    ~DiscoveredDock();

    Q_INVOKABLE QString itemId() const { return m_id; }
    bool                itemConfigured() const { return m_configured; }
    Q_INVOKABLE QString itemFriendlyName() const { return m_friendlyName; }
    Q_INVOKABLE QString itemAddress() const { return m_address; }
    QString             itemModel() const { return m_model; }
    QString             itemRevision() const { return m_revision; }
    QString             itemSerial() const { return m_serial; }
    QString             itemVersion() const { return m_version; }
    Q_INVOKABLE QString itemDiscoveryType() const { return m_discoveryType; }
    int                 itemBluetoothSignal() const { return m_bluetoothSignal; }
    int                 itemBluetoothLastSeenSeconds() const { return m_bluetoothLastSeenSeconds; }

 private:
    QString m_id;
    bool    m_configured;
    QString m_friendlyName;
    QString m_address;
    QString m_model;
    QString m_revision;
    QString m_serial;
    QString m_version;
    QString m_discoveryType;
    int     m_bluetoothSignal;
    int     m_bluetoothLastSeenSeconds;
};

class DiscoveredDocks : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

 public:
    enum SearchRoles {
        KeyRole = Qt::UserRole + 1,
        ConfiguredRole,
        FirendlyNameRole,
        AddressRole,
        ModelRole,
        RevisionRole,
        SerialRole,
        VersionRole,
        DiscoveryTypeRole,
        BluetoothSignalRole,
        BluetoothLastSeenSecondsRole,
    };

    explicit DiscoveredDocks(QObject* parent = nullptr);
    ~DiscoveredDocks() = default;

 public:
    // Q_PROPERTY methods
    int count() const;

 public slots:
    // Q_PROPERTY methods
    void setCount(int count);

 public:
    // QQAbstractListModel overrides and required emthods
    int                    rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool                   removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QVariant               data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

 public:
    // Custom methods
    void append(DiscoveredDock* o);
    void clear();

    QModelIndex getModelIndexByKey(const QString& key);

    bool contains(const QString& key);

    DiscoveredDock* get(const QString& key);
    DiscoveredDock* get(int row);

    void removeItem(const QString& key);
    void removeItem(int row);

 signals:
    void countChanged(int count);

 private:
    int                    m_count;
    QList<DiscoveredDock*> m_data;
};

}  // namespace dock
}  // namespace uc
