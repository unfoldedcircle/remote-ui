// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The screen stack of the real main.qml: its containers with the real main screen (MainContainer.qml), the real
// profile page and settings (Profile.qml, settings/Settings.qml), a real entity screen (the button screen, a
// BaseDetail) for a real entity::Button and the real charging screen, with the real InputController behind
// ui.inputController and stubs for the rest. The keys are sent to the window like the device delivers them.
//
// Regression: a black screen after HOME that no key and no touch left. An entity screen opened on top of another
// screen (main.qml's third container, open(true)) hid the main screen when it opened and hid it again when it closed,
// expecting the screen below to show it once it closes itself. Below a screen that never hid it, the profile page
// with the settings, nothing ever showed it again: after the screen on top was closed, BACK and HOME closed the
// profile page and the main screen owned the input while it was hidden - transparent, shrunk and disabled for
// touches. The charging screen returned to it as well. The same happened when the screen
// below closed first (an activity turned off from its POWER button while an entity of its menu was open).
//
// Regression: an activity started through the Core-API, with "Open activities started with the API" on, replaced the
// open screens while an entity screen was open on top of an activity's screen, and opened out of view: the screen on
// top had moved the second container aside, and only its closing moved it back. A black screen until BACK, and the
// next screens of the container out of view as well.

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
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
    Q_PROPERTY(double globalBrightness READ zero CONSTANT)
    Q_PROPERTY(double ratio READ one CONSTANT)
    // hides the button simulator window of main.qml
    Q_PROPERTY(bool showRegulatoryInfo READ yes CONSTANT)
    Q_PROPERTY(QObject *notification READ notification CONSTANT)

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
    double   zero() const { return 0; }
    double   one() const { return 1; }
    QObject *notification() { return nullptr; }

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
    // main.qml loads the main screen, the start-up screen closes
    void configLoaded();

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
    Q_INVOKABLE QString getIcon(const QString &id, const QString &suffix = QString()) {
        Q_UNUSED(id)
        Q_UNUSED(suffix)
        return QString();
    }
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
    Q_PROPERTY(bool openActivityOnApiStart READ yes CONSTANT)
    Q_PROPERTY(QString language READ language CONSTANT)
    Q_PROPERTY(QString voiceAssistantId READ empty CONSTANT)

 public:
    bool    yes() const { return true; }
    bool    no() const { return false; }
    QString empty() const { return QString(); }
    QString pin() const { return QStringLiteral("0000"); }
    QString language() const { return QStringLiteral("en_US"); }

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

// Battery: the power supply brings up the charging screen (main.qml)
class BatteryStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(int level READ level CONSTANT)
    Q_PROPERTY(bool isCharging READ yes CONSTANT)
    Q_PROPERTY(bool low READ no CONSTANT)
    Q_PROPERTY(bool powerSupply MEMBER m_powerSupply NOTIFY powerSupplyChanged)

 public:
    int  level() const { return 80; }
    bool yes() const { return true; }
    bool no() const { return false; }

    void setPowerSupply(bool value) {
        m_powerSupply = value;
        emit powerSupplyChanged(value);
    }

    // the instance of the current engine
    static BatteryStub *s_current;
    static QObject     *qmlInstance(QQmlEngine *, QJSEngine *) {
        s_current = new BatteryStub;
        return s_current;
    }

 signals:
    void powerSupplyChanged(bool value);

 private:
    bool m_powerSupply = false;
};

BatteryStub *BatteryStub::s_current = nullptr;

// Power: main.qml shows its window while the display is on
class PowerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool windowShown READ yes CONSTANT)

 public:
    bool yes() const { return true; }

 signals:
    void powerModeChanged(int fromPowerMode, int toPowerMode);
};

class SoundEffectsStub : public QObject {
    Q_OBJECT
 public:
    enum Effects { BatteryCharge = 0, Confirm = 1, Error = 2 };
    Q_ENUM(Effects)

    Q_INVOKABLE void play(int) {}
};

class SoftwareUpdateStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool updateAvailable READ no CONSTANT)

 public:
    bool no() const { return false; }
};

