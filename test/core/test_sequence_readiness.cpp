// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonDocument>
#include <QtTest>

#include "core/core.h"
#include "ui/entity/sequenceReadinessReport.h"

using uc::ui::entity::SequenceReadinessReport;

// The worked example of the SequenceReadiness schema in the core API specification: an activity-group switch with a
// blocked transition step the policy continues past, a skipped step whose entity was deleted, and a power-on step
// omitted because the device is already on.
static const QByteArray kFullReport = R"({
  "entity_id": "uc.main.watch_tv",
  "entity_type": "activity",
  "name": "Fernsehen",
  "cmd_id": "activity.on",
  "lang": "de",
  "error_policy": "continue_transition",
  "timestamp": "2026-08-31T09:14:22.481Z",
  "ready": true,
  "total_steps": 4,
  "transition_steps": 1,
  "blocked_steps": 1,
  "skipped_steps": 1,
  "aborting_steps": 0,
  "omitted_steps": 1,
  "activity_group": {
    "group_id": "1ef02dc9-9054-45f8-a0ea-ae519cc88f0f",
    "name": "Wohnzimmer",
    "turn_off_unused_entities": "always",
    "members": [
      { "entity_id": "uc.main.listen_music", "name": "Musik hören", "state": "ON" },
      { "entity_id": "uc.main.watch_apple_tv", "name": "Apple TV schauen", "state": "STOPPED" }
    ]
  },
  "depends_on": {
    "integration_ids": ["denon"],
    "emitter_ids": ["uc.dock.living_room"],
    "dock_ids": ["uc.dock.living_room"],
    "bt": false
  },
  "switch_from": {
    "entity_id": "uc.main.listen_music",
    "name": "Musik hören",
    "state": "ON"
  },
  "steps": [
    {
      "index": 1,
      "switch_from_index": 2,
      "part": "transition",
      "type": "command",
      "entity_id": "denon.media_player.avr",
      "entity_type": "media_player",
      "name": "Denon AVR",
      "cmd_id": "media_player.off",
      "ready": false,
      "aborts_run": false,
      "reason": {
        "code": "INTEGRATION_NOT_CONNECTED",
        "message": "integration 'Denon AVR' (denon) is DISCONNECTED",
        "group_id": "INTEGRATION_NOT_CONNECTED:denon",
        "integration_id": "denon",
        "integration_name": "Denon AVR",
        "state": "DISCONNECTED"
      }
    },
    {
      "index": 2,
      "authored_index": 2,
      "part": "sequence",
      "type": "command",
      "entity_id": "uc.main.remote.tv",
      "entity_type": "remote",
      "name": "Fernseher",
      "cmd_id": "remote.send",
      "params": { "command": "POWER_ON" },
      "ready": true
    },
    {
      "index": 3,
      "authored_index": 3,
      "part": "sequence",
      "type": "delay",
      "delay": 2000,
      "ready": true
    },
    {
      "index": 4,
      "authored_index": 4,
      "nested_index": 1,
      "part": "sequence",
      "type": "command",
      "entity_id": "uc.main.switch.old_lamp",
      "name": "Alte Lampe",
      "cmd_id": "switch.on",
      "parents": [ { "entity_id": "uc.main.macro.lights", "name": "Licht" } ],
      "ready": true,
      "skipped": true,
      "reason": {
        "code": "ENTITY_NOT_FOUND",
        "message": "entity 'Alte Lampe' (uc.main.switch.old_lamp) no longer exists: the step is skipped",
        "group_id": "ENTITY_NOT_FOUND:uc.main.switch.old_lamp"
      }
    }
  ],
  "omitted": [
    {
      "authored_index": 1,
      "type": "command",
      "entity_id": "uc.main.switch.floor_lamp",
      "entity_type": "switch",
      "name": "Stehlampe",
      "cmd_id": "switch.on",
      "reason": {
        "code": "ALREADY_IN_STATE",
        "message": "'Stehlampe' (uc.main.switch.floor_lamp) is already on: the step is not run",
        "group_id": "ALREADY_IN_STATE:uc.main.switch.floor_lamp"
      }
    }
  ]
})";

