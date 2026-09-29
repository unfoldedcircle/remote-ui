// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "info.h"

#include "../logging.h"

namespace uc {
namespace hw {

Info *Info::s_instance = nullptr;

Info::Info(QObject *parnet) : QObject(parnet) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

Info::~Info() {
    s_instance = nullptr;
}

void Info::set(HardwareModel::Enum modelNumber, const QString &serialNumber, const QString &revision) {
    m_modelNumber = Util::convertEnumToString(modelNumber);
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
