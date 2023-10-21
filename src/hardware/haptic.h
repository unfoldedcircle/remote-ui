// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

namespace uc {
namespace hw {

class Haptic : public QObject {
    Q_OBJECT

 public:
    explicit Haptic(QObject *parent = nullptr);
    ~Haptic();

    enum Effects { Click = 0, Buzz = 1, Error = 2, Bump = 3 };
    Q_ENUM(Effects)

    /**
     * @brief returns if the haptic motor is enabled or not
     * @return true if enabled
     */
    virtual bool getEnabled();

    /**
     * @brief enable or disable the haptic effects
     * @param enabled
     */
    virtual void setEnabled(bool enabled);

    /**
     * @brief Play an effect
     * @param effect
     */
    virtual Q_INVOKABLE void play(Effects effect);

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 private:
    static Haptic *s_instance;

    bool m_enabled;
};

}  // namespace hw
}  // namespace uc
