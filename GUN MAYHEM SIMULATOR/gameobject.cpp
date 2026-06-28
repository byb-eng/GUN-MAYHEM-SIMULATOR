#include "gameobject.h"

GameObject::GameObject(double x, double y, double width, double height)
    : m_x(x),
      m_y(y),
      m_width(width),
      m_height(height)
{
}

GameObject::~GameObject()
{
}

void GameObject::update()
{
}

void GameObject::draw(QPainter &painter) const
{
    Q_UNUSED(painter);
}

QRect GameObject::rect() const
{
    return QRectF(m_x, m_y, m_width, m_height).toRect();
}

QRectF GameObject::rectF() const
{
    return QRectF(m_x, m_y, m_width, m_height);
}

QPointF GameObject::center() const
{
    return QPointF(m_x + m_width / 2.0, m_y + m_height / 2.0);
}

double GameObject::x() const
{
    return m_x;
}

double GameObject::y() const
{
    return m_y;
}

double GameObject::width() const
{
    return m_width;
}

double GameObject::height() const
{
    return m_height;
}

void GameObject::setPosition(double x, double y)
{
    m_x = x;
    m_y = y;
}

void GameObject::setX(double x)
{
    m_x = x;
}

void GameObject::setY(double y)
{
    m_y = y;
}
