// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "../../core/structs.h"

namespace uc {
namespace ui {
namespace entity {

/**
 * @brief Interpretation of a core sequence readiness report.
 *
 * The core answers a readiness check with the steps the run would execute and, per step, whether its target can
 * receive its command. Turning that into something to show a user is the same everywhere: a single verdict, and one
 * entry per underlying cause rather than one per blocked step - several steps of the same activity are regularly
 * blocked by one disconnected integration.
 *
 * The report's friendly names are already in the user's language, its `message` is an English diagnostic and is
 * never shown: the sentence a user reads is composed from the reason code and the detail fields.
 */
class SequenceReadinessReport {
 public:
    enum class Verdict { Ready, RunsWithWarnings, NotReady };

    /// Worst first: a cause is reported with the severity of its worst affected step.
    enum class Severity { Aborting = 0, Blocked = 1, Skipped = 2, Omitted = 3 };

    struct Cause {
        QString                           groupId;
        Severity                          severity = Severity::Omitted;
        core::ReadinessReason             reason;      // of the first affected step: every member shares the cause
        QString                           entityName;  // target of the first affected step, may be empty
        QList<core::ReadinessStep>        steps;       // execution order
        QList<core::ReadinessOmittedStep> omitted;     // authored order
    };

    /// NotReady if the run would abort, RunsWithWarnings if it runs but a device is not reached, else Ready.
    static Verdict verdict(const core::SequenceReadiness& report);

    /// One entry per cause, keyed by the report's opaque reason group id, worst severity first.
    static QList<Cause> causes(const core::SequenceReadiness& report);

    /// Names of the devices a run would not reach, without duplicates, in execution order.
    static QStringList unresponsiveDeviceNames(const core::SequenceReadiness& report);

    /**
     * The predicted run as rows for a list, in execution order, with the omitted steps woven back in at their
     * authored positions.
     *
     * Row keys: label (authored position, "4.1" for a nested step, empty for a transition step), name, cmdId,
     * type ("command" | "delay"), delay (ms), marker ("ok" | "aborting" | "blocked" | "skipped" | "notNeeded"),
     * reached (false for every row after the aborting one), stopsRun (true for the first aborting step only: the
     * core flags every not-ready step behind it as aborting as well, but a run only stops once), code and the
     * reason detail fields integrationName, dockName, emitterName, state.
     *
     * @param stopLabel receives the label of the first aborting step, empty when the run does not abort
     */
    static QVariantList plan(const core::SequenceReadiness& report, QString* stopLabel = nullptr);

    /// Flattened report for QML: verdict, counts, causes, plus plan(), stepCount and stopLabel.
    static QVariantMap toSummary(const core::SequenceReadiness& report);

 private:
    SequenceReadinessReport() {}
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
