#ifndef GAMECONFIG_H
#define GAMECONFIG_H

#include <Qt>

struct PlayerKeys {
    int up;
    int down;
    int left;
    int right;
    int shoot;
    int bomb;
};

class GameConfig {
public:
    PlayerKeys p1;
    PlayerKeys p2;
    int pauseKey;

    static GameConfig& instance() {
        static GameConfig config;
        return config;
    }

private:
    GameConfig() {
        // P1: WASD + T(射击) + Y(炸弹)
        p1 = {Qt::Key_W, Qt::Key_S, Qt::Key_A, Qt::Key_D, Qt::Key_T, Qt::Key_Y};
        // P2: 方向键 + 逗号(射击) + 句号(炸弹)
        p2 = {Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right, Qt::Key_Comma, Qt::Key_Period};
        pauseKey = Qt::Key_P;
    }
};

#endif // GAMECONFIG_H
