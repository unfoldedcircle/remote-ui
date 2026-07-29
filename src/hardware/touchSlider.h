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
    Q_PROPERTY(int touchXMin READ getTouchXMin CONSTANT)
    Q_PROPERTY(int touchXMax READ getTouchXMax CONSTANT)

 public:
    explicit TouchSlider(QObject *parent = nullptr);
    ~TouchSlider();

    int getTouchX() { return m_touchX; }
    int getTouchXMin() { return m_touchXMin; }
    int getTouchXMax() { return m_touchXMax; }

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

    int m_touchX = 0;

 signals:
    void touchXChanged(int x);
    void touchPressed();
    void touchReleased();

 protected:
    /**
     * @brief Sets the raw axis range reported by the device driver. 0/0 means unknown,
     * consumers fall back to a default range.
     */
    void setTouchXRange(int min, int max) {
        m_touchXMin = min;
        m_touchXMax = max;
    }

 private:
    static TouchSlider *s_instance;

    int m_touchXMin = 0;
    int m_touchXMax = 0;
};

}  // namespace hw
}  // namespace uc

