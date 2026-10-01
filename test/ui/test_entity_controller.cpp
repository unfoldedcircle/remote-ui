// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "config/config.h"
#include "core/core.h"
#include "ui/entity/activity.h"
#include "ui/entity/climate.h"
#include "ui/entity/entityController.h"
#include "ui/entity/light.h"
#include "ui/entity/mediaPlayer.h"
#include "ui/entity/sensor.h"
#include "ui/mediaImageProvider.h"
#include "ui/notification.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

static QStringList      s_warnings;
static QtMessageHandler s_previousHandler = nullptr;

static void collectWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (type == QtWarningMsg) {
        s_warnings.append(message);
    }
    if (s_previousHandler) {
        s_previousHandler(type, context, message);
    }
}

/**
 * Only the entities the UI has asked for are kept in the controller, and an entity can be deleted while a
 * screen for it is still open. Everything that looks an entity up by id therefore has to cope with an id
 * that is not in the map: QHash::value hands out a null pointer for it.
 */
class testEntityController : public QObject {
    Q_OBJECT

 private slots:
    void init();
    void cleanup();

    void setEntityName_unknownEntity_doesNotCrash();
    void setEntityName_deletedEntity_doesNotCrash();
    void setEntityIcon_unknownEntity_doesNotCrash();
    void setEntityName_knownEntity_doesNotWarn();

    void voiceEnd_droppedPendingStart_reportsUnavailable();
    void voiceEnd_nothingPending_reportsNothing();

    void mediaPlayerRepeat_invalidAttribute_keepsModeAndSendsValidCommand();

    void activity_onAgainAfterReconnect_isNotStartedExternally();

    void config_reloadedDeviceName_isNotAnnouncedAgain();

    void climate_stateInfo_usesTheUnitOfTheEntity();
    void climate_stateInfo_followsTheUnitSystem();
    void climate_currentTemperature_zeroIsShownAndNullIsNotAvailable();

    void binarySensor_valueIsTranslatedWhenRead();

    void light_brightnessText_survivesUnavailable();

    void mediaPlayer_positionTimer_onlyAnnouncesAChange();

    void mediaPlayer_embeddedArtwork_isDecodedOnAWorkerThread();
    void mediaPlayer_deletedWhileArtworkIsDecoded_doesNotCrash();

 private:
    static uc::core::Entity makeEntity(const QString& entityId);
    static QVariantMap      voiceStartParams(int sessionId);
    // a small PNG as a data URL, the way an integration embeds artwork in media_image_url
    static QString embeddedArtwork(const QColor& color);
    // the unreachable test socket warns about the connection on its own, so only the warnings of the entity
    // lookup are counted
    static int notLoadedWarnings(const QString& entityId);
};

int testEntityController::notLoadedWarnings(const QString& entityId) {
    int count = 0;

    for (const QString& warning : qAsConst(s_warnings)) {
        if (warning.contains(QStringLiteral("not loaded")) && warning.contains(entityId)) {
            count++;
        }
    }

    return count;
}

uc::core::Entity testEntityController::makeEntity(const QString& entityId) {
    uc::core::Entity entity;
    entity.id = entityId;
    entity.type = QStringLiteral("light");
    entity.name = QVariantMap({{QStringLiteral("en"), QStringLiteral("Ceiling light")}});
    entity.enabled = true;
    return entity;
}

void testEntityController::init() {
    s_warnings.clear();
    s_previousHandler = qInstallMessageHandler(collectWarnings);
}

void testEntityController::cleanup() {
    qInstallMessageHandler(s_previousHandler);
    s_previousHandler = nullptr;
    s_warnings.clear();
}

void testEntityController::setEntityName_unknownEntity_doesNotCrash() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    // renaming an entity that was never loaded used to dereference the null pointer QHash::value returns
    controller.setEntityName(QStringLiteral("light.not_loaded"), QStringLiteral("New name"));

    QCOMPARE(notLoadedWarnings(QStringLiteral("light.not_loaded")), 1);
}

