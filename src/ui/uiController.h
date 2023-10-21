// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFontDatabase>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "../config/config.h"
#include "../core/core.h"
#include "../hardware/hardwareModel.h"
#include "colors.h"
#include "entity/entityController.h"
#include "fonts.h"
#include "group/groupController.h"
#include "inputController.h"
#include "notification.h"
#include "onboardingController.h"
#include "page/pages.h"
#include "profile/profile.h"
#include "profile/profiles.h"
#include "resources.h"
#include "soundEffects.h"

using uc::hw::HardwareModel;

namespace uc {
namespace ui {

class Controller : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool editMode READ getEditMode WRITE setEditMode NOTIFY editModeChanged)
    Q_PROPERTY(bool keyNavigationEnabled READ getKeyNavigationEnabled CONSTANT)
    Q_PROPERTY(bool showRegulatoryInfo READ getShowRegulatoryInfo CONSTANT)
    Q_PROPERTY(bool coreConnected READ getCoreConnected NOTIFY coreConnectedChanged)
    Q_PROPERTY(bool rotateScreen READ getRotateScreen CONSTANT)
    Q_PROPERTY(double globalBrightness READ getGlobalBrightness NOTIFY globalBrightnessChanged)

    Q_PROPERTY(double ratio READ ratio CONSTANT)
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(int height READ height CONSTANT)
    Q_PROPERTY(int cornerRadiusSmall READ getCornerRadiusSmall CONSTANT)
    Q_PROPERTY(int cornerRadiusLarge READ getCornerRadiusLarge CONSTANT)

    Q_PROPERTY(QTime time READ getTime NOTIFY timeChanged)

    Q_PROPERTY(bool isConnecting READ getIsConnecting NOTIFY isConnectingChanged)
    Q_PROPERTY(bool isOnboarding READ getIsOnboarding NOTIFY isOnboardingChanged)
    Q_PROPERTY(bool isNoProfile READ getIsNoProfile NOTIFY isNoProfileChanged)
    Q_PROPERTY(bool showHelp READ getShowHelp WRITE setShowHelp NOTIFY showHelpChanged)

    Q_PROPERTY(Profiles* profiles READ getProfiles CONSTANT)
    Q_PROPERTY(Profile* profile READ getProfile NOTIFY profileChanged)
    Q_PROPERTY(Pages* pages READ getPages CONSTANT)

    Q_PROPERTY(InputController* inputController READ getInputController CONSTANT)
    Q_PROPERTY(Notification* notification READ getNotification CONSTANT)

 public:
    explicit Controller(HardwareModel::Enum model, int width, int height, QQmlApplicationEngine* engine, Config* config,
                        core::Api* core, QObject* parent = nullptr);
    ~Controller();

    /**
     * @brief Initialises the UI controller. Can only be called after the main.qml file is loaded
     */
    void init();

 public:
    // Q_PROPERTY methods
    bool   getEditMode() { return m_editMode; }
    void   setEditMode(bool editMode);
    bool   getKeyNavigationEnabled();
    bool   getShowRegulatoryInfo();
    bool   getCoreConnected() { return m_coreConnected; }
    bool   getRotateScreen() { return m_rotateScreen; }
    double getGlobalBrightness() { return m_globalBrightness; }

    double ratio() { return m_ratio; }
    int    width() { return m_width; }
    int    height() { return m_height; }

    int getCornerRadiusSmall() { return 8; }
    int getCornerRadiusLarge() { return 22; }

    QTime getTime() { return m_time; }

    bool getIsConnecting() { return m_isConnecting; }
    bool getIsOnboarding() { return m_isOnboarding; }
    bool getIsNoProfile() { return m_isNoProfile; }
    bool getShowHelp() { return m_showHelp; }
    void setShowHelp(bool value);

    Profiles* getProfiles() { return &m_profiles; }
    Profile*  getProfile() { return &m_profile; }
    Pages*    getPages() { return &m_pages; }

    InputController* getInputController() { return &m_inputController; }
    Notification*    getNotification() { return &m_notification; }

 public:
    // QML accesible methods
    Q_INVOKABLE void getProfilesFromCore();
    Q_INVOKABLE int  switchProfile(const QString& profileId, const QString& pin = "");

    Q_INVOKABLE int addProfile(const QString& name, bool restricted = false);
    Q_INVOKABLE int renameProfile(const QString& profileId, const QString& name, int pin = -1);
    Q_INVOKABLE int changeProfileIcon(const QString& profileId, const QString& icon, int pin = -1);
    Q_INVOKABLE int deleteProfile(const QString& profileId, int pin = -1);

    Q_INVOKABLE int addPage(const QString& name);
    Q_INVOKABLE int renamePage(const QString& pageId, const QString& name);
    Q_INVOKABLE int deletePage(const QString& pageId);

