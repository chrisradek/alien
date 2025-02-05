#pragma once

#include "GenomeDescriptions.h"
#include "SimulationParameters.h"
#include "PreviewDescriptions.h"


class PreviewDescriptionConverter
{
public:
    static PreviewDescription convert(GenomeDescription const& genome, std::optional<int> selectedNode, SimulationParameters const& parameters);
};

