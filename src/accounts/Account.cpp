#include "accounts/Account.h"

#include <QCoreApplication>
#include <QUrl>

namespace iec {

namespace {

QString typeKey(AccountType type) { return type == AccountType::Imap ? QStringLiteral("imap") : QStringLiteral("gmail"); }

QString securityKey(ImapSecurity security)
{
    switch (security) {
    case ImapSecurity::StartTls:
        return QStringLiteral("starttls");
    case ImapSecurity::None:
        return QStringLiteral("none");
    case ImapSecurity::Tls:
        break;
    }
    return QStringLiteral("tls");
}

ImapSecurity securityFromKey(const QString &key)
{
    if (key == QLatin1String("starttls"))
        return ImapSecurity::StartTls;
    if (key == QLatin1String("none"))
        return ImapSecurity::None;
    return ImapSecurity::Tls;
}

} // namespace

QString Account::displayName() const
{
    if (!name.trimmed().isEmpty())
        return name.trimmed();
    if (!email.isEmpty())
        return email;
    if (!username.isEmpty())
        return username;
    return host;
}

QString Account::inboxUrl() const
{
    if (!webUrl.trimmed().isEmpty())
        return webUrl.trimmed();
    if (type == AccountType::Gmail || host.compare(QLatin1String("imap.gmail.com"), Qt::CaseInsensitive) == 0) {
        const QString who = !email.isEmpty() ? email : username;
        // The address picks the right account when several are signed in.
        return who.isEmpty() ? QStringLiteral("https://mail.google.com/mail/")
                             : QStringLiteral("https://mail.google.com/mail/u/%1/#inbox")
                                   .arg(QString::fromLatin1(QUrl::toPercentEncoding(who)));
    }
    return {};
}

QJsonObject Account::toJson() const
{
    QJsonObject json;
    json[QLatin1String("id")] = id;
    json[QLatin1String("name")] = name;
    json[QLatin1String("type")] = typeKey(type);
    json[QLatin1String("enabled")] = enabled;
    json[QLatin1String("notify")] = notify;
    json[QLatin1String("intervalMinutes")] = intervalMinutes;
    json[QLatin1String("email")] = email;
    json[QLatin1String("gmailQuery")] = gmailQuery;
    json[QLatin1String("host")] = host;
    json[QLatin1String("port")] = port;
    json[QLatin1String("security")] = securityKey(security);
    json[QLatin1String("username")] = username;
    json[QLatin1String("folder")] = folder;
    json[QLatin1String("webUrl")] = webUrl;
    return json;
}

Account Account::fromJson(const QJsonObject &json)
{
    Account a;
    a.id = json.value(QLatin1String("id")).toString();
    a.name = json.value(QLatin1String("name")).toString();
    a.type = json.value(QLatin1String("type")).toString() == QLatin1String("imap") ? AccountType::Imap
                                                                                   : AccountType::Gmail;
    a.enabled = json.value(QLatin1String("enabled")).toBool(true);
    a.notify = json.value(QLatin1String("notify")).toBool(true);
    a.intervalMinutes = qBound(1, json.value(QLatin1String("intervalMinutes")).toInt(5), 24 * 60);
    a.email = json.value(QLatin1String("email")).toString();
    a.gmailQuery = json.value(QLatin1String("gmailQuery")).toString();
    a.host = json.value(QLatin1String("host")).toString();
    a.port = json.value(QLatin1String("port")).toInt(993);
    a.security = securityFromKey(json.value(QLatin1String("security")).toString());
    a.username = json.value(QLatin1String("username")).toString();
    a.folder = json.value(QLatin1String("folder")).toString(QStringLiteral("INBOX"));
    if (a.folder.isEmpty())
        a.folder = QStringLiteral("INBOX");
    a.webUrl = json.value(QLatin1String("webUrl")).toString();
    return a;
}

QString typeName(AccountType type)
{
    return type == AccountType::Imap ? QCoreApplication::translate("Account", "IMAP")
                                     : QCoreApplication::translate("Account", "Gmail");
}

} // namespace iec
