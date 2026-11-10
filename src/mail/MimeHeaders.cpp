#include "mail/MimeHeaders.h"

#include <QList>
#include <QRegularExpression>
#include <QStringDecoder>

namespace iec::mime {

namespace {

QByteArray decodeQ(const QByteArray &text)
{
    QByteArray out;
    out.reserve(text.size());
    for (qsizetype i = 0; i < text.size(); ++i) {
        const char c = text.at(i);
        if (c == '_') {
            out.append(' ');
        } else if (c == '=' && i + 2 < text.size()) {
            bool ok = false;
            const int value = text.mid(i + 1, 2).toInt(&ok, 16);
            if (ok) {
                out.append(char(value));
                i += 2;
            } else {
                out.append(c);
            }
        } else {
            out.append(c);
        }
    }
    return out;
}

QString toUnicode(const QByteArray &charset, const QByteArray &bytes)
{
    QStringDecoder decoder(charset.constData());
    if (!decoder.isValid())
        return QString::fromLatin1(bytes);
    return decoder.decode(bytes);
}

} // namespace

QString decodeWords(const QByteArray &value)
{
    static const QRegularExpression word(QStringLiteral(R"(=\?([^?]+)\?([bBqQ])\?([^?]*)\?=)"));
    // Matched on Latin-1, so each byte stays one character and offsets agree.
    const QString latin = QString::fromLatin1(value);
    QString out;
    qsizetype last = 0;
    bool lastWasWord = false;
    auto it = word.globalMatch(latin);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString between = latin.mid(last, m.capturedStart() - last);
        if (!(lastWasWord && between.trimmed().isEmpty()))
            out += QString::fromUtf8(between.toLatin1());
        QByteArray charset = m.captured(1).toLatin1();
        // RFC 2231 allows a language after the charset: "UTF-8*en".
        const qsizetype star = charset.indexOf('*');
        if (star >= 0)
            charset.truncate(star);
        const QByteArray text = m.captured(3).toLatin1();
        const bool base64 = m.captured(2).compare(QLatin1String("b"), Qt::CaseInsensitive) == 0;
        out += toUnicode(charset, base64 ? QByteArray::fromBase64(text) : decodeQ(text));
        last = m.capturedEnd();
        lastWasWord = true;
    }
    out += QString::fromUtf8(latin.mid(last).toLatin1());
    return out;
}

QString headerField(const QByteArray &headers, const QByteArray &name)
{
    const QList<QByteArray> lines = headers.split('\n');
    const QByteArray wanted = name.toLower() + ':';
    QByteArray value;
    bool inField = false;
    for (QByteArray line : lines) {
        if (line.endsWith('\r'))
            line.chop(1);
        if (inField) {
            // A folded line starts with whitespace and continues the field.
            if (!line.isEmpty() && (line.at(0) == ' ' || line.at(0) == '\t')) {
                value += ' ' + line.trimmed();
                continue;
            }
            break;
        }
        if (line.toLower().startsWith(wanted)) {
            value = line.mid(wanted.size()).trimmed();
            inField = true;
        }
    }
    return decodeWords(value).simplified();
}

QString displayFrom(const QString &from)
{
    const QString text = from.trimmed();
    const qsizetype open = text.lastIndexOf(QLatin1Char('<'));
    if (open > 0) {
        QString name = text.left(open).trimmed();
        if (name.size() >= 2 && name.startsWith(QLatin1Char('"')) && name.endsWith(QLatin1Char('"')))
            name = name.mid(1, name.size() - 2).replace(QLatin1String("\\\""), QLatin1String("\""));
        if (!name.isEmpty())
            return name;
    }
    if (open == 0) {
        const qsizetype close = text.indexOf(QLatin1Char('>'), open);
        return text.mid(1, close > 0 ? close - 1 : -1);
    }
    return text;
}

} // namespace iec::mime
