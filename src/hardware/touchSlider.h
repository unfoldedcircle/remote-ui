// Copyright (c) 2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

namespace uc {
namespace hw {


class TouchSlider : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int touchX READ getTouchX NOTIFY touchXChanged)

 public:
    explicit TouchSlider(QObject *parent = nullptr);
    ~TouchSlider();

    int getTouchX() { return m_touchX; }

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

    int m_touchX;

 signals:
    void touchXChanged(int x);
    void touchPressed();
    void touchReleased();

 private:
    static TouchSlider *s_instance;
};

}  // namespace hw
}  // namespace uc

