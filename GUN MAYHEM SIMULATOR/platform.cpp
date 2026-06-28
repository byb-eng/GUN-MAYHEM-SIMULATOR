#include "platform.h"
#include <QColor>
#include <QPen>

Platform::Platform(int x, int y, int width, int height)
    : m_rect(x, y, width, height)
{
}

void Platform::draw(QPainter &painter)
{
    // 默认只作为碰撞接口存在。需要调试时 GameWidget 会自行画碰撞盒。
    painter.save();
    painter.setPen(QPen(QColor(255, 0, 0, 130), 1, Qt::DashLine));
    painter.setBrush(QColor(255, 0, 0, 35));
    painter.drawRect(m_rect);
    painter.restore();
}

QRect Platform::rect() const
{
    return m_rect.toRect();
}

QRectF Platform::rectF() const
{
    return m_rect;
}
