// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uiController.h"

#include "../logging.h"

namespace uc {
namespace ui {

Controller::Controller(HardwareModel::Enum model, int width, int height, QQmlApplicationEngine *engine, Config *config,
                       core::Api *core, QObject *parent)
    : QObject(parent),
      m_engine(engine),
      m_config(config),
      m_core(core),
      m_model(model),
      m_resources(qgetenv("UC_RESOURCE_PATH"), qgetenv("UC_LEGAL_PATH"), this),
      m_profiles(this),
      m_profile("-1", "", false, "", this),
      m_pages(this),
      m_width(width),
      m_height(height),
      m_inputController(m_model) {
    qCDebug(lcUi) << "Loading UI for model:" << HardwareModel::toString(m_model);

    switch (m_model) {
        case hw::HardwareModel::UCR2:
            m_rotateScreen = true;
            m_width = height;
            m_height = width;
            break;
        case hw::HardwareModel::DEV:
            m_ratio = qgetenv("QT_SCALE_FACTOR").toDouble() != 0.0 ? (1 / qgetenv("QT_SCALE_FACTOR").toDouble()) : 1;
            break;
        default:
            break;
    }

    qCDebug(lcUi()) << "Ui resolution:" << m_width << "x" << m_height;

    // set ownership to c++ to prevent qml deleting the object
    engine->setObjectOwnership(this, QQmlEngine::CppOwnership);

    // load icons
    if (loadFont(":icons.ttf")) {
        qCDebug(lcUi()) << "Icons loaded";
    } else {
        qCWarning(lcUi()) << "Icons failed to load";
    }

    // set rendering of text
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    qCDebug(lcUi()) << "Font rendering set";

    // make ui object globally available for qml
    engine->rootContext()->setContextProperty("ui", this);

    // make fonts, colors and resources globally available for qml
    m_engine->rootContext()->setContextProperty("fonts", &m_fonts);
    m_engine->rootContext()->setContextProperty("colors", &m_colors);
    m_engine->rootContext()->setContextProperty("resource", &m_resources);

    // load controllers
    m_soundEffects = new SoundEffects(m_config->getSoundVolume(), m_config->getSoundEnabled(),
                                      qgetenv("UC_SOUND_EFFECTS_PATH"), m_model, this);
    m_soundEffects->initialize();
    qmlRegisterSingletonType<SoundEffects>("SoundEffects", 1, 0, "SoundEffects", &SoundEffects::qmlInstance);

    // TODO(#279) climate entity requires localization info: current value & signal handler for changed setting
    m_entityController = new EntityController(m_core, m_config->getLanguage(), m_config->getUnitSystemEnum(), this);
    qmlRegisterSingletonType<EntityController>("Entity.Controller", 1, 0, "EntityController",
                                               &EntityController::qmlInstance);

    m_groupController = new GroupController(m_core, this);
    qmlRegisterSingletonType<GroupController>("Group.Controller", 1, 0, "GroupController",
                                              &GroupController::qmlInstance);

    QString onBoardingPath = qgetenv("UC_ONBOARDING_PATH");
    m_isOnboarding = QFile::exists(onBoardingPath);

    if (m_isOnboarding) {
        qCDebug(lcUi()) << "ONBOARDING";
        m_onboardingController = new OnboardingController(this);
        qmlRegisterSingletonType<OnboardingController>("Onboarding", 1, 0, "OnboardingController",
                                                       &OnboardingController::qmlInstance);
    }

    // hook up signals and slots
    QObject::connect(m_config, &uc::Config::currentProfileIdChanged, this, &Controller::onProfileIdChanged);
    QObject::connect(m_config, &uc::Config::noCurrentProfileFound, this, &Controller::onNoCurrentProfileFound);

    QObject::connect(m_config, &Config::soundEnabledChanged, this,
                     [=](bool enabled) { m_soundEffects->setEnabled(enabled); });
    QObject::connect(m_config, &Config::soundVolumeChanged, this,
                     [=](int volume) { m_soundEffects->setVolume(volume); });

    QObject::connect(m_config, &Config::languageChanged, this, &Controller::onLanguageChanged);
    QObject::connect(m_config, &Config::unitSystemChanged, m_entityController, &EntityController::onUnitSystemChanged);

    QObject::connect(m_config, &Config::displayBrightnessChanged, this, &Controller::onBrightnessChanged);

    QObject::connect(m_core, &core::Api::connected, this, &Controller::onCoreConnected);
    QObject::connect(m_core, &core::Api::connectionProblem, this, &Controller::onCoreProblem);

    QObject::connect(m_core, &core::Api::warning, this, &Controller::onWarning);

    QObject::connect(m_core, &core::Api::powerModeChanged, &m_inputController, &InputController::onPowerModeChanged);

    QObject::connect(m_core, &core::Api::profileAdded, this, &Controller::onProfileAdded);
    QObject::connect(m_core, &core::Api::profileChanged, this, &Controller::onProfileChanged);
    QObject::connect(m_core, &core::Api::profileDeleted, this, &Controller::onProfileDeleted);

    QObject::connect(m_core, &core::Api::pageAdded, this, &Controller::onPageAdded);
    QObject::connect(m_core, &core::Api::pageChanged, this, &Controller::onPageChanged);
    QObject::connect(m_core, &core::Api::pageDeleted, this, &Controller::onPageDeleted);

    QObject::connect(m_core, &core::Api::entityDeleted, this, &Controller::onEntityDeleted);
    QObject::connect(m_core, &core::Api::groupDeleted, this, &Controller::onGroupDeleted);

    QObject::connect(m_groupController, &GroupController::requestEntity, this, &Controller::onEntityRequested);

    QObject::connect(m_entityController, &EntityController::activityAdded, this, &Controller::onActivityAdded);
    QObject::connect(m_entityController, &EntityController::activityRemoved, this, &Controller::onActivityRemoved);
}

Controller::~Controller() {}

void Controller::init() {
    int r = 0;

    auto keyboard = getQMLObject("keyboard");
    int  keyboardHeight = keyboard->property("height").toInt() - 2;
    int  hiddenX = 0;
    int  hiddenY = m_height;
    int  visibleX = 0;
    int  visibleY = m_height - keyboardHeight;

    if (m_model == HardwareModel::UCR2) {
        auto appWindow = getQMLObject("applicationWindow");
        appWindow->setProperty("contentOrientation", Qt::InvertedLandscapeOrientation);

        r = -90;
        hiddenX = m_height + 20;
        hiddenY = -14;
        visibleX = m_height - keyboardHeight + 14;
        visibleY = -14;
    }

    auto obj = getQMLObject("root");
    obj->setProperty("rotation", r);

    keyboard->setProperty("hiddenX", hiddenX);
    keyboard->setProperty("hiddenY", hiddenY);
    keyboard->setProperty("visibleX", visibleX);
    keyboard->setProperty("visibleY", visibleY);
    keyboard->setProperty("rotation", r);

    // set up timer for time display
    m_clockTimer.setInterval(1000);
    m_clockTimer.setTimerType(Qt::VeryCoarseTimer);

    QObject::connect(&m_clockTimer, &QTimer::timeout, this, &Controller::onClockTimerTimeOut);

    m_clockTimer.start();
    onClockTimerTimeOut();

    qCDebug(lcUi()) << "Init done";
}

void Controller::setEditMode(bool editMode) {
    m_editMode = editMode;
    emit editModeChanged();
}

//============================================================================================================================================//
// Q_PROPERTY methods

bool Controller::getKeyNavigationEnabled() {
    switch (m_model) {
        case HardwareModel::UCR2:
        case HardwareModel::YIO1:
        case HardwareModel::DEV:
            return true;
        default:
            return false;
    }
}

bool Controller::getShowRegulatoryInfo() {
    switch (m_model) {
        case HardwareModel::UCR2:
            return true;
        default:
            return false;
    }
}

void Controller::setShowHelp(bool value) {
    m_showHelp = value;
    emit showHelpChanged();
}

//============================================================================================================================================//
// QML accesible methods

void Controller::getProfilesFromCore() {
    int id = m_core->getProfiles();

    m_core->onResponseWithErrorResult(
        id, &core::Api::respProfiles,
        [=](QList<core::Profile> profiles) {
            // success
            m_profiles.clear();

            if (profiles.size() > 0) {
                for (QList<core::Profile>::iterator i = profiles.begin(); i != profiles.end(); i++) {
                    QString icon = i->icon;
                    if (icon.isEmpty()) {
                        icon = "uc:profile";
                    }
                    m_profiles.append(new Profile(i->id, i->name, i->restricted, icon, this));
                }
            }
            m_profilesLoaded = true;
            checkConfigLoaded();
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcUi()) << "Error retrieving the profiles:" << code << message << "Trying again.";
            QTimer::singleShot(2000, [=] { getProfilesFromCore(); });
        });
}

int Controller::switchProfile(const QString &profileId, const QString &pin) {
    int id = m_core->switchProfile(profileId, pin);

    m_core->onResult(
        id,
        [=]() {
            // success
            m_config->setCurrentProfileId(profileId);
            emit profileSwitch(true);
        },
        [=](int code, QString message) {
            // fail
            qCCritical(lcUi()) << "Error switching profile:" << code << message;
            m_notification.createNotification(message, true);
            emit profileSwitch(false);
        });

    return id;
}

int Controller::addProfile(const QString &name, bool restricted) {
    int id = m_core->addProfile(name, restricted);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respProfile,
        [=](core::Profile profile) {
            // success
            m_config->setCurrentProfileId(profile.id);
            emit profileAdded(true);
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error adding profile: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            if (code != 422) {
                m_notification.createNotification(errorMsg, true);
            }
            emit profileAdded(false, code);
        });