void testEntityController::setEntityName_deletedEntity_doesNotCrash() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString entityId = QStringLiteral("light.deleted");
    controller.onEntityAdded(makeEntity(entityId));
    controller.onEntityDeleted(entityId);

    controller.setEntityName(entityId, QStringLiteral("New name"));

    QCOMPARE(notLoadedWarnings(entityId), 1);
}

void testEntityController::setEntityIcon_unknownEntity_doesNotCrash() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    controller.setEntityIcon(QStringLiteral("light.not_loaded"), QStringLiteral("uc:lightbulb"));

    QCOMPARE(notLoadedWarnings(QStringLiteral("light.not_loaded")), 1);
}

void testEntityController::setEntityName_knownEntity_doesNotWarn() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString entityId = QStringLiteral("light.loaded");
    controller.onEntityAdded(makeEntity(entityId));

    controller.setEntityName(entityId, QStringLiteral("New name"));
    controller.setEntityIcon(entityId, QStringLiteral("uc:lightbulb"));

    // a loaded entity takes the regular path: the request is built and handed to the core, which is not
    // connected in a test and reports that separately - nothing warns about a missing entity
    QCOMPARE(notLoadedWarnings(entityId), 0);
}

QString testEntityController::embeddedArtwork(const QColor& color) {
    QImage image(64, 64, QImage::Format_RGB32);
    image.fill(color);

    QByteArray png;
    QBuffer    buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
}

QVariantMap testEntityController::voiceStartParams(int sessionId) {
    QVariantMap params;
    params.insert(QStringLiteral("session_id"), sessionId);
    params.insert(QStringLiteral("speech_response"), true);
    params.insert(QStringLiteral("timeout"), 15);
    return params;
}

/**
 * A voice_start that failed around a wakeup stays pending for the resume window. Ending the session drops it,
 * and the overlay is told so at once: the dropped start never reaches the assistant, so nothing else would
 * answer the session before the overlay's own timeout.
 */
void testEntityController::voiceEnd_droppedPendingStart_reportsUnavailable() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 2);

    const QString entityId = QStringLiteral("uc.main:voice");
    QSignalSpy    errors(&controller, &uc::ui::EntityController::voiceAssistantCommandError);

    // the remote went to sleep: a command that fails from here on is one to send again after the wakeup
    controller.onPowerModeChanged(uc::core::PowerEnums::PowerMode::SUSPEND);

    // the core is not connected in a test, so the start fails right away and is scheduled to be sent again
    controller.onEntityCommand(entityId, QStringLiteral("voice_start"), voiceStartParams(1));
    QCOMPARE(errors.count(), 0);

    controller.onEntityCommand(entityId, QStringLiteral("voice_end"), QVariantMap());

    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.at(0).at(0).toString(), entityId);
    QCOMPARE(errors.at(0).at(1).toInt(), 503);

    // the dropped start is not sent again: waiting past the resend delay reports no second failure
    QTest::qWait(700);
    QCOMPARE(errors.count(), 1);
}

void testEntityController::voiceEnd_nothingPending_reportsNothing() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 2);
    // outside a resume window the failed voice_end goes to the "not responding" notification, which needs
    // the notification singleton
    uc::ui::Notification notification;

    QSignalSpy errors(&controller, &uc::ui::EntityController::voiceAssistantCommandError);

    // a normal session end: the start was acknowledged long ago, nothing is pending
    controller.onEntityCommand(QStringLiteral("uc.main:voice"), QStringLiteral("voice_end"), QVariantMap());

    QCOMPARE(errors.count(), 0);
}

/**
 * An unknown repeat value converts to -1, which is no repeat mode. Stored as the current mode it made repeat()
 * fall through its switch and send a variable that was never assigned.
 */
