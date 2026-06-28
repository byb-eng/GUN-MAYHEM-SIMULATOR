#include "gamewidget.h"
#include "pausedialog.h"
#include "audiomanager.h"
#include <QtMath>
#include <QFont>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPixmap>
#include <QPen>
#include <QRect>
#include <QSizeF>
#include <algorithm>
#include <QCoreApplication>
#include <QDir>
#include <QStringList>

namespace {
constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 600;
}

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent),
      m_timer(new QTimer(this)),
      m_player1(1, 450, -125),
      m_player2(2, 520, -125),
      m_player1JumpHeld(false),
      m_player2JumpHeld(false),
      m_player1DropHeld(false),
      m_player2DropHeld(false),
      m_player1DropTimer(0),
      m_player2DropTimer(0),
      m_frameCounter(0),
      m_shakeFrames(0),
      m_shakeStrength(0),
      m_gameMode(GameMode::HumanVsHuman),
      m_lastAIAction(AIAction::Idle),
      m_aiDecisionCounter(0),
      m_aiModelLoaded(false)
{
    setFocusPolicy(Qt::StrongFocus);
    setFixedSize(kWindowWidth, kWindowHeight);
    m_gameManager.buildMap();

    m_player1.setInvincibleFrames(GameConst::kRespawnInvincibleFrames);
    m_player2.setInvincibleFrames(GameConst::kRespawnInvincibleFrames);

    // 尝试加载 AI 模型权重。支持运行目录和 qrc 内置权重。
    m_aiModelLoaded = loadAIModel();

    connect(m_timer, &QTimer::timeout, this, [=]() {
        updateGame();
        update();
    });

    // 不在构造函数里启动计时器，避免玩家停在主菜单时后台游戏已经开始，
    // 从而出现刚打开界面就有枪声/爆炸声的 bug。
}


void GameWidget::startGame(GameMode mode)
{
    AudioManager::instance().playBackgroundMusic();
    setGameMode(mode);
    restartGame();
    if (!m_timer->isActive()) {
        m_timer->start(16);
    }
}

void GameWidget::stopGame()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    m_keys.clear();
}

void GameWidget::setGameMode(GameMode mode)
{
    m_gameMode = mode;
    if (m_gameMode == GameMode::HumanVsHardAI && !m_aiModelLoaded) {
        m_aiModelLoaded = loadAIModel();
    }
}

GameMode GameWidget::gameMode() const
{
    return m_gameMode;
}

bool GameWidget::loadAIModel()
{
    QStringList paths;
    paths << ":/weights.txt"
          << "weights.txt"
          << (QDir::current().absoluteFilePath("weights.txt"))
          << (QCoreApplication::applicationDirPath() + "/weights.txt");

    for (const QString &path : paths) {
        if (m_aiController.loadModel(path.toStdString())) {
            return true;
        }
    }
    return false;
}


void GameWidget::restartGame()
{
    m_gameManager.resetState();

    m_player1.reset(450, -125);
    m_player2.reset(520, -125);
    m_player1.setInvincibleFrames(GameConst::kRespawnInvincibleFrames);
    m_player2.setInvincibleFrames(GameConst::kRespawnInvincibleFrames);

    m_bullets.clear();
    m_bombs.clear();
    m_effects.clear();

    m_player1JumpHeld = false;
    m_player2JumpHeld = false;
    m_player1DropHeld = false;
    m_player2DropHeld = false;
    m_player1DropTimer = 0;
    m_player2DropTimer = 0;
    m_shakeFrames = 0;
    m_shakeStrength = 0;

    m_keys.clear();

    // AI 状态重置
    m_lastAIAction = AIAction::Idle;
    m_aiDecisionCounter = 0;

    addRespawnEffect(m_player1);
    addRespawnEffect(m_player2);

    if (!m_timer->isActive()) {
        m_timer->start(16);
    }
}

// ===================== 主循环 =====================