// A ready activity with none of the optional members: no group switch, no reasons, no omitted steps.
static const QByteArray kMinimalReport = R"({
  "entity_id": "uc.main.watch_tv",
  "entity_type": "activity",
  "name": "Watch TV",
  "cmd_id": "activity.on",
  "lang": "en_US",
  "error_policy": "continue_transition",
  "timestamp": "2026-09-01T16:01:49.456240255Z",
  "ready": true,
  "total_steps": 1,
  "transition_steps": 0,
  "blocked_steps": 0,
  "skipped_steps": 0,
  "aborting_steps": 0,
  "omitted_steps": 0,
  "depends_on": { "integration_ids": [], "emitter_ids": [], "dock_ids": [], "bt": false },
  "steps": [
    { "index": 1, "authored_index": 1, "part": "sequence", "type": "command",
      "entity_id": "uc.main.remote.tv", "name": "TV", "cmd_id": "remote.send", "ready": true }
  ],
  "omitted": []
})";

static QVariantMap toMap(const QByteArray& json) {
    return QJsonDocument::fromJson(json).toVariant().toMap();
}

static uc::core::ReadinessStep makeStep(int index, bool ready, bool skipped, bool abortsRun, const QString& name,
                                        const QString& groupId, const QString& code = QStringLiteral("CODE")) {
    uc::core::ReadinessStep step;
    step.index = index;
    step.part = QStringLiteral("sequence");
    step.type = QStringLiteral("command");
    step.name = name;
    step.ready = ready;
    step.skipped = skipped;
    step.abortsRun = abortsRun;

    if (!groupId.isEmpty()) {
        step.hasReason = true;
        step.reason.code = code;
        step.reason.groupId = groupId;
    }

    return step;
}

/**
 * The readiness report of the core is deep and mostly optional, and the interpretation on top of it decides what a
 * user gets told before an activity runs. Both are verified here without a websocket connection: the parsing
 * against the payload of the API specification, the interpretation against reports built by hand.
 */
class testSequenceReadiness : public QObject {
    Q_OBJECT

 private slots:
    void parse_fullExample();
    void parse_minimalOptionalFieldsAbsent();

    void verdict_ready();
    void verdict_runsWithWarnings();
    void verdict_skippedStepAloneStillWarns();
    void verdict_notReady();

    void causes_bucketStepsOfTheSameCause();
    void causes_worstSeverityWins();
    void causes_omittedStepJoinsWithoutRaisingSeverity();

    void unresponsiveNames_distinctInExecutionOrder();

    void toSummary_reportsVerdictAndCauses();
    void toSummary_omitsOmittedOnlyCauses();

    void plan_weavesOmittedStepAtAuthoredPosition();
    void plan_reachedFlipsAfterAbortingStep();
    void plan_appendsLeftoverOmittedSteps();
    void plan_stopLabelIsFirstAbortingStep();
    void plan_carriesReasonDetail();
};

