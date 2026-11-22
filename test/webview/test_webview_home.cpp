// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// HOME on the fullscreen web view of a web view entity that the `open` command opened on top of another screen: the
// real main.qml with its containers, the real screens and a real entity::Webview, the real InputController behind
// ui.inputController and stubs for the rest. The keys are sent to the window like the device delivers them.
//
// Regression: HOME closes the web view and the screen below it (main.qml, the third container's onClosed), and called
// close() on that screen. The rename dialogs and the profile switcher have none: a TypeError, and the screen below
// stayed open instead of the main screen.

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
#include "ui/entity/entity.h"
#include "ui/entity/webview.h"
#include "ui/entity/webviewButtons.h"
#include "ui/fonts.h"
#include "ui/inputController.h"

using uc::ui::Colors;
using uc::ui::Fonts;
using uc::ui::InputController;
using uc::ui::entity::Webview;

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
    // no web view library: the fullscreen web view shows its message instead of a page
    Q_PROPERTY(bool webViewAvailable READ no CONSTANT)

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

// Entity.Controller: BaseDetail fetches the entity when its screen opens, main.qml opens a web view by its id
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
        return QUrl(QStringLiteral("qrc:/components/entities/webview/deviceclass/Webview.qml"));
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
};

QHash<QString, QObject *> EntityControllerStub::s_entities;
EntityControllerStub     *EntityControllerStub::s_current = nullptr;

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

// WebView.Controller (webviewController.h): the `open` command of the core, and what the fullscreen web view reads
class WebViewControllerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString fullscreenEntityId MEMBER fullscreenEntityId NOTIFY fullscreenEntityIdChanged)
    Q_PROPERTY(QStringList buttonIds READ buttonIds CONSTANT)
    Q_PROPERTY(QVariantMap buttons READ buttons CONSTANT)

 public:
    QString fullscreenEntityId;
    // the buttons of the desktop model with their default actions
    QStringList buttonIds() const { return uc::ui::entity::WebviewButtons::ofModel(uc::hw::HardwareModel::DEV); }
    QVariantMap buttons() const { return uc::ui::entity::WebviewButtons::withDefaults(buttonIds(), QVariantMap()); }
    Q_INVOKABLE bool buttonRepeats(const QVariantMap &action) const {
        return uc::ui::entity::WebviewButtons::repeats(action);
    }
    Q_INVOKABLE void viewClosed() {}

    // the instance of the current engine
    static WebViewControllerStub *s_current;
    static QObject               *qmlInstance(QQmlEngine *, QJSEngine *) {
        s_current = new WebViewControllerStub;
        return s_current;
    }

 signals:
    void fullscreenEntityIdChanged();
    void openRequested(QString entityId);
};

WebViewControllerStub *WebViewControllerStub::s_current = nullptr;

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

// the warnings of the QML engine
QStringList warnings;

QtMessageHandler previousHandler = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    if (type == QtWarningMsg) {
        warnings.append(message);
    }
    previousHandler(type, context, message);
}

}  // namespace

class testWebViewHome : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void homeClosesTheWebViewAndTheScreenBelow_data();
    void homeClosesTheWebViewAndTheScreenBelow();

 private:
    QQuickItem *mainScreen() const;
    QObject    *loader(const char *name, const char *alias) const;
    QObject    *secondLoader() const { return loader("containerSecond", "loader"); }
    QObject    *thirdLoader() const { return loader("containerThird", "loaderThird"); }
    QQuickItem *secondScreen() const;
    QQuickItem *thirdScreen() const;
    bool        secondActive() const { return secondLoader()->property("active").toBool(); }
    bool        thirdActive() const { return thirdLoader()->property("active").toBool(); }
    QObject    *owner() const { return ui->input()->activeItem(); }
    void        pressButton(Qt::Key key);
    bool        call(const QString &expression, const QVariantMap &arguments);

    QQmlApplicationEngine *engine = nullptr;
    QQuickWindow          *window = nullptr;
    QObject               *stackRoot = nullptr;
    UiStub                *ui = nullptr;
    ResourceStub           resource;
    Colors                 colors;
    Fonts                  fonts;
    Webview               *webview = nullptr;
};

void testWebViewHome::initTestCase() {
    // chained to the handler of QtTest, which handles ignoreMessage()
    previousHandler = qInstallMessageHandler(messageHandler);
}

void testWebViewHome::cleanupTestCase() {
    qInstallMessageHandler(previousHandler);
}

