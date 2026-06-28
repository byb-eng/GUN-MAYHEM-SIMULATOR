#include "player.h"
#include <QtMath>
#include <QColor>
#include <QPen>
#include <QPixmap>

Player::Player(int id, double x, double y)
    : GameObject(x, y, 28, 44),
      m_id(id),
      m_hp(100),
      m_direction(1),
      m_dead(false),
      m_onGround(false),
      m_vx(0),
      m_vy(0),
      m_moveSpeed(4.45),
      m_accel(0.62),
      m_airAccel(0.36),
      m_jumpSpeed(9.45),
      m_gravity(0.54),
      m_maxFallSpeed(12.6),
      m_moveIntent(0),
      m_jumpCount(0),
      m_maxJumpCount(2),
      m_maxAmmo(8),
      m_ammo(8),
      m_shootCooldown(0),
      m_reloadTimer(0),
      m_reloadFrames(78),
      m_bombCooldown(0),
      m_bombCooldownFrames(132),
      m_hurtFlashTimer(0),
      m_invincibleTimer(0),
      m_runFrame(0),
      m_jumpPoseTimer(0),
      m_firePoseTimer(0)
{
    if (m_id == 2) {
        m_direction = -1;
    }
}

void Player::moveLeft()
{
    if (m_dead) {
        return;
    }
    m_moveIntent = -1;
    m_direction = -1;
}

void Player::moveRight()
{
    if (m_dead) {
        return;
    }
    m_moveIntent = 1;
    m_direction = 1;
}

void Player::stopMove()
{
    if (m_dead) {
        return;
    }
    m_moveIntent = 0;
}

void Player::jump()
{
    if (m_dead) {
        return;
    }

    if (m_onGround || m_jumpCount < m_maxJumpCount) {
        // 原视频的跳跃幅度比较克制，二段跳也不是很高。
        const bool secondJump = !m_onGround;
        m_vy = secondJump ? -m_jumpSpeed * 0.91 : -m_jumpSpeed;

        if (m_onGround) {
            m_jumpCount = 1;
        } else {
            ++m_jumpCount;
        }

        m_onGround = false;
        m_jumpPoseTimer = 10;
    }
}

void Player::update()
{
    if (m_dead) {
        return;
    }

    if (m_shootCooldown > 0) {
        --m_shootCooldown;
    }
    if (m_bombCooldown > 0) {
        --m_bombCooldown;
    }
    if (m_reloadTimer > 0) {
        --m_reloadTimer;
        if (m_reloadTimer == 0) {
            m_ammo = m_maxAmmo;
        }
    }
    if (m_hurtFlashTimer > 0) {
        --m_hurtFlashTimer;
    }
    if (m_invincibleTimer > 0) {
        --m_invincibleTimer;
    }
    if (m_jumpPoseTimer > 0) {
        --m_jumpPoseTimer;
    }
    if (m_firePoseTimer > 0) {
        --m_firePoseTimer;
    }

    if (m_moveIntent != 0) {
        double accel = m_onGround ? m_accel : m_airAccel;
        m_vx += m_moveIntent * accel;
        if (m_vx > m_moveSpeed) {
            m_vx = m_moveSpeed;
        }
        if (m_vx < -m_moveSpeed) {
            m_vx = -m_moveSpeed;
        }
        ++m_runFrame;
    } else {
        double friction = m_onGround ? 0.78 : 0.965;
        m_vx *= friction;
        if (qAbs(m_vx) < 0.05) {
            m_vx = 0;
        }
    }

    m_vy += m_gravity;
    if (m_vy > m_maxFallSpeed) {
        m_vy = m_maxFallSpeed;
    }

    m_x += m_vx;
    m_y += m_vy;
}

