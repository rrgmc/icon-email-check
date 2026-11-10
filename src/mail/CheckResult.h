#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace iec {

// One unread message, as the menu and notifications show it.
struct MailSummary
{
    QString id;   // Gmail message id, or IMAP UID
    QString from;
    QString subject;
};

// The result of one check of one account.
struct CheckResult
{
    bool ok = false;
    QString error;
    // True when the server refused the password or sign-in, so checking again
    // will not help until the user fixes the account.
    bool authFailed = false;
    int unread = 0;
    // The ids of the unread messages this check saw, newest first. A new id
    // means new mail. It may hold fewer ids than unread.
    QStringList unreadIds;
    // The newest unread messages, newest first, at most a few.
    QList<MailSummary> recent;
};

} // namespace iec
