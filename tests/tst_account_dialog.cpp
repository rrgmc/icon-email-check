#include "accounts/SecretStore.h"
#include "ui/AccountDialog.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>

using namespace iec;

namespace {

gmail::OAuthClient someClient() { return {QStringLiteral("id"), QStringLiteral("secret")}; }

QLineEdit *fieldWithPlaceholder(const QWidget &dialog, const QString &start)
{
    for (QLineEdit *edit : dialog.findChildren<QLineEdit *>()) {
        if (edit->placeholderText().startsWith(start))
            return edit;
    }
    return nullptr;
}

// Saves a picture of the dialog when IEC_GRAB_DIR names a folder, to look at
// the layout.
void grab(QWidget &dialog, const QString &name)
{
    const QString folder = qEnvironmentVariable("IEC_GRAB_DIR");
    if (!folder.isEmpty())
        dialog.grab().save(folder + QLatin1Char('/') + name + QStringLiteral(".png"));
}

} // namespace

class TestAccountDialog : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showsTheGmailRowsOnly()
    {
        SecretStore secrets;
        Account account;
        account.type = AccountType::Gmail;
        account.email = QStringLiteral("someone.with.a.long.address@gmail.com");
        AccountDialog dialog(account, true, someClient, &secrets);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        grab(dialog, QStringLiteral("gmail"));

        QLineEdit *query = fieldWithPlaceholder(dialog, QStringLiteral("Empty: unread"));
        QLineEdit *server = fieldWithPlaceholder(dialog, QStringLiteral("imap.example.com"));
        QVERIFY(query && server);
        QVERIFY(query->isVisible());
        QVERIFY(!server->isVisible());

        // The address shows in full, on one line.
        QLabel *status = nullptr;
        for (QLabel *label : dialog.findChildren<QLabel *>()) {
            if (label->text().contains(account.email))
                status = label;
        }
        QVERIFY(status);
        QVERIFY(status->isVisible());
        QVERIFY(status->width() >= status->sizeHint().width());

        // Text fields fill the field column instead of keeping a narrow width.
        for (QLineEdit *edit : dialog.findChildren<QLineEdit *>()) {
            // A spin box's own text field keeps the spin box's width.
            if (edit->isVisible() && !qobject_cast<QAbstractSpinBox *>(edit->parentWidget()))
                QVERIFY2(edit->width() >= 300, qPrintable(edit->placeholderText()));
        }
    }

    void showsTheImapRowsOnly()
    {
        SecretStore secrets;
        Account account;
        account.type = AccountType::Imap;
        account.host = QStringLiteral("imap.gmail.com");
        AccountDialog dialog(account, true, someClient, &secrets);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        grab(dialog, QStringLiteral("imap"));

        QLineEdit *query = fieldWithPlaceholder(dialog, QStringLiteral("Empty: unread"));
        QLineEdit *server = fieldWithPlaceholder(dialog, QStringLiteral("imap.example.com"));
        QVERIFY(query && server);
        QVERIFY(!query->isVisible());
        QVERIFY(server->isVisible());
        for (QPushButton *button : dialog.findChildren<QPushButton *>())
            QVERIFY(!button->text().startsWith(QStringLiteral("Sign in")) || !button->isVisible());
        for (QLineEdit *edit : dialog.findChildren<QLineEdit *>()) {
            // A spin box's own text field keeps the spin box's width.
            if (edit->isVisible() && !qobject_cast<QAbstractSpinBox *>(edit->parentWidget()))
                QVERIFY2(edit->width() >= 300, qPrintable(edit->placeholderText()));
        }
    }
};

QTEST_MAIN(TestAccountDialog)
#include "tst_account_dialog.moc"
