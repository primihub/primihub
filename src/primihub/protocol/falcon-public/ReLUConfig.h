
#pragma once
#include "LayerConfig.h"
#include "globals.h"
using namespace std;

class ReLUConfig : public LayerConfig
{
public:
	size_t inputDim = 0;
	size_t batchSize = 0;

	ReLUConfig(size_t _inputDim, size_t _batchSize)
	:inputDim(_inputDim),
	 batchSize(_batchSize), 
	 LayerConfig("ReLU")
	{};
};
