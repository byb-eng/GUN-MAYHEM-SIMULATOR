#include "bomb.h"
#include <QtMath>
#include <QPixmap>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QRadialGradient>

Bomb::Bomb(double x, double y, int ownerId, double initialVx, double initialVy)
    : GameObject(x, y, 28, 28),
      m_ownerId(ownerId),
      m_damage(45),
      m_radius(120),
      m_knockbackPower(18.0),
      m_vx(initialVx),
      m_vy(initialVy),
      m_gravity(0.42),
      m_maxFallSpeed(12.0),
      m_onGround(false),
      m_countdownFrames(105),
      m_explosionFrames(28),
      m_exploded(false),
      m_damageApplied(false)
{
}

void Bomb::update()
{
    if (!m_exploded) {
        if (!m_onGround) {
            m_vy += m_gravity;
            if (m_vy > m_maxFallSpeed) {
                m_vy = m_maxFallSpeed;
            }
        } else {
            m_vx *= 0.84;
            if (qAbs(m_vx) < 0.05) {
                m_vx = 0;
            }
        }

        m_x += m_vx;
        m_y += m_vy;

        --m_countdownFrames;
        if (m_countdownFrames <= 0) {
            m_exploded = true;
        }
    } else {
        --m_explosionFrames;
    }
}

void Bomb::draw(QPainter &painter) const
{
    if (!m_exploded) {
        QPixmap crate(":/assets/bomb_crate.png");
        QRectF target = rectF().adjusted(-6, -8, 6, 6);
        if (!crate.isNull()) {
            painter.drawPixmap(target.toRect(), crate);
        } else {
            painter.setPen(QPen(Qt::black, 2));
            painter.setBrush(QColor(150, 95, 35));
            painter.drawRoundedRect(target, 4, 4);
        }

        painter.setPen(QColor(255, 255, 255));
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        painter.drawText(target.adjusted(0, 2, 0, 0), Qt::AlignCenter,
                         QString::number(m_countdownFrames / 35 + 1));
    } else {
        double t = qBound(0.0, static_cast<double>(m_explosionFrames) / 28.0, 1.0);
        int alpha = qBound(0, static_cast<int>(220 * t), 220);

        QRadialGradient burst(center(), m_radius * (1.12 - 0.20 * t));
        burst.setColorAt(0.00, QColor(255, 245, 175, alpha));
        burst.setColorAt(0.35, QColor(255, 150, 45, alpha));
        burst.setColorAt(1.00, QColor(255, 80, 10, 0));

        painter.setPen(QPen(QColor(80, 40, 0, alpha), 4));
        painter.setBrush(burst);
        painter.drawEllipse(center(), m_radius * (1.10 - 0.18 * t),
                             m_radius * (1.10 - 0.18 * t));

        painter.setPen(QPen(QColor(255, 240, 160, alpha), 3));
        for (int i = 0; i < 12; ++i) {
            double a = i * 0.523 + (1.0 - t) * 0.4;
            QPointF inner = center() + QPointF(qCos(a) * 18, qSin(a) * 18);
            QPointF outer = center() + QPointF(qCos(a) * m_radius * (1.0 - 0.16 * t),
                                               qSin(a) * m_radius * (1.0 - 0.16 * t));
            painter.drawLine(inner, outer);
        }
    }
}

int Bomb::ownerId() const { return m_ownerId; }
int Bomb::damage() const { return m_damage; }
int Bomb::radius() const { return m_radius; }
double Bomb::knockbackPower() const { return m_knockbackPower; }
bool Bomb::isExploded() const { return m_exploded; }
bool Bomb::canApplyDamage() const { return m_exploded && !m_damageApplied; }
void Bomb::markDamageApplied() { m_damageApplied = true; }
bool Bomb::shouldRemove() const { return m_exploded && m_explosionFrames <= 0; }

void Bomb::landOn(double groundY)
{
    if (m_exploded) {
        return;
    }

    if (m_y + m_height >= groundY) {
        m_y = groundY - m_height;
        if (m_vy > 0) {
            m_vy = 0;
        }
        m_onGround = true;
    } else {
        m_onGround = false;
    }
}

double Bomb::velocityY() const
{
    return m_vy;
}