void testSequenceReadiness::parse_fullExample() {
    uc::core::SequenceReadiness report = uc::core::Api::parseSequenceReadiness(toMap(kFullReport));

    QCOMPARE(report.entityId, QStringLiteral("uc.main.watch_tv"));
    QCOMPARE(report.entityType, QStringLiteral("activity"));
    QCOMPARE(report.name, QStringLiteral("Fernsehen"));
    QCOMPARE(report.cmdId, QStringLiteral("activity.on"));
    QCOMPARE(report.lang, QStringLiteral("de"));
    QCOMPARE(report.errorPolicy, QStringLiteral("continue_transition"));
    QVERIFY(report.timestamp.isValid());
    QCOMPARE(report.timestamp.toUTC().toString(Qt::ISODateWithMs), QStringLiteral("2026-08-31T09:14:22.481Z"));

    QCOMPARE(report.ready, true);
    QCOMPARE(report.totalSteps, 4);
    QCOMPARE(report.transitionSteps, 1);
    QCOMPARE(report.blockedSteps, 1);
    QCOMPARE(report.skippedSteps, 1);
    QCOMPARE(report.abortingSteps, 0);
    QCOMPARE(report.omittedSteps, 1);

    QCOMPARE(report.dependsOn.integrationIds, QStringList({"denon"}));
    QCOMPARE(report.dependsOn.emitterIds, QStringList({"uc.dock.living_room"}));
    QCOMPARE(report.dependsOn.dockIds, QStringList({"uc.dock.living_room"}));
    QCOMPARE(report.dependsOn.bt, false);

    QCOMPARE(report.hasActivityGroup, true);
    QCOMPARE(report.activityGroup.groupId, QStringLiteral("1ef02dc9-9054-45f8-a0ea-ae519cc88f0f"));
    QCOMPARE(report.activityGroup.name, QStringLiteral("Wohnzimmer"));
    QCOMPARE(report.activityGroup.turnOffUnusedEntities, QStringLiteral("always"));
    QCOMPARE(report.activityGroup.members.count(), 2);
    QCOMPARE(report.activityGroup.members.at(0).entityId, QStringLiteral("uc.main.listen_music"));
    QCOMPARE(report.activityGroup.members.at(0).state, QStringLiteral("ON"));
    QCOMPARE(report.activityGroup.members.at(1).state, QStringLiteral("STOPPED"));

    QCOMPARE(report.hasSwitchFrom, true);
    QCOMPARE(report.switchFrom.entityId, QStringLiteral("uc.main.listen_music"));
    QCOMPARE(report.switchFrom.state, QStringLiteral("ON"));

    QCOMPARE(report.steps.count(), 4);

    // the transition step of the activity switched away from: it has no row in the checked sequence
    const uc::core::ReadinessStep& transition = report.steps.at(0);
    QCOMPARE(transition.index, 1);
    QCOMPARE(transition.part, QStringLiteral("transition"));
    QCOMPARE(transition.switchFromIndex, 2);
    QCOMPARE(transition.authoredIndex, -1);
    QCOMPARE(transition.nestedIndex, -1);
    QCOMPARE(transition.name, QStringLiteral("Denon AVR"));
    QCOMPARE(transition.cmdId, QStringLiteral("media_player.off"));
    QCOMPARE(transition.ready, false);
    QCOMPARE(transition.abortsRun, false);
    QCOMPARE(transition.skipped, false);
    QCOMPARE(transition.hasReason, true);
    QCOMPARE(transition.reason.code, QStringLiteral("INTEGRATION_NOT_CONNECTED"));
    QCOMPARE(transition.reason.groupId, QStringLiteral("INTEGRATION_NOT_CONNECTED:denon"));
    QCOMPARE(transition.reason.integrationId, QStringLiteral("denon"));
    QCOMPARE(transition.reason.integrationName, QStringLiteral("Denon AVR"));
    QCOMPARE(transition.reason.state, QStringLiteral("DISCONNECTED"));

    const uc::core::ReadinessStep& command = report.steps.at(1);
    QCOMPARE(command.authoredIndex, 2);
    QCOMPARE(command.part, QStringLiteral("sequence"));
    QCOMPARE(command.cmdId, QStringLiteral("remote.send"));
    QCOMPARE(command.params.value("command").toString(), QStringLiteral("POWER_ON"));
    QCOMPARE(command.ready, true);
    QCOMPARE(command.hasReason, false);

    const uc::core::ReadinessStep& delay = report.steps.at(2);
    QCOMPARE(delay.type, QStringLiteral("delay"));
    QCOMPARE(delay.delay, 2000);
    QCOMPARE(delay.entityId, QString());
    QCOMPARE(delay.hasReason, false);

    // a step of a macro nested in the activity, whose entity was deleted: the run skips it, it never blocks
    const uc::core::ReadinessStep& nested = report.steps.at(3);
    QCOMPARE(nested.authoredIndex, 4);
    QCOMPARE(nested.nestedIndex, 1);
    QCOMPARE(nested.parents.count(), 1);
    QCOMPARE(nested.parents.at(0).entityId, QStringLiteral("uc.main.macro.lights"));
    QCOMPARE(nested.parents.at(0).name, QStringLiteral("Licht"));
    QCOMPARE(nested.ready, true);
    QCOMPARE(nested.skipped, true);
    QCOMPARE(nested.reason.code, QStringLiteral("ENTITY_NOT_FOUND"));

    QCOMPARE(report.omitted.count(), 1);
    QCOMPARE(report.omitted.at(0).authoredIndex, 1);
    QCOMPARE(report.omitted.at(0).name, QStringLiteral("Stehlampe"));
    QCOMPARE(report.omitted.at(0).cmdId, QStringLiteral("switch.on"));
    QCOMPARE(report.omitted.at(0).reason.code, QStringLiteral("ALREADY_IN_STATE"));
}