void testEntityController::mediaPlayerRepeat_invalidAttribute_keepsModeAndSendsValidCommand() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);
    // the core is not connected in a test: the failed command goes to the "not responding" notification, which
    // needs the notification singleton
    uc::ui::Notification notification;

    const QString    entityId = QStringLiteral("media_player.living_room");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Media_player");
    controller.onEntityAdded(entity);

    auto mediaPlayer = qobject_cast<uc::ui::entity::MediaPlayer*>(controller.get(entityId));
    QVERIFY(mediaPlayer);
    QCOMPARE(mediaPlayer->getRepeat(), static_cast<int>(uc::ui::entity::MediaPlayerRepeatMode::OFF));

    mediaPlayer->updateAttribute(QStringLiteral("Repeat"), QStringLiteral("SOMETIMES"));
    QCOMPARE(mediaPlayer->getRepeat(), static_cast<int>(uc::ui::entity::MediaPlayerRepeatMode::OFF));

    mediaPlayer->updateAttribute(QStringLiteral("Repeat"), QVariant());
    QCOMPARE(mediaPlayer->getRepeat(), static_cast<int>(uc::ui::entity::MediaPlayerRepeatMode::OFF));

    // the core API documents upper case values, an integration sending lower case is understood as well
    mediaPlayer->updateAttribute(QStringLiteral("Repeat"), QStringLiteral("all"));
    QCOMPARE(mediaPlayer->getRepeat(), static_cast<int>(uc::ui::entity::MediaPlayerRepeatMode::ALL));

    QSignalSpy commands(mediaPlayer, &uc::ui::entity::Base::command);
    mediaPlayer->repeat();

    QCOMPARE(commands.count(), 1);
    QCOMPARE(commands.at(0).at(2).toMap().value(QStringLiteral("repeat")).toString(), QStringLiteral("OFF"));
}

/**
 * Every entity is set to Unavailable while the core is disconnected, and its state is reported again after the
 * reconnect. An activity that was running all along must not be announced as started by someone else, which
 * opens its screen when the user has that option on.
 */
void testEntityController::activity_onAgainAfterReconnect_isNotStartedExternally() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString    entityId = QStringLiteral("activity.watch_tv");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Activity");
    entity.attributes = QVariantMap({{QStringLiteral("state"), QStringLiteral("ON")}});
    controller.onEntityAdded(entity);

    auto activity = qobject_cast<uc::ui::entity::Activity*>(controller.get(entityId));
    QVERIFY(activity);
    QCOMPARE(activity->getState(), static_cast<int>(uc::ui::entity::ActivityStates::On));

    QSignalSpy startedExternally(activity, &uc::ui::entity::Activity::startedExternally);

    // what EntityController::onCoreDisconnected() does to every entity, and the reload after the reconnect
    activity->setState(uc::ui::entity::ActivityStates::Unavailable);
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("ON"));
    QCOMPARE(activity->getState(), static_cast<int>(uc::ui::entity::ActivityStates::On));
    QCOMPARE(startedExternally.count(), 0);

    // an activity that was off and is started by another client is still reported
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("OFF"));
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("ON"));
    QCOMPARE(startedExternally.count(), 1);

    // also when it was started while the connection was down
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("OFF"));
    activity->setState(uc::ui::entity::ActivityStates::Unavailable);
    activity->updateAttribute(QStringLiteral("State"), QStringLiteral("ON"));
    QCOMPARE(startedExternally.count(), 2);
}

/**
 * The onboarding reads deviceNameChanged(true) as "the name was accepted" and moves to the next step. The
 * configuration is loaded again after every reconnect, which must not look like that.
 */
void testEntityController::config_reloadedDeviceName_isNotAnnouncedAgain() {
    uc::core::Api api(kTestUrl);
    uc::Config    config(&api);

    QSignalSpy nameChanged(&config, &uc::Config::deviceNameChanged);

    uc::core::cfgDevice device;
    device.name = QStringLiteral("Living room remote");

    emit api.cfgDeviceChanged(device);
    QCOMPARE(nameChanged.count(), 1);
    QCOMPARE(config.getDeviceName(), device.name);

    // the same configuration again
    emit api.cfgDeviceChanged(device);
    QCOMPARE(nameChanged.count(), 1);

    device.name = QStringLiteral("Bedroom remote");
    emit api.cfgDeviceChanged(device);
    QCOMPARE(nameChanged.count(), 2);
    QCOMPARE(config.getDeviceName(), device.name);
}

