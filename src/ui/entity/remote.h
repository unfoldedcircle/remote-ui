// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSet>

#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class RemoteFeatures : public QObject {
    Q_GADGET
 public:
    // Core-API RemoteFeature: send and on_off. The core's own remotes send "send", the remotes of integration
    // drivers "send_cmd" (the name the entity document uses), and both send toggle.
    enum Enum { Send, Send_cmd, On_off, Toggle };
    Q_ENUM(Enum)
};

class RemoteAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State };
    Q_ENUM(Enum)
};

class RemoteStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On, Off };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Remote state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Remote state", "Unknown");
            case Enum::On:
                //: Device state: switched on. Not the preposition "on".
                return QCoreApplication::translate("Remote state", "On");
            case Enum::Off:
                //: Device state: switched off.
                return QCoreApplication::translate("Remote state", "Off");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

class RemoteCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { On, Off };
    Q_ENUM(Enum)
};

class RemoteDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Remote };
    Q_ENUM(Enum)
};

class Remote : public Base {
    Q_OBJECT

    // options
    Q_PROPERTY(QVariantList buttonMapping READ getButtonMapping NOTIFY buttonMappingChanged)
    Q_PROPERTY(QVariantMap ui READ getUiConfig NOTIFY uiConfigChanged)

 public:
    explicit Remote(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                    const QString &area, const QString &deviceClass, const QStringList &features, bool enabled,
                    QVariantMap attributes, QVariantMap options, const QString &integrationId, QObject *parent);
    ~Remote();

    // options
    QVariantList getButtonMapping() { return m_buttonMapping; }
    QVariantMap  getUiConfig() { return m_uiConfig; }

    Q_INVOKABLE void turnOn() override;
    Q_INVOKABLE void turnOff() override;
    Q_INVOKABLE void toggle();

    /**
     * @brief A physical button whose short press is mapped to a command of this remote, on the remote's screen or in
     * an activity. The screens forward the press, the auto-repeats and the release of their ButtonNavigation handlers;
     * the entity decides what is sent and emits it with command().
     *
     * A simple command (a `cmd_id` without "remote.") of an IR remote without a long-press mapping sends nothing on the
     * press: released before the first auto-repeat it is sent once (a tap), an auto-repeat starts the continuous IR
     * repeat instead (a hold). Every other mapping sends its command on the press.
     *
     * @param button the physical button, e.g. "NEXT"
     * @param cmdId the short-press `cmd_id`
     * @param params the short-press `params`
     * @param hasLongPress the button also has a long-press mapping. ButtonNavigation then calls the press at the
     * release and never an auto-repeat.
     */
    Q_INVOKABLE void mappedButtonPressed(const QString &button, const QString &cmdId, const QVariantMap &params,
                                         bool hasLongPress);
    /**
     * @brief An auto-repeat of a held mapped button: a simple command continues as `remote.send` with `repeat`, any
     * other command is sent again as configured.
     * @param sendPress add `press: true` to the `remote.send`
     */
    Q_INVOKABLE void mappedButtonRepeated(const QString &button, const QString &cmdId, const QVariantMap &params,
                                          bool sendPress);
    /**
     * @brief The release of a mapped button: ends a held simple command with `remote.stop_send`, or sends the tap of
     * an IR remote's simple command.
     */
    Q_INVOKABLE void mappedButtonReleased(const QString &button, const QString &cmdId, const QVariantMap &params,
                                          bool hasLongPress);

    /**
     * @return true for a remote the core provides for an IR codeset, false for a Bluetooth remote and the remotes of
     * integration drivers
     */
    bool isIrRemote() const;

    // `repeat` of the `remote.send` sent for every auto-repeat of a held simple command: keeps the dock's continuous
    // IR repeat going between the auto-repeats
    static constexpr int kHoldRepeatCount = 4;

    void sendCommand(RemoteCommands::Enum cmd, QVariantMap params);
    void sendCommand(RemoteCommands::Enum cmd);
    bool updateAttribute(const QString &attribute, QVariant data) override;
    bool updateOptions(QVariant data) override;

    void onLanguageChangedTypeSpecific() override;

 signals:
    void buttonMappingChanged();
    void uiConfigChanged();

 private:
    static bool isSimpleCommand(const QString &cmdId);
    // the press of the button waits for the release (a tap) or the first auto-repeat (a hold)
    bool isIrTapOrHold(const QString &cmdId, bool hasLongPress) const;

    // options
    QVariantList m_buttonMapping;
    QVariantMap  m_uiConfig;

    // mapped buttons that got an auto-repeat since their press
    QSet<QString> m_heldButtons;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
