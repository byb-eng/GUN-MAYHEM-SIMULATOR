#include "gamemanager.h"
#include "player.h"
#include "bullet.h"
#include "bomb.h"
#include <QtMath>

GameManager::GameManager()
    : m_player1Lives(GameConst::kStartLives),
      m_player2Lives(GameConst::kStartLives),
      m_gameOver(false),
      m_killFeedFrames(0)
{
}

void GameManager::buildMap()
{
    m_platforms.clear();

    // 平台严格按用户红线标注设置，只放在草地顶边。
    m_platforms.push_back(Platform(432, 104, 136, 16));  // 顶部小草地
    m_platforms.push_back(Platform(265, 181, 430, 18));  // 中央主草地
    m_platforms.push_back(Platform(120, 254, 130, 16));  // 左上草地
    m_platforms.push_back(Platform(732, 254, 99, 16));   // 右上草地
    m_platforms.push_back(Platform(58,  324, 291, 18));  // 左中草地
    m_platforms.push_back(Platform(585, 324, 288, 18));  // 右中草地
    m_platforms.push_back(Platform(388, 393, 171, 16));  // 中下小草地
    m_platforms.push_back(Platform(75,  469, 319, 18));  // 左下草地
    m_platforms.push_back(Platform(544, 469, 351, 18));  // 右下草地
}

const std::vector<Platform>& GameManager::platforms() const
{
    return m_platforms;
}

// ===================== 碰撞检测 =====================

bool GameManager::collidePlayerWithPlatforms(Player &player, double previousBottom, bool ignorePlatform)
{
    bool landed = false;
    double fallSpeed = player.velocityY();

    if (!ignorePlatform) {
        QRectF playerRect = player.rectF();

        for (const Platform &platform : m_platforms) {
            if (!rectOverlapsPlatformX(playerRect, platform.rectF())) {
                continue;
            }

            double platformTop = platform.rectF().top();
            double currentBottom = player.y() + player.height();

            if (fallSpeed >= 0 &&
                previousBottom <= platformTop + 9 &&
                currentBottom >= platformTop &&
                currentBottom <= platformTop + 30) {
                player.setY(platformTop - player.height());
                player.setVelocityY(0);
                player.setOnGround(true);
                landed = true;
                break;
            }
        }
    }

    if (!landed) {
        player.setOnGround(false);
    }

    return landed;
}

bool GameManager::collideBulletWithPlatforms(Bullet &bullet, SparkEvent &spark)
{
    if (!bullet.isAlive()) {
        return false;
    }

    for (const Platform &platform : m_platforms) {
        QRectF surface = platform.rectF().adjusted(-4, -9, 4, 6);
        if (bullet.rectF().intersects(surface)) {
            spark.pos = bullet.center();
            spark.direction = bullet.direction();
            bullet.kill();
            return true;
        }
    }
    return false;
}

void GameManager::collideBombWithPlatforms(Bomb &bomb, double previousBottom)
{
    QRectF bombRect = bomb.rectF();

    for (const Platform &platform : m_platforms) {
        if (!rectOverlapsPlatformX(bombRect, platform.rectF())) {
            continue;
        }

        double currentBottom = bomb.y() + bomb.height();
        if (bomb.velocityY() >= 0 &&
            previousBottom <= platform.rectF().top() + 8 &&
            currentBottom >= platform.rectF().top()) {
            bomb.landOn(platform.rectF().top());
            break;
        }
    }
}

// ===================== 规则判定 =====================

std::vector<HitEvent> GameManager::checkBulletHits(std::vector<Bullet> &bullets, Player &p1, Player &p2)
{
    std::vector<HitEvent> events;

    for (Bullet &bullet : bullets) {
        if (!bullet.isAlive()) {
            continue;
        }

        Player *target = nullptr;
        if (bullet.ownerId() != p1.id() && bullet.rectF().intersects(p1.rectF())) {
            target = &p1;
        }
        if (bullet.ownerId() != p2.id() && bullet.rectF().intersects(p2.rectF())) {
            target = &p2;
        }

        if (target != nullptr && !target->isInvincible()) {
            int before = target->hp();
            target->takeDamage(bullet.damage());
            if (target->hp() != before) {
                target->applyKnockback(bullet.knockbackX(), bullet.knockbackY());
                HitEvent e;
                e.pos = target->center();
                events.push_back(e);
            }
            bullet.kill();
        }
    }

    return events;
}

