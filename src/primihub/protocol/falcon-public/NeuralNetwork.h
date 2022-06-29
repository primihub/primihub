
#pragma once
#include "NeuralNetConfig.h"
#include "Layer.h"
#include "globals.h"
using namespace std;

namespace primihub
{
	namespace falcon
	{
		class NeuralNetwork
		{
		public:
			RSSVectorMyType inputData;
			RSSVectorMyType outputData;
			vector<Layer *> layers;

			NeuralNetwork(NeuralNetConfig *config);
			~NeuralNetwork();
			void forward();
			void backward();
			void computeDelta();
			void updateEquations();
			void predict(RSSVectorMyType &maxIndex);
			void getAccuracy(const RSSVectorMyType &maxIndex, vector<size_t> &counter);
		};
	} // namespace primihub{
}