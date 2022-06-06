
#pragma once
#include "LayerConfig.h"
#include "globals.h"
using namespace std;

class FCConfig : public LayerConfig
{
public:
	size_t inputDim = 0;
	size_t batchSize = 0;
	size_t outputDim = 0;

	FCConfig(size_t _inputDim, size_t _batchSize, size_t _outputDim)
	:inputDim(_inputDim),
	 batchSize(_batchSize), 
	 outputDim(_outputDim),
	 LayerConfig("FC")
	{};
};
