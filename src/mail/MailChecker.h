#pragma once

#include "accounts/Account.h"
#include "mail/CheckResult.h"

#include <QObject>

namespace iec {

// Reads the unread count of one account once. A checker is used for one
// check and then deleted.
class MailChecker : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    // secret is the account's password or refresh token. finished() comes
    // once, later, also on failure.
    virtual void start(const Account &account, const QString &secret) = 0;

    // How many of the newest unread messages a check describes.
    static constexpr int recentCount = 5;

signals:
    void finished(const iec::CheckResult &result);
};

} // namespace iec