    return id;
}

int Controller::renameProfile(const QString &profileId, const QString &name, int pin) {
    return updateProfile(profileId, name, "-1", pin);
}

int Controller::changeProfileIcon(const QString &profileId, const QString &icon, int pin) {
    return updateProfile(profileId, "", icon, pin);
}

int Controller::deleteProfile(const QString &profileId, int pin) {
    if (profileId.contains(m_profile.getId())) {
        m_notification.createActionableWarningNotification(
            tr("Error"),
            tr("Deleting a current profile is not permitted. Please switch to another profile and try again."),
            "warning");
        return -1;
    }

    int id = m_core->deleteProfile(profileId, pin);

    m_core->onResult(
        id,
        [=]() {
            // success
            qCDebug(lcUi()) << "Profile deleted successfully";
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error deleting profile: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            m_notification.createNotification(errorMsg, true);
        });

    return id;
}

int Controller::addPage(const QString &name) {
    qCDebug(lcUi()).noquote() << "Adding page:" << name;

    int id = m_core->addPage(m_profile.getId(), name, m_pages.count() + 1, -1);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error adding page: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            m_notification.createNotification(errorMsg, true);

            syncWithCore();
        });

    return id;
}

int Controller::renamePage(const QString &pageId, const QString &name) {
    int id = m_core->updatePage(pageId, m_profile.getId(), name);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respPage,
        [=](core::Page page) {
            // success
            Q_UNUSED(page)
            qCDebug(lcUi()) << "Page rename successful";
            m_pages.updatePageName(pageId, name);
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error renaming page: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            m_notification.createNotification(errorMsg, true);
            syncWithCore();
        });

    return id;
}