// Entity.Controller: BaseDetail fetches the entity when its screen opens, main.qml opens an activity the core reports
// as started through the API. Every entity of the test has the button screen.
class EntityControllerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool resumePending READ no CONSTANT)
    Q_PROPERTY(bool resumeWindow READ no CONSTANT)
    Q_PROPERTY(int resumeTimeout READ zero CONSTANT)
    Q_PROPERTY(bool commandInProgress READ no CONSTANT)

 public:
    bool                 no() const { return false; }
    int                  zero() const { return 0; }
    Q_INVOKABLE void     refreshEntity(const QString &) {}
    Q_INVOKABLE QObject *get(const QString &id) { return s_entities.value(id); }
    Q_INVOKABLE QUrl     screenUrl(QObject *) {
        return QUrl(QStringLiteral("qrc:/components/entities/button/deviceclass/Button.qml"));
    }
    Q_INVOKABLE int checkSequenceReadiness(const QString &, const QString &) { return -1; }

    // the entities of the current test, owned by the test
    static QHash<QString, QObject *> s_entities;

    // the instance of the current engine
    static EntityControllerStub *s_current;
    static QObject              *qmlInstance(QQmlEngine *, QJSEngine *) {
        s_current = new EntityControllerStub;
        return s_current;
    }

 signals:
    void activityStartedExternally(QString entityId);
};

QHash<QString, QObject *> EntityControllerStub::s_entities;
EntityControllerStub     *EntityControllerStub::s_current = nullptr;

// WebView.Controller: main.qml opens the web view that the core's `open` command asks for
class WebViewControllerStub : public QObject {
    Q_OBJECT

 signals:
    void openRequested(QString entityId);
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
    void anActivityStartedThroughTheApiOverAScreenOnTopOpensInView();
    void homeShowsAHiddenMainScreen();
    void theChargingScreenReturnsToAShownMainScreen();

 private:
    Button     *createEntity(const QString &id);
    QQuickItem *mainScreen() const;
    // main.qml's containerSecond and containerThird (Popups) and their loaders
    QObject    *container(const char *name) const { return window->findChild<QObject *>(name); }
    QObject    *loader(const char *name, const char *alias) const;
    QObject    *secondLoader() const { return loader("containerSecond", "loader"); }
    QObject    *thirdLoader() const { return loader("containerThird", "loaderThird"); }
    QQuickItem *secondScreen() const {
        return qobject_cast<QQuickItem *>(secondLoader()->property("item").value<QObject *>());
    }
    QQuickItem *thirdScreen() const {
        return qobject_cast<QQuickItem *>(thirdLoader()->property("item").value<QObject *>());
    }
    bool        secondActive() const { return secondLoader()->property("active").toBool(); }
    bool        thirdActive() const { return thirdLoader()->property("active").toBool(); }
    QObject    *chargingScreen() const;
    QObject    *owner() const { return ui->input()->activeItem(); }
    bool        ownerIs(const char *qmlType) const;
    void        pressButton(Qt::Key key);
    void        tap(QQuickItem *item);
    bool        call(const QString &expression, const QVariantMap &arguments);
    bool        load(const QString &source, Button *screenEntity, const char *function = "loadSecondContainer");
    QQuickItem *openEntityScreen(Button *screenEntity);
    void        openProfileSettings();
    QQuickItem *openScreenOnTop();
    void        closeScreen(QQuickItem *screen, bool withCloseButton);
    void        verifyMainScreenShownAndInFront();
    bool        shownWarning() const { return warnings.contains(kShownWarning); }

    QQmlApplicationEngine *engine = nullptr;
    QQuickWindow          *window = nullptr;
    QObject               *stackRoot = nullptr;
    UiStub                *ui = nullptr;
    ResourceStub           resource;
    Colors                 colors;
    Fonts                  fonts;
    Button                *entity = nullptr;
    Button                *entityBelow = nullptr;
    Button                *activity = nullptr;
};

void testScreenStack::initTestCase() {
    // chained to the handler of QtTest, which handles ignoreMessage()
    previousHandler = qInstallMessageHandler(messageHandler);
}

void testScreenStack::cleanupTestCase() {
    qInstallMessageHandler(previousHandler);
}

Button *testScreenStack::createEntity(const QString &id) {
    auto *created = new Button(id, QVariantMap{{QStringLiteral("en"), id}}, QStringLiteral("en"), QString(), QString(),
                               QString(), QStringList(), true, available(), QStringLiteral("test"), this);
    QQmlEngine::setObjectOwnership(created, QQmlEngine::CppOwnership);
    EntityControllerStub::s_entities.insert(id, created);
    return created;
}

