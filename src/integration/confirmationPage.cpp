// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "confirmationPage.h"

#include "../logging.h"
#include "../util.h"

namespace uc {
namespace integration {

ConfirmationPage::ConfirmationPage(QVariantMap title, QVariantMap message1, const QString &image, QVariantMap message2,
                                   const QString &language, QObject *parent)
    : QObject(parent), m_title_i18n(title), m_message1_i18n(message1), m_image(image), m_message2_i18n(message2) {
    qCDebug(lcIntegrationDriver()) << "ConfirmationPage constructor";

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    m_title = Util::getLanguageString(m_title_i18n, language);
    m_message1 = Util::getLanguageString(m_message1_i18n, language);
    m_message2 = Util::getLanguageString(m_message2_i18n, language);
}

ConfirmationPage::~ConfirmationPage() {
    qCDebug(lcIntegrationDriver()) << "ConfirmationPage destructor";
}

}  // namespace integration
}  // namespace uc