void testSequenceReadiness::parse_minimalOptionalFieldsAbsent() {
    uc::core::SequenceReadiness report = uc::core::Api::parseSequenceReadiness(toMap(kMinimalReport));

    // the core stamps its reports with nanoseconds, more digits than the specification's own example shows:
    // the parser has to keep them out of the milliseconds
    QVERIFY(report.timestamp.isValid());
    QCOMPARE(report.timestamp.toUTC().toString(Qt::ISODateWithMs), QStringLiteral("2026-09-01T16:01:49.456Z"));

    QCOMPARE(report.hasActivityGroup, false);
    QCOMPARE(report.hasSwitchFrom, false);
    QCOMPARE(report.activityGroup.members.count(), 0);
    QCOMPARE(report.omitted.count(), 0);
    QCOMPARE(report.dependsOn.integrationIds.count(), 0);
    QCOMPARE(report.dependsOn.bt, false);

    QCOMPARE(report.steps.count(), 1);
    const uc::core::ReadinessStep& step = report.steps.at(0);
    QCOMPARE(step.hasReason, false);
    QCOMPARE(step.switchFromIndex, -1);
    QCOMPARE(step.nestedIndex, -1);
    QCOMPARE(step.skipped, false);
    QCOMPARE(step.abortsRun, false);
    QCOMPARE(step.delay, 0);
    QCOMPARE(step.params.count(), 0);
    QCOMPARE(step.parents.count(), 0);

    QCOMPARE(SequenceReadinessReport::verdict(report), SequenceReadinessReport::Verdict::Ready);
}

void testSequenceReadiness::verdict_ready() {
    uc::core::SequenceReadiness report;
    report.ready = true;

    QCOMPARE(SequenceReadinessReport::verdict(report), SequenceReadinessReport::Verdict::Ready);
}

void testSequenceReadiness::verdict_runsWithWarnings() {
    // the example: the run reaches its final state, but one device on the way is not reached
    uc::core::SequenceReadiness report = uc::core::Api::parseSequenceReadiness(toMap(kFullReport));

    QCOMPARE(SequenceReadinessReport::verdict(report), SequenceReadinessReport::Verdict::RunsWithWarnings);
}

void testSequenceReadiness::verdict_skippedStepAloneStillWarns() {
    // a deleted entity does not block the run, but the user should still hear that a device is missing
    uc::core::SequenceReadiness report;
    report.ready = true;
    report.skippedSteps = 1;

    QCOMPARE(SequenceReadinessReport::verdict(report), SequenceReadinessReport::Verdict::RunsWithWarnings);
}

void testSequenceReadiness::verdict_notReady() {
    uc::core::SequenceReadiness report;
    report.ready = false;
    report.blockedSteps = 1;
    report.abortingSteps = 1;

    QCOMPARE(SequenceReadinessReport::verdict(report), SequenceReadinessReport::Verdict::NotReady);
}

