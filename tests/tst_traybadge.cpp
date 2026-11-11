#include "platform/TrayBadge.h"

#include <QGuiApplication>
#include <QIcon>
#include <QPainter>
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

    void paintsAccountDotsAlongTheTop()
    {
        QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QCOMPARE(paintAccountDots(image, QList<QColor>()), image);

        // One dot sits at the middle of the top edge.
        const QImage one = paintAccountDots(image, {Qt::red});
        QCOMPARE(one.size(), image.size());
        QCOMPARE(QColor(one.pixel(16, 3)), QColor(Qt::red));
        QCOMPARE(qAlpha(one.pixel(16, 28)), 0);

        // Three dots sit side by side, in their order; a fourth is left out.
        const QImage three = paintAccountDots(image, {Qt::red, Qt::green, Qt::blue, Qt::yellow});
        QCOMPARE(QColor(three.pixel(7, 3)), QColor(Qt::red));
        QCOMPARE(QColor(three.pixel(16, 3)), QColor(Qt::green));
        QCOMPARE(QColor(three.pixel(25, 3)), QColor(Qt::blue));
    }

    // Saves the icons with dots, at tray sizes, when IEC_GRAB_DIR names a
    // folder, to look at them.
    void grabsTheIcons()
    {
        const QString folder = qEnvironmentVariable("IEC_GRAB_DIR");
        if (folder.isEmpty())
            return;
        const QList<QColor> colours{QColor(0xef, 0x44, 0x44), QColor(0x22, 0xc5, 0x5e), QColor(0xf5, 0x9e, 0x0b)};
        const QString icons[] = {QStringLiteral(":/icons/tray.svg"), QStringLiteral(":/icons/tray-unread.svg")};
        const int sizes[] = {16, 18, 24, 36, 64};
        QImage sheet(8 + 72 * int(std::size(sizes)), 8 + 72 * 3 * int(std::size(icons)), QImage::Format_ARGB32);
        sheet.fill(QColor(0xdd, 0xdd, 0xdd));
        QPainter painter(&sheet);
        int y = 8;
        for (const QString &path : icons) {
            for (int n = 1; n <= 3; ++n) {
                int x = 8;
                for (const int size : sizes) {
                    const QImage image = QIcon(path).pixmap(QSize(size, size)).toImage();
                    painter.drawImage(x, y, paintTrayBadge(paintAccountDots(image, colours.mid(0, n)), 7));
                    x += 72;
                }
                y += 72;
            }
        }
        painter.end();
        QVERIFY(sheet.save(folder + QStringLiteral("/tray-dots.png")));
    }

    void cutsTheDotsOutOfTheIcon()
    {
        // Large enough for the rim to cover whole pixels.
        QImage image(64, 64, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
        const QImage dotted = paintAccountDots(image, {Qt::red});
        // The rim around the dot is transparent, and the rest is untouched.
        const qreal diameter = 64 * 0.2;
        const int left = int((64 - diameter) / 2);
        QCOMPARE(qAlpha(dotted.pixel(left - 1, int(diameter / 2))), 0);
        QCOMPARE(dotted.pixel(2, 60), image.pixel(2, 60));
    }
};

QTEST_MAIN(TestTrayBadge)
#include "tst_traybadge.moc"
