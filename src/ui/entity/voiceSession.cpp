// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "voiceSession.h"

namespace uc {
namespace ui {

// The voice assistant entity sends its commands without the entity type prefix every other entity uses, so
// these are the command ids as they arrive at the entity controller (entity::Base::sendCommand).
bool startsVoiceSession(const QString& command) {
    return command == QStringLiteral("voice_start");
}

bool endsVoiceSession(const QString& command) {
    return command == QStringLiteral("voice_end");
}

int voiceSessionIdOf(const QVariantMap& params) {
    bool      ok        = false;
    const int sessionId = params.value(QStringLiteral("session_id")).toInt(&ok);

    return ok ? sessionId : kAnyVoiceSession;
}

bool isVoiceStartOfEndedSession(const QString& pendingEntityId, const QString& pendingCommand,
                                const QVariantMap& pendingParams, const QString& endedEntityId,
                                int endedSessionId) {
    if (pendingEntityId != endedEntityId || !startsVoiceSession(pendingCommand)) {
        return false;
    }

    // `voice_end` names no session of its own, and the overlay runs one session at a time per assistant: a
    // session end without an id ends whatever start of that entity is still on its way
    if (endedSessionId == kAnyVoiceSession) {
        return true;
    }

    return voiceSessionIdOf(pendingParams) == endedSessionId;
}

}  // namespace ui
}  // namespace uc
