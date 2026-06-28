#ifndef BOMB_H
#define BOMB_H

#include "gameobject.h"

class Bomb : public GameObject
{
public:
    Bomb(double x = 0, double y = 0, int ownerId = 1, double initialVx = 0.0, double initialVy = -3.5);

    void update() override;
    void draw(QPainter &painter) const override;

    int ownerId() const;
    int damage() const;
    int radius() const;
    double knockbackPower() const;

    bool isExploded() const;
    bool canApplyDamage() const;
    void markDamageApplied();
    bool shouldRemove() const;

    void landOn(double groundY);
    double velocityY() const;

private:
    int m_ownerId;
    int m_damage;
    int m_radius;
    double m_knockbackPower;

    double m_vx;
    double m_vy;
    double m_gravity;
    double m_maxFallSpeed;
    bool m_onGround;

    int m_countdownFrames;
    int m_explosionFrames;

    bool m_exploded;
    bool m_damageApplied;
};

#endif // BOMB_H