void testSequenceReadiness::causes_bucketStepsOfTheSameCause() {
    // one disconnected integration regularly blocks several steps of the same activity: the user is told once
    uc::core::SequenceReadiness report;
    report.steps.append(makeStep(1, false, false, false, "TV", "INTEGRATION_NOT_CONNECTED:denon"));
    report.steps.append(makeStep(2, false, false, false, "AVR", "INTEGRATION_NOT_CONNECTED:denon"));
    report.steps.append(makeStep(3, true, true, false, "Lamp", "ENTITY_NOT_FOUND:uc.main.switch.lamp"));

    QList<SequenceReadinessReport::Cause> causes = SequenceReadinessReport::causes(report);

    QCOMPARE(causes.count(), 2);
    QCOMPARE(causes.at(0).groupId, QStringLiteral("INTEGRATION_NOT_CONNECTED:denon"));
    QCOMPARE(causes.at(0).severity, SequenceReadinessReport::Severity::Blocked);
    QCOMPARE(causes.at(0).steps.count(), 2);
    QCOMPARE(causes.at(0).entityName, QStringLiteral("TV"));
    QCOMPARE(causes.at(1).severity, SequenceReadinessReport::Severity::Skipped);
    QCOMPARE(causes.at(1).steps.count(), 1);
}

void testSequenceReadiness::causes_worstSeverityWins() {
    // the same cause blocks two steps and aborts the run at the second one: it is reported as aborting, and
    // before the merely blocked one
    uc::core::SequenceReadiness report;
    report.steps.append(makeStep(1, false, false, false, "TV", "BLOCKS:other"));
    report.steps.append(makeStep(2, false, false, false, "AVR", "ABORTS:denon"));
    report.steps.append(makeStep(3, false, false, true, "AVR 2", "ABORTS:denon"));

    QList<SequenceReadinessReport::Cause> causes = SequenceReadinessReport::causes(report);

    QCOMPARE(causes.count(), 2);
    QCOMPARE(causes.at(0).groupId, QStringLiteral("ABORTS:denon"));
    QCOMPARE(causes.at(0).severity, SequenceReadinessReport::Severity::Aborting);
    QCOMPARE(causes.at(1).groupId, QStringLiteral("BLOCKS:other"));
    QCOMPARE(causes.at(1).severity, SequenceReadinessReport::Severity::Blocked);
}

void testSequenceReadiness::causes_omittedStepJoinsWithoutRaisingSeverity() {
    // the turn-on delay dropped with a power-on step carries that step's group id
    uc::core::SequenceReadiness report;

    uc::core::ReadinessOmittedStep omitted;
    omitted.authoredIndex = 1;
    omitted.type = QStringLiteral("command");
    omitted.name = QStringLiteral("Floor lamp");
    omitted.reason.code = QStringLiteral("ALREADY_IN_STATE");
    omitted.reason.groupId = QStringLiteral("ALREADY_IN_STATE:uc.main.switch.floor_lamp");

    uc::core::ReadinessOmittedStep omittedDelay;
    omittedDelay.authoredIndex = 2;
    omittedDelay.type = QStringLiteral("delay");
    omittedDelay.delay = 1000;
    omittedDelay.reason.code = QStringLiteral("ALREADY_IN_STATE");
    omittedDelay.reason.groupId = QStringLiteral("ALREADY_IN_STATE:uc.main.switch.floor_lamp");

    report.omitted.append(omitted);
    report.omitted.append(omittedDelay);

    QList<SequenceReadinessReport::Cause> causes = SequenceReadinessReport::causes(report);

    QCOMPARE(causes.count(), 1);
    QCOMPARE(causes.at(0).severity, SequenceReadinessReport::Severity::Omitted);
    QCOMPARE(causes.at(0).omitted.count(), 2);
    QCOMPARE(causes.at(0).steps.count(), 0);
}

