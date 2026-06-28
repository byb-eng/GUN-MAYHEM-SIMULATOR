#ifndef AI_TYPES_H
#define AI_TYPES_H

#include <vector>

enum class GameMode {
    HumanVsHuman,
    HumanVsSimpleAI,
    HumanVsHardAI
};

enum class AIAction {
    Idle = 0,
    MoveLeft = 1,
    MoveRight = 2,
    Jump = 3,
    Shoot = 4,
    Bomb = 5,
    MoveLeftShoot = 6,
    MoveRightShoot = 7
};

struct GameState {
    std::vector<float> values;
};

constexpr int AI_STATE_SIZE = 21;
constexpr int AI_ACTION_SIZE = 8;
constexpr int AI_DECISION_INTERVAL = 5;

#endif // AI_TYPES_H
