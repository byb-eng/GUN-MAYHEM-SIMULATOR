#ifndef AI_CONTROLLER_H
#define AI_CONTROLLER_H

#include "ai_types.h"
#include "simple_mlp.h"
#include <string>

class AIController {
public:
    AIController();

    bool loadModel(const std::string &path);
    AIAction decideAction(const GameState &state);
    AIAction fallbackRuleAction(const GameState &state);
    bool isModelLoaded() const;

private:
    SimpleMLP m_model;
    bool m_loaded;
};

#endif // AI_CONTROLLER_H
