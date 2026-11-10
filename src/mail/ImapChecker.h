#pragma once

#include "mail/ImapProtocol.h"
#include "mail/MailChecker.h"

#include <QMap>
#include <QTimer>

class QSslSocket;

namespace iec {

// Checks an IMAP mailbox: LOGIN, EXAMINE (read-only, so nothing is marked
// read), UID SEARCH UNSEEN for the unread messages, and UID FETCH of the
// newest ones' From and Subject with BODY.PEEK.
class ImapChecker : public MailChecker
{
    Q_OBJECT

public:
    explicit ImapChecker(QObject *parent = nullptr);
    ~ImapChecker() override;

    void start(const Account &account, const QString &secret) override;

private:
    enum class Step { Greeting, StartTls, Login, Examine, Search, Fetch, Logout, Done };

    void send(const QByteArray &command);
    void onReadyRead();
    void handle(const imap::Response &response);
    void advance();
    void fail(const QString &error, bool authFailed = false);
    void finish();

    Account m_account;
    QString m_password;
    QSslSocket *m_socket = nullptr;
    QTimer m_timeout;
    imap::Reader m_reader;
    Step m_step = Step::Greeting;
    int m_tagNumber = 0;
    QByteArray m_tag;
    qint64 m_uidValidity = 0;
    QList<qint64> m_unseen;
    QMap<qint64, MailSummary> m_fetched;
    CheckResult m_result;
    bool m_done = false;
};

} // namespace iec
