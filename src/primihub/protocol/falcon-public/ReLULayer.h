
#pragma once
#include "ReLUConfig.h"
#include "Layer.h"
#include "tools.h"
#include "connect.h"
#include "globals.h"
using namespace std;

namespace primihub{
    namespace falcon
{
extern int partyNum;


class ReLULayer : public Layer
{
private:
	ReLUConfig conf;
	RSSVectorMyType activations;
	RSSVectorMyType deltas;
	RSSVectorSmallType reluPrime;

public:
	//Constructor and initializer
	ReLULayer(ReLUConfig* conf, int _layerNum);

	//Functions
	void printLayer() override;
	void forward(const RSSVectorMyType& inputActivation) override;
	void computeDelta(RSSVectorMyType& prevDelta) override;
	void updateEquations(const RSSVectorMyType& prevActivations) override;

	//Getters
	RSSVectorMyType* getActivation() {return &activations;};
	RSSVectorMyType* getDelta() {return &deltas;};
};
}// namespace primihub{
}