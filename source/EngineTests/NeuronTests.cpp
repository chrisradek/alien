#include <cmath>
#include <gtest/gtest.h>

#include "EngineInterface/DescriptionHelper.h"
#include "EngineInterface/Descriptions.h"
#include "EngineInterface/SimulationController.h"
#include "IntegrationTestFramework.h"

class NeuronTests : public IntegrationTestFramework
{
public:
    NeuronTests()
        : IntegrationTestFramework()
    {}

    ~NeuronTests() = default;

protected:
    float scaledSigmoid(float value) const { return 2.0f / (1.0f + std::exp(-value)) - 1.0f; }
};

TEST_F(NeuronTests, bias)
{
    NeuronDescription neuron;
    neuron.biases = {0, 0, 1, 0, 0, 0, 0, -1};

    auto data = DataDescription().addCells({CellDescription().setId(1).setCellFunction(neuron).setMaxConnections(2).setExecutionOrderNumber(0)});

    _simController->setSimulationData(data);
    _simController->calcTimesteps(1);

    auto actualData = _simController->getSimulationData();
    auto actualCellById = getCellById(actualData);

    EXPECT_TRUE(approxCompare({0, 0, scaledSigmoid(1), 0, 0, 0, 0, scaledSigmoid(-1)}, actualCellById.at(1).activity.channels));
}

TEST_F(NeuronTests, weight)
{
    NeuronDescription neuron;
    neuron.weights[2][3] = 1;
    neuron.weights[2][7] = 0.5f;
    neuron.weights[5][3] = -3.5f;

    ActivityDescription activity;
    activity.channels = {0, 0, 0, 1, 0, 0, 0, 0.5f};

    auto data = DataDescription().addCells({
        CellDescription()
            .setId(1)
            .setPos({1.0f, 1.0f})
            .setCellFunction(NerveDescription())
            .setMaxConnections(2)
            .setExecutionOrderNumber(5)
            .setActivity(activity),
        CellDescription().setId(2).setPos({2.0f, 1.0f}).setCellFunction(neuron).setMaxConnections(2).setExecutionOrderNumber(0).setInputExecutionOrderNumber(5),
    });
    data.addConnection(1, 2);

    _simController->setSimulationData(data);
    _simController->calcTimesteps(1);

    auto actualData = _simController->getSimulationData();
    auto actualCellById = getCellById(actualData);

    EXPECT_TRUE(approxCompare({0, 0, scaledSigmoid(1.0f + 0.5f * 0.5f), 0, 0, scaledSigmoid(-3.5f), 0, 0}, actualCellById.at(2).activity.channels));
}