int Controller::deletePage(const QString &pageId) {
    int id = m_core->deletePage(pageId, -1);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error deleting page: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            m_notification.createNotification(errorMsg, true);
        });

    return id;
}

int Controller::updatePagePos() {
    QStringList list;

    for (int i = 0; i < m_pages.count(); i++) {
        list.append(m_pages.getPage(i)->pageId());
    }

    return updateProfile(m_profile.getId(), "", "-1", -1, list);
}

int Controller::updatePageItems(const QString &pageId) {
    qCDebug(lcUi()) << "Update page items for" << pageId;

    QVariantList  itemsList;
    PageItemList *items = m_pages.getPage(pageId)->pageItems();

    // iterate
    for (int i = 0; i < items->count(); i++) {
        QVariantMap map;
        PageItem   *page = items->getPageItem(i);

        map.insert(page->pageItemType() == PageItem::Entity ? "entity_id" : "group_id", page->pageItemId());
        itemsList.append(map);
    }

    return updatePage(pageId, "", "-1", -1, itemsList);
}

void Controller::getFactoryResetToken() {
    int id = m_core->getFactoryResetToken();

    m_core->onResponseWithErrorResult(
        id, &core::Api::respFactoryResetToken,
        [=](QString token) {
            // success
            m_factoryResetToken = token;
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error getting factory reset token: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            m_notification.createNotification(errorMsg, true);
        });
}

