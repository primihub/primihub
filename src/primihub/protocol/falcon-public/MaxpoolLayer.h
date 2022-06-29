
#pragma once
#include "MaxpoolConfig.h"
#include "Layer.h"
#include "tools.h"
#include "connect.h"
#include "globals.h"
using namespace std;

namespace primihub
{
	namespace falcon
	{
		class MaxpoolLayer : public Layer
		{
		private:
			MaxpoolConfig conf;
			RSSVectorMyType activations;
			RSSVectorMyType deltas;
			RSSVectorSmallType maxPrime;

		public:
			// Constructor and initializer
			MaxpoolLayer(MaxpoolConfig *conf, int _layerNum);

			// Functions
			void printLayer() override;
			void forward(const RSSVectorMyType &inputActivation) override;
			void computeDelta(RSSVectorMyType &prevDelta) override;
			void updateEquations(const RSSVectorMyType &prevActivations) override;

			// Getters
			RSSVectorMyType *getActivation() { return &activations; };
			RSSVectorMyType *getDelta() { return &deltas; };
		};
	} // namespace primihub{
}