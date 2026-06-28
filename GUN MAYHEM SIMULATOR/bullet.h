#ifndef BULLET_H
#define BULLET_H

#include "gameobject.h"

class Bullet : public GameObject
{
public:
    Bullet(double x = 0, double y = 0, int direction = 1, int ownerId = 1);

    void update() override;
    void draw(QPainter &painter) const override;

    int ownerId() const;
    int direction() const;
    int damage() const;
    double knockbackX() const;
    double knockbackY() const;
    double recoilPower() const;

    bool isAlive() const;
    void kill();

    bool isOutOfScreen(int screenWidth, int screenHeight) const;

private:
    int m_ownerId;
    int m_direction;
    int m_damage;
    double m_speed;
    double m_knockbackX;
    double m_knockbackY;
    double m_recoilPower;
    int m_lifeFrames;
    bool m_alive;
};

#endif // BULLET_H
