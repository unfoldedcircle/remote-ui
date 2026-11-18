// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The screen stack of main.qml with the real main screen (MainContainer.qml), the real profile page and settings
// (Profile.qml, settings/Settings.qml), a real entity screen (the button screen, a BaseDetail) for a real
// entity::Button and the real charging screen, in copies of main.qml's containers (qml/ScreenStackHost.qml), with the
// real InputController behind ui.inputController. The keys are sent to the window like the device delivers them.
//
// Regression: a black screen after HOME that no key and no touch left. An entity screen opened on top of another
// screen (main.qml's third container, open(true)) hid the main screen when it opened and hid it again when it closed,
// expecting the screen below to show it once it closes itself. Below a screen that never hid it, the profile page
// with the settings, nothing ever showed it again: after the screen on top was closed, BACK and HOME closed the
// profile page and the main screen owned the input while it was hidden - transparent, shrunk and disabled for
// touches. The charging screen returned to it as well. The same happened when the screen
// below closed first (an activity turned off from its POWER button while an entity of its menu was open).

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QStringListModel>
#include <QTime>
#include <QTimer>
#include <QtTest>

#include "hardware/hardwareModel.h"
#include "ui/colors.h"
#include "ui/entity/button.h"
#include "ui/entity/entity.h"
#include "ui/fonts.h"
#include "ui/inputController.h"

using uc::ui::Colors;
using uc::ui::Fonts;
using uc::ui::InputController;
using uc::ui::entity::Button;

namespace {

// a model the screens count, for the badges of the profile page
class CountStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)

 public:
    using QObject::QObject;
    int count() const { return 0; }
};

// ui.profile: an unrestricted profile
class ProfileStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(bool restricted READ restricted CONSTANT)

 public:
    using QObject::QObject;
    QString id() const { return QStringLiteral("profile-1"); }
    QString name() const { return QStringLiteral("Home"); }
    QString icon() const { return QStringLiteral("uc:user"); }
    bool    restricted() const { return false; }
};

// The `ui` context property as far as the main screen, the profile page and the screens use it (uiController.h).
class UiStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *inputController READ inputController CONSTANT)
    Q_PROPERTY(QObject *profile READ profile CONSTANT)
    Q_PROPERTY(QObject *profiles READ profiles CONSTANT)
    Q_PROPERTY(QObject *pages READ pages CONSTANT)
    Q_PROPERTY(bool editMode MEMBER m_editMode NOTIFY editModeChanged)
    Q_PROPERTY(bool showHelp MEMBER m_showHelp NOTIFY showHelpChanged)
    Q_PROPERTY(bool keyNavigationEnabled READ yes CONSTANT)
    Q_PROPERTY(bool keyNavigationActive READ yes CONSTANT)
    Q_PROPERTY(bool coreConnected READ yes CONSTANT)
    Q_PROPERTY(bool isConnecting READ no CONSTANT)
    Q_PROPERTY(bool isOnboarding READ no CONSTANT)
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(int height READ height CONSTANT)
    Q_PROPERTY(int cornerRadiusSmall READ cornerRadiusSmall CONSTANT)
    Q_PROPERTY(int cornerRadiusLarge READ cornerRadiusLarge CONSTANT)
    Q_PROPERTY(QTime time READ time CONSTANT)

 public:
    explicit UiStub(QObject *parent = nullptr)
        : QObject(parent), m_input(uc::hw::HardwareModel::DEV), m_profiles(this), m_pages(this), m_profile(this) {}

    QObject *inputController() { return &m_input; }
    QObject *profile() { return &m_profile; }
    QObject *profiles() { return &m_profiles; }
    QObject *pages() { return &m_pages; }
    bool     yes() const { return true; }
    bool     no() const { return false; }
    int      width() const { return 480; }
    int      height() const { return 850; }
    int      cornerRadiusSmall() const { return 10; }
    int      cornerRadiusLarge() const { return 20; }
    QTime    time() const { return QTime(12, 0); }

    Q_INVOKABLE QString createQrCode(const QString &) { return QString(); }
    Q_INVOKABLE void    createNotification(const QString &, bool = false) {}
    Q_INVOKABLE void    createActionableNotification(const QString &, const QString &) {}
    Q_INVOKABLE void    getProfilesFromCore() {}
    Q_INVOKABLE void    setTimeOut(int msec, QJSValue callback) {
        QTimer::singleShot(msec, this, [callback]() mutable { callback.call(); });
    }

    InputController *input() { return &m_input; }

 signals:
    void editModeChanged();
    void showHelpChanged();

 private:
    InputController  m_input;
    QStringListModel m_profiles;
    QStringListModel m_pages;
    ProfileStub      m_profile;
    bool             m_editMode = false;
    bool             m_showHelp = false;
};