void Controller::factoryReset() {
    int id = m_core->factoryReset(m_factoryResetToken);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error factory reset: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            m_notification.createNotification(errorMsg, true);
        });
}

void Controller::cancelFactoryReset() {
    qCDebug(lcUi()) << "Factory reset token cleared";
    m_factoryResetToken.clear();
}

void Controller::createNotification(const QString &message, bool warning) {
    m_notification.createNotification(message, warning);
}

void Controller::createActionableWarningNotification(const QString &title, const QString &message, const QString &icon,
                                                     QJSValue action, const QString &actionLabel) {
    m_notification.createActionableWarningNotification(
        title, message, icon,
        [](QVariant param) {
            QJSValue val = param.value<QJSValue>();

            if (val.isCallable()) {
                qCDebug(lcUi()) << "Calling action";
                val.call();
            }
        },
        QVariant::fromValue(action), actionLabel);
}

void Controller::createActionableNotification(const QString &title, const QString &message, const QString &icon,
                                              QJSValue action, const QString &actionLabel) {
    m_notification.createActionableNotification(
        title, message, icon,
        [](QVariant param) {
            QJSValue val = param.value<QJSValue>();

            if (val.isCallable()) {
                qCDebug(lcUi()) << " Calling action";
                val.call();
            }
        },
        QVariant::fromValue(action), actionLabel);
}

QString Controller::createQrCode(const QString &message) {
    auto qr = Util::generateQrCode(message, 200);

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    qr.save(&buffer, "PNG");
    auto encoded = buffer.data().toBase64();

    return encoded;
}

void Controller::setOnboarding(bool value) {
    if (m_isOnboarding != value) {
        m_isOnboarding = value;
        emit isOnboardingChanged();

        if (!m_isOnboarding) {
            QString onBoardingPath = qgetenv("UC_ONBOARDING_PATH");
            QFile::remove(onBoardingPath);
        }
    }
}

void Controller::setTimeOut(int msec, QJSValue callback) {
    QTimer::singleShot(msec, this, [callback]() mutable {
        if (callback.isCallable()) {
            callback.call();
        }
    });
}

//============================================================================================================================================//
// Core communication methods

void Controller::syncWithCore() {
    qCDebug(lcUi()) << "Syncing with core";

    m_pages.clear();
    loadPages(m_profile.getId(), -1);
}

