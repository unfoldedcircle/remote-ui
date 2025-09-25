// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "configuredDocks.h"

#include "../logging.h"

namespace uc {
namespace dock {

ConfiguredDock::ConfiguredDock(const QString &id, const QString &name, const QString &customWsUrl, bool active,
                               const QString &model, const QString& revision, const QString &serial, const QString &connectionType, const QString &version, State state,
                               bool learningActive, const QString &description, int ledBrightness, QObject *parent)
    : QObject(parent),
      m_id(id),
      m_name(name),
      m_customWsUrl(customWsUrl),
      m_active(active),
      m_model(model),
      m_revision(revision),
      m_serial(serial),
      m_connectionType(connectionType),
      m_version(version),
      m_state(state),
      m_learningActive(learningActive),
      m_description(description),
      m_ledBrightness(ledBrightness) {
    qCDebug(lcDockController()) << "Configured dock created" << m_id << m_model << m_revision << m_serial;
}

ConfiguredDock::~ConfiguredDock() {
    qCDebug(lcDockController()) << "Configured dock destructor" << m_id;
}

void ConfiguredDock::setName(const QString &name) {
    m_name = name;
    emit nameChanged();
}

void ConfiguredDock::setCustomWsUrl(const QString &customWsUrl) {
    m_customWsUrl = customWsUrl;
    emit customWsUrlChanged();
}

void ConfiguredDock::setActive(bool active) {
    m_active = active;
    emit activeChanged();
}

void ConfiguredDock::setConnectionType(const QString &connectionType) {
    m_connectionType = connectionType;
    emit connectionTypeChanged();
}

void ConfiguredDock::setVersion(const QString &version) {
    m_version = version;
    emit versionChanged();
}

void ConfiguredDock::setState(State state) {
    m_state = state;
    emit stateChanged();
}

void ConfiguredDock::setLearningActive(bool learningActive) {
    m_learningActive = learningActive;
    emit learningActiveChanged();
}

void ConfiguredDock::setDescription(const QString &description) {
    m_description = description;
    emit descriptionChanged();
}

void ConfiguredDock::setLedBrgithess(int brightness)
{
    m_ledBrightness = brightness;
    emit ledBrightnessChanged();
}

ConfiguredDocks::ConfiguredDocks(QObject *parent) : QAbstractListModel(parent), m_count(0) {}

int ConfiguredDocks::count() const {
    return m_data.size();
}

void ConfiguredDocks::setCount(int count) {
    if (m_count == count) {
        return;
    }

    m_count = count;
    emit countChanged(m_count);
}

int ConfiguredDocks::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_data.size();
}

bool ConfiguredDocks::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row + count - 1; i >= row; i--) {
        m_data.at(i)->deleteLater();
        m_data.removeAt(i);
    }
    endRemoveRows();
    return true;
}

QVariant ConfiguredDocks::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    const ConfiguredDock *item = m_data[index.row()];
    switch (role) {
        case KeyRole:
            return item->getId();
        case NameRole:
            return item->getName();
        case CustomWsUrlRole:
            return item->getCustomWsUrl();
        case ActiveRole:
            return item->getActive();
        case ModelRole:
            return item->getModel();
        case RevisionRole:
            return item->getRevision();
        case SerialRole:
            return item->getSerial();
        case ConnectionTypeRole:
            return item->getConnectionType();
        case VersionRole:
            return item->getVersion();
        case StateRole:
            return item->getState();
        case LearningActiveRole:
            return item->getLearningActive();
        case DescriptionRole:
            return item->getDescription();
        case LedBrightnessRole:
            return item->getLedBrightness();
    }
    return QVariant();
}

QHash<int, QByteArray> ConfiguredDocks::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "dockId";
    roles[NameRole] = "dockName";
    roles[CustomWsUrlRole] = "dockCustomWsUrl";
    roles[ActiveRole] = "dockActive";
    roles[ModelRole] = "dockModel";
    roles[RevisionRole] = "dockRevision";
    roles[SerialRole] = "dockSerial";
    roles[ConnectionTypeRole] = "dockConnectionType";
    roles[VersionRole] = "dockVersion";
    roles[StateRole] = "dockState";
    roles[LearningActiveRole] = "dockLearningActive";
    roles[DescriptionRole] = "dockDescription";
    roles[LedBrightnessRole] = "dockLedBrightness";
    return roles;
}

void ConfiguredDocks::append(ConfiguredDock *o) {
    //    int i = m_data.size();
    emit layoutAboutToBeChanged();

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_data.append(o);

    emit countChanged(count());
    endInsertRows();
}

void ConfiguredDocks::clear() {
    for (int i = 0; i < m_data.count(); i++) {
        m_data[i]->deleteLater();
    }

    beginResetModel();
    m_data.clear();
    endResetModel();
    emit countChanged(count());

    totalPages = 0;
    lastPageLoaded = 0;
    totalItems = 0;
    limit = 0;
}

QModelIndex ConfiguredDocks::getModelIndexByKey(const QString &key) {
    QModelIndex idx;

    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId() == key) {
            idx = index(i, 0);
            break;
        }
    }

    return idx;
}

bool ConfiguredDocks::contains(const QString &key) {
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId().contains(key)) {
            return true;
        }
    }
    return false;
}

ConfiguredDock *ConfiguredDocks::get(const QString &key) {
    //    return m_data[getModelIndexByKey(key).row()];
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i]->getId().contains(key)) {
            return m_data[i];
        }
    }
    return nullptr;
}

ConfiguredDock *ConfiguredDocks::get(int row) {
    return m_data[row];
}

void ConfiguredDocks::removeItem(const QString &key) {
    removeItem(getModelIndexByKey(key).row());
}

void ConfiguredDocks::removeItem(int row) {
    removeRows(row, 1, QModelIndex());
    emit countChanged(count());
}

void ConfiguredDocks::updateName(const QString &key, const QString &name) {
    auto dock = get(key);

    if (dock) {
        dock->setName(name);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

void ConfiguredDocks::updateCustomWsUrl(const QString &key, const QString &customWsUrl) {
    auto dock = get(key);

    if (dock) {
        dock->setCustomWsUrl(customWsUrl);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

void ConfiguredDocks::updateActive(const QString &key, bool active) {
    auto dock = get(key);

    if (dock) {
        dock->setActive(active);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

void ConfiguredDocks::updateConnectionType(const QString &key, const QString &connectionType) {
    auto dock = get(key);

    if (dock) {
        dock->setConnectionType(connectionType);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

void ConfiguredDocks::updateVersion(const QString &key, const QString &version) {
    auto dock = get(key);

    if (dock) {
        dock->setVersion(version);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

void ConfiguredDocks::updateState(const QString &key, ConfiguredDock::State state) {
    auto dock = get(key);

    if (dock) {
        dock->setState(state);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

void ConfiguredDocks::updateLearningActive(const QString &key, bool learningActive) {
    auto dock = get(key);

    if (dock) {
        dock->setLearningActive(learningActive);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

void ConfiguredDocks::updateDescription(const QString &key, const QString &description) {
    auto dock = get(key);

    if (dock) {
        dock->setDescription(description);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

void ConfiguredDocks::updateLedBrightness(const QString &key, int brightness)
{
    auto dock = get(key);

    if (dock) {
        dock->setLedBrgithess(brightness);
        emit dataChanged(getModelIndexByKey(key), getModelIndexByKey(key));
    }
}

}  // namespace dock
}  // namespace uc