// `resource` (resources.h): no icon files
class ResourceStub : public QObject {
    Q_OBJECT
 public:
    Q_INVOKABLE QString     getIcon(const QString &, const QString &) { return QString(); }
    Q_INVOKABLE QSize       getIconSize(const QString &, int, int) { return QSize(-1, -1); }
    Q_INVOKABLE QStringList getIconList() { return {}; }
    Q_INVOKABLE QStringList getCustomIconList() { return {}; }
    Q_INVOKABLE QString     getIconPlaceholder() { return QString(); }
};

// the singletons the screens import, as far as they use them
class HapticStub : public QObject {
    Q_OBJECT
 public:
    enum Effects { Click = 0, Buzz = 1, Error = 2, Bump = 3 };
    Q_ENUM(Effects)

    Q_INVOKABLE void play(int) {}
};

class ConfigStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool clock24h READ yes CONSTANT)
    Q_PROPERTY(bool webConfiguratorEnabled READ no CONSTANT)
    Q_PROPERTY(QString webConfiguratorAddress READ empty CONSTANT)
    Q_PROPERTY(QString webConfiguratorPin READ pin CONSTANT)
    Q_PROPERTY(bool entityButtonFuncInverted READ no CONSTANT)
    Q_PROPERTY(bool showBatteryPercentage READ no CONSTANT)
    Q_PROPERTY(bool showBatteryEveryWhere READ no CONSTANT)
    Q_PROPERTY(QString currentProfileId READ empty CONSTANT)

 public:
    bool    yes() const { return true; }
    bool    no() const { return false; }
    QString empty() const { return QString(); }
    QString pin() const { return QStringLiteral("0000"); }

    Q_INVOKABLE void    getConfig() {}
    Q_INVOKABLE void    generateNewWebConfigPin() {}
    Q_INVOKABLE QString webConfiguratorUrl(const QString &, bool) { return QString(); }
};

// Wifi.currentNetwork
class NetworkStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(int signalStrength READ signalStrength CONSTANT)

 public:
    using QObject::QObject;
    int signalStrength() const { return 0; }
};

class WifiStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString ipAddress READ empty CONSTANT)
    Q_PROPERTY(bool isConnected READ yes CONSTANT)
    Q_PROPERTY(QObject *currentNetwork READ currentNetwork CONSTANT)

 public:
    QString          empty() const { return QString(); }
    bool             yes() const { return true; }
    QObject         *currentNetwork() { return &m_network; }
    Q_INVOKABLE void getWifiStatus() {}

 private:
    NetworkStub m_network;
};

class BatteryStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(int level READ level CONSTANT)
    Q_PROPERTY(bool isCharging READ yes CONSTANT)
    Q_PROPERTY(bool low READ no CONSTANT)

 public:
    int  level() const { return 80; }
    bool yes() const { return true; }
    bool no() const { return false; }
};

class SoftwareUpdateStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool updateAvailable READ no CONSTANT)

 public:
    bool no() const { return false; }
};

// Entity.Controller: BaseDetail fetches the entity when its screen opens
class EntityControllerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool resumePending READ no CONSTANT)
    Q_PROPERTY(bool commandInProgress READ no CONSTANT)

 public:
    bool                 no() const { return false; }
    Q_INVOKABLE void     refreshEntity(const QString &) {}
    Q_INVOKABLE QObject *get(const QString &) { return nullptr; }
};

class IntegrationControllerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *integrationsModel READ model CONSTANT)

 public:
    QObject *model() { return &m_model; }

 private:
    CountStub m_model;
};

class DockControllerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *configuredDocks READ model CONSTANT)

 public:
    QObject *model() { return &m_model; }

 private:
    CountStub m_model;
};

// an empty singleton for a module that is imported, but not used by the screens of the test
class EmptyStub : public QObject {
    Q_OBJECT
};

// the enum types of a module that is imported, but whose values the screens of the test do not need
class EnumStub : public QObject {
    Q_OBJECT
};

template <typename T>
QObject *singleton(QQmlEngine *, QJSEngine *) {
    return new T;
}

