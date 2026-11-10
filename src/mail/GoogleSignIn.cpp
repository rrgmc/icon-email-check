#include "mail/GoogleSignIn.h"

#include "mail/Network.h"

#include <QDesktopServices>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

namespace iec {

namespace {

const QUrl authUrl(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"));
const QUrl tokenUrl(QStringLiteral("https://oauth2.googleapis.com/token"));
const QUrl profileUrl(QStringLiteral("https://gmail.googleapis.com/gmail/v1/users/me/profile"));

QNetworkRequest formRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    return request;
}

QByteArray page(const QString &title, const QString &text)
{
    const QByteArray body = QStringLiteral("<!doctype html><html><head><meta charset=\"utf-8\"><title>%1</title>"
                                           "<style>body{font-family:system-ui,sans-serif;margin:3em;color:#222}"
                                           "</style></head><body><h2>%1</h2><p>%2</p></body></html>")
                                .arg(title.toHtmlEscaped(), text.toHtmlEscaped())
                                .toUtf8();
    return "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\nContent-Length: "
        + QByteArray::number(body.size()) + "\r\n\r\n" + body;
}

QString describe(const gmail::TokenResponse &token, QNetworkReply *reply)
{
    if (!token.errorDescription.isEmpty())
        return token.errorDescription;
    if (!token.error.isEmpty())
        return token.error;
    return reply->errorString();
}

} // namespace

GoogleSignIn::GoogleSignIn(QObject *parent)
    : QObject(parent)
{
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(5 * 60 * 1000);
    connect(&m_timeout, &QTimer::timeout, this,
            [this] { done(false, {}, {}, tr("The sign-in was not finished within five minutes.")); });
}

GoogleSignIn::~GoogleSignIn() = default;

void GoogleSignIn::start(const gmail::OAuthClient &client)
{
    m_client = client;
    m_server = new QTcpServer(this);
    if (!m_server->listen(QHostAddress::LocalHost, 0)) {
        QMetaObject::invokeMethod(
            this, [this] { done(false, {}, {}, tr("No local port could be opened for the sign-in.")); },
            Qt::QueuedConnection);
        return;
    }
    connect(m_server, &QTcpServer::newConnection, this, &GoogleSignIn::onConnection);
    m_redirectUri = QStringLiteral("http://127.0.0.1:%1").arg(m_server->serverPort());
    m_verifier = gmail::randomToken();
    m_state = gmail::randomToken();

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), m_client.id);
    query.addQueryItem(QStringLiteral("redirect_uri"), m_redirectUri);
    query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    query.addQueryItem(QStringLiteral("scope"), QString::fromLatin1(gmail::scope));
    query.addQueryItem(QStringLiteral("code_challenge"), QString::fromLatin1(gmail::pkceChallenge(m_verifier)));
    query.addQueryItem(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
    query.addQueryItem(QStringLiteral("state"), QString::fromLatin1(m_state));
    // offline and consent make Google return a refresh token every time.
    query.addQueryItem(QStringLiteral("access_type"), QStringLiteral("offline"));
    query.addQueryItem(QStringLiteral("prompt"), QStringLiteral("consent select_account"));
    QUrl url = authUrl;
    url.setQuery(query);
    m_timeout.start();
    if (!QDesktopServices::openUrl(url))
        done(false, {}, {}, tr("The browser could not be opened."));
}

void GoogleSignIn::cancel() { done(false, {}, {}, tr("The sign-in was cancelled.")); }

