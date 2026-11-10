#include "mail/GmailApi.h"

#include "mail/MimeHeaders.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>

namespace iec::gmail {

namespace {

QJsonObject object(const QByteArray &json) { return QJsonDocument::fromJson(json).object(); }

} // namespace

TokenResponse parseTokenResponse(const QByteArray &json)
{
    const QJsonObject o = object(json);
    TokenResponse r;
    r.accessToken = o.value(QLatin1String("access_token")).toString();
    r.refreshToken = o.value(QLatin1String("refresh_token")).toString();
    r.expiresIn = o.value(QLatin1String("expires_in")).toInt();
    r.error = o.value(QLatin1String("error")).toString();
    r.errorDescription = o.value(QLatin1String("error_description")).toString();
    return r;
}

int parseLabelUnread(const QByteArray &json)
{
    const QJsonValue v = object(json).value(QLatin1String("messagesUnread"));
    return v.isDouble() ? v.toInt() : -1;
}

void parseMessageList(const QByteArray &json, QStringList *ids, QString *nextPageToken)
{
    const QJsonObject o = object(json);
    const QJsonArray messages = o.value(QLatin1String("messages")).toArray();
    for (const QJsonValue &m : messages) {
        const QString id = m.toObject().value(QLatin1String("id")).toString();
        if (!id.isEmpty())
            ids->append(id);
    }
    *nextPageToken = o.value(QLatin1String("nextPageToken")).toString();
}

MailSummary parseMessageMetadata(const QByteArray &json)
{
    const QJsonObject o = object(json);
    MailSummary s;
    s.id = o.value(QLatin1String("id")).toString();
    const QJsonArray headers = o.value(QLatin1String("payload")).toObject().value(QLatin1String("headers")).toArray();
    for (const QJsonValue &h : headers) {
        const QJsonObject header = h.toObject();
        const QString name = header.value(QLatin1String("name")).toString();
        const QString value = header.value(QLatin1String("value")).toString();
        if (name.compare(QLatin1String("From"), Qt::CaseInsensitive) == 0)
            s.from = mime::displayFrom(value);
        else if (name.compare(QLatin1String("Subject"), Qt::CaseInsensitive) == 0)
            s.subject = value.simplified();
    }
    return s;
}

QString parseProfileEmail(const QByteArray &json)
{
    return object(json).value(QLatin1String("emailAddress")).toString();
}

QString parseApiError(const QByteArray &json)
{
    const QJsonValue error = object(json).value(QLatin1String("error"));
    if (error.isObject())
        return error.toObject().value(QLatin1String("message")).toString();
    return error.toString();
}

QByteArray randomToken()
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    QByteArray out;
    out.reserve(64);
    for (int i = 0; i < 64; ++i)
        out.append(alphabet[QRandomGenerator::system()->bounded(int(sizeof(alphabet) - 1))]);
    return out;
}

QByteArray pkceChallenge(const QByteArray &verifier)
{
    return QCryptographicHash::hash(verifier, QCryptographicHash::Sha256)
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

} // namespace iec::gmail