void GameWidget::updateGame()
{
    ++m_frameCounter;

    if (m_gameManager.isGameOver()) {
        if (m_keys.contains(Qt::Key_R)) {
            restartGame();
        }
        updateEffects();
        return;
    }

    handleInput();
    updatePlayers();
    updateBullets();
    updateBombs();

    // 子弹命中检测（委托GameManager）
    auto bulletHits = m_gameManager.checkBulletHits(m_bullets, m_player1, m_player2);
    for (const auto &hit : bulletHits) {
        addHitEffect(hit.pos);
        addFloatingText(hit.pos + QPointF(-6, -44), "+100", QColor(255, 255, 255), 26);
        startShake(7, 4.0);
    }

    // 炸弹命中检测（委托GameManager）
    int bombsExploded = 0;
    auto bombHits = m_gameManager.checkBombHits(m_bombs, m_player1, m_player2, bombsExploded);
    if (bombsExploded > 0) {
        startShake(16, 7.0);
        AudioManager::instance().playExplosion();
    }
    for (const auto &hit : bombHits) {
        addHitEffect(hit.pos);
        addFloatingText(hit.pos + QPointF(-12, -48), "+100", QColor(255, 255, 255), 28);
    }

    // 生命状态检测（委托GameManager）
    auto lifeEvents = m_gameManager.checkLifeState(m_player1, m_player2);
    for (const auto &le : lifeEvents) {
        addVoidEffect(le.effectX);
        startShake(12, 5.2);
        if (le.respawned) {
            Player &p = (le.playerId == 1) ? m_player1 : m_player2;
            addRespawnEffect(p);
        }
    }

    updateEffects();

    m_gameManager.tickKillFeed();
    if (m_shakeFrames > 0) {
        --m_shakeFrames;
        if (m_shakeFrames == 0) {
            m_shakeStrength = 0;
        }
    }
}

// ===================== 输入处理（读取GameConfig键位） =====================

void GameWidget::handleInput()
{
    m_player1.stopMove();
    m_player2.stopMove();

    handlePlayer1Input();

    if (m_gameMode == GameMode::HumanVsHuman) {
        handlePlayer2HumanInput();
    } else {
        handlePlayer2AIInput();
    }
}

void GameWidget::handlePlayer1Input()
{
    const PlayerKeys &k1 = GameConfig::instance().p1;

    if (m_keys.contains(k1.left)) {
        m_player1.moveLeft();
    }
    if (m_keys.contains(k1.right)) {
        m_player1.moveRight();
    }

    bool player1JumpDown = m_keys.contains(k1.up);
    if (player1JumpDown && !m_player1JumpHeld) {
        m_player1.jump();
        addJumpEffect(m_player1);
    }
    m_player1JumpHeld = player1JumpDown;

    if (m_keys.contains(k1.shoot) && m_player1.canShoot()) {
        QPointF p = m_player1.shootPosition();
        Bullet bullet(p.x(), p.y(), m_player1.direction(), m_player1.id());
        m_player1.applyRecoil(m_player1.direction(), bullet.recoilPower());
        m_player1.consumeAmmo();
        m_bullets.push_back(bullet);
        addMuzzleEffect(p, m_player1.direction());
        startShake(3, 1.6);
        AudioManager::instance().playShoot();
    }

    if (m_keys.contains(k1.bomb) && m_player1.canPlaceBomb()) {
        QPointF p = m_player1.bombPosition();
        m_bombs.push_back(Bomb(p.x(), p.y(), m_player1.id(), m_player1.direction() * 3.3, -5.0));
        m_player1.consumeBomb();
    }
}

void GameWidget::handlePlayer2HumanInput()
{
    const PlayerKeys &k2 = GameConfig::instance().p2;

    if (m_keys.contains(k2.left)) {
        m_player2.moveLeft();
    }
    if (m_keys.contains(k2.right)) {
        m_player2.moveRight();
    }

    bool player2JumpDown = m_keys.contains(k2.up);
    if (player2JumpDown && !m_player2JumpHeld) {
        m_player2.jump();
        addJumpEffect(m_player2);
    }
    m_player2JumpHeld = player2JumpDown;

    if (m_keys.contains(k2.shoot) && m_player2.canShoot()) {
        QPointF p = m_player2.shootPosition();
        Bullet bullet(p.x(), p.y(), m_player2.direction(), m_player2.id());
        m_player2.applyRecoil(m_player2.direction(), bullet.recoilPower());
        m_player2.consumeAmmo();
        m_bullets.push_back(bullet);
        addMuzzleEffect(p, m_player2.direction());
        startShake(3, 1.6);
        AudioManager::instance().playShoot();
    }

    if (m_keys.contains(k2.bomb) && m_player2.canPlaceBomb()) {
        QPointF p = m_player2.bombPosition();
        m_bombs.push_back(Bomb(p.x(), p.y(), m_player2.id(), m_player2.direction() * 3.3, -5.0));
        m_player2.consumeBomb();
    }
}


