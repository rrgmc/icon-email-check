#pragma once

#include <QColor>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace iec {

enum class AccountType {
    // Gmail through the Gmail API, after "Sign in with Google". The refresh
    // token is kept in the keychain.
    Gmail,
    // Any IMAP server, including Gmail with an app password. The password is
    // kept in the keychain.
    Imap,
};

enum class ImapSecurity {
    Tls,        // TLS from the start, usually port 993
    StartTls,   // plain, then STARTTLS, usually port 143
    None,       // no encryption; only for servers on this computer
};

// One mailbox to check. Saved in the settings, without its secret.
struct Account
{
    QString id;
    QString name;
    AccountType type = AccountType::Gmail;
    bool enabled = true;
    bool notify = true;
    int intervalMinutes = 5;
    // Marks the account's unread mail on the tray icon. Invalid until one is
    // chosen; Settings gives every saved account one.
    QColor colour;

    // Gmail: the address of the signed-in account. IMAP: shown in the menu.
    QString email;
    // Gmail: a search such as "is:unread category:primary". Empty counts the
    // unread messages in the inbox.
    QString gmailQuery;

    // IMAP only.
    QString host;
    int port = 993;
    ImapSecurity security = ImapSecurity::Tls;
    QString username;
    QString folder = QStringLiteral("INBOX");

    // Opened when the account is clicked. Empty uses Gmail's inbox for Gmail
    // accounts, and nothing for IMAP accounts.
    QString webUrl;

    // The name to show: name, else the address, else the host.
    QString displayName() const;
    // The page to open for this account, or empty.
    QString inboxUrl() const;

    QJsonObject toJson() const;
    static Account fromJson(const QJsonObject &json);
};

QString typeName(AccountType type);

// A colour for a new account: the first of a fixed palette that no account in
// accounts uses yet, or one picked by their number when all are used.
QColor nextAccountColour(const QList<Account> &accounts);

} // namespace iec
