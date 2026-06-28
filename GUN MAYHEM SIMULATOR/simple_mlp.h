#ifndef SIMPLE_MLP_H
#define SIMPLE_MLP_H

#include <vector>
#include <string>

class SimpleMLP {
public:
    bool loadWeights(const std::string &path);
    std::vector<float> forward(const std::vector<float> &input) const;
    int argmaxAction(const std::vector<float> &qValues) const;
    bool isLoaded() const;

private:
    struct Layer {
        int inputSize;
        int outputSize;
        std::vector<std::vector<float>> weights; // [out][in]
        std::vector<float> bias;                 // [out]
    };

    std::vector<Layer> m_layers;
    bool m_loaded = false;

    static float relu(float x);
};

#endif // SIMPLE_MLP_H
