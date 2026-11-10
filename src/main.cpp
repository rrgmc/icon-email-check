#include "App.h"
#include "Settings.h"
#include "SingleInstance.h"
#include "accounts/SecretStore.h"
#include "mail/MailMonitor.h"
#include "platform/LoginItem.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("rrgmc"));
    QApplication::setOrganizationDomain(QStringLiteral("rrgmc.com"));
    QApplication::setApplicationName(QStringLiteral("IconEmailCheck"));
    QApplication::setApplicationDisplayName(QStringLiteral("Icon Email Check"));
    QApplication::setApplicationVersion(QStringLiteral(IEC_VERSION));
#if !defined(Q_OS_MACOS)
    // On macOS the Dock shows the bundle's icon file, which is drawn on the
    // macOS icon grid; a run-time icon would replace it with a larger one.
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Counts unread email in the tray."));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption hiddenOption(
        QStringLiteral("hidden"),
        QStringLiteral("Start in the tray without the window, as at sign-in. Starting the app again shows it."));
    parser.addOption(hiddenOption);
    parser.process(app);

    // A second start shows the running instance's window and exits.
    iec::SingleInstance instance;
    bool handedOver = false;
    if (!instance.claim(&handedOver))
        return handedOver ? 0 : 1;

    iec::Settings settings;
    iec::SecretStore secrets;
    iec::MailMonitor monitor(&secrets, [&settings] { return settings.googleClient(); });
    iec::MainWindow window(&settings, &secrets, &monitor);
    iec::App tray(&settings, &monitor, &window);
    monitor.setAccounts(settings.accounts());

    QObject::connect(&instance, &iec::SingleInstance::activationRequested, &window, &iec::MainWindow::showAndRaise);
    QObject::connect(&app, &QApplication::aboutToQuit, &window, &iec::MainWindow::saveState);

    tray.startTray();

    // Started at sign-in, the app waits in the tray until it is called.
    // Windows and Linux pass --hidden; a macOS login item gets no arguments,
    // so the launch event tells instead. Without accounts the window shows,
    // since there is nothing to check yet. With no tray, App shows it once it
    // stops waiting for one.
    const bool hidden = parser.isSet(hiddenOption) || settings.startHidden();
    iec::platform::loginitem::whenLaunched([&](bool atSignIn) {
        const bool noAccounts = settings.accounts().isEmpty();
        if (noAccounts || !(hidden || atSignIn))
            window.showAndRaise();
    });

    return QApplication::exec();
}
