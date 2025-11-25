// Copyright (c) 2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "touchSlider.h"
#include <QTimer>

#include "../logging.h"

namespace uc {
namespace hw {

TouchSlider *TouchSlider::s_instance = nullptr;

TouchSlider::TouchSlider(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

TouchSlider::~TouchSlider() {
    s_instance = nullptr;
}

QObject *TouchSlider::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}
}  // namespace hw
}  // namespace uc