void testScreenStack::init() {
    warnings.clear();
    ui = new UiStub(this);
    entity = createEntity(QStringLiteral("button.top"));
    entityBelow = createEntity(QStringLiteral("button.below"));
    activity = createEntity(QStringLiteral("button.activity"));

    engine = new QQmlApplicationEngine;
    engine->rootContext()->setContextProperty("ui", ui);
    engine->rootContext()->setContextProperty("colors", &colors);
    engine->rootContext()->setContextProperty("fonts", &fonts);
    engine->rootContext()->setContextProperty("resource", &resource);
    engine->load(QUrl("qrc:/main.qml"));
    QVERIFY(!engine->rootObjects().isEmpty());
    window = qobject_cast<QQuickWindow *>(engine->rootObjects().first());
    QVERIFY(window);
    // the size of the display, which main.qml sets as the window's minimum and maximum: the offscreen platform does
    // not apply them
    window->resize(480, 850);
    stackRoot = window->findChild<QObject *>("root");
    QVERIFY(stackRoot);
    // the on-screen keyboard out of view below the display, where the UI controller puts it (uiController.cpp)
    QObject *keyboard = window->findChild<QObject *>("keyboard");
    QVERIFY(keyboard);
    keyboard->setProperty("hiddenY", 850);
    keyboard->setProperty("visibleY", 850 - keyboard->property("height").toInt() + 2);
    QVERIFY(container("containerSecond") && container("containerThird"));
    QVERIFY(QTest::qWaitForWindowExposed(window));

    // the configuration arrived: main.qml loads the main screen
    emit ui->configLoaded();
    QTRY_VERIFY(mainScreen());
    QTRY_COMPARE(owner(), static_cast<QObject *>(mainScreen()));
    QCOMPARE(mainScreen()->state(), QStringLiteral("visible"));
}

void testScreenStack::cleanup() {
    delete engine;
    engine = nullptr;
    window = nullptr;
    stackRoot = nullptr;
    EntityControllerStub::s_entities.clear();
    delete entity;
    entity = nullptr;
    delete entityBelow;
    entityBelow = nullptr;
    delete activity;
    activity = nullptr;
    delete ui;
    ui = nullptr;
}

QQuickItem *testScreenStack::mainScreen() const {
    return qobject_cast<QQuickItem *>(stackRoot->property("containerMainItem").value<QObject *>());
}

QObject *testScreenStack::loader(const char *name, const char *alias) const {
    return container(name)->property(alias).value<QObject *>();
}

QObject *testScreenStack::chargingScreen() const {
    for (QObject *object : window->findChildren<QObject *>()) {
        if (QString::fromLatin1(object->metaObject()->className())
                .startsWith(QLatin1String("ChargingScreen_QMLTYPE"))) {
            return object;
        }
    }
    return nullptr;
}

bool testScreenStack::ownerIs(const char *qmlType) const {
    return owner() && QString::fromLatin1(owner()->metaObject()->className()).startsWith(QLatin1String(qmlType));
}

// A button of the remote as the device delivers it to the window: no text, a press and a release. The input
// controller (event filter, path 1) and the focused item (path 2) see it.
void testScreenStack::pressButton(Qt::Key key) {
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &release);
}

void testScreenStack::tap(QQuickItem *item) {
    const QPoint center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
}

// Evaluates a JavaScript expression in main.qml, with the arguments as names: its functions take default
// arguments, which leaves them out of the meta-object.
bool testScreenStack::call(const QString &expression, const QVariantMap &arguments) {
    QQmlContext context(qmlContext(window));
    for (auto i = arguments.cbegin(); i != arguments.cend(); ++i) {
        context.setContextProperty(i.key(), i.value());
    }
    QQmlExpression evaluation(&context, window, expression);
    evaluation.evaluate();
    if (evaluation.hasError()) {
        qWarning() << evaluation.error().toString();
        return false;
    }
    return true;
}

// main.qml loadSecondContainer() or loadThirdContainer(), with the parameters of an entity screen
bool testScreenStack::load(const QString &source, Button *screenEntity, const char *function) {
    QVariantMap parameters;
    if (screenEntity) {
        parameters.insert(QStringLiteral("entityId"), screenEntity->getId());
        parameters.insert(QStringLiteral("entityObj"), QVariant::fromValue<QObject *>(screenEntity));
    }
    return call(QString::fromLatin1(function) + QStringLiteral("(testSource, testParameters)"),
                {{QStringLiteral("testSource"), QUrl(source)}, {QStringLiteral("testParameters"), parameters}});
}