    Q_INVOKABLE int updatePagePos();
    Q_INVOKABLE int updatePageItems(const QString& pageId);

    // Factory reset
    Q_INVOKABLE void getFactoryResetToken();
    Q_INVOKABLE void factoryReset();
    Q_INVOKABLE void cancelFactoryReset();

    // Notifications
    Q_INVOKABLE void createNotification(const QString& message, bool warning = false);
    Q_INVOKABLE void createActionableNotification(const QString& title, const QString& message,
                                                  const QString& icon = QString(), QJSValue action = QJSValue(),
                                                  const QString& actionLabel = QString());
    Q_INVOKABLE void createActionableWarningNotification(const QString& title, const QString& message,
                                                         const QString& icon = QString(), QJSValue action = QJSValue(),
                                                         const QString& actionLabel = QString());

    // QR code generator
    Q_INVOKABLE QString createQrCode(const QString& message);

    Q_INVOKABLE void setOnboarding(bool value);

    Q_INVOKABLE void setTimeOut(int msec, QJSValue callback);

    // Core communication methods
 public:
    /**
     * @brief Syncs UI with the core, call it only on error
     */
    void syncWithCore();

    void loadProfile(const QString& profileId);
    int  updateProfile(const QString& profileId, const QString& name, const QString& icon = "-1", int pin = -1,
                       const QStringList& pages = QStringList({"-1"}));

    void loadPages(const QString& profileId, int pin = -1);
    int  updatePage(const QString& pageId, const QString& name, const QString& image, int pos = -1,
                    const QVariantList& items = QVariantList());

 signals:
    void editModeChanged();
    void configLoaded();
    void coreConnectedChanged();
    void globalBrightnessChanged();
    void timeChanged();
    void isConnectingChanged();
    void isOnboardingChanged();
    void isNoProfileChanged();
    void showHelpChanged();
    void profilesChanged();
    void profileChanged();
    void profileAdded(bool success, int code = 200);
    void profileSwitch(bool success);

 public slots:
    void onCoreConnected();
    void onCoreProblem();
    void onWarning(core::MsgEventTypes::WarningEvent event, bool shutdown, QString message);

    void onLanguageChanged(QString language);
    void onBrightnessChanged(int brightness);

    void onProfileIdChanged();
    void onNoCurrentProfileFound();
    void onClockTimerTimeOut();

    void onIntegrationIsConnecting(bool value);
    void onIntegrationError(QString name, QString id);

    void onProfileAdded(QString profileId, core::Profile profile);
    void onProfileChanged(QString profileId, core::Profile profile);
    void onProfileDeleted(QString profileId);

    void onPageAdded(QString profileId, core::Page page);
    void onPageChanged(QString profileId, core::Page page);
    void onPageDeleted(QString profileId, QString pageId);

    void onEntityDeleted(QString entityId);
    void onGroupDeleted(QString profileId, QString groupId);

    void onIntegrationDeleted(QString integrationId);

    void onActivityAdded(QString entityId);
    void onActivityRemoved(QString entityId);

    void onEntityRequested(QString entityId);

 private:
    QQmlApplicationEngine* m_engine;
    Config*                m_config;
    core::Api*             m_core;

    HardwareModel::Enum m_model;
    Colors              m_colors;
    Resources           m_resources;
    Fonts               m_fonts;

    bool m_editMode = false;

    Profiles m_profiles;
    Profile  m_profile;
    Pages    m_pages;

    bool m_isNoProfile = false;

    bool m_coreConnected = false;
    bool m_isConnecting = false;
    bool m_isOnboarding = false;
    bool m_showHelp = false;

    double m_ratio = 1;
    int    m_width;
    int    m_height;
    bool   m_rotateScreen = false;
    double m_globalBrightness = 0;

    QTimer m_clockTimer;
    QTime  m_time;

    QFontDatabase m_fontDatabase;
    bool          loadFont(const QString& path);

    void onActivity(QString entityId, bool remove = false);

 private:
    /**
     * @brief get a QML object, you need to have objectName property of the QML object set to be able to use this
     * @param nodes
     * @param name
     * @return QML object
     */
    QObject* getQMLObject(QList<QObject*> nodes, const QString& name);

    /**
     * @brief get a QML object
     * @param name
     * @return QML object
     */
    QObject* getQMLObject(const QString& name);

    SoundEffects*         m_soundEffects;
    InputController       m_inputController;
    Notification          m_notification;
    EntityController*     m_entityController;
    GroupController*      m_groupController;
    OnboardingController* m_onboardingController;

    QString m_factoryResetToken;

    void checkConfigLoaded();
    bool m_profilesLoaded = false;
    bool m_pagesLoaded = false;
};

}  // namespace ui
}  // namespace uc