void Player::draw(QPainter &painter) const
{
    painter.save();

    if (m_invincibleTimer > 0 && (m_invincibleTimer / 5) % 2 == 0) {
        painter.setOpacity(0.55);
    }

    QPixmap sprite(m_id == 1 ? ":/assets/player_blue.png" : ":/assets/player_red.png");
    const bool moving = qAbs(m_vx) > 0.35;
    const double bob = (moving && m_onGround) ? qSin(m_runFrame * 0.45) * 1.5 : 0.0;
    const double squash = m_onGround ? 1.0 : 1.05;
    const double visualW = 58.0 + (m_firePoseTimer > 0 ? 3.0 : 0.0);
    const double visualH = 65.0 * squash;
    QRectF target(m_x + m_width / 2.0 - visualW / 2.0,
                  m_y + m_height - visualH + bob + 9.0,
                  visualW, visualH);

    if (!sprite.isNull()) {
        if (m_dead) {
            painter.setOpacity(0.40);
        }

        if (m_direction < 0) {
            painter.translate(target.center());
            painter.scale(-1, 1);
            QRectF mirrored(-target.width() / 2.0, -target.height() / 2.0,
                            target.width(), target.height());
            painter.drawPixmap(mirrored.toRect(), sprite);
        } else {
            painter.drawPixmap(target.toRect(), sprite);
        }
    } else {
        QColor mainColor = (m_id == 1) ? QColor(45, 100, 210) : QColor(220, 85, 50);
        painter.setPen(QPen(Qt::black, 2));
        painter.setBrush(mainColor);
        painter.drawRoundedRect(rectF(), 6, 6);
    }

    painter.restore();

    if (m_hurtFlashTimer > 0) {
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 240, 80, 90));
        painter.drawEllipse(center(), 28, 34);
        painter.restore();
    }

    if (m_invincibleTimer > 0) {
        painter.save();
        painter.setPen(QPen(QColor(255, 255, 255, 185), 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(center(), 31, 39);
        painter.restore();
    }
}

void Player::takeDamage(int damage)
{
    if (m_dead || isInvincible()) {
        return;
    }

    m_hp -= damage;
    if (m_hp < 0) {
        m_hp = 0;
    }

    m_hurtFlashTimer = 13;

    if (m_hp == 0) {
        m_dead = true;
    }
}

void Player::applyKnockback(double vx, double vy)
{
    if (m_dead) {
        return;
    }
    m_vx = vx;
    m_vy = vy;
    m_onGround = false;
}

void Player::applyRecoil(int direction, double power)
{
    if (m_dead || direction == 0) {
        return;
    }
    m_vx -= direction * power;
    m_firePoseTimer = 7;
}

bool Player::isDead() const
{
    return m_dead;
}

bool Player::isOnGround() const
{
    return m_onGround;
}

bool Player::isInvincible() const
{
    return m_invincibleTimer > 0;
}

int Player::hp() const
{
    return m_hp;
}

int Player::id() const
{
    return m_id;
}

int Player::direction() const
{
    return m_direction;
}

int Player::invincibleFramesRemaining() const
{
    return m_invincibleTimer;
}

bool Player::canShoot() const
{
    return !m_dead && m_ammo > 0 && m_reloadTimer == 0 && m_shootCooldown == 0;
}

void Player::consumeAmmo()
{
    if (!canShoot()) {
        return;
    }
    --m_ammo;
    m_shootCooldown = 9;
    m_firePoseTimer = 7;
    startReloadIfNeeded();
}

int Player::ammo() const
{
    return m_ammo;
}

int Player::maxAmmo() const
{
    return m_maxAmmo;
}

bool Player::isReloading() const
{
    return m_reloadTimer > 0;
}

int Player::reloadFramesRemaining() const
{
    return m_reloadTimer;
}

bool Player::canPlaceBomb() const
{
    return !m_dead && m_bombCooldown == 0;
}

void Player::consumeBomb()
{
    if (!canPlaceBomb()) {
        return;
    }
    m_bombCooldown = m_bombCooldownFrames;
}

int Player::bombCooldownRemaining() const
{
    return m_bombCooldown;
}

void Player::setOnGround(bool onGround)
{
    if (onGround && !m_onGround && m_vy > 4.5) {
        m_jumpPoseTimer = 6;
    }

    m_onGround = onGround;
    if (m_onGround) {
        m_jumpCount = 0;
        if (m_vy > 0) {
            m_vy = 0;
        }
    }
}

void Player::setVelocityY(double vy)
{
    m_vy = vy;
}

void Player::setVelocityX(double vx)
{
    m_vx = vx;
}

void Player::setInvincibleFrames(int frames)
{
    m_invincibleTimer = frames;
}

double Player::velocityX() const
{
    return m_vx;
}

double Player::velocityY() const
{
    return m_vy;
}

QPointF Player::shootPosition() const
{
    double shootX = m_x + m_width / 2.0 + m_direction * 31.0;
    double shootY = m_y + 24.0;
    return QPointF(shootX, shootY);
}

QPointF Player::bombPosition() const
{
    double bombX = m_x + m_width / 2.0 + m_direction * 18.0 - 12.0;
    double bombY = m_y + 10.0;
    return QPointF(bombX, bombY);
}

void Player::reset(double x, double y)
{
    m_x = x;
    m_y = y;
    m_vx = 0;
    m_vy = 0;
    m_hp = 100;
    m_dead = false;
    m_onGround = false;
    m_moveIntent = 0;
    m_jumpCount = 0;
    m_ammo = m_maxAmmo;
    m_shootCooldown = 0;
    m_reloadTimer = 0;
    m_bombCooldown = 0;
    m_hurtFlashTimer = 0;
    m_invincibleTimer = 0;
    m_runFrame = 0;
    m_jumpPoseTimer = 0;
    m_firePoseTimer = 0;

    if (m_id == 1) {
        m_direction = 1;
    } else {
        m_direction = -1;
    }
}

void Player::startReloadIfNeeded()
{
    if (m_ammo == 0 && m_reloadTimer == 0) {
        m_reloadTimer = m_reloadFrames;
    }
}
