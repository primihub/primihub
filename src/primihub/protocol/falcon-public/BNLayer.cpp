#pragma once
#include "BNLayer.h"
#include "Functionalities.h"
using namespace std;

namespace primihub{
    namespace falcon
{

	BNLayer::BNLayer(BNConfig *conf, int _layerNum)
		: Layer(_layerNum),
		  conf(conf->inputSize, conf->numBatches),
		  gamma(conf->numBatches),
		  beta(conf->numBatches),
		  xhat(conf->numBatches * conf->inputSize),
		  sigma(conf->numBatches),
		  activations(conf->inputSize * conf->numBatches),
		  deltas(conf->inputSize * conf->numBatches)
	{
		initialize();
	};

	void BNLayer::initialize(){};

	void BNLayer::printLayer()
	{
		cout << "----------------------------------------------" << endl;
		cout << "(" << layerNum + 1 << ") BN Layer\t\t  " << conf.inputSize << " x "
			 << conf.numBatches << endl;
	}

	void BNLayer::forward(const RSSVectorMyType &inputActivation)
	{
		log_print("BN.forward");

		size_t B = conf.numBatches;
		size_t m = conf.inputSize;
		size_t EPSILON = (myType)(1 << (FLOAT_PRECISION - 8));
		// TODO: Accept initialization from the paper
		size_t INITIAL_GUESS = (myType)(1 << (FLOAT_PRECISION));
		size_t SQRT_ROUNDS = 4;

		vector<myType> eps(B, EPSILON), initG(B, INITIAL_GUESS);
		RSSVectorMyType epsilon(B), mu(B, make_pair(0, 0)), b(B);
		RSSVectorMyType divisor(B, make_pair(0, 0));

		// Compute mean
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				mu[i] = mu[i] + inputActivation[i * m + j];
		funcTruncatePublic(mu, m, B);

		// Compute x - mean
		RSSVectorMyType temp1(B * m);
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				temp1[i * m + j] = inputActivation[i * m + j] - mu[i];

		// Compute (x-mean)^2
		RSSVectorMyType temp2(B * m), temp3(B, make_pair(0, 0));
		funcDotProduct(temp1, temp1, temp2, B * m, true, FLOAT_PRECISION);
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				temp3[i] = temp3[i] + temp2[i * m + j];

		// Compute (variance + epsilon)
		funcTruncatePublic(temp3, m, B);
		funcGetShares(epsilon, eps);
		addVectors<RSSMyType>(temp3, epsilon, temp3, B);

		// Square Root
		funcGetShares(sigma, initG);
		for (int i = 0; i < SQRT_ROUNDS; ++i)
		{
			funcDivision(temp3, sigma, b, B);
			addVectors<RSSMyType>(sigma, b, sigma, B);
			funcTruncatePublic(sigma, 2, B);
		}

		// Normalized x (xhat)
		funcBatchNorm(temp1, sigma, xhat, m, B);

		// Scaling
		RSSVectorMyType g_repeat(B * m);
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				g_repeat[i * m + j] = gamma[i];

		funcDotProduct(g_repeat, xhat, activations, B * m, true, FLOAT_PRECISION);
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				activations[i * m + j] = activations[i * m + j] + beta[i];
	}

	// https://kevinzakka.github.io/2016/09/14/batch_normalization/
	void BNLayer::computeDelta(RSSVectorMyType &prevDelta)
	{
		log_print("BN.computeDelta");

		size_t B = conf.numBatches;
		size_t m = conf.inputSize;

		// Derivative with xhat
		RSSVectorMyType g_repeat(B * m), dxhat(B * m);
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				g_repeat[i * m + j] = gamma[i];

		funcDotProduct(g_repeat, deltas, dxhat, B * m, true, FLOAT_PRECISION);

		// First term
		RSSVectorMyType temp1(B * m);
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				temp1[i * m + j] = ((myType)m) * dxhat[i * m + j];

		// Second term
		RSSVectorMyType temp2(B * m, make_pair(0, 0));
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				temp2[i * m] = temp2[i * m] + dxhat[i * m + j];

		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				temp2[i * m + j] = temp2[i * m];

		// Third term
		RSSVectorMyType temp3(B * m, make_pair(0, 0));
		funcDotProduct(dxhat, xhat, temp3, B * m, true, FLOAT_PRECISION);
		for (int i = 0; i < B; ++i)
			for (int j = 1; j < m; ++j)
				temp3[i * m] = temp3[i * m] + temp3[i * m + j];

		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				temp3[i * m + j] = temp3[i * m];

		funcDotProduct(temp3, xhat, temp3, B * m, true, FLOAT_PRECISION);

		// Numerator
		subtractVectors<RSSMyType>(temp1, temp2, temp1, B * m);
		subtractVectors<RSSMyType>(temp1, temp3, temp1, B * m);

		RSSVectorMyType temp4(B);
		for (int i = 0; i < B; ++i)
			temp4[i] = ((myType)m) * sigma[i];

		funcBatchNorm(temp1, temp4, prevDelta, m, B);
	}

	void BNLayer::updateEquations(const RSSVectorMyType &prevActivations)
	{
		log_print("BN.updateEquations");

		size_t B = conf.numBatches;
		size_t m = conf.inputSize;

		// Update beta
		RSSVectorMyType temp1(B, make_pair(0, 0));
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				temp1[i] = temp1[i] + deltas[i * m + j];

		subtractVectors<RSSMyType>(beta, temp1, beta, B);

		// Update gamma
		RSSVectorMyType temp2(B * m, make_pair(0, 0)), temp3(B, make_pair(0, 0));
		funcDotProduct(xhat, deltas, temp2, B * m, true, FLOAT_PRECISION);
		for (int i = 0; i < B; ++i)
			for (int j = 0; j < m; ++j)
				temp3[i] = temp3[i] + temp2[i * m + j];

		subtractVectors<RSSMyType>(gamma, temp3, gamma, B);
	}
}// namespace primihub{
}