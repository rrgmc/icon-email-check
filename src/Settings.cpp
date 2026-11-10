#include "Settings.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>

namespace iec {

QList<Account> Settings::accounts() const
{
    const QJsonArray array = QJsonDocument::fromJson(QSettings().value(QStringLiteral("accounts")).toByteArray()).array();
    QList<Account> result;
    for (const QJsonValue &value : array) {
        const Account account = Account::fromJson(value.toObject());
        if (!account.id.isEmpty())
            result << account;
    }
    return result;
}

void Settings::setAccounts(const QList<Account> &accounts)
{
    QJsonArray array;
    for (const Account &account : accounts)
        array.append(account.toJson());
    QSettings().setValue(QStringLiteral("accounts"), QJsonDocument(array).toJson(QJsonDocument::Compact));
}

bool Settings::startHidden() const { return QSettings().value(QStringLiteral("app/startHidden"), true).toBool(); }
void Settings::setStartHidden(bool on) { QSettings().setValue(QStringLiteral("app/startHidden"), on); }

bool Settings::notifications() const { return QSettings().value(QStringLiteral("app/notifications"), true).toBool(); }
void Settings::setNotifications(bool on) { QSettings().setValue(QStringLiteral("app/notifications"), on); }

gmail::OAuthClient Settings::googleClient() const
{
    if (!googleClientSettings)
        return builtInGoogleClient();
    const gmail::OAuthClient custom = customGoogleClient();
    return custom.isValid() ? custom : builtInGoogleClient();
}

gmail::OAuthClient Settings::customGoogleClient() const
{
    QSettings s;
    return {s.value(QStringLiteral("google/clientId")).toString().trimmed(),
            s.value(QStringLiteral("google/clientSecret")).toString().trimmed()};
}

void Settings::setCustomGoogleClient(const gmail::OAuthClient &client)
{
    QSettings s;
    s.setValue(QStringLiteral("google/clientId"), client.id.trimmed());
    s.setValue(QStringLiteral("google/clientSecret"), client.secret.trimmed());
}

gmail::OAuthClient Settings::builtInGoogleClient()
{
    return {QStringLiteral(IEC_GOOGLE_CLIENT_ID), QStringLiteral(IEC_GOOGLE_CLIENT_SECRET)};
}

QByteArray Settings::windowGeometry() const { return QSettings().value(QStringLiteral("window/geometry")).toByteArray(); }
void Settings::setWindowGeometry(const QByteArray &geometry) { QSettings().setValue(QStringLiteral("window/geometry"), geometry); }

} // namespace iec