void testSequenceReadiness::unresponsiveNames_distinctInExecutionOrder() {
    uc::core::SequenceReadiness report;
    report.steps.append(makeStep(1, false, false, false, "AVR", "A"));
    report.steps.append(makeStep(2, true, false, false, "TV", QString()));  // reached, not reported
    report.steps.append(makeStep(3, false, false, false, "AVR", "A"));      // same device again
    report.steps.append(makeStep(4, true, true, false, "Lamp", "B"));       // skipped: also unreached
    report.steps.append(makeStep(5, false, false, false, QString(), "C"));  // a delay step has no name

    QCOMPARE(SequenceReadinessReport::unresponsiveDeviceNames(report), QStringList({"AVR", "Lamp"}));
}

void testSequenceReadiness::toSummary_reportsVerdictAndCauses() {
    uc::core::SequenceReadiness report = uc::core::Api::parseSequenceReadiness(toMap(kFullReport));
    QVariantMap                 summary = SequenceReadinessReport::toSummary(report);

    QCOMPARE(summary.value("supported").toBool(), true);
    QCOMPARE(summary.value("ready").toBool(), true);
    QCOMPARE(summary.value("verdict").toString(), QStringLiteral("runs_with_warnings"));
    QCOMPARE(summary.value("name").toString(), QStringLiteral("Fernsehen"));
    QCOMPARE(summary.value("entityId").toString(), QStringLiteral("uc.main.watch_tv"));
    QCOMPARE(summary.value("cmdId").toString(), QStringLiteral("activity.on"));
    QCOMPARE(summary.value("totalSteps").toInt(), 4);
    QCOMPARE(summary.value("blockedSteps").toInt(), 1);
    QCOMPARE(summary.value("skippedSteps").toInt(), 1);
    QCOMPARE(summary.value("abortingSteps").toInt(), 0);
    QCOMPARE(summary.value("omittedSteps").toInt(), 1);
    QCOMPARE(summary.value("unresponsiveNames").toStringList(), QStringList({"Denon AVR", "Alte Lampe"}));

    // the blocked integration first, the skipped entity after it, and the omitted step not at all
    QVariantList causes = summary.value("causes").toList();
    QCOMPARE(causes.count(), 2);

    QVariantMap blocked = causes.at(0).toMap();
    QCOMPARE(blocked.value("code").toString(), QStringLiteral("INTEGRATION_NOT_CONNECTED"));
    QCOMPARE(blocked.value("severity").toString(), QStringLiteral("blocked"));
    QCOMPARE(blocked.value("entityName").toString(), QStringLiteral("Denon AVR"));
    QCOMPARE(blocked.value("integrationName").toString(), QStringLiteral("Denon AVR"));
    QCOMPARE(blocked.value("state").toString(), QStringLiteral("DISCONNECTED"));
    QCOMPARE(blocked.value("count").toInt(), 1);

    QVariantMap skipped = causes.at(1).toMap();
    QCOMPARE(skipped.value("code").toString(), QStringLiteral("ENTITY_NOT_FOUND"));
    QCOMPARE(skipped.value("severity").toString(), QStringLiteral("skipped"));
    QCOMPARE(skipped.value("entityName").toString(), QStringLiteral("Alte Lampe"));
}

void testSequenceReadiness::toSummary_omitsOmittedOnlyCauses() {
    // an activity whose devices are all on already: nothing to run, and nothing to tell the user about
    uc::core::SequenceReadiness report;
    report.ready = true;
    report.omittedSteps = 1;

    uc::core::ReadinessOmittedStep omitted;
    omitted.authoredIndex = 1;
    omitted.name = QStringLiteral("Floor lamp");
    omitted.reason.code = QStringLiteral("ALREADY_IN_STATE");
    omitted.reason.groupId = QStringLiteral("ALREADY_IN_STATE:uc.main.switch.floor_lamp");
    report.omitted.append(omitted);

    QVariantMap summary = SequenceReadinessReport::toSummary(report);

    QCOMPARE(summary.value("verdict").toString(), QStringLiteral("ready"));
    QCOMPARE(summary.value("causes").toList().count(), 0);
}