// the warnings of the QML engine, with the one of the main screen that showed itself (MainContainer.qml)
QStringList warnings;

const QString kShownWarning = QStringLiteral("Main screen hidden with nothing open on top of it: showing it");

QtMessageHandler previousHandler = nullptr;

QVariantMap available() {
    return QVariantMap{{QStringLiteral("state"), QStringLiteral("AVAILABLE")}};
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    if (type == QtWarningMsg) {
        warnings.append(message);
    }
    previousHandler(type, context, message);
}

}  // namespace

class testScreenStack : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void homeAfterAScreenOnTopOfTheSettingsShowsTheMainScreen_data();
    void homeAfterAScreenOnTopOfTheSettingsShowsTheMainScreen();
    void aScreenOnTheMainScreenShowsItWhenItCloses_data();
    void aScreenOnTheMainScreenShowsItWhenItCloses();
    void aScreenOnTopOfAnEntityScreenKeepsTheMainScreenHidden();
    void aScreenOnTopOfAnEntityScreenThatClosedFirstLeavesTheMainScreenShown();
    void homeShowsAHiddenMainScreen();
    void theChargingScreenReturnsToAShownMainScreen();

 private:
    QQuickItem *hostItem(const char *name) const;
    QQuickItem *mainScreen() const { return hostItem("containerMainItem"); }
    QObject    *owner() const { return ui->input()->activeItem(); }
    bool        ownerIs(const char *qmlType) const;
    void        pressButton(Qt::Key key);
    void        tap(QQuickItem *item);
    void        openProfileSettings();
    QQuickItem *openScreenOnTop();
    void        closeScreen(QQuickItem *screen, bool withCloseButton);
    void        verifyMainScreenShownAndInFront();
    bool        shownWarning() const { return warnings.contains(kShownWarning); }

    QQuickView  *view = nullptr;
    UiStub      *ui = nullptr;
    ResourceStub resource;
    Colors       colors;
    Fonts        fonts;
    QQuickItem  *host = nullptr;
    Button      *entity = nullptr;
    Button      *entityBelow = nullptr;
};

void testScreenStack::initTestCase() {
    // chained to the handler of QtTest, which handles ignoreMessage()
    previousHandler = qInstallMessageHandler(messageHandler);
}

void testScreenStack::cleanupTestCase() {
    qInstallMessageHandler(previousHandler);
}

void testScreenStack::init() {
    warnings.clear();
    ui = new UiStub(this);
    entity = new Button(QStringLiteral("button.top"), QVariantMap{{QStringLiteral("en"), QStringLiteral("Top")}},
                        QStringLiteral("en"), QString(), QString(), QString(), QStringList(), true, available(),
                        QStringLiteral("test"), this);
    entityBelow =
        new Button(QStringLiteral("button.below"), QVariantMap{{QStringLiteral("en"), QStringLiteral("Below")}},
                   QStringLiteral("en"), QString(), QString(), QString(), QStringList(), true, available(),
                   QStringLiteral("test"), this);
    QQmlEngine::setObjectOwnership(entity, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(entityBelow, QQmlEngine::CppOwnership);

    view = new QQuickView;
    view->rootContext()->setContextProperty("ui", ui);
    view->rootContext()->setContextProperty("colors", &colors);
    view->rootContext()->setContextProperty("fonts", &fonts);
    view->rootContext()->setContextProperty("resource", &resource);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->resize(480, 850);
    // the button simulator of the desktop: the keys of the computer keyboard (docs/key-navigation.md section 8)
    ui->input()->setSource(view);
    view->setSource(QUrl("qrc:/test/ScreenStackHost.qml"));
    QStringList errors;
    for (const QQmlError &error : view->errors()) {
        errors.append(error.toString());
    }
    QVERIFY2(view->status() == QQuickView::Ready, qPrintable(errors.join("\n")));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    host = view->rootObject();

    QVERIFY(mainScreen());
    QTRY_COMPARE(owner(), static_cast<QObject *>(mainScreen()));
    QCOMPARE(mainScreen()->state(), QStringLiteral("visible"));
}

void testScreenStack::cleanup() {
    delete view;
    view = nullptr;
    host = nullptr;
    delete entity;
    entity = nullptr;
    delete entityBelow;
    entityBelow = nullptr;
    delete ui;
    ui = nullptr;
}

QQuickItem *testScreenStack::hostItem(const char *name) const {
    return qobject_cast<QQuickItem *>(host->property(name).value<QObject *>());
}

bool testScreenStack::ownerIs(const char *qmlType) const {
    return owner() && QString::fromLatin1(owner()->metaObject()->className()).startsWith(QLatin1String(qmlType));
}

// A button of the remote as the device delivers it to the window: no text, a press and a release. The input
// controller (event filter, path 1) and the focused item (path 2) see it.
void testScreenStack::pressButton(Qt::Key key) {
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &release);
}

