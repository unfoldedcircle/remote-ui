// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "haptic.h"

#include "../logging.h"

namespace uc {
namespace hw {

Haptic *Haptic::s_instance = nullptr;

Haptic::Haptic(QObject *parent) : QObject(parent) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

Haptic::~Haptic() {
    s_instance = nullptr;
}

bool Haptic::getEnabled() {
    return m_enabled;
}

void Haptic::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void Haptic::play(Haptic::Effects effect) {
    switch (effect) {
        default:
            break;
    }
}

QObject *Haptic::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}
}  // namespace hw
}  // namespace uc
