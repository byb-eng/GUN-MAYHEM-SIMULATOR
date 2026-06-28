#include "simple_mlp.h"

#include <QFile>
#include <QTextStream>
#include <algorithm>

bool SimpleMLP::loadWeights(const std::string &path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_loaded = false;
        return false;
    }

    QTextStream in(&file);

    int numLayers = 0;
    in >> numLayers;
    if (in.status() != QTextStream::Ok || numLayers <= 0 || numLayers > 10) {
        m_loaded = false;
        return false;
    }

    m_layers.clear();
    m_layers.resize(numLayers);

    for (int l = 0; l < numLayers; ++l) {
        int inSize = 0;
        int outSize = 0;
        in >> inSize >> outSize;
        if (in.status() != QTextStream::Ok || inSize <= 0 || outSize <= 0) {
            m_loaded = false;
            return false;
        }

        m_layers[l].inputSize = inSize;
        m_layers[l].outputSize = outSize;
        m_layers[l].weights.resize(outSize, std::vector<float>(inSize, 0.0f));
        m_layers[l].bias.resize(outSize, 0.0f);

        for (int o = 0; o < outSize; ++o) {
            for (int i = 0; i < inSize; ++i) {
                in >> m_layers[l].weights[o][i];
                if (in.status() != QTextStream::Ok) {
                    m_loaded = false;
                    return false;
                }
            }
        }

        for (int o = 0; o < outSize; ++o) {
            in >> m_layers[l].bias[o];
            if (in.status() != QTextStream::Ok) {
                m_loaded = false;
                return false;
            }
        }
    }

    if (m_layers.empty() ||
        m_layers.front().inputSize <= 0 ||
        m_layers.back().outputSize <= 0) {
        m_loaded = false;
        return false;
    }

    m_loaded = true;
    return true;
}

std::vector<float> SimpleMLP::forward(const std::vector<float> &input) const
{
    if (!m_loaded || m_layers.empty()) {
        return {};
    }

    std::vector<float> current = input;

    for (size_t l = 0; l < m_layers.size(); ++l) {
        const Layer &layer = m_layers[l];

        if (static_cast<int>(current.size()) != layer.inputSize) {
            return {};
        }

        std::vector<float> output(layer.outputSize, 0.0f);

        for (int o = 0; o < layer.outputSize; ++o) {
            float sum = layer.bias[o];
            for (int i = 0; i < layer.inputSize; ++i) {
                sum += layer.weights[o][i] * current[i];
            }
            if (l < m_layers.size() - 1) {
                sum = relu(sum);
            }
            output[o] = sum;
        }

        current = output;
    }

    return current;
}

int SimpleMLP::argmaxAction(const std::vector<float> &qValues) const
{
    if (qValues.empty()) {
        return 0;
    }
    return static_cast<int>(std::max_element(qValues.begin(), qValues.end()) - qValues.begin());
}

bool SimpleMLP::isLoaded() const
{
    return m_loaded;
}

float SimpleMLP::relu(float x)
{
    return x > 0.0f ? x : 0.0f;
}
