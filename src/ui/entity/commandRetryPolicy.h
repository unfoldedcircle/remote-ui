// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QVariantMap>

namespace uc {
namespace ui {

/**
 * Which entity commands may be sent again after they failed inside the resume window of a wakeup.
 *
 * The window exists because the core and the integration drivers are still coming back up while the remote is
 * already awake and taking input: a command that lands in that gap fails for a reason which is gone a moment
 * later. Only those failures are worth repeating. A request the core or the driver rejected fails again with
 * the same code and the repetition only delays the error message to the user until the window has closed.
 */

/// true for a key repeat: `remote.send` with a repeat count, sent while a button is held down
bool isRepeatingCommand(const QString& command, const QVariantMap& params);

/// true if the failure code says the request itself was rejected, which sending it again cannot change
bool isRequestRejected(int code);

/// true if a command that failed inside the resume window may be sent again
bool mayResendAfterWakeup(const QString& command, const QVariantMap& params, int code);

}  // namespace ui
}  // namespace uc
