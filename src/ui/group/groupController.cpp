// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "groupController.h"

#include "../../logging.h"

namespace uc {
namespace ui {

GroupController *GroupController::s_instance = nullptr;

GroupController::GroupController(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    QObject::connect(m_core, &core::Api::groupAdded, this, &GroupController::onGroupAdded);
    QObject::connect(m_core, &core::Api::groupChanged, this, &GroupController::onGroupChanged);
    QObject::connect(m_core, &core::Api::groupDeleted, this, &GroupController::onGroupDeleted);

    QObject::connect(m_core, &core::Api::entityDeleted, this, &GroupController::onEntityDeleted);
}

GroupController::~GroupController() {
    s_instance = nullptr;
}

int GroupController::addGroup(const QString &profileId, const QString &name, const QStringList &entities) {
    int id = m_core->addGroup(profileId, name, "", entities);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respGroup,
        [=](core::Group group) {
            // success
            qCDebug(lcGroupController()) << "Group added successfully" << group.id << group.name;
            emit groupAdded(group.id, true);
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcGroupController()) << "Cannot add group" << code << message;
            if (code == 422) {
                emit groupAlreadyExists();
            }
        });

    return id;
}

int GroupController::updateGroup(const QString &groupId, const QString &profileId, const QString &name,
                                 const QVariant &entities) {
    // a QStringList from C++, a JS array of ids from QML; nothing at all for a rename
    const bool setEntities = entities.isValid();
    int        id = m_core->updateGroup(groupId, profileId, name, QString(), entities.toStringList(), setEntities);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respGroup,
        [=](core::Group group) {
            // success
            qCDebug(lcGroupController()) << "Group updated successfully" << group.id << group.name;
            // the groups are dropped while a profile is (re)loaded: the group may be gone by now
            if (auto groupObj = m_groups.value(group.id)) {
                groupObj->setItemName(group.name);
                groupObj->setItemIcon(group.icon);
            }
            emit groupUpdated(group.id, true);
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcGroupController()) << "Cannot update group" << code << message;
        });

    return id;
}

int GroupController::deleteGroup(const QString &groupId) {
    int id = m_core->deleteGroup(groupId);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcGroupController()) << "Group deleted successfully";
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcGroupController()) << "Cannot delete group" << code << message;
        });

    return id;
}

QObject *GroupController::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void GroupController::setProfileId(const QString &profileId) {
    if (m_groups.size() > 0) {
        for (QHash<QString, Group *>::iterator i = m_groups.begin(); i != m_groups.end(); ++i) {
            i.value()->deleteLater();
        }
    }

    m_groups.clear();
    m_profileId = profileId;

    int id = m_core->getGroups(m_profileId);
    // only the newest request counts: a late answer for a profile that is no longer the current one is dropped
    m_groupsRequestId = id;

    m_core->onResponseWithErrorResult(
        id, &core::Api::respGroups,
        [=](QList<core::Group> groups) {
            // success
            if (id != m_groupsRequestId) {
                qCDebug(lcGroupController()) << "Ignoring stale groups response of profile" << profileId;
                return;
            }

            if (groups.size() > 0) {
                for (QList<core::Group>::iterator i = groups.begin(); i != groups.end(); i++) {
                    onGroupAdded(profileId, *i);
                }
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcGroupController()) << "Cannot get groups" << code << message;
        });
}

QObject *GroupController::get(const QString &groupId) {
    return m_groups.value(groupId);
}

Group *GroupController::getGroup(const QString &groupId) {
    return m_groups.value(groupId);
}

void GroupController::onGroupAdded(QString profileId, core::Group group) {
    if (m_profileId != profileId) {
        return;
    }

    // a group announced by an event while the groups are loading arrives a second time with the answer
    if (auto existing = m_groups.take(group.id)) {
        existing->deleteLater();
    }

    Group *obj = new Group(group.id, group.profileId, group.name, group.icon, this);
    obj->init(group.entities);
    m_groups.insert(group.id, obj);

    qCDebug(lcGroupController()) << "Group added" << group.id;
    emit groupItemsChanged();
}

void GroupController::onGroupChanged(QString profileId, core::Group group) {
    if (m_profileId != profileId) {
        return;
    }

    // QHash::value() hands out a null pointer for a group that is not loaded: the groups are dropped while a
    // profile is (re)loaded, and the group load that follows delivers the changed group anyway
    auto groupObj = m_groups.value(group.id);
    if (!groupObj) {
        qCWarning(lcGroupController()) << "Ignoring change of a group that is not loaded:" << group.id;
        return;
    }

    if (!group.name.isEmpty()) {
        groupObj->setItemName(group.name);
    }

    if (!group.icon.isEmpty()) {
        groupObj->setItemIcon(group.icon);
    }

    groupObj->clearEntities();
    groupObj->addEntities(group.entities);
    emit groupItemsChanged();
}

void GroupController::onGroupDeleted(QString profileId, QString groupId) {
    if (m_profileId != profileId) {
        return;
    }

    // take() returns a null pointer for a group that is not loaded, e.g. while a profile is (re)loaded
    if (auto groupObj = m_groups.take(groupId)) {
        groupObj->deleteLater();
    }
}

void GroupController::onEntityDeleted(QString entityId) {
    bool removed = false;
    for (QHash<QString, Group *>::iterator i = m_groups.begin(); i != m_groups.end(); ++i) {
        if (i.value()->m_items->contains(entityId)) {
            i.value()->removeEntity(entityId);
            removed = true;
        }
    }
    if (removed) {
        emit groupItemsChanged();
    }
}

}  // namespace ui
}  // namespace uc