// An entity screen as a page tile opens it (Page.qml, entities/Base.qml): in the second container. It slides up.
QQuickItem *testScreenStack::openEntityScreen(Button *screenEntity) {
    if (!load(QStringLiteral("qrc:/components/entities/button/deviceclass/Button.qml"), screenEntity)) {
        return nullptr;
    }
    if (!QTest::qWaitFor([&]() { return secondScreen() && owner() == secondScreen(); })) {
        return nullptr;
    }
    QQuickItem *screen = secondScreen();
    if (!QTest::qWaitFor([&]() { return screen->y() == 0.0; })) {
        return nullptr;
    }
    return screen;
}

// Profiles, then Settings with the keypad: the profile page as the status bar's profile icon opens it, DOWN shows the
// selection on its first entry, DOWN moves it to Settings, OK opens Settings.
void testScreenStack::openProfileSettings() {
    QVERIFY(load(QStringLiteral("qrc:/components/Profile.qml"), nullptr));
    QTRY_VERIFY(ownerIs("Profile_QMLTYPE"));
    QQuickItem *profile = secondScreen();
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
    if (!load(QStringLiteral("qrc:/components/entities/button/deviceclass/Button.qml"), entity, "loadThirdContainer")) {
        return nullptr;
    }
    if (!QTest::qWaitFor([&]() { return thirdScreen() && owner() == thirdScreen(); })) {
        return nullptr;
    }
    QQuickItem *screen = thirdScreen();
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
    QVERIFY(!secondActive());
    QVERIFY(!thirdActive());
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
    QTRY_VERIFY(!thirdActive());
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

    QQuickItem *screen = openEntityScreen(entity);
    QVERIFY(screen);
    QCOMPARE(mainScreen()->state(), QStringLiteral("hidden"));

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
    QQuickItem *below = openEntityScreen(entityBelow);
    QVERIFY(below);

    QQuickItem *screen = openScreenOnTop();
    QVERIFY(screen);

    pressButton(Qt::Key_Exit);
    QTRY_COMPARE(owner(), static_cast<QObject *>(below));
    QTRY_VERIFY(!thirdActive());
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
    QQuickItem *below = openEntityScreen(entityBelow);
    QVERIFY(below);

    QQuickItem *screen = openScreenOnTop();
    QVERIFY(screen);

    QVERIFY(QMetaObject::invokeMethod(below, "close"));
    QTRY_VERIFY(!secondActive());
    QCOMPARE(owner(), static_cast<QObject *>(screen));
    QCOMPARE(mainScreen()->state(), QStringLiteral("visible"));

    pressButton(Qt::Key_Exit);
    QTRY_VERIFY(!thirdActive());
    verifyMainScreenShownAndInFront();
    // past the screen's close animation
    QTest::qWait(800);
    QCOMPARE(mainScreen()->state(), QStringLiteral("visible"));
    QVERIFY2(!shownWarning(), "the main screen was left hidden");
}

// An activity started through the Core-API, with "open activity on API start", while an entity screen is open on top
// of an activity's screen: both are dropped and the started activity's screen opens in view, where the screen on top
// had moved the second container away from.
void testScreenStack::anActivityStartedThroughTheApiOverAScreenOnTopOpensInView() {
    QQuickItem *below = openEntityScreen(entityBelow);
    QVERIFY(below);
    QVERIFY(openScreenOnTop());
    QObject *second = container("containerSecond");
    QTRY_COMPARE(second->property("x").toReal(), -480.0);

    emit EntityControllerStub::s_current->activityStartedExternally(activity->getId());
    QTRY_VERIFY(secondScreen() && secondScreen() != below && owner() == secondScreen());
    QQuickItem *screen = secondScreen();
    QCOMPARE(screen->property("entityId").toString(), activity->getId());
    QVERIFY(!thirdActive());
    QTRY_COMPARE(screen->y(), 0.0);
    // in view, at full size
    QCOMPARE(second->property("x").toReal(), 0.0);
    QCOMPARE(second->property("scale").toReal(), 1.0);
    QCOMPARE(screen->mapToScene(QPointF(0, 0)), QPointF(0, 0));
    QCOMPARE(mainScreen()->state(), QStringLiteral("hidden"));

    pressButton(Qt::Key_Exit);
    verifyMainScreenShownAndInFront();
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

// Whatever left the main screen hidden with nothing on top of it: the charging screen, which the power supply brings
// up and a tap closes, returns to it shown.
void testScreenStack::theChargingScreenReturnsToAShownMainScreen() {
    mainScreen()->setState(QStringLiteral("hidden"));
    QTRY_COMPARE(mainScreen()->opacity(), 0.0);

    BatteryStub::s_current->setPowerSupply(true);
    QTRY_VERIFY(chargingScreen());
    QTRY_VERIFY(owner() && owner() != mainScreen());
    // fully faded in
    QTest::qWait(600);

    QTest::ignoreMessage(QtWarningMsg, qPrintable(kShownWarning));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(240, 425));
    QTRY_VERIFY(!chargingScreen());
    verifyMainScreenShownAndInFront();
}