void GameWidget::handlePlayer2AIInput()
{
    if (m_aiDecisionCounter <= 0) {
        GameState state = collectStateForP2();

        if (m_gameMode == GameMode::HumanVsSimpleAI) {
            // 简单 AI：强制使用写死规则，不走神经网络权重。
            m_lastAIAction = m_aiController.fallbackRuleAction(state);
        } else {
            // 困难 AI：优先使用训练权重；如果加载失败，AIController 内部会自动 fallback。
            m_lastAIAction = m_aiController.decideAction(state);
        }

        m_aiDecisionCounter = AI_DECISION_INTERVAL;
    } else {
        --m_aiDecisionCounter;
    }

    applyAIActionToP2(m_lastAIAction);
}

// ===================== 玩家更新（碰撞委托GameManager） =====================

void GameWidget::updatePlayers()
{
    const PlayerKeys &k1 = GameConfig::instance().p1;
    const PlayerKeys &k2 = GameConfig::instance().p2;

    bool player1DropDown = m_keys.contains(k1.down);
    bool player2DropDown = m_keys.contains(k2.down);

    bool p1DropPressed = player1DropDown && !m_player1DropHeld;
    bool p2DropPressed = player2DropDown && !m_player2DropHeld;

    m_player1DropHeld = player1DropDown;
    m_player2DropHeld = player2DropDown;

    updateOnePlayer(m_player1, p1DropPressed, m_player1DropTimer);
    updateOnePlayer(m_player2, p2DropPressed, m_player2DropTimer);
}

void GameWidget::updateOnePlayer(Player &player, bool dropDownKey, int &dropTimer)
{
    if (dropTimer > 0) {
        --dropTimer;
    }

    if (dropDownKey && player.isOnGround() && dropTimer == 0) {
        dropTimer = 1;
        player.setOnGround(false);
        player.setY(player.y() + 8);
        player.setVelocityY(2.2);
    }

    double previousBottom = player.y() + player.height();
    player.update();

    if (player.x() < 4) {
        player.setX(4);
        if (player.velocityX() < 0) {
            player.setVelocityX(0);
        }
    }
    if (player.x() + player.width() > width() - 4) {
        player.setX(width() - player.width() - 4);
        if (player.velocityX() > 0) {
            player.setVelocityX(0);
        }
    }

    bool ignorePlatform = dropTimer > 0;
    bool wasOnGround = player.isOnGround();
    bool landed = m_gameManager.collidePlayerWithPlatforms(player, previousBottom, ignorePlatform);
    if (!wasOnGround && landed) {
        addLandingEffects(player);
    }
}

// ===================== 子弹/炸弹更新（平台碰撞委托GameManager） =====================

void GameWidget::updateBullets()
{
    for (Bullet &bullet : m_bullets) {
        bullet.update();
        if (!bullet.isAlive()) {
            continue;
        }

        SparkEvent spark;
        if (m_gameManager.collideBulletWithPlatforms(bullet, spark)) {
            addSparkEffect(spark.pos, spark.direction);
        }
    }

    m_bullets.erase(
        std::remove_if(m_bullets.begin(), m_bullets.end(),
                       [=](const Bullet &bullet) {
                           return !bullet.isAlive() || bullet.isOutOfScreen(width(), height());
                       }),
        m_bullets.end());
}

void GameWidget::updateBombs()
{
    for (Bomb &bomb : m_bombs) {
        double previousBottom = bomb.y() + bomb.height();
        bomb.update();
        m_gameManager.collideBombWithPlatforms(bomb, previousBottom);
    }

    m_bombs.erase(
        std::remove_if(m_bombs.begin(), m_bombs.end(),
                       [](const Bomb &bomb) {
                           return bomb.shouldRemove() || bomb.y() > GameConst::kVoidDeathY;
                       }),
        m_bombs.end());
}

// ===================== 视觉特效 =====================

void GameWidget::updateEffects()
{
    for (VisualEffect &effect : m_effects) {
        effect.pos += effect.velocity;
        if (effect.type == 6) {
            effect.velocity.setY(effect.velocity.y() - 0.015);
        }
        --effect.frames;
    }

    m_effects.erase(
        std::remove_if(m_effects.begin(), m_effects.end(),
                       [](const VisualEffect &effect) {
                           return effect.frames <= 0;
                       }),
        m_effects.end());
}

void GameWidget::addLandingEffects(const Player &player)
{
    VisualEffect e;
    e.pos = QPointF(player.center().x(), player.y() + player.height() + 2);
    e.velocity = QPointF(0, 0);
    e.frames = 18;
    e.maxFrames = 18;
    e.type = 0;
    e.text.clear();
    e.color = QColor(230, 225, 190);
    e.size = 1.0;
    e.direction = 1;
    m_effects.push_back(e);
}

