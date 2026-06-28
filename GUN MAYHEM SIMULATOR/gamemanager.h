#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <vector>
#include <QPointF>
#include <QString>
#include "platform.h"

class Player;
class Bullet;
class Bomb;

namespace GameConst {
    constexpr double kVoidDeathY = 705.0;
    constexpr int kStartLives = 5;
    constexpr int kRespawnInvincibleFrames = 95;
}

struct HitEvent {
    QPointF pos;
};

struct SparkEvent {
    QPointF pos;
    int direction;
};

struct LifeEvent {
    int playerId;
    double effectX;
    bool respawned;
};

class GameManager {
public:
    GameManager();

    void buildMap();
    const std::vector<Platform>& platforms() const;

    // 碰撞检测
    bool collidePlayerWithPlatforms(Player &player, double previousBottom, bool ignorePlatform);
    bool collideBulletWithPlatforms(Bullet &bullet, SparkEvent &spark);
    void collideBombWithPlatforms(Bomb &bomb, double previousBottom);

    // 规则判定
    std::vector<HitEvent> checkBulletHits(std::vector<Bullet> &bullets, Player &p1, Player &p2);
    std::vector<HitEvent> checkBombHits(std::vector<Bomb> &bombs, Player &p1, Player &p2, int &bombsExploded);
    std::vector<LifeEvent> checkLifeState(Player &p1, Player &p2);

    // 状态管理
    void respawnPlayer(Player &player, double spawnX);
    void resetState();

    int player1Lives() const;
    int player2Lives() const;
    bool isGameOver() const;
    QString winnerText() const;
    QString killFeed() const;
    int killFeedFrames() const;
    void tickKillFeed();

private:
    std::vector<Platform> m_platforms;
    int m_player1Lives;
    int m_player2Lives;
    bool m_gameOver;
    QString m_winnerText;
    QString m_killFeed;
    int m_killFeedFrames;

    bool rectOverlapsPlatformX(const QRectF &a, const QRectF &b) const;
    double distance(QPointF a, QPointF b) const;
};

#endif // GAMEMANAGER_H
