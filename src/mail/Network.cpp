#include "mail/Network.h"

#include <QCoreApplication>
#include <QNetworkAccessManager>

namespace iec {

QNetworkAccessManager *network()
{
    static QNetworkAccessManager *manager = [] {
        auto *m = new QNetworkAccessManager(QCoreApplication::instance());
        m->setTransferTimeout(30 * 1000);
        return m;
    }();
    return manager;
}

} // namespace iec
