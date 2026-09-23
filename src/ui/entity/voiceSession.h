// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QVariantMap>

namespace uc {
namespace ui {

/**
 * Which pending voice assistant command the end of a voice session leaves behind.
 *
 * A session is opened with `voice_start`, carrying the session id the UI created for it, and closed with
 * `voice_end`. A command that failed inside the resume window of a wakeup is sent again every 500 ms until the
 * window closes, and a `voice_start` is no exception: the user can hold the microphone button while the
 * integration is still coming back, let go again, and the repetition then delivers the start of a session the UI
 * has already ended - the assistant begins listening with nobody speaking. Once a session is over, a start of it
 * that is still pending is therefore dropped instead of being sent again.
 */

/// Session id that matches every session of an entity, for a session end that names none of its own
constexpr int kAnyVoiceSession = -1;

/// true if the command opens a voice session: the assistant starts listening when it arrives
bool startsVoiceSession(const QString& command);

/// true if the command closes a voice session
bool endsVoiceSession(const QString& command);

/// The session id in the parameters of a voice assistant command, kAnyVoiceSession if it carries none
int voiceSessionIdOf(const QVariantMap& params);

/**
 * @brief true if a command that is still pending opens a voice session which has already ended.
 *
 * @param pendingEntityId: entity of the command that is still pending
 * @param pendingCommand: command id of the command that is still pending, e.g. "voice_start"
 * @param pendingParams: parameters of the command that is still pending
 * @param endedEntityId: the voice assistant whose session ended
 * @param endedSessionId: the session that ended, kAnyVoiceSession for every session of that entity
 */
bool isVoiceStartOfEndedSession(const QString& pendingEntityId, const QString& pendingCommand,
                                const QVariantMap& pendingParams, const QString& endedEntityId,
                                int endedSessionId);

}  // namespace ui
}  // namespace uc
