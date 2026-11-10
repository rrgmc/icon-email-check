#include "mail/MimeHeaders.h"

#include <QTest>

using namespace iec::mime;

class TestMime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void decodesEncodedWords()
    {
        QCOMPARE(decodeWords("=?UTF-8?B?Q2Fmw6k=?="), QStringLiteral("Café"));
        QCOMPARE(decodeWords("=?ISO-8859-1?Q?Caf=E9_au_lait?="), QStringLiteral("Café au lait"));
        QCOMPARE(decodeWords("Re: =?utf-8?q?Ol=C3=A1?= mundo"), QStringLiteral("Re: Olá mundo"));
        // Whitespace between two encoded words is dropped.
        QCOMPARE(decodeWords("=?UTF-8?Q?a?= =?UTF-8?Q?b?="), QStringLiteral("ab"));
        QCOMPARE(decodeWords("plain text"), QStringLiteral("plain text"));
    }

    void readsFoldedHeaderFields()
    {
        const QByteArray headers = "From: \"Doe, Jane\" <jane@example.com>\r\n"
                                   "Subject: A long\r\n"
                                   " subject line\r\n"
                                   "\r\n";
        QCOMPARE(headerField(headers, "subject"), QStringLiteral("A long subject line"));
        QCOMPARE(headerField(headers, "From"), QStringLiteral("\"Doe, Jane\" <jane@example.com>"));
        QCOMPARE(headerField(headers, "To"), QString());
    }

    void showsTheSender()
    {
        QCOMPARE(displayFrom(QStringLiteral("\"Doe, Jane\" <jane@example.com>")), QStringLiteral("Doe, Jane"));
        QCOMPARE(displayFrom(QStringLiteral("Bob <bob@example.com>")), QStringLiteral("Bob"));
        QCOMPARE(displayFrom(QStringLiteral("<only@example.com>")), QStringLiteral("only@example.com"));
        QCOMPARE(displayFrom(QStringLiteral("bare@example.com")), QStringLiteral("bare@example.com"));
    }
};

QTEST_GUILESS_MAIN(TestMime)
#include "tst_mime.moc"