void testScreenStack::tap(QQuickItem *item) {
    const QPoint center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, center);
}

// Profiles, then Settings with the keypad: the profile page as the status bar's profile icon opens it, DOWN shows the
// selection on its first entry, DOWN moves it to Settings, OK opens Settings.
void testScreenStack::openProfileSettings() {
    QVERIFY(QMetaObject::invokeMethod(host, "openProfilePage"));
    QTRY_VERIFY(ownerIs("Profile_QMLTYPE"));
    QQuickItem *profile = hostItem("secondScreen");
    QVERIFY(profile);
    // the menu fades in
    QTRY_COMPARE(profile->property("settingsSwipeView").value<QQuickItem *>()->opacity(), 1.0);

    pressButton(Qt::Key_Down);
    pressButton(Qt::Key_Down);
    pressButton(Qt::Key_Return);
    QTRY_VERIFY(ownerIs("Settings_QMLTYPE"));
    QCOMPARE(mainScreen()->state(), QStringLiteral("visible"));
}

// An entity screen on top of the screen that is open, as an activity's menu opens it (main.qml loadThirdContainer()).
QQuickItem *testScreenStack::openScreenOnTop() {
    QObject *ownerBefore = owner();
    if (!QMetaObject::invokeMethod(host, "openEntityScreenOnTop",
                                   Q_ARG(QVariant, QVariant::fromValue<QObject *>(entity)))) {
        return nullptr;
    }
    if (!QTest::qWaitFor([&]() { return hostItem("thirdScreen") && owner() == hostItem("thirdScreen"); })) {
        return nullptr;
    }
    QQuickItem *screen = hostItem("thirdScreen");
    // the third container slides in
    if (!QTest::qWaitFor([&]() { return screen->mapToScene(QPointF(0, 0)) == QPointF(0, 0); })) {
        return nullptr;
    }
    // the screen covers the main screen, which hides
    if (mainScreen()->state() != QStringLiteral("hidden") || ownerBefore == owner()) {
        return nullptr;
    }
    return screen;
}

// the screen's own close button (BaseDetail's iconClose) or BACK
void testScreenStack::closeScreen(QQuickItem *screen, bool withCloseButton) {
    if (withCloseButton) {
        QQuickItem *icon = screen->property("iconClose").value<QQuickItem *>();
        QVERIFY(icon);
        QVERIFY(!icon->childItems().isEmpty());
        tap(icon->childItems().first());
    } else {
        pressButton(Qt::Key_Exit);
    }
}

void testScreenStack::verifyMainScreenShownAndInFront() {
    QTRY_COMPARE(owner(), static_cast<QObject *>(mainScreen()));
    QVERIFY(!host->property("secondActive").toBool());
    QVERIFY(!host->property("thirdActive").toBool());
    QTRY_COMPARE(mainScreen()->state(), QStringLiteral("visible"));
    QTRY_COMPARE(mainScreen()->opacity(), 1.0);
    QTRY_COMPARE(mainScreen()->scale(), 1.0);
    QVERIFY(mainScreen()->isEnabled());
    QVERIFY(mainScreen()->isVisible());
}

// The steps of the report: Profiles, Settings, an entity screen on top of them, the screen closed with its close
// button or BACK, BACK to the profile page, HOME. The main screen is shown again when the screen on top closes, not
// only once nothing is left on top of it.
void testScreenStack::homeAfterAScreenOnTopOfTheSettingsShowsTheMainScreen_data() {
    QTest::addColumn<bool>("withCloseButton");

    QTest::newRow("closed with its close button") << true;
    QTest::newRow("closed with BACK") << false;
}

