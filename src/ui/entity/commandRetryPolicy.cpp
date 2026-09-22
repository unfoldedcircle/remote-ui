// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commandRetryPolicy.h"

namespace uc {
namespace ui {

bool isRepeatingCommand(const QString& command, const QVariantMap& params) {
    return command == QStringLiteral("remote.send") && params.contains(QStringLiteral("repeat"));
}

bool isRequestRejected(int code) {
    switch (code) {
        // Bad Request: the core answers it for an invalid command or invalid parameters, and it relays the
        // same code from an integration driver that rejected the request. The request is wrong, not its timing
        case 400:
        // Unauthorized / Forbidden: the command is not allowed, not delayed
        case 401:
        case 403:
        // Unprocessable Entity: the request data was understood and refused
        case 422:
        // Not Implemented: the integration does not implement this command
        case 501:
            return true;
        default:
            // Everything else can succeed once the core and the integrations are back, which is what the
            // resume window is for: 404 (the entity is not registered yet), 408 (no answer in time),
            // 409 (conflicts with a state that is still being restored), 500 and 503 (not connected)
            return false;
    }
}

bool mayResendAfterWakeup(const QString& command, const QVariantMap& params, int code) {
    // a key repeat is stale the moment it fails: sending it again would replay a button press the user has
    // long released, and a held button would queue one repetition per repeat
    if (isRepeatingCommand(command, params)) {
        return false;
    }

    return !isRequestRejected(code);
}

}  // namespace ui
}  // namespace uc
