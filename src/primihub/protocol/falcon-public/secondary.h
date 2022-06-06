#ifndef SECONDARY_H
#define SECONDARY_H

#pragma once
#include "globals.h"
#include "NeuralNetConfig.h"
#include "NeuralNetwork.h"

/******************* Main train and test functions *******************/
void parseInputs(int argc, char* argv[]);
void train(NeuralNetwork* net);
void test(bool PRELOADING, string network, NeuralNetwork* net);
// void generate_zeros(string name, size_t number, string network);
void preload_network(bool PRELOADING, string network, NeuralNetwork* net);
void loadData(string net, string dataset);
void readMiniBatch(NeuralNetwork* net, string phase);
void printNetwork(NeuralNetwork* net);
void selectNetwork(string network, string dataset, string security, NeuralNetConfig* config);
void runOnly(NeuralNetwork* net, size_t l, string what, string& network);

/********************* COMMUNICATION AND HELPERS *********************/
void start_m();
void end_m(std::string str);
void start_time();
void end_time(std::string str);
void start_rounds();
void end_rounds(std::string str);
void aggregateCommunication();
void print_usage(const char * bin);
double diff(timespec start, timespec end);
void deleteObjects();
#endif