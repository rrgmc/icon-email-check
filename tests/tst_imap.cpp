#include "mail/ImapProtocol.h"

#include <QTest>

using namespace iec::imap;

class TestImap : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readsLinesAcrossFeeds()
    {
        Reader reader;
        Response r;
        reader.feed("* OK IMAP4rev1 ready\r\na1 OK LOG");
        QVERIFY(reader.next(&r));
        QCOMPARE(r.tag(), QByteArray("*"));
        QCOMPARE(r.status(), QByteArray("OK"));
        QVERIFY(!reader.next(&r));
        reader.feed("IN done\r\n");
        QVERIFY(reader.next(&r));
        QCOMPARE(r.tag(), QByteArray("a1"));
        QCOMPARE(r.status(), QByteArray("OK"));
        QCOMPARE(r.text, QByteArray("a1 OK LOGIN done"));
    }

    void readsLiterals()
    {
        Reader reader;
        Response r;
        const QByteArray header = "Subject: Hi\r\nFrom: A <a@b>\r\n\r\n";
        reader.feed("* 1 FETCH (UID 42 BODY[HEADER.FIELDS (FROM SUBJECT)] {" + QByteArray::number(header.size())
                    + "}\r\n");
        QVERIFY(!reader.next(&r));    // the literal has not arrived
        reader.feed(header.left(10));
        QVERIFY(!reader.next(&r));
        reader.feed(header.mid(10) + ")\r\na3 OK FETCH done\r\n");
        QVERIFY(reader.next(&r));
        QCOMPARE(r.literals.size(), 1);
        QCOMPARE(r.literals.first(), header);
        QVERIFY(r.text.endsWith(")"));
        QCOMPARE(numberAfter(r.text, "UID"), 42);
        QVERIFY(reader.next(&r));
        QCOMPARE(r.tag(), QByteArray("a3"));
    }

    void parsesSearch()
    {
        QCOMPARE(parseSearch("* SEARCH 3 17 200"), (QList<qint64>{3, 17, 200}));
        QCOMPARE(parseSearch("* SEARCH"), QList<qint64>{});
        QCOMPARE(parseSearch("* 3 EXISTS"), QList<qint64>{});
    }

    void findsNumbersByWord()
    {
        QCOMPARE(numberAfter("* OK [UIDVALIDITY 1234] UIDs valid", "UIDVALIDITY"), 1234);
        // UID must not match inside UIDNEXT.
        QCOMPARE(numberAfter("* OK [UIDNEXT 9] next", "UID"), -1);
        QCOMPARE(numberAfter("* 5 FETCH (FLAGS () UID 77)", "uid"), 77);
    }

    void quotesStrings()
    {
        QCOMPARE(quote(QStringLiteral("plain")), QByteArray("\"plain\""));
        QCOMPARE(quote(QStringLiteral("a\"b\\c")), QByteArray("\"a\\\"b\\\\c\""));
    }
};

QTEST_GUILESS_MAIN(TestImap)
#include "tst_imap.moc"
