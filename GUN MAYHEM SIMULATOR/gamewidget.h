#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QKeyEvent>
#include <QSet>
#include <QString>
#include <QRectF>
#include <QPointF>
#include <QColor>
#include <vector>

#include "player.h"
#include "bullet.h"
#include "bomb.h"
#include "platform.h"
#include "gameconfig.h"
#include "gamemanager.h"
#include "ai_types.h"
#include "ai_controller.h"

struct VisualEffect
{
    QPointF pos;
    QPointF velocity;
    int frames;
    int maxFrames;
    int type; // 0 dust, 1 respawn, 2 void, 3 HIT, 4 muzzle, 5 spark, 6 text
    QString text;
    QColor color;
    double size;
    int direction;
};

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);
    void startGame(GameMode mode);
    void stopGame();
    void setGameMode(GameMode mode);
    GameMode gameMode() const;
    void restartGame();

signals:
    void sigGoToMainMenu();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void updateGame();
    void handleInput();
    void handlePlayer1Input();
    void handlePlayer2HumanInput();
    void handlePlayer2AIInput();
    GameState collectStateForP2() const;
    void applyAIActionToP2(AIAction action);
    bool loadAIModel();
    void updatePlayers();
    void updateOnePlayer(Player &player, bool dropDownKey, int &dropTimer);
    void updateBullets();
    void updateBombs();
    void updateEffects();

    void addLandingEffects(const Player &player);
    void addJumpEffect(const Player &player);
    void addRespawnEffect(const Player &player);
    void addVoidEffect(double x);
    void addHitEffect(const QPointF &pos);
    void addMuzzleEffect(const QPointF &pos, int direction);
    void addSparkEffect(const QPointF &pos, int direction);
    void addFloatingText(const QPointF &pos, const QString &text, const QColor &color, double size);
    void startShake(int frames, double strength);

    void drawBackground(QPainter &painter) const;
    void drawPlatforms(QPainter &painter) const;
    void drawEffects(QPainter &painter) const;
    void drawStatus(QPainter &painter) const;
    void drawHealthAndAmmo(QPainter &painter, const Player &player, int lives, bool leftSide) const;
    void drawPlayerBadges(QPainter &painter) const;
    void drawOneBadge(QPainter &painter, const Player &player) const;
    void drawOffscreenMarker(QPainter &painter, const Player &player) const;

private:
    QTimer *m_timer;
    QSet<int> m_keys;
    GameManager m_gameManager;

    Player m_player1;
    Player m_player2;

    std::vector<Bullet> m_bullets;
    std::vector<Bomb> m_bombs;
    std::vector<VisualEffect> m_effects;

    bool m_player1JumpHeld;
    bool m_player2JumpHeld;
    bool m_player1DropHeld;
    bool m_player2DropHeld;
    int m_player1DropTimer;
    int m_player2DropTimer;

    int m_frameCounter;
    int m_shakeFrames;
    double m_shakeStrength;

    // AI 相关
    GameMode m_gameMode;
    AIController m_aiController;
    AIAction m_lastAIAction;
    int m_aiDecisionCounter;
    bool m_aiModelLoaded;
};

#endif // GAMEWIDGET_H
