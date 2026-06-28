#include "bullet.h"
#include <QPixmap>
#include <QPen>
#include <QColor>

Bullet::Bullet(double x, double y, int direction, int ownerId)
    : GameObject(x, y, 30, 8),
      m_ownerId(ownerId),
      m_direction(direction),
      m_damage(12),
      m_speed(16.5),
      m_knockbackX(16.1 * direction),
      m_knockbackY(-10.5),
      m_recoilPower(2.9),
      m_lifeFrames(80),
      m_alive(true)
{
    if (m_direction == 0) {
        m_direction = 1;
        m_knockbackX = 16.1;
    }
}

void Bullet::update()
{
    if (!m_alive) {
        return;
    }
    m_x += m_speed * m_direction;
    --m_lifeFrames;
    if (m_lifeFrames <= 0) {
        m_alive = false;
    }
}

void Bullet::draw(QPainter &painter) const
{
    if (!m_alive) {
        return;
    }

    QPixmap pix(":/assets/bullet_tracer.png");
    QRectF target = rectF().adjusted(-8, -5, 12, 5);

    if (!pix.isNull()) {
        painter.save();
        if (m_direction < 0) {
            painter.translate(target.center());
            painter.scale(-1, 1);
            QRectF mirrored(-target.width() / 2.0, -target.height() / 2.0,
                            target.width(), target.height());
            painter.drawPixmap(mirrored.toRect(), pix);
        } else {
            painter.drawPixmap(target.toRect(), pix);
        }
        painter.restore();
    } else {
        painter.setPen(QPen(QColor(45, 35, 10), 2));
        painter.setBrush(QColor(255, 230, 80));
        painter.drawRoundedRect(target, 4, 4);
    }
}

int Bullet::ownerId() const { return m_ownerId; }
int Bullet::direction() const { return m_direction; }
int Bullet::damage() const { return m_damage; }
double Bullet::knockbackX() const { return m_knockbackX; }
double Bullet::knockbackY() const { return m_knockbackY; }
double Bullet::recoilPower() const { return m_recoilPower; }
bool Bullet::isAlive() const { return m_alive; }
void Bullet::kill() { m_alive = false; }

bool Bullet::isOutOfScreen(int screenWidth, int screenHeight) const
{
    return m_x + m_width < -80 ||
           m_x > screenWidth + 80 ||
           m_y + m_height < -80 ||
           m_y > screenHeight + 80;
}
