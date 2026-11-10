#include "mail/ImapChecker.h"

#include "mail/MimeHeaders.h"

#include <QSslSocket>
#include <algorithm>

namespace iec {

ImapChecker::ImapChecker(QObject *parent)
    : MailChecker(parent)
{
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(60 * 1000);
    connect(&m_timeout, &QTimer::timeout, this, [this] { fail(tr("The server did not answer in time.")); });
}

ImapChecker::~ImapChecker() = default;

void ImapChecker::start(const Account &account, const QString &secret)
{
    m_account = account;
    m_password = secret;
    m_socket = new QSslSocket(this);
    connect(m_socket, &QSslSocket::readyRead, this, &ImapChecker::onReadyRead);
    connect(m_socket, &QSslSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        // The server closes the connection after LOGOUT.
        if (m_step == Step::Logout || m_step == Step::Done) {
            finish();
            return;
        }
        fail(m_socket->errorString());
    });
    connect(m_socket, &QSslSocket::sslErrors, this, [this](const QList<QSslError> &errors) {
        fail(errors.isEmpty() ? tr("The server's certificate is not trusted.") : errors.first().errorString());
    });
    connect(m_socket, &QSslSocket::encrypted, this, [this] {
        // After STARTTLS the server sends nothing, so log in at once.
        if (m_step == Step::StartTls)
            advance();
    });
    connect(m_socket, &QSslSocket::disconnected, this, [this] {
        if (m_step == Step::Logout || m_step == Step::Done)
            finish();
        else
            fail(tr("The server closed the connection."));
    });

    m_timeout.start();
    m_step = Step::Greeting;
    const quint16 port = quint16(account.port > 0 ? account.port : 993);
    if (account.security == ImapSecurity::Tls)
        m_socket->connectToHostEncrypted(account.host, port);
    else
        m_socket->connectToHost(account.host, port);
}

void ImapChecker::send(const QByteArray &command)
{
    m_tag = "a" + QByteArray::number(++m_tagNumber);
    m_socket->write(m_tag + ' ' + command + "\r\n");
}

void ImapChecker::onReadyRead()
{
    m_reader.feed(m_socket->readAll());
    imap::Response response;
    while (!m_done && m_reader.next(&response))
        handle(response);
}

void ImapChecker::handle(const imap::Response &response)
{
    const QByteArray tag = response.tag();
    const QByteArray status = response.status();

    if (m_step == Step::Greeting) {
        if (tag != "*")
            return;
        if (status == "OK" || status == "PREAUTH") {
            advance();
        } else {
            fail(tr("The server refused the connection: %1").arg(QString::fromUtf8(response.text)));
        }
        return;
    }

    if (tag == "*") {
        if (status == "BYE" && m_step != Step::Logout) {
            fail(tr("The server ended the session: %1").arg(QString::fromUtf8(response.text.mid(6))));
            return;
        }
        if (m_step == Step::Examine) {
            const qint64 validity = imap::numberAfter(response.text, "UIDVALIDITY");
            if (validity >= 0)
                m_uidValidity = validity;
        } else if (m_step == Step::Search) {
            m_unseen += imap::parseSearch(response.text);
        } else if (m_step == Step::Fetch && !response.literals.isEmpty()) {
            const qint64 uid = imap::numberAfter(response.text, "UID");
            if (uid >= 0) {
                const QByteArray &headers = response.literals.first();
                MailSummary summary;
                summary.id = QString::number(uid);
                summary.from = mime::displayFrom(mime::headerField(headers, "From"));
                summary.subject = mime::headerField(headers, "Subject");
                m_fetched.insert(uid, summary);
            }
        }
        return;
    }

    if (tag != m_tag)
        return;
    const QString reason = QString::fromUtf8(response.text.mid(tag.size() + 1 + status.size())).trimmed();
    if (status != "OK") {
        if (m_step == Step::Login)
            fail(tr("The server refused the user name or password: %1").arg(reason), true);
        else if (m_step == Step::Examine)
            fail(tr("The folder \"%1\" could not be opened: %2").arg(m_account.folder, reason));
        else if (m_step == Step::StartTls)
            fail(tr("The server does not offer STARTTLS: %1").arg(reason));
        else
            fail(tr("The server reported an error: %1").arg(reason));
        return;
    }
    if (m_step == Step::StartTls) {
        m_socket->startClientEncryption();
        return;    // advance() follows the encrypted() signal
    }
    advance();
}

void ImapChecker::advance()
{
    switch (m_step) {
    case Step::Greeting:
        if (m_account.security == ImapSecurity::StartTls) {
            m_step = Step::StartTls;
            send("STARTTLS");
            return;
        }
        [[fallthrough]];
    case Step::StartTls:
        m_step = Step::Login;
        send("LOGIN " + imap::quote(m_account.username) + ' ' + imap::quote(m_password));
        return;
    case Step::Login:
        m_step = Step::Examine;
        send("EXAMINE " + imap::quote(m_account.folder.isEmpty() ? QStringLiteral("INBOX") : m_account.folder));
        return;
    case Step::Examine:
        m_step = Step::Search;
        send("UID SEARCH UNSEEN");
        return;
    case Step::Search: {
        std::sort(m_unseen.begin(), m_unseen.end(), std::greater<>());
        m_result.unread = int(m_unseen.size());
        for (const qint64 uid : std::as_const(m_unseen))
            m_result.unreadIds << QStringLiteral("%1:%2").arg(m_uidValidity).arg(uid);
        if (!m_unseen.isEmpty()) {
            QByteArray set;
            for (qsizetype i = 0; i < m_unseen.size() && i < recentCount; ++i)
                set += (set.isEmpty() ? "" : ",") + QByteArray::number(m_unseen.at(i));
            m_step = Step::Fetch;
            send("UID FETCH " + set + " (UID BODY.PEEK[HEADER.FIELDS (FROM SUBJECT)])");
            return;
        }
        [[fallthrough]];
    }
    case Step::Fetch:
        for (qsizetype i = 0; i < m_unseen.size() && i < recentCount; ++i) {
            const auto found = m_fetched.constFind(m_unseen.at(i));
            if (found != m_fetched.constEnd()) {
                MailSummary summary = *found;
                summary.id = QStringLiteral("%1:%2").arg(m_uidValidity).arg(summary.id);
                m_result.recent << summary;
            }
        }
        m_result.ok = true;
        m_step = Step::Logout;
        send("LOGOUT");
        return;
    case Step::Logout:
        finish();
        return;
    case Step::Done:
        return;
    }
}

void ImapChecker::fail(const QString &error, bool authFailed)
{
    if (m_done)
        return;
    m_result = CheckResult{};
    m_result.ok = false;
    m_result.error = error;
    m_result.authFailed = authFailed;
    m_step = Step::Done;
    finish();
}

void ImapChecker::finish()
{
    if (m_done)
        return;
    m_done = true;
    m_step = Step::Done;
    m_timeout.stop();
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
    }
    emit finished(m_result);
}

} // namespace iec
