// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/entity/entities.h"

namespace {

/**
 * The stale response guard lives in the abstract uc::ui::Entities base class, shared by the available and
 * the configured entity list. A minimal model makes it reachable without a core connection.
 */
class TestEntities : public uc::ui::Entities {
 public:
    TestEntities() : uc::ui::Entities(nullptr) {}

    void init(const QString &integrationId) override { Q_UNUSED(integrationId) }
    void search(const QString &searchString) override { Q_UNUSED(searchString) }
    void setIntegrationIds(const QStringList &integrationIds) override { Q_UNUSED(integrationIds) }
    void removeIntegrationIds(const QStringList &integrationIds) override { Q_UNUSED(integrationIds) }
    void clearIntegrationIds() override {}
    void setEntityType(int type) override { Q_UNUSED(type) }
    void removeEntityType(int type) override { Q_UNUSED(type) }
    bool containsEntityType(int type) override {
        Q_UNUSED(type)
        return false;
    }
    void cleanEntityTypes() override {}
    void loadMore() override {}

    using uc::ui::Entities::isStaleResponse;
    using uc::ui::Entities::setActiveRequest;

 protected:
    void loadFromCore(int limit, int page) override {
        Q_UNUSED(limit)
        Q_UNUSED(page)
    }
    void onFilterChanged() override {}
};

}  // namespace

/**
 * Typing in the entity list search field sends one request per keystroke, and the answers are not
 * guaranteed to arrive in the order they were requested. The list keeps the newest request and drops
 * everything else, so rows of an outdated search term can never be mixed into the list.
 */
class testEntitiesStaleResponse : public QObject {
    Q_OBJECT

 private slots:
    void answerOfTheNewestRequestIsAccepted();
    void answerOfAnOlderRequestIsDropped();
    void acceptedResponseSettlesTheRequest();
    void requestThatWasNotSentDropsThePendingAnswer();
    void consecutiveRequestsAreAllAccepted();
};

void testEntitiesStaleResponse::answerOfTheNewestRequestIsAccepted() {
    TestEntities entities;

    entities.setActiveRequest(1);

    QVERIFY(!entities.isStaleResponse(1));
}

void testEntitiesStaleResponse::answerOfAnOlderRequestIsDropped() {
    TestEntities entities;

    // "so" is still in flight when "sof" is typed
    entities.setActiveRequest(1);
    entities.setActiveRequest(2);

    // the newer answer arrives first, the older one afterwards
    QVERIFY(!entities.isStaleResponse(2));
    QVERIFY(entities.isStaleResponse(1));
}

void testEntitiesStaleResponse::acceptedResponseSettlesTheRequest() {
    TestEntities entities;

    entities.setActiveRequest(3);

    QVERIFY(!entities.isStaleResponse(3));
    // a late duplicate of the same request must not add its rows a second time
    QVERIFY(entities.isStaleResponse(3));
}

void testEntitiesStaleResponse::requestThatWasNotSentDropsThePendingAnswer() {
    TestEntities entities;

    entities.setActiveRequest(4);
    // the next request could not be sent (not connected): its id is negative and no answer will arrive,
    // but the answer of the superseded request must not repopulate the list either
    entities.setActiveRequest(-1);

    QVERIFY(entities.isStaleResponse(4));
}

void testEntitiesStaleResponse::consecutiveRequestsAreAllAccepted() {
    TestEntities entities;

    // answers that arrive in order, e.g. paging through the list
    entities.setActiveRequest(5);
    QVERIFY(!entities.isStaleResponse(5));

    entities.setActiveRequest(6);
    QVERIFY(!entities.isStaleResponse(6));
}

QTEST_GUILESS_MAIN(testEntitiesStaleResponse)

#include "test_entities_stale_response.moc"