void testWebViewHome::init() {
    warnings.clear();
    ui = new UiStub(this);
    webview = new Webview(
        QStringLiteral("web.main.1"), QVariantMap{{QStringLiteral("en"), QStringLiteral("Dashboard")}},
        QStringLiteral("en"), QString(), QString(), true, QVariantMap{{QStringLiteral("state"), QStringLiteral("ON")}},
        QVariantMap{{QStringLiteral("url"), QStringLiteral("https://example.com/")}}, QStringLiteral("web.main"), this);
    QQmlEngine::setObjectOwnership(webview, QQmlEngine::CppOwnership);
    EntityControllerStub::s_entities.insert(webview->getId(), webview);

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
    QVERIFY(QTest::qWaitForWindowExposed(window));

    // the configuration arrived: main.qml loads the main screen
    emit ui->configLoaded();
    QTRY_VERIFY(mainScreen());
    QTRY_COMPARE(owner(), static_cast<QObject *>(mainScreen()));
}

void testWebViewHome::cleanup() {
    delete engine;
    engine = nullptr;
    window = nullptr;
    stackRoot = nullptr;
    EntityControllerStub::s_entities.clear();
    delete webview;
    webview = nullptr;
    delete ui;
    ui = nullptr;
}

QQuickItem *testWebViewHome::mainScreen() const {
    return qobject_cast<QQuickItem *>(stackRoot->property("containerMainItem").value<QObject *>());
}

QObject *testWebViewHome::loader(const char *name, const char *alias) const {
    return window->findChild<QObject *>(name)->property(alias).value<QObject *>();
}

QQuickItem *testWebViewHome::secondScreen() const {
    return qobject_cast<QQuickItem *>(secondLoader()->property("item").value<QObject *>());
}

QQuickItem *testWebViewHome::thirdScreen() const {
    return qobject_cast<QQuickItem *>(thirdLoader()->property("item").value<QObject *>());
}

// A button of the remote as the device delivers it to the window: no text, a press and a release.
void testWebViewHome::pressButton(Qt::Key key) {
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &release);
}

// Evaluates a JavaScript expression in main.qml, with the arguments as names: its functions take default
// arguments, which leaves them out of the meta-object.
bool testWebViewHome::call(const QString &expression, const QVariantMap &arguments) {
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

// The screen below the web view: one without a close() of its own (the rename dialog of an entity, opened from the
// page menu) and one with it (the profile page).
void testWebViewHome::homeClosesTheWebViewAndTheScreenBelow_data() {
    QTest::addColumn<QString>("source");
    QTest::addColumn<QVariantMap>("parameters");

    QTest::newRow("rename dialog, no close()")
        << QStringLiteral("qrc:/components/entities/EntityRename.qml")
        << QVariantMap{{QStringLiteral("entityId"), QStringLiteral("web.main.1")},
                       {QStringLiteral("entityName"), QStringLiteral("Dashboard")}};
    QTest::newRow("profile page") << QStringLiteral("qrc:/components/Profile.qml") << QVariantMap();
}

void testWebViewHome::homeClosesTheWebViewAndTheScreenBelow() {
    QFETCH(QString, source);
    QFETCH(QVariantMap, parameters);

    QVERIFY(call(QStringLiteral("loadSecondContainer(testSource, testParameters)"),
                 {{QStringLiteral("testSource"), QUrl(source)}, {QStringLiteral("testParameters"), parameters}}));
    QTRY_VERIFY(secondScreen() && owner() && owner() != mainScreen());
    // the screen is in place
    QTest::qWait(600);

    // the `open` command of the core: the web view opens on top of the screen
    emit WebViewControllerStub::s_current->openRequested(webview->getId());
    QTRY_VERIFY(thirdScreen() && owner() == thirdScreen());
    QTRY_COMPARE(thirdScreen()->property("state").toString(), QStringLiteral("open"));
    QTest::qWait(600);

    pressButton(Qt::Key_Home);
    QTRY_VERIFY(!thirdActive());
    QTRY_VERIFY(!secondActive());
    QTRY_COMPARE(owner(), static_cast<QObject *>(mainScreen()));
    QTRY_COMPARE(mainScreen()->state(), QStringLiteral("visible"));
    QTRY_COMPARE(mainScreen()->opacity(), 1.0);

    // main.qml called no function the screen below lacks
    for (const QString &warning : warnings) {
        QVERIFY2(!warning.startsWith(QLatin1String("qrc:/main.qml")), qPrintable(warning));
    }
    // the closing screens showed the main screen (BaseDetail.qml), not the safety net of MainContainer.qml
    QVERIFY(!warnings.contains(QStringLiteral("Main screen hidden with nothing open on top of it: showing it")));
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
                                                    &WebViewControllerStub::qmlInstance);
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

    testWebViewHome test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "test_webview_home.moc"
