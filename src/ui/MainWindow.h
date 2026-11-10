#pragma once

#include <QMainWindow>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QVBoxLayout;

namespace iec {

class MailMonitor;
class SecretStore;
class Settings;

namespace platform {
class Tray;
}

// The window that manages the accounts and the settings. The app lives in the
// tray; this window comes and goes. Closing it hides it while a tray is
// present, and quits otherwise.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Settings *settings, SecretStore *secrets, MailMonitor *monitor, QWidget *parent = nullptr);
    ~MainWindow() override;

    // The tray, once it exists. With a tray, closing hides the window.
    void setTray(platform::Tray *tray);

    void showAndRaise();
    void hideToTray();

    void addAccount();
    void editAccount(const QString &id);
    // Shows the About window: the version, the author and the links.
    void showAbout();

    // Saves the window's place. Called before the app quits.
    void saveState();

protected:
    void closeEvent(QCloseEvent *event) override;
    // Watches the app for its quit event. Qt 6 closes every window before it
    // quits, and a window that refuses the close stops the quit, so a close
    // during a quit must go through.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void buildUi();
    // The "Google sign-in" section, when the build has IEC_GOOGLE_CLIENT_SETTINGS.
    void buildGoogleSettings(QVBoxLayout *layout);
    void refresh();
    void removeAccount(const QString &id);
    void moveAccount(int delta);
    QString selectedId() const;
    void updateButtons();
    void updateStartAtSignIn();
    void setStartAtSignIn(bool on);
    void saveGoogleClient();

    Settings *m_settings;
    SecretStore *m_secrets;
    MailMonitor *m_monitor;
    platform::Tray *m_tray = nullptr;
    bool m_quitting = false;

    QTreeWidget *m_list = nullptr;
    QPushButton *m_add = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_remove = nullptr;
    QPushButton *m_up = nullptr;
    QPushButton *m_down = nullptr;
    QPushButton *m_check = nullptr;
    QPushButton *m_open = nullptr;
    QCheckBox *m_startAtSignIn = nullptr;
    QLabel *m_signInApproval = nullptr;
    QCheckBox *m_startHidden = nullptr;
    QCheckBox *m_notifications = nullptr;
    QLineEdit *m_clientId = nullptr;
    QLineEdit *m_clientSecret = nullptr;
};

} // namespace iec