std::vector<HitEvent> GameManager::checkBombHits(std::vector<Bomb> &bombs, Player &p1, Player &p2, int &bombsExploded)
{
    std::vector<HitEvent> events;
    bombsExploded = 0;

    for (Bomb &bomb : bombs) {
        if (!bomb.canApplyDamage()) {
            continue;
        }

        ++bombsExploded;
        Player *players[2] = { &p1, &p2 };

        for (Player *player : players) {
            if (player->isInvincible()) {
                continue;
            }

            double d = distance(player->center(), bomb.center());

            if (d <= bomb.radius()) {
                // 自己的炸弹只炸飞不扣血，敌人的炸弹正常扣血
                bool isSelf = (player->id() == bomb.ownerId());
                if (!isSelf) {
                    player->takeDamage(bomb.damage());
                }

                double dx = player->center().x() - bomb.center().x();
                double dy = player->center().y() - bomb.center().y();
                double len = qSqrt(dx * dx + dy * dy);

                if (len < 0.001) {
                    dx = 0;
                    dy = -1;
                    len = 1;
                }

                dx /= len;
                dy /= len;

                double kx = dx * bomb.knockbackPower();
                double ky = dy * bomb.knockbackPower() - 7.5;

                player->applyKnockback(kx, ky);

                HitEvent e;
                e.pos = player->center();
                events.push_back(e);
            }
        }

        bomb.markDamageApplied();
    }

    return events;
}

std::vector<LifeEvent> GameManager::checkLifeState(Player &p1, Player &p2)
{
    std::vector<LifeEvent> events;

    if (p1.y() > GameConst::kVoidDeathY || p1.isDead()) {
        m_killFeed = p1.isDead() ? "Player 2 killed Player 1" : "Player 1 fell";
        m_killFeedFrames = 130;

        LifeEvent le;
        le.playerId = 1;
        le.effectX = p1.center().x();
        le.respawned = false;

        --m_player1Lives;
        if (m_player1Lives <= 0) {
            m_player1Lives = 0;
            m_gameOver = true;
            m_winnerText = "Player 2 Wins!";
        } else {
            respawnPlayer(p1, 450);
            le.respawned = true;
        }
        events.push_back(le);
    }

    if (!m_gameOver && (p2.y() > GameConst::kVoidDeathY || p2.isDead())) {
        m_killFeed = p2.isDead() ? "Player 1 killed Player 2" : "Player 2 fell";
        m_killFeedFrames = 130;

        LifeEvent le;
        le.playerId = 2;
        le.effectX = p2.center().x();
        le.respawned = false;

        --m_player2Lives;
        if (m_player2Lives <= 0) {
            m_player2Lives = 0;
            m_gameOver = true;
            m_winnerText = "Player 1 Wins!";
        } else {
            respawnPlayer(p2, 520);
            le.respawned = true;
        }
        events.push_back(le);
    }

    return events;
}

// ===================== 状态管理 =====================

void GameManager::respawnPlayer(Player &player, double spawnX)
{
    player.reset(spawnX, -125);
    player.setInvincibleFrames(GameConst::kRespawnInvincibleFrames);
}

void GameManager::resetState()
{
    m_player1Lives = GameConst::kStartLives;
    m_player2Lives = GameConst::kStartLives;
    m_gameOver = false;
    m_winnerText.clear();
    m_killFeed.clear();
    m_killFeedFrames = 0;
}

int GameManager::player1Lives() const { return m_player1Lives; }
int GameManager::player2Lives() const { return m_player2Lives; }
bool GameManager::isGameOver() const { return m_gameOver; }
QString GameManager::winnerText() const { return m_winnerText; }
QString GameManager::killFeed() const { return m_killFeed; }
int GameManager::killFeedFrames() const { return m_killFeedFrames; }

void GameManager::tickKillFeed()
{
    if (m_killFeedFrames > 0) {
        --m_killFeedFrames;
    }
}

// ===================== 工具函数 =====================

bool GameManager::rectOverlapsPlatformX(const QRectF &a, const QRectF &b) const
{
    return a.right() > b.left() + 5 && a.left() < b.right() - 5;
}

double GameManager::distance(QPointF a, QPointF b) const
{
    double dx = a.x() - b.x();
    double dy = a.y() - b.y();
    return qSqrt(dx * dx + dy * dy);
}
