#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QPointF>

class GameObject
{
public:
    GameObject(double x = 0, double y = 0, double width = 0, double height = 0);
    virtual ~GameObject();

    virtual void update();
    virtual void draw(QPainter &painter) const;

    // 公共协作接口：分工文档要求 QRect rect() const。
    QRect rect() const;

    // 内部物理和细腻碰撞用 QRectF，避免牺牲小数坐标手感。
    QRectF rectF() const;

    QPointF center() const;

    double x() const;
    double y() const;
    double width() const;
    double height() const;

    void setPosition(double x, double y);
    void setX(double x);
    void setY(double y);

protected:
    double m_x;
    double m_y;
    double m_width;
    double m_height;
};

#endif // GAMEOBJECT_H