void testScreenStack::homeAfterAScreenOnTopOfTheSettingsShowsTheMainScreen() {
    QFETCH(bool, withCloseButton);

    openProfileSettings();
    QQuickItem *screen = openScreenOnTop();
    QVERIFY(screen);

    closeScreen(screen, withCloseButton);
    QTRY_VERIFY(ownerIs("Settings_QMLTYPE"));
    QTRY_VERIFY(!host->property("thirdActive").toBool());
    // the screen on top found the main screen shown, and shows it again
    QTRY_COMPARE(mainScreen()->state(), QStringLiteral("visible"));

    pressButton(Qt::Key_Exit);
    QTRY_VERIFY(ownerIs("Profile_QMLTYPE"));

    pressButton(Qt::Key_Home);
    verifyMainScreenShownAndInFront();
    QVERIFY2(!shownWarning(), "the main screen was left hidden");
}

// Without the profile page: the entity screen opened on the main screen (a page tile, a command of the core), closed
// with its close button, BACK or HOME.
void testScreenStack::aScreenOnTheMainScreenShowsItWhenItCloses_data() {
    QTest::addColumn<int>("how");

    QTest::newRow("closed with its close button") << 0;
    QTest::newRow("closed with BACK") << 1;
    QTest::newRow("closed with HOME") << 2;
}

void testScreenStack::aScreenOnTheMainScreenShowsItWhenItCloses() {
    QFETCH(int, how);

    QVERIFY(
        QMetaObject::invokeMethod(host, "openEntityScreen", Q_ARG(QVariant, QVariant::fromValue<QObject *>(entity))));
    QTRY_VERIFY(hostItem("secondScreen") && owner() == hostItem("secondScreen"));
    QQuickItem *screen = hostItem("secondScreen");
    QCOMPARE(mainScreen()->state(), QStringLiteral("hidden"));
    // the screen slides up
    QTRY_COMPARE(screen->y(), 0.0);

    if (how == 2) {
        pressButton(Qt::Key_Home);
    } else {
        closeScreen(screen, how == 0);
    }
    verifyMainScreenShownAndInFront();
    QVERIFY2(!shownWarning(), "the main screen was left hidden");
}

// An entity screen on top of another entity screen (an entity of an activity's menu): the main screen stays hidden
// until the screen below closes as well.
void testScreenStack::aScreenOnTopOfAnEntityScreenKeepsTheMainScreenHidden() {
    QVERIFY(QMetaObject::invokeMethod(host, "openEntityScreen",
                                      Q_ARG(QVariant, QVariant::fromValue<QObject *>(entityBelow))));
    QTRY_VERIFY(hostItem("secondScreen") && owner() == hostItem("secondScreen"));
    QQuickItem *below = hostItem("secondScreen");
    QTRY_COMPARE(below->y(), 0.0);

    QQuickItem *screen = openScreenOnTop();
    QVERIFY(screen);

    pressButton(Qt::Key_Exit);
    QTRY_COMPARE(owner(), static_cast<QObject *>(below));
    QTRY_VERIFY(!host->property("thirdActive").toBool());
    // the screen below is shown again, the main screen stays behind it, hidden
    QTest::qWait(800);
    QCOMPARE(mainScreen()->state(), QStringLiteral("hidden"));
    QCOMPARE(mainScreen()->opacity(), 0.0);

    pressButton(Qt::Key_Exit);
    verifyMainScreenShownAndInFront();
    QVERIFY2(!shownWarning(), "the main screen was left hidden");
}

// The screen below closes while the entity screen on top is still open (an activity turned off from its POWER
// button, the core's readiness check took its time): it shows the main screen, and the screen on top leaves it shown.
void testScreenStack::aScreenOnTopOfAnEntityScreenThatClosedFirstLeavesTheMainScreenShown() {
    QVERIFY(QMetaObject::invokeMethod(host, "openEntityScreen",
                                      Q_ARG(QVariant, QVariant::fromValue<QObject *>(entityBelow))));
    QTRY_VERIFY(hostItem("secondScreen") && owner() == hostItem("secondScreen"));
    QQuickItem *below = hostItem("secondScreen");
    QTRY_COMPARE(below->y(), 0.0);

    QQuickItem *screen = openScreenOnTop();
    QVERIFY(screen);

    QVERIFY(QMetaObject::invokeMethod(below, "close"));
    QTRY_VERIFY(!host->property("secondActive").toBool());
    QCOMPARE(owner(), static_cast<QObject *>(screen));
    QCOMPARE(mainScreen()->state(), QStringLiteral("visible"));

    pressButton(Qt::Key_Exit);
    QTRY_VERIFY(!host->property("thirdActive").toBool());
    verifyMainScreenShownAndInFront();
    // past the screen's close animation
    QTest::qWait(800);
    QCOMPARE(mainScreen()->state(), QStringLiteral("visible"));
    QVERIFY2(!shownWarning(), "the main screen was left hidden");
}

