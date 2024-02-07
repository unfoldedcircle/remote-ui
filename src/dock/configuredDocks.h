// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVariant>

namespace uc {
namespace dock {

class ConfiguredDock : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString id READ getId CONSTANT)
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
    Q_PROPERTY(QString customWsUrl READ getCustomWsUrl NOTIFY customWsUrlChanged)
    Q_PROPERTY(bool active READ getActive NOTIFY activeChanged)
    Q_PROPERTY(QString model READ getModel CONSTANT)
    Q_PROPERTY(QString connectionType READ getConnectionType NOTIFY connectionTypeChanged)
    Q_PROPERTY(QString version READ getVersion NOTIFY versionChanged)
    Q_PROPERTY(State state READ getState NOTIFY stateChanged)
    Q_PROPERTY(bool learningActive READ getLearningActive NOTIFY learningActiveChanged)
    Q_PROPERTY(QString description READ getDescription NOTIFY descriptionChanged)
    Q_PROPERTY(int ledBrightness READ getLedBrightness NOTIFY ledBrightnessChanged)

 public:
    enum State {
        IDLE,
        CONNECTING,
        ACTIVE,
        RECONNECTING,
        ERROR,
    };
    Q_ENUM(State)

    explicit ConfiguredDock(const QString& id, const QString& name, const QString& customWsUrl, bool active,
                            const QString& model, const QString& connectionType, const QString& version, State state,
                            bool learningActive, const QString& description, int ledBrightness, QObject* parent = nullptr);
    ~ConfiguredDock();

    QString getId() const { return m_id; }
    QString getName() const { return m_name; }
    void    setName(const QString& name);
    QString getCustomWsUrl() const { return m_customWsUrl; }
    void    setCustomWsUrl(const QString& customWsUrl);
    bool    getActive() const { return m_active; }
    void    setActive(bool active);
    QString getModel() const { return m_model; }
    QString getConnectionType() const { return m_connectionType; }
    void    setConnectionType(const QString& connectionType);
    QString getVersion() const { return m_version; }
    void    setVersion(const QString& version);
    State   getState() const { return m_state; }
    void    setState(State state);
    bool    getLearningActive() const { return m_learningActive; }
    void    setLearningActive(bool learningActive);
    QString getDescription() const { return m_description; }
    void    setDescription(const QString& description);
    int     getLedBrightness() const { return m_ledBrightness; }
    void    setLedBrgithess(int brightness);

 signals:
    void nameChanged();
    void customWsUrlChanged();
    void activeChanged();
    void connectionTypeChanged();
    void versionChanged();
    void stateChanged();
    void learningActiveChanged();
    void descriptionChanged();
    void ledBrightnessChanged();

 private:
    QString m_id;
    QString m_name;
    QString m_customWsUrl;
    bool    m_active;
    QString m_model;
    QString m_connectionType;
    QString m_version;
    State   m_state;
    bool    m_learningActive;
    QString m_description;
    int     m_ledBrightness;
};

class ConfiguredDocks : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

 public:
    enum SearchRoles {
        KeyRole = Qt::UserRole + 1,
        NameRole,
        CustomWsUrlRole,
        ActiveRole,
        ModelRole,
        ConnectionTypeRole,
        VersionRole,
        StateRole,
        LearningActiveRole,
        DescriptionRole,
        LedBrightnessRole,
    };

    explicit ConfiguredDocks(QObject* parent = nullptr);
    ~ConfiguredDocks() = default;

 public:
    int count() const;

 public slots:
    void setCount(int count);

 public:
    int                    rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool                   removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QVariant               data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

 public:
    void append(ConfiguredDock* o);
    void clear();

    QModelIndex getModelIndexByKey(const QString& key);

    bool contains(const QString& key);

    ConfiguredDock* get(const QString& key);
    ConfiguredDock* get(int row);

    void removeItem(const QString& key);
    void removeItem(int row);

    void updateName(const QString& key, const QString& name);
    void updateCustomWsUrl(const QString& key, const QString& customWsUrl);
    void updateActive(const QString& key, bool active);
    void updateConnectionType(const QString& key, const QString& connectionType);
    void updateVersion(const QString& key, const QString& version);
    void updateState(const QString& key, ConfiguredDock::State state);
    void updateLearningActive(const QString& key, bool learningActive);
    void updateDescription(const QString& key, const QString& description);
    void updateLedBrightness(const QString &key, int brightness);

    int totalPages = 0;
    int lastPageLoaded = 0;
    int totalItems = 0;
    int limit = 0;

 signals:
    void countChanged(int count);

 private:
    int                    m_count;
    QList<ConfiguredDock*> m_data;
};

}  // namespace dock
}  // namespace uc