void Controller::loadProfile(const QString &profileId) {
    qCDebug(lcUi()) << "Loading profile" << profileId;

    int id = m_core->getProfile(profileId);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respProfile,
        [=](core::Profile profile) {
            // success
            m_profile.setId(profile.id);
            m_profile.setName(profile.name);
            QString icon = profile.icon;
            if (icon.isEmpty()) {
                icon = "uc:profile";
            }
            m_profile.setIcon(icon);
            m_profile.setRestricted(profile.restricted);

            qCDebug(lcUi()) << "Loaded profile, id:" << m_profile.getId() << "name:" << m_profile.getName()
                            << "icon:" << m_profile.getIcon() << "restricted:" << m_profile.restricted();
            emit profileChanged();

            m_groupController->setProfileId(m_profile.getId());

            loadPages(m_profile.getId(), -1);
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error loading profile: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            m_notification.createNotification(errorMsg, true);
        });
}

int Controller::updateProfile(const QString &profileId, const QString &name, const QString &icon, int pin,
                              const QStringList &pages) {
    int id = m_core->updateProfile(profileId, name, icon, pin, pages);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respProfile,
        [=](core::Profile profile) {
            // success
            qCDebug(lcUi()) << "Profile updated successfully";
            QString icon = profile.icon;
            if (icon.isEmpty()) {
                icon = "uc:profile";
            }

            m_profiles.updateProfileName(profileId, profile.name);
            m_profiles.updateProfileIcon(profileId, icon);
            m_profiles.updateProfileRestricted(profileId, profile.restricted);

            if (profile.id == m_profile.getId()) {
                m_profile.setName(profile.name);
                m_profile.setIcon(icon);
                m_profile.setRestricted(profile.restricted);
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcUi()) << "Error:" << code << message;
            m_notification.createActionableWarningNotification(tr("Profile update error"), message, "uc:warning");
        });

    return id;
}

void Controller::loadPages(const QString &profileId, int pin) {
    qCDebug(lcUi()) << "Loading pages";
    m_pages.clear();

    int id = m_core->getPages(profileId, pin);

    m_core->onResponseWithErrorResult(
        id, &core::Api::respPages,
        [=](QList<core::Page> pages) {
            // success
            if (pages.size() > 0) {
                for (QList<core::Page>::iterator i = pages.begin(); i != pages.end(); i++) {
                    auto page = new Page(i->id, i->name, i->image, this);
                    QObject::connect(page, &Page::requestEntity, this, &Controller::onEntityRequested);
                    page->init(i->items);
                    m_pages.append(page);
                }
            }
            m_pagesLoaded = true;
            checkConfigLoaded();
            qCDebug(lcUi()).noquote() << pages.count() << "pages added";

            QStringList activities = m_entityController->getActivities();
            if (activities.length() > 0) {
                for (QStringList::iterator i = activities.begin(); i != activities.end(); i++) {
                    onActivityAdded(*i);
                }
            }
        },
        [=](int code, QString message) {
            // fail
            qCWarning(lcUi()) << "Error:" << code << message;
        });
}

int Controller::updatePage(const QString &pageId, const QString &name, const QString &image, int pos,
                           const QVariantList &items) {
    int id = m_core->updatePage(pageId, m_profile.getId(), name, image, pos, items, -1);

    m_core->onResult(
        id,
        [=]() {
            // success
        },
        [=](int code, QString message) {
            // fail
            QString errorMsg = "Error updating page: " + message;
            qCWarning(lcUi()) << code << errorMsg;
            m_notification.createNotification(errorMsg, true);
            syncWithCore();
        });

    return id;
}

//============================================================================================================================================//
// Public slots

void Controller::onCoreConnected() {
    m_inputController.blockInput(false);

    m_coreConnected = true;
    emit coreConnectedChanged();

    getProfilesFromCore();
}

void Controller::onCoreProblem() {
    m_coreConnected = false;
    emit coreConnectedChanged();
}

void Controller::onWarning(core::MsgEventTypes::WarningEvent event, bool shutdown, QString message) {
    Q_UNUSED(shutdown)
    Q_UNUSED(message)

    qCDebug(lcUi()) << event << shutdown << message;

    if (event == core::MsgEventTypes::WarningEvent::OPEN_CASE) {
        qCDebug(lcUi()) << "Case open, show warning";

        auto obj = getQMLObject("remoteOpenLoader");
        obj->setProperty("active", true);
    }
}

