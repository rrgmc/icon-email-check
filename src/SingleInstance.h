#pragma once

#include <QObject>
#include <QString>

#include <memory>

namespace iec {

// Keeps one running instance per user. The first start takes a lock file in
// the app's data folder and listens on a local socket. A later start finds
// the lock held, asks the running instance to show its window, and exits.
// A crashed instance's lock is taken over, since its process no longer runs.
class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);
    ~SingleInstance() override;

    // True when this process is now the running instance. False when another
    // instance runs; it was asked to show its window if handedOver is set.
    bool claim(bool *handedOver);

signals:
    // A later start asked this instance to show its window.
    void activationRequested();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace iec
