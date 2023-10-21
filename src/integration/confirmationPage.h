// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>

namespace uc {
namespace integration {

class ConfirmationPage : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString title READ getTitle CONSTANT)
    Q_PROPERTY(QString message1 READ getMessage1 CONSTANT)
    Q_PROPERTY(QString image READ getImage CONSTANT)
    Q_PROPERTY(QString message2 READ getMessage2 CONSTANT)

 public:
    explicit ConfirmationPage(QVariantMap title, QVariantMap message1, const QString &image, QVariantMap message2,
                              const QString &language, QObject *parent = nullptr);
    ~ConfirmationPage();

    QString getTitle() { return m_title; }
    QString getMessage1() { return m_message1; }
    QString getImage() { return m_image; }
    QString getMessage2() { return m_message2; }

 private:
    QVariantMap m_title_i18n;
    QString     m_title;
    QVariantMap m_message1_i18n;
    QString     m_message1;
    QString     m_image;
    QVariantMap m_message2_i18n;
    QString     m_message2;
};
}  // namespace integration
}  // namespace uc
