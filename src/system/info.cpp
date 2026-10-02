// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "info.h"

#include "../logging.h"

namespace uc {
namespace hw {

Info *Info::s_instance = nullptr;

Info::Info(HardwareModel::Enum model, QObject *parent)
    : QObject(parent), m_modelNumber(Util::convertEnumToString(model)) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

Info::~Info() {
    s_instance = nullptr;
}

void Info::set(const QString &serialNumber, const QString &revision) {
    m_serialNumber = serialNumber;
    m_revision = revision;
}

QObject *Info::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

}  // namespace hw
}  // namespace uc
