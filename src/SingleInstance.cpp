#include "SingleInstance.h"

#include "platform/Foreground.h"

#include <QCryptographicHash>
#include <QDeadlineTimer>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include <QThread>

namespace iec {

namespace {

// The socket name is per user, so two users on one computer each get their
// own instance.
QString socketName()
{
    const QByteArray key = QDir::homePath().toUtf8() + "|icon-email-check";
    return QStringLiteral("icon-email-check-")
        + QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex().left(16));
}

bool askToShow(qint64 holderPid)
{
    platform::allowForegroundFor(holderPid);
    QLocalSocket socket;
    socket.connectToServer(socketName());
    if (!socket.waitForConnected(500))
        return false;
    socket.write("show\n");
    if (!socket.waitForBytesWritten(1000) || !socket.waitForReadyRead(2000))
        return false;
    return socket.readAll().startsWith("ok");
}

} // namespace

struct SingleInstance::Private
{
    std::unique_ptr<QLockFile> lock;
    QLocalServer *server = nullptr;
};

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
}

SingleInstance::~SingleInstance() = default;

bool SingleInstance::claim(bool *handedOver)
{
    *handedOver = false;
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(folder);
    d->lock = std::make_unique<QLockFile>(folder + QStringLiteral("/instance.lock"));
    // The lock is held for as long as the app runs, so it never goes stale by
    // age; QLockFile still takes over the lock of a process that is gone.
    d->lock->setStaleLockTime(0);

    // An instance that has just started may not listen yet, so a later start
    // tries for a few seconds.
    const QDeadlineTimer deadline(5000);
    while (!d->lock->tryLock(0)) {
        qint64 pid = 0;
        QString host;
        QString app;
        d->lock->getLockInfo(&pid, &host, &app);
        if (askToShow(pid)) {
            *handedOver = true;
            return false;
        }
        if (deadline.hasExpired())
            return false;
        QThread::msleep(250);
    }

    d->server = new QLocalServer(this);
    d->server->setSocketOptions(QLocalServer::UserAccessOption);
    // A crashed instance may leave its socket file behind on macOS and Linux.
    QLocalServer::removeServer(socketName());
    d->server->listen(socketName());
    connect(d->server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *socket = d->server->nextPendingConnection()) {
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
                if (!socket->canReadLine())
                    return;
                const QByteArray line = socket->readLine().trimmed();
                if (line == "show") {
                    socket->write("ok\n");
                    socket->flush();
                    emit activationRequested();
                }
                socket->disconnectFromServer();
            });
        }
    });
    return true;
}

} // namespace iec
