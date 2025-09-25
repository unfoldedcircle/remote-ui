// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QTimer>
#include <QMutex>

#include "../core/enums.h"
#include "../hardware/hardwareModel.h"

namespace uc {
namespace ui {

class InputController : public QQuickItem {
    Q_OBJECT

    Q_PROPERTY(QString activeObject READ getActiveObject NOTIFY activeObjectChanged)
    Q_PROPERTY(int repeatCount READ getRepeatCount CONSTANT)

 public:
    explicit InputController(hw::HardwareModel::Enum model);
    ~InputController();

    enum Buttons {
        BACK = Qt::Key::Key_Exit,
        HOME = Qt::Key::Key_Home,
        VOICE = Qt::Key::Key_F3,
        VOLUME_UP = Qt::Key::Key_VolumeUp,
        VOLUME_DOWN = Qt::Key::Key_VolumeDown,
        GREEN = Qt::Key::Key_Green,
        DPAD_UP = Qt::Key::Key_Up,
        YELLOW = Qt::Key::Key_Yellow,
        DPAD_LEFT = Qt::Key::Key_Left,
        DPAD_MIDDLE = Qt::Key::Key_Return,
        DPAD_RIGHT = Qt::Key::Key_Right,
        RED = Qt::Key::Key_Red,
        DPAD_DOWN = Qt::Key::Key_Down,
        BLUE = Qt::Key::Key_Blue,
        CHANNEL_UP = Qt::Key::Key_ChannelUp,
        CHANNEL_DOWN = Qt::Key::Key_ChannelDown,
        MUTE = Qt::Key::Key_VolumeMute,
        PREV = Qt::Key::Key_AudioRewind,
        PLAY = Qt::Key::Key_MediaTogglePlayPause,
        NEXT = Qt::Key::Key_AudioForward,
        POWER = Qt::Key::Key_PowerOff,
        STOP = Qt::Key::Key_Stop,
        RECORD = Qt::Key::Key_MediaRecord,
        MENU = Qt::Key::Key_F4
    };
    Q_ENUM(Buttons)

    QString getActiveObject() { return m_activeObject; }
    int     getRepeatCount() { return m_repeatCount; }

    Q_INVOKABLE void setSource(QObject *source);
    Q_INVOKABLE void emitKey(Qt::Key key, bool release = false);
    Q_INVOKABLE void blockInput(bool value);

    Q_INVOKABLE void takeControl(const QString &activeObject);
    Q_INVOKABLE void releaseControl(const QString &activeObject = QString());

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void keyPressed(QString key);
    void keyReleased(QString key);
    void activeObjectChanged();

 public slots:
    void onPowerModeChanged(core::PowerEnums::PowerMode powerMode);

 protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

 private:
    static InputController *s_instance;

    QMutex m_mutex;

    hw::HardwareModel::Enum m_model;

    QObject *m_source;

    QString m_activeObject;
    QString m_prevActiveObject;

    bool m_blockInput = false;
    bool m_blockTouchInput = false;

    QHash<int, QString> m_keyCodeMapping{
        {Qt::Key::Key_Exit, "BACK"},
        {Qt::Key::Key_Home, "HOME"},
        {Qt::Key::Key_F3, "VOICE"},
        {Qt::Key::Key_VolumeUp, "VOLUME_UP"},
        {Qt::Key::Key_VolumeDown, "VOLUME_DOWN"},
        {Qt::Key::Key_Green, "GREEN"},
        {Qt::Key::Key_Up, "DPAD_UP"},
        {Qt::Key::Key_Yellow, "YELLOW"},
        {Qt::Key::Key_Left, "DPAD_LEFT"},
        {Qt::Key::Key_Return, "DPAD_MIDDLE"},
        {Qt::Key::Key_Right, "DPAD_RIGHT"},
        {Qt::Key::Key_Red, "RED"},
        {Qt::Key::Key_Down, "DPAD_DOWN"},
        {Qt::Key::Key_Blue, "BLUE"},
        {Qt::Key::Key_ChannelUp, "CHANNEL_UP"},
        {Qt::Key::Key_ChannelDown, "CHANNEL_DOWN"},
        {Qt::Key::Key_VolumeMute, "MUTE"},
        {Qt::Key::Key_AudioRewind, "PREV"},
        {Qt::Key::Key_MediaTogglePlayPause, "PLAY"},
        {Qt::Key::Key_AudioForward, "NEXT"},
        {Qt::Key::Key_PowerOff, "POWER"},
        {Qt::Key::Key_Stop, "STOP"},
        {Qt::Key::Key_MediaRecord, "RECORD"},
        {Qt::Key::Key_F4, "MENU"},
    };

    int m_repeatCount = 4;
};

}  // namespace ui
}  // namespace uc