/**
 * The attributes of a climate entity are applied before its temperature unit is known. The temperature in the
 * state info, which the entity tile shows, was built with the Celsius label at that point and kept it.
 */
void testEntityController::climate_stateInfo_usesTheUnitOfTheEntity() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString    entityId = QStringLiteral("climate.living_room");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Climate");
    entity.options = QVariantMap({{QStringLiteral("temperature_unit"), QStringLiteral("FAHRENHEIT")}});
    entity.attributes = QVariantMap({{QStringLiteral("current_temperature"), 72}});
    controller.onEntityAdded(entity);

    auto climate = qobject_cast<uc::ui::entity::Climate*>(controller.get(entityId));
    QVERIFY(climate);
    QVERIFY2(climate->getStateInfo().contains(QStringLiteral("72°F")), qPrintable(climate->getStateInfo()));
}

void testEntityController::climate_stateInfo_followsTheUnitSystem() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    // no temperature_unit option: the entity follows the unit system of the remote
    const QString    entityId = QStringLiteral("climate.bedroom");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Climate");
    entity.attributes = QVariantMap({{QStringLiteral("current_temperature"), 21}});
    controller.onEntityAdded(entity);

    auto climate = qobject_cast<uc::ui::entity::Climate*>(controller.get(entityId));
    QVERIFY(climate);
    QVERIFY2(climate->getStateInfo().contains(QStringLiteral("21°C")), qPrintable(climate->getStateInfo()));

    QSignalSpy stateInfoChanged(climate, &uc::ui::entity::Base::stateInfoChanged);
    climate->onUnitSystemChanged(uc::Config::UnitSystems::Us);

    // the value is the one the integration reported, only the label follows the unit
    QVERIFY2(climate->getStateInfo().contains(QStringLiteral("21°F")), qPrintable(climate->getStateInfo()));
    QCOMPARE(stateInfoChanged.count(), 1);
}

/**
 * An integration that reconnects reports its lights as unavailable for a moment. A light that comes back on with
 * the brightness it had before must still show the percentage on its tile.
 */
void testEntityController::light_brightnessText_survivesUnavailable() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString    entityId = QStringLiteral("light.desk");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Light");
    controller.onEntityAdded(entity);

    auto light = qobject_cast<uc::ui::entity::Light*>(controller.get(entityId));
    QVERIFY(light);

    light->updateAttribute(QStringLiteral("State"), QStringLiteral("ON"));
    light->updateAttribute(QStringLiteral("Brightness"), 128);
    QVERIFY2(light->getStateInfo().contains(QStringLiteral("50%")), qPrintable(light->getStateInfo()));

    light->updateAttribute(QStringLiteral("State"), QStringLiteral("UNAVAILABLE"));
    QVERIFY2(!light->getStateInfo().contains(QStringLiteral("%")), qPrintable(light->getStateInfo()));

    light->updateAttribute(QStringLiteral("State"), QStringLiteral("ON"));
    light->updateAttribute(QStringLiteral("Brightness"), 128);
    QVERIFY2(light->getStateInfo().contains(QStringLiteral("50%")), qPrintable(light->getStateInfo()));
}

/**
 * The position of a playing media player is counted up once a second. Live content has no duration and the
 * position stands still, as it does at the end of the media: that is not a change to announce every second.
 */
