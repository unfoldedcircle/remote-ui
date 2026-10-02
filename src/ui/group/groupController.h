// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>

#include "../../core/core.h"
#include "group.h"

namespace uc {
namespace ui {

class GroupController : public QObject {
    Q_OBJECT

 public:
    explicit GroupController(core::Api* core, QObject* parent = nullptr);
    ~GroupController();

    Q_INVOKABLE QObject* get(const QString& groupId);
    Group*               getGroup(const QString& groupId);

    Q_INVOKABLE int addGroup(const QString& profileId, const QString& name,
                             const QStringList& entities = QStringList());
    /**
     * @param name the new name, an empty name keeps the current one
     * @param entities the new entity list, which replaces the current one: an empty list removes the last entity.
     *                 Left out (an invalid QVariant), the entities are not touched, which is what a rename passes.
     */
    Q_INVOKABLE int updateGroup(const QString& groupId, const QString& profileId, const QString& name,
                                const QVariant& entities = QVariant());
    Q_INVOKABLE int deleteGroup(const QString& groupId);

    void setProfileId(const QString& profileId);

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 signals:
    void groupAdded(QString groupId, bool success);
    void groupUpdated(QString groupId, bool success);
    void groupAlreadyExists();

    // a group was loaded or added, or the entities of a group changed
    void groupItemsChanged();

 public slots:
    void onEntityDeleted(QString entityId);

 private:
    static GroupController* s_instance;
    core::Api*              m_core;

    QHash<QString, Group*> m_groups;
    QString                m_profileId;
    // id of the newest group list request, answers to older ones are ignored
    int m_groupsRequestId = -1;

 private slots:
    void onGroupAdded(QString profileId, core::Group group);
    void onGroupChanged(QString profileId, core::Group group);
    void onGroupDeleted(QString profileId, QString groupId);
};

}  // namespace ui
}  // namespace uc
