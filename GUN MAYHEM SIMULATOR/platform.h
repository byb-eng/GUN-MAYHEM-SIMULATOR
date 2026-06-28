#ifndef PLATFORM_H
#define PLATFORM_H

#include <QPainter>
#include <QRect>
#include <QRectF>

class Platform
{
public:
    Platform(int x = 0, int y = 0, int width = 0, int height = 0);

    void draw(QPainter &painter);
    QRect rect() const;

    // Internal helper for floating-point collision with existing GameWidget logic.
    QRectF rectF() const;

private:
    QRectF m_rect;
};

#endif // PLATFORM_H
