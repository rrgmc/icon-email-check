#include "mail/GmailApi.h"

#include <QTest>

using namespace iec::gmail;

class TestGmail : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesTokens()
    {
        const TokenResponse ok = parseTokenResponse(
            R"({"access_token":"at","expires_in":3599,"refresh_token":"rt","scope":"x","token_type":"Bearer"})");
        QCOMPARE(ok.accessToken, QStringLiteral("at"));
        QCOMPARE(ok.refreshToken, QStringLiteral("rt"));
        QCOMPARE(ok.expiresIn, 3599);
        const TokenResponse bad = parseTokenResponse(R"({"error":"invalid_grant","error_description":"Bad"})");
        QVERIFY(bad.accessToken.isEmpty());
        QCOMPARE(bad.error, QStringLiteral("invalid_grant"));
        QCOMPARE(bad.errorDescription, QStringLiteral("Bad"));
    }

    void parsesLabelAndList()
    {
        QCOMPARE(parseLabelUnread(R"({"id":"INBOX","messagesUnread":12,"threadsUnread":9})"), 12);
        QCOMPARE(parseLabelUnread(R"({"id":"INBOX"})"), -1);
        QStringList ids;
        QString next;
        parseMessageList(R"({"messages":[{"id":"a","threadId":"t"},{"id":"b"}],"nextPageToken":"p2"})", &ids, &next);
        QCOMPARE(ids, (QStringList{QStringLiteral("a"), QStringLiteral("b")}));
        QCOMPARE(next, QStringLiteral("p2"));
        parseMessageList(R"({"resultSizeEstimate":0})", &ids, &next);
        QCOMPARE(ids.size(), 2);
        QVERIFY(next.isEmpty());
    }

    void parsesMetadata()
    {
        const iec::MailSummary s = parseMessageMetadata(
            R"({"id":"m1","payload":{"headers":[{"name":"From","value":"Ann <ann@x.com>"},)"
            R"({"name":"Subject","value":"Lunch   today?"}]}})");
        QCOMPARE(s.id, QStringLiteral("m1"));
        QCOMPARE(s.from, QStringLiteral("Ann"));
        QCOMPARE(s.subject, QStringLiteral("Lunch today?"));
    }

    void parsesErrors()
    {
        QCOMPARE(parseApiError(R"({"error":{"code":403,"message":"Gmail API has not been used"}})"),
                 QStringLiteral("Gmail API has not been used"));
        QCOMPARE(parseProfileEmail(R"({"emailAddress":"me@gmail.com"})"), QStringLiteral("me@gmail.com"));
    }

    void makesPkce()
    {
        const QByteArray verifier = randomToken();
        QCOMPARE(verifier.size(), 64);
        QVERIFY(verifier != randomToken());
        // The example of RFC 7636, appendix B.
        QCOMPARE(pkceChallenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk"),
                 QByteArray("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
    }
};

QTEST_GUILESS_MAIN(TestGmail)
#include "tst_gmail.moc"
