#include "platform/TrayBadge.h"

#include <QGuiApplication>
#include <QTest>

using namespace iec::platform;

class TestTrayBadge : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void writesTheCount()
    {
        QCOMPARE(trayBadgeText(7), QStringLiteral("7"));
        QCOMPARE(trayBadgeText(99), QStringLiteral("99"));
        QCOMPARE(trayBadgeText(100), QStringLiteral("99+"));
    }

    void paintsOnlyACount()
    {
        QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QCOMPARE(paintTrayBadge(image, 0), image);
        const QImage badged = paintTrayBadge(image, 3);
        QCOMPARE(badged.size(), image.size());
        // The badge sits at the lower right and leaves the upper left alone.
        QVERIFY(qAlpha(badged.pixel(26, 26)) > 0);
        QCOMPARE(qAlpha(badged.pixel(2, 2)), 0);
    }
};

QTEST_MAIN(TestTrayBadge)
#include "tst_traybadge.moc"
