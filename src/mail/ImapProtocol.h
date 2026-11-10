#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace iec::imap {

// One response from an IMAP server: a line, and the literals ("{12}\r\n"
// followed by 12 bytes) inside it. text keeps each literal's "{n}" marker and
// drops its bytes, which are in literals, in order.
struct Response
{
    QByteArray text;
    QList<QByteArray> literals;

    // "*" for untagged data, "+" for a continuation, else the command's tag.
    QByteArray tag() const;
    // For a tagged or untagged status: "OK", "NO", "BAD", "BYE" or "PREAUTH",
    // upper case. Empty for other responses.
    QByteArray status() const;
};

// Splits the bytes a server sends into responses. Feed it what the socket
// reads, then take responses until next() returns false.
class Reader
{
public:
    void feed(const QByteArray &bytes);
    bool next(Response *response);

private:
    QByteArray m_buffer;
};

// A string as an IMAP quoted string: in double quotes, with \ and " escaped.
QByteArray quote(const QString &text);

// The numbers of a "* SEARCH 1 2 3" response, in the order given.
QList<qint64> parseSearch(const QByteArray &text);

// The number after the given item name, such as UID in "* 3 FETCH (UID 45 ...",
// or UIDVALIDITY in "* OK [UIDVALIDITY 7] ...". -1 when it is missing.
qint64 numberAfter(const QByteArray &text, const QByteArray &name);

} // namespace iec::imap
