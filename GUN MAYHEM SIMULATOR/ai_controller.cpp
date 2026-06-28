#include "ai_controller.h"
#include <cmath>

AIController::AIController()
    : m_loaded(false)
{
}

bool AIController::loadModel(const std::string &path)
{
    m_loaded = m_model.loadWeights(path);
    return m_loaded;
}

AIAction AIController::decideAction(const GameState &state)
{
    if (m_loaded) {
        std::vector<float> qValues = m_model.forward(state.values);
        int action = m_model.argmaxAction(qValues);
        if (action < 0 || action >= AI_ACTION_SIZE) {
            return AIAction::Idle;
        }
        return static_cast<AIAction>(action);
    }
    return fallbackRuleAction(state);
}

AIAction AIController::fallbackRuleAction(const GameState &state)
{
    if (static_cast<int>(state.values.size()) < AI_STATE_SIZE) {
        return AIAction::Idle;
    }

    // 状态索引参考 ai_types.h / 文档第十节
    // 0:self_x, 1:self_y, 5:self_on_ground, 6:self_direction
    // 7:enemy_x, 8:enemy_y
    // 12:dx(enemy_x - self_x 归一化), 13:dy
    // 14:self_shoot_cd, 15:self_bomb_cd
    // 16:nearest_enemy_bullet_dx, 17:nearest_enemy_bullet_dy

    float dx = state.values[12]; // (enemy_x - self_x) / 800
    float dy = state.values[13]; // (enemy_y - self_y) / 600
    float selfOnGround = state.values[5];
    float shootCd = state.values[14];
    float bombCd = state.values[15];
    float bulletDx = state.values[16];
    float bulletDy = state.values[17];

    float absDx = std::fabs(dx);
    float absDy = std::fabs(dy);

    // 如果敌方子弹很近，跳跃躲避
    float bulletDist = std::sqrt(bulletDx * bulletDx + bulletDy * bulletDy);
    if (bulletDist < 0.15f && selfOnGround > 0.5f) {
        return AIAction::Jump;
    }

    // 距离很近且炸弹冷却好了，扔炸弹
    if (absDx < 0.125f && absDy < 0.15f && bombCd < 0.01f) {
        return AIAction::Bomb;
    }

    // 水平距离适中且基本同高度，射击
    if (absDx < 0.44f && absDy < 0.2f && shootCd < 0.01f) {
        if (dx < 0) {
            return AIAction::MoveLeftShoot;
        } else {
            return AIAction::MoveRightShoot;
        }
    }

    // 追踪敌人
    if (dx < -0.02f) {
        return AIAction::MoveLeft;
    } else if (dx > 0.02f) {
        return AIAction::MoveRight;
    }

    // 敌人在上方，跳跃
    if (dy < -0.1f && selfOnGround > 0.5f) {
        return AIAction::Jump;
    }

    return AIAction::Idle;
}

bool AIController::isModelLoaded() const
{
    return m_loaded;
}