void GameWidget::addJumpEffect(const Player &player)
{
    VisualEffect e;
    e.pos = QPointF(player.center().x(), player.y() + player.height() + 3);
    e.velocity = QPointF(0, 0.15);
    e.frames = 14;
    e.maxFrames = 14;
    e.type = 0;
    e.text.clear();
    e.color = QColor(225, 225, 210);
    e.size = 0.8;
    e.direction = 1;
    m_effects.push_back(e);
}

void GameWidget::addRespawnEffect(const Player &player)
{
    VisualEffect e;
    e.pos = player.center();
    e.velocity = QPointF(0, 0);
    e.frames = 40;
    e.maxFrames = 40;
    e.type = 1;
    e.text.clear();
    e.color = QColor(120, 220, 255);
    e.size = 1.0;
    e.direction = 1;
    m_effects.push_back(e);
}

void GameWidget::addVoidEffect(double x)
{
    VisualEffect e;
    e.pos = QPointF(x, height() - 50);
    e.velocity = QPointF(0, 0);
    e.frames = 28;
    e.maxFrames = 28;
    e.type = 2;
    e.text = "LOST";
    e.color = QColor(255, 80, 60);
    e.size = 1.0;
    e.direction = 1;
    m_effects.push_back(e);
}

void GameWidget::addHitEffect(const QPointF &pos)
{
    VisualEffect hit;
    hit.pos = pos + QPointF(0, -34);
    hit.velocity = QPointF(0, -0.25);
    hit.frames = 16;
    hit.maxFrames = 16;
    hit.type = 3;
    hit.text.clear();
    hit.color = QColor(255, 230, 70);
    hit.size = 1.0;
    hit.direction = 1;
    m_effects.push_back(hit);

    addSparkEffect(pos, 1);
}

void GameWidget::addMuzzleEffect(const QPointF &pos, int direction)
{
    VisualEffect e;
    e.pos = pos + QPointF(direction * 13, -1);
    e.velocity = QPointF(direction * 0.3, 0);
    e.frames = 7;
    e.maxFrames = 7;
    e.type = 4;
    e.text.clear();
    e.color = QColor(255, 235, 90);
    e.size = 1.0;
    e.direction = direction;
    m_effects.push_back(e);
}

void GameWidget::addSparkEffect(const QPointF &pos, int direction)
{
    VisualEffect e;
    e.pos = pos;
    e.velocity = QPointF(direction * 0.25, -0.15);
    e.frames = 11;
    e.maxFrames = 11;
    e.type = 5;
    e.text.clear();
    e.color = QColor(255, 230, 70);
    e.size = 1.0;
    e.direction = direction;
    m_effects.push_back(e);
}

void GameWidget::addFloatingText(const QPointF &pos, const QString &text, const QColor &color, double size)
{
    VisualEffect e;
    e.pos = pos;
    e.velocity = QPointF(0, -1.45);
    e.frames = 34;
    e.maxFrames = 34;
    e.type = 6;
    e.text = text;
    e.color = color;
    e.size = size;
    e.direction = 1;
    m_effects.push_back(e);
}

void GameWidget::startShake(int frames, double strength)
{
    if (strength >= m_shakeStrength || frames >= m_shakeFrames) {
        m_shakeFrames = frames;
        m_shakeStrength = strength;
    }
}

// ===================== 绘制 =====================

void GameWidget::drawBackground(QPainter &painter) const
{
    painter.fillRect(rect(), QColor(230, 240, 160));

    QPixmap bg(":/assets/map_user_clean.png");
    if (!bg.isNull()) {
        painter.drawPixmap(rect(), bg);
    } else {
        QLinearGradient fallback(0, 0, 0, height());
        fallback.setColorAt(0.0, QColor(234, 245, 150));
        fallback.setColorAt(1.0, QColor(115, 140, 80));
        painter.fillRect(rect(), fallback);
    }
}

void GameWidget::drawPlatforms(QPainter &painter) const
{
    const bool debugCollision = false;

    if (!debugCollision) {
        Q_UNUSED(painter);
        return;
    }

    painter.save();
    painter.setPen(QPen(QColor(255, 0, 0, 130), 1, Qt::DashLine));
    painter.setBrush(QColor(255, 0, 0, 35));
    for (const Platform &platform : m_gameManager.platforms()) {
        painter.drawRect(platform.rectF());
    }
    painter.restore();
}

