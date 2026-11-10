#pragma once

#include "mail/GmailApi.h"
#include "mail/MailChecker.h"

#include <QUrlQuery>

class QNetworkReply;

namespace iec {

// Checks a Gmail account through the Gmail API. Without a search, the count
// is the inbox label's messagesUnread, which is exact. With a search, it is
// the number of messages the search lists, up to maxListed.
class GmailChecker : public MailChecker
{
    Q_OBJECT

public:
    explicit GmailChecker(const gmail::OAuthClient &client, QObject *parent = nullptr);

    void start(const Account &account, const QString &secret) override;

    static constexpr int maxListed = 2000;

private:
    void withToken(std::function<void()> next);
    void get(const QString &path, const QUrlQuery &query, std::function<void(const QByteArray &)> next);
    void readLabel();
    void listPage(const QString &pageToken);
    void readDetails();
    void fail(const QString &error, bool authFailed = false);
    void finish();

    gmail::OAuthClient m_client;
    Account m_account;
    QString m_refreshToken;
    bool m_retried = false;
    int m_labelUnread = -1;
    QStringList m_ids;
    int m_pending = 0;
    CheckResult m_result;
    bool m_done = false;
};

} // namespace iec
