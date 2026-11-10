#include "mail/ImapChecker.h"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

using namespace iec;

namespace {

// A scripted IMAP server on localhost: it answers each command by its verb.
class FakeImapServer : public QObject
{
public:
    bool loginOk = true;
    QList<QByteArray> received;

    FakeImapServer()
    {
        server.listen(QHostAddress::LocalHost, 0);
        connect(&server, &QTcpServer::newConnection, this, [this] {
            QTcpSocket *socket = server.nextPendingConnection();
            socket->write("* OK fake ready\r\n");
            connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                while (socket->canReadLine())
                    answer(socket, socket->readLine().trimmed());
            });
        });
    }

    quint16 port() const { return server.serverPort(); }

private:
    void answer(QTcpSocket *socket, const QByteArray &line)
    {
        received << line;
        const QList<QByteArray> words = line.split(' ');
        const QByteArray tag = words.value(0);
        const QByteArray verb = words.value(1).toUpper();
        if (verb == "LOGIN") {
            socket->write(loginOk ? tag + " OK logged in\r\n" : tag + " NO [AUTHENTICATIONFAILED] wrong\r\n");
        } else if (verb == "EXAMINE") {
            socket->write("* 3 EXISTS\r\n* OK [UIDVALIDITY 7] ok\r\n" + tag + " OK [READ-ONLY] done\r\n");
        } else if (verb == "UID" && words.value(2).toUpper() == "SEARCH") {
            socket->write("* SEARCH 10 12 11\r\n" + tag + " OK done\r\n");
        } else if (verb == "UID" && words.value(2).toUpper() == "FETCH") {
            for (const int uid : {12, 11, 10}) {
                const QByteArray header = "From: Sender " + QByteArray::number(uid)
                    + " <s@x.com>\r\nSubject: =?UTF-8?B?T2zDoQ==?= " + QByteArray::number(uid) + "\r\n\r\n";
                socket->write("* " + QByteArray::number(uid - 9) + " FETCH (UID " + QByteArray::number(uid)
                              + " BODY[HEADER.FIELDS (FROM SUBJECT)] {" + QByteArray::number(header.size()) + "}\r\n"
                              + header + ")\r\n");
            }
            socket->write(tag + " OK done\r\n");
        } else if (verb == "LOGOUT") {
            socket->write("* BYE bye\r\n" + tag + " OK done\r\n");
            socket->disconnectFromHost();
        } else {
            socket->write(tag + " BAD unknown\r\n");
        }
    }

    QTcpServer server;
};

Account accountFor(quint16 port)
{
    Account a;
    a.id = QStringLiteral("t");
    a.type = AccountType::Imap;
    a.host = QStringLiteral("127.0.0.1");
    a.port = port;
    a.security = ImapSecurity::None;
    a.username = QStringLiteral("me@x.com");
    return a;
}

CheckResult run(const Account &account, const QString &password)
{
    ImapChecker checker;
    QSignalSpy spy(&checker, &MailChecker::finished);
    checker.start(account, password);
    if (!spy.wait(5000))
        return {};
    return spy.first().first().value<CheckResult>();
}

} // namespace

Q_DECLARE_METATYPE(iec::CheckResult)

class TestImapChecker : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { qRegisterMetaType<CheckResult>(); }

    void countsAndDescribesUnreadMail()
    {
        FakeImapServer server;
        const CheckResult result = run(accountFor(server.port()), QStringLiteral("pa\"ss"));
        QVERIFY2(result.ok, qPrintable(result.error));
        QCOMPARE(result.unread, 3);
        // Newest first, with the UID validity in each id.
        QCOMPARE(result.unreadIds, (QStringList{QStringLiteral("7:12"), QStringLiteral("7:11"), QStringLiteral("7:10")}));
        QCOMPARE(result.recent.size(), 3);
        QCOMPARE(result.recent.first().from, QStringLiteral("Sender 12"));
        QCOMPARE(result.recent.first().subject, QStringLiteral("Olá 12"));
        // Read-only, and the password quoted.
        QVERIFY(server.received.contains(QByteArray("a1 LOGIN \"me@x.com\" \"pa\\\"ss\"")));
        QVERIFY(server.received.contains(QByteArray("a2 EXAMINE \"INBOX\"")));
    }

    void reportsARefusedLogin()
    {
        FakeImapServer server;
        server.loginOk = false;
        const CheckResult result = run(accountFor(server.port()), QStringLiteral("wrong"));
        QVERIFY(!result.ok);
        QVERIFY(result.authFailed);
        QVERIFY(!result.error.isEmpty());
    }

    void reportsAServerThatIsNotThere()
    {
        QTcpServer probe;
        probe.listen(QHostAddress::LocalHost, 0);
        const quint16 port = probe.serverPort();
        probe.close();
        const CheckResult result = run(accountFor(port), QStringLiteral("x"));
        QVERIFY(!result.ok);
        QVERIFY(!result.authFailed);
    }
};

QTEST_GUILESS_MAIN(TestImapChecker)
#include "tst_imap_checker.moc"
