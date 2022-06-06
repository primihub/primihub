#include <pybind11/pybind11.h>
#include <iostream>
#include "paillier.h"
#include "crt_datapack.h"
#include <string>

#define BASE 62
#define PYTHON_INPUT_BASE 10