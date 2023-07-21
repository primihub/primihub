#include <pybind11/pybind11.h>
#include <iostream>
#include "src/primihub/algorithm/opt_paillier/include/paillier.h"
#include "src/primihub/algorithm/opt_paillier/include/crt_datapack.h"
#include <string>

#define BASE 10
#define PYTHON_INPUT_BASE 10
#define CRT_MOD_MAX_DIMENSION 28
#define CRT_MOD_SIZE 70
