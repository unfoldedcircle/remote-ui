// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_cover.md
 */

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class CoverFeatures : public QObject {
    Q_GADGET
 public:
    enum Enum { Open, Close, Stop, Position, Tilt, Tilt_stop, Tilt_position };
    Q_ENUM(Enum)
};

class CoverAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State, Position, Tilt_position };
    Q_ENUM(Enum)
};

class CoverStates : public QObject {
    Q_GADGET
 public:
    enum Enum { Unavailable = 0, Unknown, Opening, Open, Closing, Closed };
    Q_ENUM(Enum)
};

class CoverCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { Open, Close, Stop, Position, Tilt, Tilt_up, Tilt_down, Tilt_stop };
    Q_ENUM(Enum)
};

class CoverDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Blind, Curtain, Garage, Shade, Door, Gate, Window };
    Q_ENUM(Enum)
};

class Cover : public Base {
    Q_OBJECT

    Q_PROPERTY(int position READ getPosition NOTIFY positionChanged)
    Q_PROPERTY(int tiltPosition READ getTiltPosition NOTIFY tiltPositionChanged)

 public:
    explicit Cover(const QString &id, const QString &name, QVariantMap nameI18n, const QString &icon,
                   const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                   QVariantMap attributes, const QString &integrationId, QObject *parent);
    ~Cover();

    int getPosition() { return m_position; }
    int getTiltPosition() { return m_tiltPosition; }

    QString getStateInfo() override { return m_stateInfo1 + " " + m_stateInfo2; }

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void turnOff() override;

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void setPosition(int position);
    Q_INVOKABLE void setTilt(int tiltPosition);
    Q_INVOKABLE void tiltUp();
    Q_INVOKABLE void tiltDown();
    Q_INVOKABLE void tiltStop();

    void sendCommand(CoverCommands::Enum cmd, QVariantMap params);
    void sendCommand(CoverCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;

 signals:
    void positionChanged();
    void tiltPositionChanged();

 private:
    int m_position;
    int m_tiltPosition;

    QString m_stateInfo1;
    QString m_stateInfo2;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
