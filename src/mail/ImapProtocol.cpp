#include "mail/ImapProtocol.h"

namespace iec::imap {

QByteArray Response::tag() const
{
    const qsizetype space = text.indexOf(' ');
    return space < 0 ? text : text.left(space);
}

QByteArray Response::status() const
{
    const QList<QByteArray> words = text.split(' ');
    if (words.size() < 2)
        return {};
    const QByteArray word = words.at(1).toUpper();
    if (word == "OK" || word == "NO" || word == "BAD" || word == "BYE" || word == "PREAUTH")
        return word;
    return {};
}

void Reader::feed(const QByteArray &bytes) { m_buffer += bytes; }

bool Reader::next(Response *response)
{
    Response result;
    qsizetype pos = 0;
    for (;;) {
        const qsizetype end = m_buffer.indexOf("\r\n", pos);
        if (end < 0)
            return false;
        const QByteArray line = m_buffer.mid(pos, end - pos);
        result.text += line;
        pos = end + 2;
        // A line that ends in {n} (or {n+}) is followed by n bytes, and then
        // the rest of the response.
        if (line.endsWith('}')) {
            const qsizetype open = line.lastIndexOf('{');
            QByteArray digits = line.mid(open + 1, line.size() - open - 2);
            if (digits.endsWith('+'))
                digits.chop(1);
            bool ok = false;
            const qsizetype size = open >= 0 ? digits.toLongLong(&ok) : 0;
            if (ok && size >= 0) {
                if (m_buffer.size() < pos + size)
                    return false;
                result.literals.append(m_buffer.mid(pos, size));
                pos += size;
                continue;
            }
        }
        break;
    }
    m_buffer.remove(0, pos);
    *response = result;
    return true;
}

QByteArray quote(const QString &text)
{
    QByteArray out = "\"";
    for (const char c : text.toUtf8()) {
        if (c == '\\' || c == '"')
            out += '\\';
        out += c;
    }
    out += '"';
    return out;
}

QList<qint64> parseSearch(const QByteArray &text)
{
    QList<qint64> numbers;
    const QList<QByteArray> words = text.simplified().split(' ');
    if (words.size() < 2 || words.at(0) != "*" || words.at(1).toUpper() != "SEARCH")
        return numbers;
    for (qsizetype i = 2; i < words.size(); ++i) {
        bool ok = false;
        const qint64 n = words.at(i).toLongLong(&ok);
        if (ok)
            numbers.append(n);
    }
    return numbers;
}

qint64 numberAfter(const QByteArray &text, const QByteArray &name)
{
    const QByteArray upper = text.toUpper();
    const QByteArray key = name.toUpper() + ' ';
    qsizetype at = 0;
    while ((at = upper.indexOf(key, at)) >= 0) {
        // The name must start a word, so UID does not match inside UIDNEXT.
        const bool starts = at == 0 || upper.at(at - 1) == ' ' || upper.at(at - 1) == '(' || upper.at(at - 1) == '[';
        if (starts) {
            qsizetype i = at + key.size();
            qsizetype j = i;
            while (j < upper.size() && upper.at(j) >= '0' && upper.at(j) <= '9')
                ++j;
            bool ok = false;
            const qint64 n = upper.mid(i, j - i).toLongLong(&ok);
            if (ok)
                return n;
        }
        at += key.size();
    }
    return -1;
}

} // namespace iec::imap