void GameWidget::drawEffects(QPainter &painter) const
{
    for (const VisualEffect &effect : m_effects) {
        double t = static_cast<double>(effect.frames) / effect.maxFrames;
        int alpha = qBound(0, static_cast<int>(230 * t), 230);

        if (effect.type == 0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(effect.color.red(), effect.color.green(), effect.color.blue(), alpha));
            double spread = (1.0 - t) * 22.0 * effect.size;
            painter.drawEllipse(effect.pos + QPointF(-spread, 0), 7 + spread * 0.25, 3 + spread * 0.08);
            painter.drawEllipse(effect.pos + QPointF(spread, 0), 7 + spread * 0.25, 3 + spread * 0.08);
            painter.drawEllipse(effect.pos, 5 + spread * 0.18, 2.5 + spread * 0.08);
        } else if (effect.type == 1) {
            painter.setPen(QPen(QColor(120, 220, 255, alpha), 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(effect.pos, 42 * (1.12 - t), 42 * (1.12 - t));
            painter.drawLine(effect.pos + QPointF(0, -80 * t), effect.pos + QPointF(0, 28));
        } else if (effect.type == 2) {
            painter.setPen(QPen(QColor(255, 90, 60, alpha), 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(effect.pos, 34 * (1 - t) + 8, 12 * (1 - t) + 4);
            painter.setFont(QFont("Arial", 11, QFont::Bold));
            painter.drawText(effect.pos + QPointF(-20, -18), effect.text);
        } else if (effect.type == 3) {
            QPixmap hit(":/assets/hit_effect.png");
            painter.save();
            painter.setOpacity(qBound(0.0, t, 1.0));
            if (!hit.isNull()) {
                double scale = 0.38 + (1.0 - t) * 0.22;
                QSizeF size(hit.width() * scale, hit.height() * scale);
                QRectF target(effect.pos.x() - size.width() / 2.0,
                              effect.pos.y() - size.height() / 2.0,
                              size.width(), size.height());
                painter.drawPixmap(target.toRect(), hit);
            } else {
                painter.setPen(QPen(QColor(70, 55, 10), 3));
                painter.setFont(QFont("Arial", 24, QFont::Bold));
                painter.drawText(effect.pos, "HIT");
            }
            painter.restore();
        } else if (effect.type == 4) {
            QPixmap flash(":/assets/muzzle_flash.png");
            QRectF target(effect.pos.x() - 8, effect.pos.y() - 12, 34, 24);
            painter.save();
            painter.setOpacity(qBound(0.0, t, 1.0));
            if (effect.direction < 0) {
                painter.translate(target.center());
                painter.scale(-1, 1);
                QRectF mirrored(-target.width() / 2.0, -target.height() / 2.0,
                                target.width(), target.height());
                if (!flash.isNull()) {
                    painter.drawPixmap(mirrored.toRect(), flash);
                }
            } else if (!flash.isNull()) {
                painter.drawPixmap(target.toRect(), flash);
            }
            if (flash.isNull()) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(255, 240, 85, alpha));
                QPointF pts[3] = { QPointF(effect.pos.x(), effect.pos.y()),
                                   QPointF(effect.pos.x() + effect.direction * 34, effect.pos.y() - 9),
                                   QPointF(effect.pos.x() + effect.direction * 34, effect.pos.y() + 9) };
                painter.drawPolygon(pts, 3);
            }
            painter.restore();
        } else if (effect.type == 5) {
            painter.setPen(QPen(QColor(255, 235, 90, alpha), 2));
            for (int i = 0; i < 7; ++i) {
                double angle = i * 0.90 + (1.0 - t) * 1.2;
                double len = 8 + 16 * (1.0 - t);
                QPointF a = effect.pos + QPointF(qCos(angle) * 3, qSin(angle) * 3);
                QPointF b = effect.pos + QPointF(qCos(angle) * len, qSin(angle) * len);
                painter.drawLine(a, b);
            }
        } else if (effect.type == 6) {
            painter.save();
            painter.setOpacity(qBound(0.0, t, 1.0));
            painter.setFont(QFont("Arial", static_cast<int>(effect.size), QFont::Bold));
            painter.setPen(QPen(QColor(80, 80, 80, alpha), 3));
            painter.drawText(effect.pos + QPointF(2, 2), effect.text);
            painter.setPen(QColor(effect.color.red(), effect.color.green(), effect.color.blue(), alpha));
            painter.drawText(effect.pos, effect.text);
            painter.restore();
        }
    }
}

void GameWidget::drawHealthAndAmmo(QPainter &painter, const Player &player, int lives, bool leftSide) const
{
    const int panelWidth = 205;
    const int panelHeight = 46;
    const int y = height() - 98;
    const int x = leftSide ? width() / 2 - panelWidth - 10 : width() / 2 + 10;

    painter.setPen(QPen(QColor(75, 75, 75), 2));
    painter.setBrush(QColor(255, 255, 255, 238));
    painter.drawRect(x, y, panelWidth, panelHeight);

    QPixmap portrait(player.id() == 1 ? ":/assets/player_blue.png" : ":/assets/player_red.png");
    painter.setPen(QPen(QColor(55, 55, 55), 1));
    painter.setBrush(QColor(238, 238, 238));
    painter.drawRect(x + 7, y + 5, 34, 34);
    if (!portrait.isNull()) {
        painter.drawPixmap(QRect(x + 5, y + 0, 42, 44), portrait);
    }

    painter.setPen(QColor(25, 25, 25));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(x + 48, y + 17, QString("Player %1").arg(player.id()));

    painter.setFont(QFont("Arial", 13, QFont::Bold));
    painter.drawText(x + panelWidth - 47, y + 18, QString::number(lives));
    painter.setPen(QColor(220, 30, 45));
    painter.drawText(x + panelWidth - 26, y + 18, QString::fromUtf8("♥"));

    const int barX = x + 48;
    const int barY = y + 26;
    const int barW = 150;
    const int barH = 12;
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    painter.setBrush(QColor(120, 120, 120));
    painter.drawRect(barX, barY, barW, barH);

    int hpW = qBound(0, player.hp(), 100) * barW / 100;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(95, 225, 55));
    painter.drawRect(barX + 1, barY + 1, qMax(0, hpW - 2), barH - 2);
}

