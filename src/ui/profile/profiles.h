// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVariant>

#include "profile.h"

namespace uc {
namespace ui {

class Profiles : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

 public:
    enum SearchRoles { KeyRole = Qt::UserRole + 1, NameRole, IconRole, RestrictedRole };

    explicit Profiles(QObject* parent = nullptr);
    ~Profiles() = default;

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
    bool                   setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

 public:
    // Custom methods
    void append(Profile* o);
    void clear();

    QModelIndex getModelIndexByKey(const QString& key);

    Profile* getProfile(const QString& key);
    Profile* getProfile(int row);

    void removeItem(const QString& key);
    void removeItem(int row);

    void updateProfileName(const QString& key, const QString& name);
    void updateProfileIcon(const QString& key, const QString& icon);
    void updateProfileRestricted(const QString& key, bool restricted);

 public:
    // QML accesible methods
    Q_INVOKABLE QString getProfileId(int row);

 signals:
    void countChanged(int count);

 private:
    int             m_count;
    QList<Profile*> m_data;
};

}  // namespace ui
}  // namespace uc