void testEntityController::mediaPlayer_positionTimer_onlyAnnouncesAChange() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString    entityId = QStringLiteral("media_player.tv");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Media_player");
    controller.onEntityAdded(entity);

    auto mediaPlayer = qobject_cast<uc::ui::entity::MediaPlayer*>(controller.get(entityId));
    QVERIFY(mediaPlayer);

    QSignalSpy positionChanged(mediaPlayer, &uc::ui::entity::MediaPlayer::mediaPositionChanged);

    // no duration
    QVERIFY(QMetaObject::invokeMethod(mediaPlayer, "onPositionTimerTimeout"));
    QCOMPARE(positionChanged.count(), 0);
    QCOMPARE(mediaPlayer->getMediaPosition(), 0);

    mediaPlayer->updateAttribute(QStringLiteral("Media_duration"), 3);
    mediaPlayer->updateAttribute(QStringLiteral("Media_position"), 1);
    positionChanged.clear();

    QVERIFY(QMetaObject::invokeMethod(mediaPlayer, "onPositionTimerTimeout"));
    QCOMPARE(mediaPlayer->getMediaPosition(), 2);
    QVERIFY(QMetaObject::invokeMethod(mediaPlayer, "onPositionTimerTimeout"));
    QCOMPARE(mediaPlayer->getMediaPosition(), 3);
    QCOMPARE(positionChanged.count(), 2);

    // the end of the media
    QVERIFY(QMetaObject::invokeMethod(mediaPlayer, "onPositionTimerTimeout"));
    QCOMPARE(mediaPlayer->getMediaPosition(), 3);
    QCOMPARE(positionChanged.count(), 2);
}

/**
 * The artwork is decoded and its average colour computed on a thread of the global pool; the result is applied
 * on the GUI thread, where the media player lives.
 */
void testEntityController::mediaPlayer_embeddedArtwork_isDecodedOnAWorkerThread() {
    uc::core::Api              api(kTestUrl);
    uc::ui::EntityController   controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);
    uc::ui::MediaImageProvider provider;

    const QString    entityId = QStringLiteral("media_player.art");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Media_player");
    controller.onEntityAdded(entity);

    auto mediaPlayer = qobject_cast<uc::ui::entity::MediaPlayer*>(controller.get(entityId));
    QVERIFY(mediaPlayer);

    QSignalSpy imageChanged(mediaPlayer, &uc::ui::entity::MediaPlayer::mediaImageChanged);
    mediaPlayer->updateAttribute(QStringLiteral("Media_image_url"), embeddedArtwork(QColor(200, 30, 30)));

    QVERIFY(imageChanged.wait(5000));
    QVERIFY(mediaPlayer->getMediaImage().startsWith(QStringLiteral("image://media-art/")));
    // the artwork is red: so is its average colour
    QVERIFY(mediaPlayer->getMediaImageColor().red() > mediaPlayer->getMediaImageColor().blue());
}

/**
 * An entity can be removed while its artwork is still being decoded, e.g. by the reload after a reconnect. The
 * worker used to check and dereference a QPointer from its own thread, which races with the deletion on the GUI
 * thread; it now only reads a shared counter and the result is dropped on the GUI thread.
 */
void testEntityController::mediaPlayer_deletedWhileArtworkIsDecoded_doesNotCrash() {
    uc::core::Api              api(kTestUrl);
    uc::ui::EntityController   controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);
    uc::ui::MediaImageProvider provider;

    for (int round = 0; round < 20; round++) {
        const QString    entityId = QStringLiteral("media_player.art%1").arg(round);
        uc::core::Entity entity = makeEntity(entityId);
        entity.type = QStringLiteral("Media_player");
        controller.onEntityAdded(entity);

        auto mediaPlayer = qobject_cast<uc::ui::entity::MediaPlayer*>(controller.get(entityId));
        QVERIFY(mediaPlayer);

        mediaPlayer->updateAttribute(QStringLiteral("Media_image_url"), embeddedArtwork(QColor(30, 30, 200)));
        // the entity is removed right away: the controller deletes it 100 ms later, while the decoding is still
        // running or about to deliver its result
        controller.onEntityDeleted(entityId);
        QTest::qWait(round % 2 == 0 ? 110 : 10);
    }

    QThreadPool::globalInstance()->waitForDone(5000);
    // the queued results of the deleted players are delivered and dropped
    QTest::qWait(200);
}

/**
 * 0 is a temperature like any other and is shown. A device that measures the temperature but has none to report
 * (null) shows "--" instead, like the cover does for an unknown position.
 */