void Controller::onLanguageChanged(QString language) {
    // TODO(Marton) why is this a manual call and not setup with a signal / slot?
    m_entityController->onLanguageChanged(language);
}

void Controller::onBrightnessChanged(int brightness) {
    if (m_model == HardwareModel::UCR2) {
        m_globalBrightness = (100 - brightness) / 100.0;
        emit globalBrightnessChanged();
    }
}

void Controller::onProfileIdChanged() {
    loadProfile(m_config->getCurrentProfileId());
}

void Controller::onNoCurrentProfileFound() {
    m_isNoProfile = true;
    emit isNoProfileChanged();
}

void Controller::onClockTimerTimeOut() {
    m_time = QDateTime::currentDateTime().toTimeZone(QTimeZone(m_config->getTimezone().toLocal8Bit())).time();
    emit timeChanged();
}

void Controller::onIntegrationIsConnecting(bool value) {
    if (m_isConnecting != value && !m_isOnboarding) {
        m_isConnecting = value;
        emit isConnectingChanged();
    }
}

void Controller::onIntegrationError(QString name, QString id) {
    if (!m_isOnboarding) {
        m_notification.createActionableWarningNotification(
            tr("%1 error").arg(name), tr("Error while connecting to %1, with id %2").arg(name).arg(id), "uc:warning");
    }
}

void Controller::onProfileAdded(QString profileId, core::Profile profile) {
    if (m_isNoProfile) {
        switchProfile(profile.id);

        m_isNoProfile = false;
        emit isNoProfileChanged();
    }
    m_profiles.append(new Profile(profileId, profile.name, profile.restricted,
                                  profile.icon.isEmpty() ? "uc:profile" : profile.icon, this));
}

void Controller::onProfileChanged(QString profileId, core::Profile profile) {
    qCDebug(lcUi()) << "Profile change" << profile.id;

    if (m_profile.getId().contains(profileId)) {
        qCDebug(lcUi()) << "Current profile change" << profile.id;

        if (!profile.name.isEmpty()) {
            m_profile.setName(profile.name);
        }
        if (!profile.icon.isEmpty()) {
            m_profile.setIcon(profile.icon);
        }
        m_profile.setRestricted(profile.restricted);
    }

    if (!profile.name.isEmpty()) {
        m_profiles.updateProfileName(profileId, profile.name);
    }
    if (!profile.icon.isEmpty()) {
        m_profiles.updateProfileIcon(profileId, profile.icon);
    }
    m_profiles.updateProfileRestricted(profileId, profile.restricted);
}

void Controller::onProfileDeleted(QString profileId) {
    if (m_profile.getId().contains(profileId)) {
        qCDebug(lcUi()) << "Profile deleted" << profileId;
        m_profile.setId("-1");
        emit profileChanged();

        m_isNoProfile = true;
        emit isNoProfileChanged();
    }

    m_profiles.removeItem(profileId);
}

void Controller::onPageAdded(QString profileId, core::Page page) {
    if (m_profile.getId().contains(profileId)) {
        qCDebug(lcUi()) << "New page added" << page.id;
        auto pageObj = new Page(page.id, page.name, page.image, this);
        QObject::connect(pageObj, &Page::requestEntity, this, &Controller::onEntityRequested);
        pageObj->init(page.items);
        m_pages.append(pageObj);
    }
}

void Controller::onPageChanged(QString profileId, core::Page page) {
    if (m_profile.getId().contains(profileId)) {
        qCDebug(lcUi()) << "Page changed" << page.id;
        m_pages.updatePageName(page.id, page.name);
        m_pages.updatePageImage(page.id, page.image);

        auto obj = m_pages.get(page.id);

        if (obj) {
            Page *pageItem = qobject_cast<Page *>(obj);

            if (pageItem) {
                pageItem->removeEntities();

                if (page.items.length() > 0) {
                    for (QList<core::PageItem>::iterator i = page.items.begin(); i != page.items.end(); i++) {
                        PageItem::Type pageItemType = Util::convertStringToEnum<PageItem::Type>(i->type);

                        switch (pageItemType) {
                            case PageItem::Entity:
                                pageItem->addEntity(i->id);
                                break;
                            case PageItem::Group:
                                pageItem->addGroup(i->id);
                                break;
                        }
                    }
                }
            }
        }
    }
}

