#include "mail/GmailChecker.h"

#include "mail/GoogleSignIn.h"
#include "mail/Network.h"

#include <QDateTime>
#include <QHash>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace iec {

namespace {

const QString apiBase = QStringLiteral("https://gmail.googleapis.com/gmail/v1/users/me/");

struct CachedToken
{
    QString accessToken;
    QDateTime expires;
};

// Access tokens last an hour, so one serves many checks. Keyed by account id.
QHash<QString, CachedToken> &tokens()
{
    static QHash<QString, CachedToken> cache;
    return cache;
}

// From and Subject never change, so each message is read once per run.
QHash<QString, MailSummary> &summaries()
{
    static QHash<QString, MailSummary> cache;
    return cache;
}

} // namespace

GmailChecker::GmailChecker(const gmail::OAuthClient &client, QObject *parent)
    : MailChecker(parent)
    , m_client(client)
{
}

void GmailChecker::start(const Account &account, const QString &secret)
{
    m_account = account;
    m_refreshToken = secret;
    if (!m_client.isValid()) {
        QMetaObject::invokeMethod(
            this, [this] { fail(tr("No Google OAuth client is set. Set one in the settings.")); },
            Qt::QueuedConnection);
        return;
    }
    withToken([this] {
        if (m_account.gmailQuery.trimmed().isEmpty())
            readLabel();
        else
            listPage({});
    });
}

void GmailChecker::withToken(std::function<void()> next)
{
    const CachedToken cached = tokens().value(m_account.id);
    if (!cached.accessToken.isEmpty() && cached.expires > QDateTime::currentDateTimeUtc().addSecs(60)) {
        next();
        return;
    }
    google::refreshAccessToken(m_client, m_refreshToken, this,
                               [this, next = std::move(next)](const QString &accessToken, int expiresIn,
                                                              const QString &error, bool authFailed) {
                                   if (accessToken.isEmpty()) {
                                       fail(authFailed ? tr("Google no longer accepts the sign-in. Sign in again. (%1)")
                                                             .arg(error)
                                                       : error,
                                            authFailed);
                                       return;
                                   }
                                   tokens().insert(m_account.id,
                                                   {accessToken, QDateTime::currentDateTimeUtc().addSecs(
                                                                     expiresIn > 0 ? expiresIn : 3000)});
                                   next();
                               });
}

void GmailChecker::get(const QString &path, const QUrlQuery &query, std::function<void(const QByteArray &)> next)
{
    QUrl url(apiBase + path);
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + tokens().value(m_account.id).accessToken.toLatin1());
    QNetworkReply *reply = network()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, path, query, next = std::move(next)]() mutable {
        reply->deleteLater();
        if (m_done)
            return;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        if (status == 401 && !m_retried) {
            // The access token expired early or was revoked: get a new one once.
            m_retried = true;
            tokens().remove(m_account.id);
            withToken([this, path, query, next = std::move(next)]() mutable { get(path, query, std::move(next)); });
            return;
        }
        if (reply->error() != QNetworkReply::NoError || status != 200) {
            const QString apiError = gmail::parseApiError(body);
            fail(apiError.isEmpty() ? reply->errorString() : apiError, status == 401);
            return;
        }
        next(body);
    });
}

void GmailChecker::readLabel()
{
    get(QStringLiteral("labels/INBOX"), {}, [this](const QByteArray &body) {
        m_labelUnread = gmail::parseLabelUnread(body);
        listPage({});
    });
}

void GmailChecker::listPage(const QString &pageToken)
{
    QUrlQuery query;
    const QString search = m_account.gmailQuery.trimmed();
    if (search.isEmpty()) {
        query.addQueryItem(QStringLiteral("labelIds"), QStringLiteral("INBOX"));
        query.addQueryItem(QStringLiteral("labelIds"), QStringLiteral("UNREAD"));
    } else {
        // QUrlQuery encodes & and =, but a + would reach Google as a space.
        query.addQueryItem(QStringLiteral("q"), QString(search).replace(QLatin1Char('+'), QLatin1String("%2B")));
    }
    query.addQueryItem(QStringLiteral("maxResults"), QStringLiteral("500"));
    if (!pageToken.isEmpty())
        query.addQueryItem(QStringLiteral("pageToken"), pageToken);
    get(QStringLiteral("messages"), query, [this](const QByteArray &body) {
        QString next;
        gmail::parseMessageList(body, &m_ids, &next);
        // The inbox count is exact, so its ids need only the first page,
        // which holds the newest messages.
        if (!next.isEmpty() && m_labelUnread < 0 && m_ids.size() < maxListed) {
            listPage(next);
            return;
        }
        m_result.unread = m_labelUnread >= 0 ? m_labelUnread : int(m_ids.size());
        m_result.unreadIds = m_ids;
        readDetails();
    });
}

void GmailChecker::readDetails()
{
    const QStringList newest = m_ids.mid(0, recentCount);
    QStringList missing;
    for (const QString &id : newest) {
        if (!summaries().contains(id))
            missing << id;
    }
    auto collect = [this, newest] {
        for (const QString &id : newest) {
            const auto found = summaries().constFind(id);
            if (found != summaries().constEnd())
                m_result.recent << *found;
        }
        m_result.ok = true;
        finish();
    };
    if (missing.isEmpty()) {
        collect();
        return;
    }
    m_pending = int(missing.size());
    for (const QString &id : std::as_const(missing)) {
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("format"), QStringLiteral("metadata"));
        query.addQueryItem(QStringLiteral("metadataHeaders"), QStringLiteral("From"));
        query.addQueryItem(QStringLiteral("metadataHeaders"), QStringLiteral("Subject"));
        get(QStringLiteral("messages/") + id, query, [this, collect](const QByteArray &body) {
            const MailSummary summary = gmail::parseMessageMetadata(body);
            if (!summary.id.isEmpty())
                summaries().insert(summary.id, summary);
            if (--m_pending == 0)
                collect();
        });
    }
}

void GmailChecker::fail(const QString &error, bool authFailed)
{
    if (m_done)
        return;
    m_result = CheckResult{};
    m_result.error = error;
    m_result.authFailed = authFailed;
    finish();
}

void GmailChecker::finish()
{
    if (m_done)
        return;
    m_done = true;
    emit finished(m_result);
}

} // namespace iec