void GameWidget::drawOneBadge(QPainter &painter, const Player &player) const
{
    if (player.y() < -10 || player.y() > height() - 30) {
        return;
    }

    QPointF c = player.center();
    QRectF box(c.x() - 15, player.y() - 26, 30, 17);
    painter.setPen(QPen(QColor(55, 55, 55), 1));
    painter.setBrush(QColor(255, 255, 255, 230));
    painter.drawRect(box);

    painter.setPen(QColor(40, 40, 40));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(box, Qt::AlignCenter, QString::number(player.ammo()));
}

void GameWidget::drawPlayerBadges(QPainter &painter) const
{
    drawOneBadge(painter, m_player1);
    drawOneBadge(painter, m_player2);
}

void GameWidget::drawOffscreenMarker(QPainter &painter, const Player &player) const
{
    if (player.y() >= 0 && player.y() <= height() - 20) {
        return;
    }

    double x = qBound(26.0, player.center().x(), static_cast<double>(width() - 26));
    int y = player.y() < 0 ? 34 : height() - 24;
    QColor color = player.id() == 1 ? QColor(60, 90, 210) : QColor(220, 65, 35);

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    if (player.y() < 0) {
        QPointF pts[3] = { QPointF(x, y - 18), QPointF(x - 10, y + 4), QPointF(x + 10, y + 4) };
        painter.drawPolygon(pts, 3);
    } else {
        QPointF pts[3] = { QPointF(x, y + 18), QPointF(x - 10, y - 4), QPointF(x + 10, y - 4) };
        painter.drawPolygon(pts, 3);
    }

    painter.setPen(QColor(65, 65, 65));
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.drawText(QRectF(x - 34, y + 5, 68, 18), Qt::AlignCenter,
                     QString::number(qAbs(static_cast<int>(player.y()))));
}

void GameWidget::drawStatus(QPainter &painter) const
{
    drawOffscreenMarker(painter, m_player1);
    drawOffscreenMarker(painter, m_player2);
    drawHealthAndAmmo(painter, m_player1, m_gameManager.player1Lives(), true);
    drawHealthAndAmmo(painter, m_player2, m_gameManager.player2Lives(), false);

    if (m_gameManager.killFeedFrames() > 0 && !m_gameManager.killFeed().isEmpty()) {
        painter.setFont(QFont("Arial", 13, QFont::Bold));
        painter.setPen(QPen(QColor(105, 35, 20, 200), 3));
        painter.drawText(QPointF(13, 39), m_gameManager.killFeed());
        painter.setPen(QColor(190, 55, 35));
        painter.drawText(QPointF(12, 38), m_gameManager.killFeed());
    }

// 当前模式提示
painter.setFont(QFont("Arial", 10, QFont::Bold));
if (m_gameMode == GameMode::HumanVsSimpleAI) {
    painter.setPen(QColor(60, 220, 160));
    painter.drawText(width() - 210, 20, "Mode: Single - Simple AI");
    painter.drawText(width() - 210, 36, "AI: fallback rules");
} else if (m_gameMode == GameMode::HumanVsHardAI) {
    painter.setPen(QColor(60, 180, 255));
    painter.drawText(width() - 210, 20, "Mode: Single - Hard AI");
    painter.drawText(width() - 210, 36, m_aiModelLoaded ? "AI Model: loaded" : "AI Model: fallback");
} else {
    painter.setPen(QColor(180, 180, 180));
    painter.drawText(width() - 210, 20, "Mode: PvP");
    painter.drawText(width() - 210, 36, "P2: keyboard controlled");
}
}

