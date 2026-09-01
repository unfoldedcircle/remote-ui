// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sequenceReadinessReport.h"

#include <QHash>
#include <algorithm>

namespace uc {
namespace ui {
namespace entity {

static QString severityToString(SequenceReadinessReport::Severity severity) {
    switch (severity) {
        case SequenceReadinessReport::Severity::Aborting:
            return QStringLiteral("aborting");
        case SequenceReadinessReport::Severity::Blocked:
            return QStringLiteral("blocked");
        case SequenceReadinessReport::Severity::Skipped:
            return QStringLiteral("skipped");
        default:
            return QStringLiteral("omitted");
    }
}

static SequenceReadinessReport::Severity stepSeverity(const core::ReadinessStep& step) {
    if (step.abortsRun) {
        return SequenceReadinessReport::Severity::Aborting;
    }
    if (!step.ready) {
        return SequenceReadinessReport::Severity::Blocked;
    }
    if (step.skipped) {
        return SequenceReadinessReport::Severity::Skipped;
    }
    return SequenceReadinessReport::Severity::Omitted;
}

SequenceReadinessReport::Verdict SequenceReadinessReport::verdict(const core::SequenceReadiness& report) {
    if (!report.ready) {
        return Verdict::NotReady;
    }
    // the run reaches its final state, but a device on the way does not react: worth telling the user about,
    // not worth stopping for
    if (report.blockedSteps > 0 || report.skippedSteps > 0) {
        return Verdict::RunsWithWarnings;
    }
    return Verdict::Ready;
}

QList<SequenceReadinessReport::Cause> SequenceReadinessReport::causes(const core::SequenceReadiness& report) {
    QList<Cause>        causes;
    QHash<QString, int> indexByGroupId;

    // the group id is assigned by the core and opaque: steps blocked by the same thing share it, and it is only
    // ever compared for equality
    for (const core::ReadinessStep& step : report.steps) {
        if (!step.hasReason) {
            continue;
        }

        const Severity severity = stepSeverity(step);
        int            index = indexByGroupId.value(step.reason.groupId, -1);

        if (index < 0) {
            Cause cause;
            cause.groupId = step.reason.groupId;
            cause.severity = severity;
            cause.reason = step.reason;
            cause.entityName = step.name;
            causes.append(cause);
            indexByGroupId.insert(step.reason.groupId, causes.count() - 1);
            index = causes.count() - 1;
        } else if (severity < causes[index].severity) {
            causes[index].severity = severity;
        }

        causes[index].steps.append(step);
    }

    // an omitted step joins the cause of the step it was dropped with, without ever making it worse: leaving out a
    // step whose device is already in the wanted state is the run working, not a problem
    for (const core::ReadinessOmittedStep& step : report.omitted) {
        int index = indexByGroupId.value(step.reason.groupId, -1);

        if (index < 0) {
            Cause cause;
            cause.groupId = step.reason.groupId;
            cause.severity = Severity::Omitted;
            cause.reason = step.reason;
            cause.entityName = step.name;
            causes.append(cause);
            indexByGroupId.insert(step.reason.groupId, causes.count() - 1);
            index = causes.count() - 1;
        }

        causes[index].omitted.append(step);
    }

    // stable: causes of equal severity keep the order the run would hit them in
    std::stable_sort(causes.begin(), causes.end(),
                     [](const Cause& lhs, const Cause& rhs) { return lhs.severity < rhs.severity; });

    return causes;
}

QStringList SequenceReadinessReport::unresponsiveDeviceNames(const core::SequenceReadiness& report) {
    QStringList names;

    for (const core::ReadinessStep& step : report.steps) {
        if (step.ready && !step.skipped) {
            continue;
        }
        if (step.name.isEmpty() || names.contains(step.name)) {
            continue;
        }
        names.append(step.name);
    }

    return names;
}

QVariantMap SequenceReadinessReport::toSummary(const core::SequenceReadiness& report) {
    QVariantMap summary;
    summary.insert("supported", true);
    summary.insert("ready", report.ready);

    switch (verdict(report)) {
        case Verdict::Ready:
            summary.insert("verdict", QStringLiteral("ready"));
            break;
        case Verdict::RunsWithWarnings:
            summary.insert("verdict", QStringLiteral("runs_with_warnings"));
            break;
        case Verdict::NotReady:
            summary.insert("verdict", QStringLiteral("not_ready"));
            break;
    }

    summary.insert("name", report.name);
    summary.insert("entityId", report.entityId);
    summary.insert("cmdId", report.cmdId);
    summary.insert("totalSteps", report.totalSteps);
    summary.insert("blockedSteps", report.blockedSteps);
    summary.insert("skippedSteps", report.skippedSteps);
    summary.insert("abortingSteps", report.abortingSteps);
    summary.insert("omittedSteps", report.omittedSteps);
    summary.insert("unresponsiveNames", unresponsiveDeviceNames(report));

    QVariantList causeList;
    for (const Cause& cause : causes(report)) {
        // an omitted step is not a problem to report: it is the run skipping work that is already done
        if (cause.severity == Severity::Omitted) {
            continue;
        }

        QVariantMap entry;
        entry.insert("code", cause.reason.code);
        entry.insert("severity", severityToString(cause.severity));
        entry.insert("entityName", cause.entityName);
        entry.insert("integrationName", cause.reason.integrationName);
        entry.insert("dockName", cause.reason.dockName);
        entry.insert("emitterName", cause.reason.emitterName);
        entry.insert("portId", cause.reason.portId);
        entry.insert("state", cause.reason.state);
        entry.insert("count", cause.steps.count());
        causeList.append(entry);
    }
    summary.insert("causes", causeList);

    return summary;
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
