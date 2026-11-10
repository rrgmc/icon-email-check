#include "platform/TrayBadge.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

namespace iec::platform {

namespace {

// The red of an unread badge.
const QColor attentionColour(0xd9, 0x48, 0x3b);

// The tray sizes of Windows, at 100% to 200% scaling, and of Linux panels.
constexpr int traySizes[] = {16, 20, 24, 32, 40, 48, 64};

} // namespace

QString trayBadgeText(int count) { return count > 99 ? QStringLiteral("99+") : QString::number(count); }

QImage paintTrayBadge(const QImage &image, int count)
{
    if (count <= 0 || image.isNull())
        return image;
    QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const qreal side = qMin(result.width(), result.height());
    const QString text = trayBadgeText(count);

    // The badge is two thirds of the icon high, so the number stays legible
    // on a 16 pixel icon, and as wide as the text needs, up to the icon.
    QFont font;
    font.setBold(true);
    font.setPixelSize(qMax(6, qRound(side * 0.55)));
    const QFontMetricsF metrics(font);
    const qreal height = side * 2 / 3;
    const qreal width = qMin<qreal>(result.width(), qMax(height, metrics.horizontalAdvance(text) + height / 3));
    const QRectF badge(result.width() - width, result.height() - height, width, height);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    // A white rim keeps the badge apart from the icon under it.
    QPainterPath shape;
    shape.addRoundedRect(badge.adjusted(0.5, 0.5, -0.5, -0.5), height / 2, height / 2);
    painter.setPen(QPen(Qt::white, qMax(1.0, side / 32)));
    painter.setBrush(attentionColour);
    painter.drawPath(shape);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(badge, Qt::AlignCenter, text);
    return result;
}

QImage paintAccountDots(const QImage &image, const QList<QColor> &colours)
{
    if (colours.isEmpty() || image.isNull())
        return image;
    QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const qreal side = qMin(result.width(), result.height());
    const int dots = qMin<int>(maxAccountDots, int(colours.size()));
    // Three dots and their gaps fill the width, so the dots stay visible on a
    // 16 pixel icon. They sit in the margin above the envelope and overlap its
    // top edge a little; the badge is in the lower two thirds.
    const qreal diameter = side * 0.3;
    const qreal gap = side * 0.04;
    const qreal rim = qMax(1.0, side / 24);
    const qreal rowWidth = dots * diameter + (dots - 1) * gap;
    qreal x = (result.width() - rowWidth) / 2;

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < dots; ++i) {
        const QRectF dot(x, 0, diameter, diameter);
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.setBrush(Qt::black);
        painter.drawEllipse(dot.adjusted(-rim, -rim, rim, rim));
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setBrush(colours.at(i));
        painter.drawEllipse(dot);
        x += diameter + gap;
    }
    return result;
}

QIcon withTrayBadge(const QIcon &icon, int count, const QList<QColor> &colours)
{
    if (count <= 0 && colours.isEmpty())
        return icon;
    QIcon result;
    for (const int size : traySizes) {
        const QImage image = icon.pixmap(QSize(size, size)).toImage();
        if (!image.isNull())
            result.addPixmap(QPixmap::fromImage(paintTrayBadge(paintAccountDots(image, colours), count)));
    }
    return result;
}

} // namespace iec::platform
