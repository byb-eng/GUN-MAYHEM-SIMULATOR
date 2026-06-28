#ifndef PLAYER_H
#define PLAYER_H

#include "gameobject.h"
#include <QPointF>

class Player : public GameObject
{
public:
    Player(int id = 1, double x = 100, double y = 100);

    void moveLeft();
    void moveRight();
    void stopMove();
    void jump();

    void update() override;
    void draw(QPainter &painter) const override;

    void takeDamage(int damage);
    void applyKnockback(double vx, double vy);
    void applyRecoil(int direction, double power);

    bool isDead() const;
    bool isOnGround() const;
    bool isInvincible() const;

    int hp() const;
    int id() const;
    int direction() const;
    int invincibleFramesRemaining() const;

    bool canShoot() const;
    void consumeAmmo();
    int ammo() const;
    int maxAmmo() const;
    bool isReloading() const;
    int reloadFramesRemaining() const;

    bool canPlaceBomb() const;
    void consumeBomb();
    int bombCooldownRemaining() const;

    void setOnGround(bool onGround);
    void setVelocityY(double vy);
    void setVelocityX(double vx);
    void setInvincibleFrames(int frames);

    double velocityX() const;
    double velocityY() const;

    QPointF shootPosition() const;
    QPointF bombPosition() const;

    void reset(double x, double y);

private:
    void startReloadIfNeeded();

private:
    int m_id;
    int m_hp;
    int m_direction;
    bool m_dead;
    bool m_onGround;

    double m_vx;
    double m_vy;
    double m_moveSpeed;
    double m_accel;
    double m_airAccel;
    double m_jumpSpeed;
    double m_gravity;
    double m_maxFallSpeed;
    int m_moveIntent;

    int m_jumpCount;
    int m_maxJumpCount;

    int m_maxAmmo;
    int m_ammo;
    int m_shootCooldown;
    int m_reloadTimer;
    int m_reloadFrames;
    int m_bombCooldown;
    int m_bombCooldownFrames;

    int m_hurtFlashTimer;
    int m_invincibleTimer;
    int m_runFrame;
    int m_jumpPoseTimer;
    int m_firePoseTimer;
};

#endif // PLAYER_H