// Whatever left the main screen hidden with nothing on top of it: HOME shows it.
void testScreenStack::homeShowsAHiddenMainScreen() {
    mainScreen()->setState(QStringLiteral("hidden"));
    QTRY_COMPARE(mainScreen()->opacity(), 0.0);
    QCOMPARE(owner(), static_cast<QObject *>(mainScreen()));

    QTest::ignoreMessage(QtWarningMsg, qPrintable(kShownWarning));
    pressButton(Qt::Key_Home);
    verifyMainScreenShownAndInFront();
}

// Whatever left the main screen hidden with nothing on top of it: the charging screen, closed with a tap, returns to
// it shown.
void testScreenStack::theChargingScreenReturnsToAShownMainScreen() {
    mainScreen()->setState(QStringLiteral("hidden"));
    QTRY_COMPARE(mainScreen()->opacity(), 0.0);

    QVERIFY(QMetaObject::invokeMethod(host, "showChargingScreen"));
    QTRY_VERIFY(host->property("chargingScreen").value<QObject *>());
    QTRY_VERIFY(owner() && owner() != mainScreen());
    // fully faded in
    QTest::qWait(600);

    QTest::ignoreMessage(QtWarningMsg, qPrintable(kShownWarning));
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(240, 425));
    QTRY_VERIFY(!host->property("chargingScreen").value<QObject *>());
    verifyMainScreenShownAndInFront();
}

int main(int argc, char *argv[]) {
    // a build host has no OpenGL context for the offscreen platform
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) {
        qputenv("QT_QUICK_BACKEND", QByteArray("software"));
    }
    QGuiApplication app(argc, argv);

    qmlRegisterSingletonType<HapticStub>("Haptic", 1, 0, "Haptic", &singleton<HapticStub>);
    qmlRegisterSingletonType<ConfigStub>("Config", 1, 0, "Config", &singleton<ConfigStub>);
    qmlRegisterSingletonType<WifiStub>("Wifi", 1, 0, "Wifi", &singleton<WifiStub>);
    qmlRegisterUncreatableType<EnumStub>("Wifi.SignalStrength", 1, 0, "SignalStrength", "Enum is not a type");
    qmlRegisterSingletonType<BatteryStub>("Battery", 1, 0, "Battery", &singleton<BatteryStub>);
    qmlRegisterSingletonType<SoftwareUpdateStub>("SoftwareUpdate", 1, 0, "SoftwareUpdate",
                                                 &singleton<SoftwareUpdateStub>);
    qmlRegisterSingletonType<EmptyStub>("HwInfo", 1, 0, "HwInfo", &singleton<EmptyStub>);
    qmlRegisterSingletonType<EntityControllerStub>("Entity.Controller", 1, 0, "EntityController",
                                                   &singleton<EntityControllerStub>);
    qmlRegisterUncreatableType<uc::ui::entity::Base>("Entity.Controller", 1, 0, "EntityTypes", "Enum is not a type");
    qmlRegisterUncreatableType<uc::ui::entity::ButtonStates>("Entity.Button", 1, 0, "ButtonStates",
                                                             "Enum is not a type");
    // imported by the page delegates of the main screen (Page.qml), which the test creates none of
    qmlRegisterUncreatableType<EnumStub>("Entity.MediaPlayer", 1, 0, "MediaPlayerFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<EnumStub>("Entity.Activity", 1, 0, "ActivityStates", "Enum is not a type");
    qmlRegisterUncreatableType<EnumStub>("Entity.Cover", 1, 0, "CoverStates", "Enum is not a type");
    qmlRegisterUncreatableType<EnumStub>("Entity.Light", 1, 0, "LightStates", "Enum is not a type");
    qmlRegisterSingletonType<EmptyStub>("Group.Controller", 1, 0, "GroupController", &singleton<EmptyStub>);
    qmlRegisterSingletonType<IntegrationControllerStub>("Integration.Controller", 1, 0, "IntegrationController",
                                                        &singleton<IntegrationControllerStub>);
    qmlRegisterSingletonType<DockControllerStub>("Dock.Controller", 1, 0, "DockController",
                                                 &singleton<DockControllerStub>);

    testScreenStack test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "test_screen_stack.moc"