void GoogleSignIn::onConnection()
{
    while (QTcpSocket *socket = m_server->nextPendingConnection()) {
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            // Wait for the request line and headers.
            if (!socket->property("buffer").toByteArray().contains("\r\n\r\n")) {
                socket->setProperty("buffer", socket->property("buffer").toByteArray() + socket->readAll());
                if (!socket->property("buffer").toByteArray().contains("\r\n\r\n"))
                    return;
            }
            const QByteArray request = socket->property("buffer").toByteArray();
            const QList<QByteArray> words = request.left(request.indexOf("\r\n")).split(' ');
            const QUrl target(QString::fromLatin1(words.value(1)));
            const QUrlQuery query(target);
            if (target.path() != QLatin1String("/") || m_finished) {
                socket->write("HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
                socket->disconnectFromHost();
                return;
            }
            const QString error = query.queryItemValue(QStringLiteral("error"));
            const QByteArray state = query.queryItemValue(QStringLiteral("state")).toLatin1();
            const QByteArray code = query.queryItemValue(QStringLiteral("code"), QUrl::FullyDecoded).toLatin1();
            if (!error.isEmpty() || state != m_state || code.isEmpty()) {
                socket->write(page(tr("Sign-in failed"), tr("You can close this page and try again.")));
                socket->disconnectFromHost();
                done(false, {}, {},
                     error.isEmpty() ? tr("Google's answer did not match the sign-in.")
                                     : tr("Google refused the sign-in: %1").arg(error));
                return;
            }
            socket->write(page(tr("Signed in"), tr("You can close this page and go back to Icon Email Check.")));
            socket->disconnectFromHost();
            exchange(code);
        });
    }
}

void GoogleSignIn::exchange(const QByteArray &code)
{
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("code"), QString::fromLatin1(code));
    form.addQueryItem(QStringLiteral("client_id"), m_client.id);
    if (!m_client.secret.isEmpty())
        form.addQueryItem(QStringLiteral("client_secret"), m_client.secret);
    form.addQueryItem(QStringLiteral("redirect_uri"), m_redirectUri);
    form.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    form.addQueryItem(QStringLiteral("code_verifier"), QString::fromLatin1(m_verifier));
    QNetworkReply *reply = network()->post(formRequest(tokenUrl), form.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        const gmail::TokenResponse token = gmail::parseTokenResponse(reply->readAll());
        if (token.refreshToken.isEmpty() || token.accessToken.isEmpty()) {
            done(false, {}, {}, tr("Google did not give a sign-in token: %1").arg(describe(token, reply)));
            return;
        }
        readProfile(token.accessToken, token.refreshToken);
    });
}

void GoogleSignIn::readProfile(const QString &accessToken, const QString &refreshToken)
{
    QNetworkRequest request(profileUrl);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toLatin1());
    QNetworkReply *reply = network()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, refreshToken] {
        reply->deleteLater();
        const QByteArray body = reply->readAll();
        const QString email = gmail::parseProfileEmail(body);
        if (email.isEmpty()) {
            const QString apiError = gmail::parseApiError(body);
            done(false, {}, {},
                 tr("Gmail could not be read: %1").arg(apiError.isEmpty() ? reply->errorString() : apiError));
            return;
        }
        done(true, refreshToken, email, {});
    });
}

void GoogleSignIn::done(bool ok, const QString &refreshToken, const QString &email, const QString &error)
{
    if (m_finished)
        return;
    m_finished = true;
    m_timeout.stop();
    if (m_server)
        m_server->close();
    emit finished(ok, refreshToken, email, error);
}

namespace google {

void refreshAccessToken(const gmail::OAuthClient &client, const QString &refreshToken, QObject *context,
                        std::function<void(const QString &, int, const QString &, bool)> done)
{
    QUrlQuery form;
    form.addQueryItem(QStringLiteral("client_id"), client.id);
    if (!client.secret.isEmpty())
        form.addQueryItem(QStringLiteral("client_secret"), client.secret);
    form.addQueryItem(QStringLiteral("refresh_token"), refreshToken);
    form.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    QNetworkReply *reply = network()->post(formRequest(tokenUrl), form.toString(QUrl::FullyEncoded).toUtf8());
    QObject::connect(reply, &QNetworkReply::finished, context, [reply, done = std::move(done)] {
        reply->deleteLater();
        const gmail::TokenResponse token = gmail::parseTokenResponse(reply->readAll());
        if (!token.accessToken.isEmpty()) {
            done(token.accessToken, token.expiresIn, {}, false);
            return;
        }
        // invalid_grant: the token was revoked, expired, or the password changed.
        const bool authFailed = token.error == QLatin1String("invalid_grant")
            || token.error == QLatin1String("unauthorized_client") || token.error == QLatin1String("invalid_client");
        done({}, 0, describe(token, reply), authFailed);
    });
}

} // namespace google

} // namespace iec