void testEntityController::climate_currentTemperature_zeroIsShownAndNullIsNotAvailable() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString    entityId = QStringLiteral("climate.freezer");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Climate");
    entity.features = QStringList({QStringLiteral("Current_temperature")});
    controller.onEntityAdded(entity);

    auto climate = qobject_cast<uc::ui::entity::Climate*>(controller.get(entityId));
    QVERIFY(climate);

    // nothing reported yet
    QVERIFY(!climate->isCurrentTemperatureAvailable());
    QVERIFY2(climate->getStateInfo().contains(QStringLiteral("--")), qPrintable(climate->getStateInfo()));

    climate->updateAttribute(QStringLiteral("Current_temperature"), 0);
    QVERIFY(climate->isCurrentTemperatureAvailable());
    QVERIFY2(climate->getStateInfo().contains(QStringLiteral("0°C")), qPrintable(climate->getStateInfo()));

    climate->updateAttribute(QStringLiteral("Current_temperature"), QVariant());
    QVERIFY(!climate->isCurrentTemperatureAvailable());
    QVERIFY2(climate->getStateInfo().contains(QStringLiteral("--")), qPrintable(climate->getStateInfo()));
    QVERIFY2(!climate->getStateInfo().contains(QStringLiteral("°C")), qPrintable(climate->getStateInfo()));

    climate->updateAttribute(QStringLiteral("Current_temperature"), -18.5);
    QVERIFY(climate->isCurrentTemperatureAvailable());
    QVERIFY2(climate->getStateInfo().contains(QStringLiteral("-18.5°C")), qPrintable(climate->getStateInfo()));

    // a device that does not measure the temperature shows no temperature part at all
    const QString    heaterId = QStringLiteral("climate.heater");
    uc::core::Entity heater = makeEntity(heaterId);
    heater.type = QStringLiteral("Climate");
    controller.onEntityAdded(heater);
    auto heaterObj = qobject_cast<uc::ui::entity::Climate*>(controller.get(heaterId));
    QVERIFY(heaterObj);
    QVERIFY2(!heaterObj->getStateInfo().contains(QStringLiteral("--")), qPrintable(heaterObj->getStateInfo()));
}

/**
 * A binary sensor reports on/off, the UI shows a text for its device class. The text is produced when the value
 * is read, so that it follows a language change and a device class that arrives after the value.
 */
void testEntityController::binarySensor_valueIsTranslatedWhenRead() {
    uc::core::Api            api(kTestUrl);
    uc::ui::EntityController controller(&api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0);

    const QString    entityId = QStringLiteral("sensor.front_door");
    uc::core::Entity entity = makeEntity(entityId);
    entity.type = QStringLiteral("Sensor");
    entity.deviceClass = QStringLiteral("Binary");
    controller.onEntityAdded(entity);

    auto sensor = qobject_cast<uc::ui::entity::Sensor*>(controller.get(entityId));
    QVERIFY(sensor);
    // no value yet: nothing, not "off"
    QCOMPARE(sensor->getValue(), QString());

    QSignalSpy valueChanged(sensor, &uc::ui::entity::Sensor::valueChanged);

    sensor->updateAttribute(QStringLiteral("Value"), QStringLiteral("on"));
    QCOMPARE(sensor->getValue(), QStringLiteral("On"));

    // the device class arrives after the value: the text follows it
    sensor->updateAttribute(QStringLiteral("Unit"), QStringLiteral("door"));
    QCOMPARE(sensor->getValue(), QStringLiteral("Opened"));
    QCOMPARE(sensor->getStateInfo(), QStringLiteral("Opened"));
    QCOMPARE(valueChanged.count(), 2);

    sensor->updateAttribute(QStringLiteral("Value"), QStringLiteral("off"));
    QCOMPARE(sensor->getValue(), QStringLiteral("Closed"));
}

QTEST_GUILESS_MAIN(testEntityController)

#include "test_entity_controller.moc"