static uc::core::ReadinessOmittedStep makeOmitted(int authoredIndex, const QString& name) {
    uc::core::ReadinessOmittedStep step;
    step.authoredIndex = authoredIndex;
    step.type = QStringLiteral("command");
    step.name = name;
    step.reason.code = QStringLiteral("ALREADY_IN_STATE");
    step.reason.groupId = QStringLiteral("ALREADY_IN_STATE:") + name;
    return step;
}

void testSequenceReadiness::plan_weavesOmittedStepAtAuthoredPosition() {
    // the example: a transition step first, then the stored sequence with the omitted first step back in its
    // place, the delay, and the step nested in a macro with its two-part label
    uc::core::SequenceReadiness report = uc::core::Api::parseSequenceReadiness(toMap(kFullReport));
    QVariantMap                 summary = SequenceReadinessReport::toSummary(report);
    QVariantList                plan = summary.value("plan").toList();

    QCOMPARE(plan.count(), 5);
    QCOMPARE(summary.value("stepCount").toInt(), 5);
    QCOMPARE(summary.value("stopLabel").toString(), QString());

    QVariantMap transition = plan.at(0).toMap();
    QCOMPARE(transition.value("label").toString(), QString());
    QCOMPARE(transition.value("name").toString(), QStringLiteral("Denon AVR"));
    QCOMPARE(transition.value("marker").toString(), QStringLiteral("blocked"));

    QVariantMap omitted = plan.at(1).toMap();
    QCOMPARE(omitted.value("label").toString(), QStringLiteral("1"));
    QCOMPARE(omitted.value("name").toString(), QStringLiteral("Stehlampe"));
    QCOMPARE(omitted.value("marker").toString(), QStringLiteral("notNeeded"));
    QCOMPARE(omitted.value("code").toString(), QStringLiteral("ALREADY_IN_STATE"));

    QVariantMap command = plan.at(2).toMap();
    QCOMPARE(command.value("label").toString(), QStringLiteral("2"));
    QCOMPARE(command.value("type").toString(), QStringLiteral("command"));
    QCOMPARE(command.value("cmdId").toString(), QStringLiteral("remote.send"));
    QCOMPARE(command.value("marker").toString(), QStringLiteral("ok"));
    QCOMPARE(command.value("code").toString(), QString());

    QVariantMap delay = plan.at(3).toMap();
    QCOMPARE(delay.value("label").toString(), QStringLiteral("3"));
    QCOMPARE(delay.value("type").toString(), QStringLiteral("delay"));
    QCOMPARE(delay.value("delay").toInt(), 2000);
    QCOMPARE(delay.value("name").toString(), QString());

    QVariantMap nested = plan.at(4).toMap();
    QCOMPARE(nested.value("label").toString(), QStringLiteral("4.1"));
    QCOMPARE(nested.value("marker").toString(), QStringLiteral("skipped"));

    // nothing aborts: the whole run is reached
    for (const QVariant& row : plan) {
        QCOMPARE(row.toMap().value("reached").toBool(), true);
    }
}

void testSequenceReadiness::plan_reachedFlipsAfterAbortingStep() {
    uc::core::SequenceReadiness report;
    report.steps.append(makeStep(1, true, false, false, "TV", QString()));
    report.steps.append(makeStep(2, false, false, true, "AVR", "INTEGRATION_NOT_CONNECTED:denon"));
    report.steps.append(makeStep(3, true, false, false, "Lamp", QString()));

    for (int i = 0; i < report.steps.count(); i++) {
        report.steps[i].authoredIndex = i + 1;
    }

    QString      stopLabel;
    QVariantList plan = SequenceReadinessReport::plan(report, &stopLabel);

    QCOMPARE(stopLabel, QStringLiteral("2"));
    QCOMPARE(plan.count(), 3);
    QCOMPARE(plan.at(0).toMap().value("marker").toString(), QStringLiteral("ok"));
    QCOMPARE(plan.at(1).toMap().value("marker").toString(), QStringLiteral("aborting"));
    QCOMPARE(plan.at(2).toMap().value("marker").toString(), QStringLiteral("ok"));
    // the aborting step itself is reached, the run stops there
    QCOMPARE(plan.at(0).toMap().value("reached").toBool(), true);
    QCOMPARE(plan.at(1).toMap().value("reached").toBool(), true);
    QCOMPARE(plan.at(2).toMap().value("reached").toBool(), false);
    QCOMPARE(plan.at(1).toMap().value("stopsRun").toBool(), true);
}