int main(int argc, char *argv[]) {
    // a build host has no OpenGL context for the offscreen platform
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) {
        qputenv("QT_QUICK_BACKEND", QByteArray("software"));
    }
    // main.qml's on-screen keyboard, as on the device
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    QGuiApplication app(argc, argv);

    qmlRegisterSingletonType<HapticStub>("Haptic", 1, 0, "Haptic", &singleton<HapticStub>);
    qmlRegisterSingletonType<ConfigStub>("Config", 1, 0, "Config", &singleton<ConfigStub>);
    qmlRegisterSingletonType<WifiStub>("Wifi", 1, 0, "Wifi", &singleton<WifiStub>);
    qmlRegisterUncreatableType<EnumStub>("Wifi.SignalStrength", 1, 0, "SignalStrength", "Enum is not a type");
    qmlRegisterSingletonType<BatteryStub>("Battery", 1, 0, "Battery", &BatteryStub::qmlInstance);
    qmlRegisterSingletonType<PowerStub>("Power", 1, 0, "Power", &singleton<PowerStub>);
    qmlRegisterUncreatableType<EnumStub>("Power.Modes", 1, 0, "PowerModes", "Enum is not a type");
    qmlRegisterSingletonType<SoundEffectsStub>("SoundEffects", 1, 0, "SoundEffects", &singleton<SoundEffectsStub>);
    qmlRegisterSingletonType<SoftwareUpdateStub>("SoftwareUpdate", 1, 0, "SoftwareUpdate",
                                                 &singleton<SoftwareUpdateStub>);
    qmlRegisterSingletonType<EmptyStub>("HwInfo", 1, 0, "HwInfo", &singleton<EmptyStub>);
    qmlRegisterSingletonType<EmptyStub>("Voice", 1, 0, "Voice", &singleton<EmptyStub>);
    qmlRegisterSingletonType<EntityControllerStub>("Entity.Controller", 1, 0, "EntityController",
                                                   &EntityControllerStub::qmlInstance);
    qmlRegisterUncreatableType<uc::ui::entity::Base>("Entity.Controller", 1, 0, "EntityTypes", "Enum is not a type");
    qmlRegisterSingletonType<WebViewControllerStub>("WebView.Controller", 1, 0, "WebViewController",
                                                    &singleton<WebViewControllerStub>);
    qmlRegisterUncreatableType<uc::ui::entity::ButtonStates>("Entity.Button", 1, 0, "ButtonStates",
                                                             "Enum is not a type");
    // imported by screens the test creates none of: the page delegates of the main screen (Page.qml), the activity
    // loading screen, the volume overlay
    qmlRegisterUncreatableType<EnumStub>("Entity.MediaPlayer", 1, 0, "MediaPlayerFeatures", "Enum is not a type");
    qmlRegisterUncreatableType<EnumStub>("Entity.Activity", 1, 0, "ActivityStates", "Enum is not a type");
    qmlRegisterUncreatableType<EnumStub>("Entity.Macro", 1, 0, "MacroStates", "Enum is not a type");
    qmlRegisterUncreatableType<EnumStub>("Entity.Cover", 1, 0, "CoverStates", "Enum is not a type");
    qmlRegisterUncreatableType<EnumStub>("Entity.Light", 1, 0, "LightStates", "Enum is not a type");
    qmlRegisterUncreatableType<EnumStub>("SequenceStep.Type", 1, 0, "SequenceStep", "Enum is not a type");
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