// ===================== 绘制主函数 =====================

void GameWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    drawBackground(painter);

    painter.save();
    if (m_shakeFrames > 0) {
        double sx = qSin(m_frameCounter * 1.73) * m_shakeStrength;
        double sy = qCos(m_frameCounter * 1.37) * m_shakeStrength;
        painter.translate(sx, sy);
    }

    drawPlatforms(painter);

    for (const Bomb &bomb : m_bombs) {
        bomb.draw(painter);
    }

    for (const Bullet &bullet : m_bullets) {
        bullet.draw(painter);
    }

    m_player1.draw(painter);
    m_player2.draw(painter);
    drawPlayerBadges(painter);

    drawEffects(painter);
    painter.restore();

    drawStatus(painter);

    if (m_gameManager.isGameOver()) {
        painter.setPen(QPen(Qt::black, 4));
        painter.setBrush(QColor(255, 255, 255, 236));
        QRectF box(width() / 2 - 230, height() / 2 - 86, 460, 160);
        painter.drawRoundedRect(box, 14, 14);

        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 30, QFont::Bold));
        painter.drawText(box.adjusted(0, 24, 0, -60), Qt::AlignCenter, m_gameManager.winnerText());

        painter.setFont(QFont("Arial", 13, QFont::Bold));
        painter.drawText(box.adjusted(0, 84, 0, -18), Qt::AlignCenter, "Press R to Restart");
        painter.drawText(box.adjusted(0, 108, 0, 0), Qt::AlignCenter, "Press P/Esc to Open Menu");
    }
}

// ===================== 键盘事件（含暂停） =====================


void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat()) {
        const bool pausePressed =
            event->key() == GameConfig::instance().pauseKey ||
            event->key() == Qt::Key_Escape ||
            event->key() == Qt::Key_P;

        if (pausePressed) {
            if (m_timer->isActive()) {
                m_timer->stop();
            }
            m_keys.clear();

            PauseDialog dialog(this);
            int result = dialog.exec();

            if (result == 1) {
                if (!m_gameManager.isGameOver()) {
                    m_timer->start(16);
                }
                setFocus();
            } else if (result == 2) {
                restartGame();
                setFocus();
            } else if (result == 3) {
                emit sigGoToMainMenu();
            } else {
                if (!m_gameManager.isGameOver()) {
                    m_timer->start(16);
                }
                setFocus();
            }
            return;
        }

        if (m_gameManager.isGameOver()) {
            if (event->key() == Qt::Key_R) {
                restartGame();
            }
            return;
        }

        m_keys.insert(event->key());
    }
}

void GameWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat()) {
        m_keys.remove(event->key());
    }
}

// ===================== AI 相关 =====================