void Controller::onPageDeleted(QString profileId, QString pageId) {
    if (m_profile.getId().contains(profileId)) {
        qCDebug(lcUi()) << "Page deleted" << pageId;
        m_pages.removeItem(pageId);
    }
}

void Controller::onEntityDeleted(QString entityId) {
    m_pages.onEntityDeleted(entityId);
}

void Controller::onGroupDeleted(QString profileId, QString groupId) {
    if (!m_profile.getId().contains(profileId)) {
        return;
    }
    m_pages.onGroupDeleted(groupId);
}

void Controller::onIntegrationDeleted(QString integrationId) {
    qCDebug(lcUi()) << "Delete entities related to integration:" << integrationId;

    auto list = m_entityController->getIdsByIntegration(integrationId);

    for (QStringList::iterator i = list.begin(); i != list.end(); ++i) {
        m_pages.onEntityDeleted(*i);
        m_groupController->onEntityDeleted(*i);
        m_entityController->onEntityDeleted(*i);
    }
}

void Controller::onActivityAdded(QString entityId) {
    onActivity(entityId);
}

void Controller::onActivityRemoved(QString entityId) {
    onActivity(entityId, true);
}

void Controller::onEntityRequested(QString entityId) {
    qCDebug(lcUi()) << "Entity requested:" << entityId;
    m_entityController->load(entityId);
}

//============================================================================================================================================//
// Private

bool Controller::loadFont(const QString &path) {
    bool success = true;
    if (m_fontDatabase.addApplicationFont(path) == -1) {
        success = false;
        qCWarning(lcUi()) << "Failed to load font" << path;
    }

    return success;
}

void Controller::onActivity(QString entityId, bool remove) {
    if (m_pages.count() == 0) {
        return;
    }

    for (int i = 0; i < m_pages.count(); i++) {
        auto page = m_pages.getPage(i);

        if (page->m_items->count() == 0) {
            return;
        }

        for (int j = 0; j < page->m_items->count(); j++) {
            auto pageItem = page->m_items->getPageItem(j);

            if (pageItem->pageItemType() == PageItem::Type::Group) {
                auto group = m_groupController->getGroup(pageItem->pageItemId());
                if (group) {
                    if (group->m_items->contains(entityId)) {
                        if (remove) {
                            page->removeActivity(entityId);
                        } else {
                            page->addActivity(entityId);
                        }
                    }
                }
            } else {
                if (pageItem->pageItemId() == entityId) {
                    if (remove) {
                        page->removeActivity(entityId);
                    } else {
                        page->addActivity(entityId);
                    }
                }
            }
        }
    }
}

QObject *Controller::getQMLObject(QList<QObject *> nodes, const QString &name) {
    for (int i = 0; i < nodes.size(); i++) {
        if (nodes.at(i) && nodes.at(i)->objectName() == name) {
            return dynamic_cast<QObject *>(nodes.at(i));
        } else if (nodes.at(i) && nodes.at(i)->children().size() > 0) {
            QObject *item = getQMLObject(nodes.at(i)->children(), name);
            if (item) {
                return item;
            }
        }
    }
    return nullptr;
}

QObject *Controller::getQMLObject(const QString &name) {
    return getQMLObject(m_engine->rootObjects(), name);
}

void Controller::checkConfigLoaded() {
    if (m_profilesLoaded && m_pagesLoaded) {
        emit configLoaded();
    }
}

}  // namespace ui
}  // namespace uc
