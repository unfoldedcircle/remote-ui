// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/core.h"
#include "voice.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

/**
 * The spoken answer of the voice assistant is downloaded and streamed into a player process. The player is
 * replaced by a shell that writes what it gets into a file, the answer is a local file: a playback is checked by
 * what reaches the player, and by the end signal the voice overlay closes on.
 */
class testVoice : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();

    void playback_streamsTheAnswerAndReportsItsEnd();
    void secondAnswer_replacesTheFirstWithoutMixingOrEndingEarly();
    void playerThatCannotStart_reportsTheEnd();
    void unsupportedType_reportsTheEnd();

 private:
    QString writeAnswer(const QString& name, char fill, int size);
    void    usePlayerWritingTo(uc::Voice* voice, const QString& output);

    QTemporaryDir m_dir;
};

void testVoice::initTestCase() {
    QVERIFY(m_dir.isValid());
}

QString testVoice::writeAnswer(const QString& name, char fill, int size) {
    const QString path = m_dir.filePath(name);
    QFile         file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return QString();
    }
    file.write(QByteArray(size, fill));
    return QUrl::fromLocalFile(path).toString();
}

void testVoice::usePlayerWritingTo(uc::Voice* voice, const QString& output) {
    // appends, so that a playback that is not stopped would show up in the file
    voice->setPlayer(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), QStringLiteral("cat >> '%1'").arg(output)});
}

void testVoice::playback_streamsTheAnswerAndReportsItsEnd() {
    uc::core::Api api(kTestUrl);
    uc::Voice     voice(&api);
    const QString output = m_dir.filePath(QStringLiteral("single.out"));
    usePlayerWritingTo(&voice, output);

    QSignalSpy ended(&voice, &uc::Voice::assistantAudioSpeechResponseEnd);
    voice.playSpeechResponse(writeAnswer(QStringLiteral("single.mp3"), 'a', 100000), QStringLiteral("audio/mpeg"));

    QTRY_COMPARE_WITH_TIMEOUT(ended.count(), 1, 5000);

    QFile played(output);
    QVERIFY(played.open(QIODevice::ReadOnly));
    QCOMPARE(played.readAll(), QByteArray(100000, 'a'));
}

/**
 * A second answer while the first one plays: the first download kept writing into the second player, and the end
 * of the killed first player was reported as the end of the second answer, which closed its voice overlay.
 */
void testVoice::secondAnswer_replacesTheFirstWithoutMixingOrEndingEarly() {
    uc::core::Api api(kTestUrl);
    uc::Voice     voice(&api);
    const QString output = m_dir.filePath(QStringLiteral("overlap.out"));
    usePlayerWritingTo(&voice, output);

    QSignalSpy ended(&voice, &uc::Voice::assistantAudioSpeechResponseEnd);

    voice.playSpeechResponse(writeAnswer(QStringLiteral("first.mp3"), 'a', 2000000), QStringLiteral("audio/mpeg"));
    // the first answer is under way
    QTest::qWait(50);
    voice.playSpeechResponse(writeAnswer(QStringLiteral("second.mp3"), 'b', 100000), QStringLiteral("audio/mpeg"));

    QTRY_VERIFY_WITH_TIMEOUT(ended.count() >= 1, 5000);
    // nothing more arrives for the first answer
    QTest::qWait(500);
    QCOMPARE(ended.count(), 1);

    QFile played(output);
    QVERIFY(played.open(QIODevice::ReadOnly));
    const QByteArray content = played.readAll();
    // the second answer arrived complete and in one piece, after whatever the first player had got
    QVERIFY(content.endsWith(QByteArray(100000, 'b')));
    QCOMPARE(content.count('b'), 100000);
    QVERIFY2(!content.mid(content.indexOf('b')).contains('a'), "the first answer reached the second player");
}

void testVoice::playerThatCannotStart_reportsTheEnd() {
    uc::core::Api api(kTestUrl);
    uc::Voice     voice(&api);
    voice.setPlayer(m_dir.filePath(QStringLiteral("no-such-player")), {});

    QSignalSpy ended(&voice, &uc::Voice::assistantAudioSpeechResponseEnd);
    voice.playSpeechResponse(writeAnswer(QStringLiteral("unplayed.mp3"), 'c', 1000), QStringLiteral("audio/mpeg"));

    QTRY_COMPARE_WITH_TIMEOUT(ended.count(), 1, 5000);
    QTest::qWait(200);
    QCOMPARE(ended.count(), 1);
}

void testVoice::unsupportedType_reportsTheEnd() {
    uc::core::Api api(kTestUrl);
    uc::Voice     voice(&api);

    QSignalSpy ended(&voice, &uc::Voice::assistantAudioSpeechResponseEnd);
    voice.playSpeechResponse(writeAnswer(QStringLiteral("text.txt"), 'd', 10), QStringLiteral("text/plain"));

    QCOMPARE(ended.count(), 1);
}

QTEST_GUILESS_MAIN(testVoice)

#include "test_voice.moc"