void testSequenceReadiness::plan_appendsLeftoverOmittedSteps() {
    // an omitted step authored after the last executed one goes to the end, and one authored after the aborting
    // step is as unreached as the executed steps behind it
    uc::core::SequenceReadiness report;
    report.steps.append(makeStep(1, true, false, false, "TV", QString()));
    report.steps.append(makeStep(2, false, false, true, "AVR", "A"));
    report.steps[0].authoredIndex = 1;
    report.steps[1].authoredIndex = 3;
    // given out of order on purpose: the plan sorts by authored position
    report.omitted.append(makeOmitted(5, "Ceiling light"));
    report.omitted.append(makeOmitted(2, "Floor lamp"));

    QVariantList plan = SequenceReadinessReport::plan(report);

    QCOMPARE(plan.count(), 4);
    QCOMPARE(plan.at(0).toMap().value("label").toString(), QStringLiteral("1"));
    QCOMPARE(plan.at(1).toMap().value("name").toString(), QStringLiteral("Floor lamp"));
    QCOMPARE(plan.at(1).toMap().value("reached").toBool(), true);
    QCOMPARE(plan.at(2).toMap().value("label").toString(), QStringLiteral("3"));
    QCOMPARE(plan.at(3).toMap().value("name").toString(), QStringLiteral("Ceiling light"));
    QCOMPARE(plan.at(3).toMap().value("marker").toString(), QStringLiteral("notNeeded"));
    QCOMPARE(plan.at(3).toMap().value("reached").toBool(), false);
}

void testSequenceReadiness::plan_stopLabelIsFirstAbortingStep() {
    uc::core::SequenceReadiness report;
    report.steps.append(makeStep(1, false, false, true, "AVR", "A"));
    report.steps.append(makeStep(2, false, false, true, "TV", "B"));
    report.steps[0].authoredIndex = 2;
    report.steps[0].nestedIndex = 1;
    report.steps[1].authoredIndex = 3;

    QString      stopLabel;
    QVariantList plan = SequenceReadinessReport::plan(report, &stopLabel);

    QCOMPARE(stopLabel, QStringLiteral("2.1"));
    // a run stops once: the second aborting step is behind the stop and never reached
    QCOMPARE(plan.at(0).toMap().value("stopsRun").toBool(), true);
    QCOMPARE(plan.at(0).toMap().value("reached").toBool(), true);
    QCOMPARE(plan.at(1).toMap().value("stopsRun").toBool(), false);
    QCOMPARE(plan.at(1).toMap().value("reached").toBool(), false);
}

void testSequenceReadiness::plan_carriesReasonDetail() {
    // the row composes its status line from the code and the names in the reason
    uc::core::SequenceReadiness report = uc::core::Api::parseSequenceReadiness(toMap(kFullReport));
    QVariantMap                 transition = SequenceReadinessReport::plan(report).at(0).toMap();

    QCOMPARE(transition.value("code").toString(), QStringLiteral("INTEGRATION_NOT_CONNECTED"));
    QCOMPARE(transition.value("integrationName").toString(), QStringLiteral("Denon AVR"));
    QCOMPARE(transition.value("state").toString(), QStringLiteral("DISCONNECTED"));
    QCOMPARE(transition.value("dockName").toString(), QString());
    QCOMPARE(transition.value("emitterName").toString(), QString());
}

QTEST_GUILESS_MAIN(testSequenceReadiness)

#include "test_sequence_readiness.moc"
