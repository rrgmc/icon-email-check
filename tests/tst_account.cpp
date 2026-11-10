#include "accounts/Account.h"

#include <QTest>

using namespace iec;

class TestAccount : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void roundTripsThroughJson()
    {
        Account a;
        a.id = QStringLiteral("id1");
        a.name = QStringLiteral("Work");
        a.type = AccountType::Imap;
        a.enabled = false;
        a.notify = false;
        a.intervalMinutes = 15;
        a.host = QStringLiteral("imap.example.com");
        a.port = 143;
        a.security = ImapSecurity::StartTls;
        a.username = QStringLiteral("me");
        a.folder = QStringLiteral("Work/Inbox");
        a.webUrl = QStringLiteral("https://mail.example.com");
        const Account b = Account::fromJson(a.toJson());
        QCOMPARE(b.id, a.id);
        QCOMPARE(b.name, a.name);
        QCOMPARE(b.type, a.type);
        QCOMPARE(b.enabled, a.enabled);
        QCOMPARE(b.notify, a.notify);
        QCOMPARE(b.intervalMinutes, a.intervalMinutes);
        QCOMPARE(b.host, a.host);
        QCOMPARE(b.port, a.port);
        QCOMPARE(b.security, a.security);
        QCOMPARE(b.username, a.username);
        QCOMPARE(b.folder, a.folder);
        QCOMPARE(b.webUrl, a.webUrl);
    }

    void fillsDefaults()
    {
        const Account a = Account::fromJson(QJsonObject{{QStringLiteral("id"), QStringLiteral("x")}});
        QCOMPARE(a.type, AccountType::Gmail);
        QVERIFY(a.enabled);
        QCOMPARE(a.intervalMinutes, 5);
        QCOMPARE(a.folder, QStringLiteral("INBOX"));
    }

    void opensTheRightInbox()
    {
        Account gmail;
        gmail.type = AccountType::Gmail;
        gmail.email = QStringLiteral("me+x@gmail.com");
        QCOMPARE(gmail.inboxUrl(), QStringLiteral("https://mail.google.com/mail/u/me%2Bx%40gmail.com/#inbox"));
        Account imap;
        imap.type = AccountType::Imap;
        imap.host = QStringLiteral("imap.example.com");
        QVERIFY(imap.inboxUrl().isEmpty());
        imap.webUrl = QStringLiteral(" https://webmail.example.com ");
        QCOMPARE(imap.inboxUrl(), QStringLiteral("https://webmail.example.com"));
        QCOMPARE(imap.displayName(), QStringLiteral("imap.example.com"));
    }
};

QTEST_GUILESS_MAIN(TestAccount)
#include "tst_account.moc"