GameState GameWidget::collectStateForP2() const
{
    GameState state;
    state.values.resize(AI_STATE_SIZE, 0.0f);

    // P2 自身状态
    state.values[0] = static_cast<float>(m_player2.x()) / 800.0f;
    state.values[1] = static_cast<float>(m_player2.y()) / 600.0f;
    state.values[2] = static_cast<float>(m_player2.velocityX()) / 10.0f;
    state.values[3] = static_cast<float>(m_player2.velocityY()) / 20.0f;
    state.values[4] = static_cast<float>(m_player2.hp()) / 100.0f;
    state.values[5] = m_player2.isOnGround() ? 1.0f : 0.0f;
    state.values[6] = static_cast<float>(m_player2.direction());

    // P1 敌人状态
    state.values[7] = static_cast<float>(m_player1.x()) / 800.0f;
    state.values[8] = static_cast<float>(m_player1.y()) / 600.0f;
    state.values[9] = static_cast<float>(m_player1.velocityX()) / 10.0f;
    state.values[10] = static_cast<float>(m_player1.velocityY()) / 20.0f;
    state.values[11] = static_cast<float>(m_player1.hp()) / 100.0f;

    // 相对位置
    state.values[12] = static_cast<float>(m_player1.x() - m_player2.x()) / 800.0f;
    state.values[13] = static_cast<float>(m_player1.y() - m_player2.y()) / 600.0f;

    // 冷却
    state.values[14] = m_player2.canShoot() ? 0.0f : 1.0f;
    state.values[15] = static_cast<float>(m_player2.bombCooldownRemaining()) / 150.0f;

    // 最近敌方子弹
    float nearestBulletDx = 1.0f;
    float nearestBulletDy = 1.0f;
    float minBulletDist = 999999.0f;
    for (const Bullet &b : m_bullets) {
        if (b.ownerId() == m_player1.id() && b.isAlive()) {
            float bdx = static_cast<float>(b.center().x() - m_player2.center().x()) / 800.0f;
            float bdy = static_cast<float>(b.center().y() - m_player2.center().y()) / 600.0f;
            float dist = bdx * bdx + bdy * bdy;
            if (dist < minBulletDist) {
                minBulletDist = dist;
                nearestBulletDx = bdx;
                nearestBulletDy = bdy;
            }
        }
    }
    state.values[16] = nearestBulletDx;
    state.values[17] = nearestBulletDy;

    // 最近敌方炸弹
    float nearestBombDx = 1.0f;
    float nearestBombDy = 1.0f;
    float minBombDist = 999999.0f;
    bool foundBomb = false;
    for (const Bomb &bomb : m_bombs) {
        if (bomb.ownerId() == m_player1.id() && !bomb.isExploded()) {
            float bdx = static_cast<float>(bomb.center().x() - m_player2.center().x()) / 800.0f;
            float bdy = static_cast<float>(bomb.center().y() - m_player2.center().y()) / 600.0f;
            float dist = bdx * bdx + bdy * bdy;
            if (dist < minBombDist) {
                minBombDist = dist;
                nearestBombDx = bdx;
                nearestBombDy = bdy;
                foundBomb = true;
            }
        }
    }
    state.values[18] = nearestBombDx;
    state.values[19] = nearestBombDy;
    state.values[20] = foundBomb ? 1.0f : 0.0f;

    return state;
}

void GameWidget::applyAIActionToP2(AIAction action)
{
    switch (action) {
    case AIAction::Idle:
        break;
    case AIAction::MoveLeft:
        m_player2.moveLeft();
        break;
    case AIAction::MoveRight:
        m_player2.moveRight();
        break;
    case AIAction::Jump:
        if (m_player2.isOnGround()) {
            m_player2.jump();
            addJumpEffect(m_player2);
        }
        break;
    case AIAction::Shoot:
        if (m_player2.canShoot()) {
            QPointF p = m_player2.shootPosition();
            Bullet bullet(p.x(), p.y(), m_player2.direction(), m_player2.id());
            m_player2.applyRecoil(m_player2.direction(), bullet.recoilPower());
            m_player2.consumeAmmo();
            m_bullets.push_back(bullet);
            addMuzzleEffect(p, m_player2.direction());
            startShake(3, 1.6);
            AudioManager::instance().playShoot();
        }
        break;
    case AIAction::Bomb:
        if (m_player2.canPlaceBomb()) {
            QPointF p = m_player2.bombPosition();
            m_bombs.push_back(Bomb(p.x(), p.y(), m_player2.id(), m_player2.direction() * 3.3, -5.0));
            m_player2.consumeBomb();
        }
        break;
    case AIAction::MoveLeftShoot:
        m_player2.moveLeft();
        if (m_player2.canShoot()) {
            QPointF p = m_player2.shootPosition();
            Bullet bullet(p.x(), p.y(), m_player2.direction(), m_player2.id());
            m_player2.applyRecoil(m_player2.direction(), bullet.recoilPower());
            m_player2.consumeAmmo();
            m_bullets.push_back(bullet);
            addMuzzleEffect(p, m_player2.direction());
            startShake(3, 1.6);
            AudioManager::instance().playShoot();
        }
        break;
    case AIAction::MoveRightShoot:
        m_player2.moveRight();
        if (m_player2.canShoot()) {
            QPointF p = m_player2.shootPosition();
            Bullet bullet(p.x(), p.y(), m_player2.direction(), m_player2.id());
            m_player2.applyRecoil(m_player2.direction(), bullet.recoilPower());
            m_player2.consumeAmmo();
            m_bullets.push_back(bullet);
            addMuzzleEffect(p, m_player2.direction());
            startShake(3, 1.6);
            AudioManager::instance().playShoot();
        }
        break;
    }
}
