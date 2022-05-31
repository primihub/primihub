# Parallel Prefix Adder

The `PPA`(`Parallel Prefix Adder`) circuit built by kogge-stone algorithm has minimun fan-out of 1 at each node which implies faster performance in theory. Spliting some node in circuit built by kogge-stone algorithm will form a new circuit that output `MSB`, see comment in `kogge_stone.cpp` for the detail.

## Build and run test code
### BinaryEvaluatorTest
The test code contian cases for `PPA` and `MSB` integrated with aby3, so more approach to actual use.    
In the root directory of the project:
```
bazel build --config=linux :protocol_aby3_test
./bazel-out/k8-fastbuild/bin/protocol_aby3_test.runfiles/__main__/protocol_aby3_test
```
Then test code output message like:
```
[==========] Running 2 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 2 tests from BinaryEvaluatorTest
[ RUN      ] BinaryEvaluatorTest.Sh3_BinaryEngine_add_test
 ⭐️ init Rumtime : 0, 0x561cdff42de0
 ⭐️ init Rumtime : 1, 0x561cdff282b0
 ⭐️ init Rumtime : 2, 0x561cdff42f70
 ⭐️ init Rumtime : 2, 0x561cdff42d30
 ⭐️ init Rumtime : 0, 0x561cdff6f310
 ⭐️ init Rumtime : 1, 0x561cdff02100
[       OK ] BinaryEvaluatorTest.Sh3_BinaryEngine_add_test (348 ms)
[ RUN      ] BinaryEvaluatorTest.Sh3_BinaryEngine_add_msb_test
 ⭐️ init Rumtime : 2, 0x561cdff07810
 ⭐️ init Rumtime : 1, 0x561cdff03af0
 ⭐️ init Rumtime : 0, 0x561cdff00240
 ⭐️ init Rumtime : 1, 0x561cdff39560
 ⭐️ init Rumtime : 2, 0x561cdff7a7b0
 ⭐️ init Rumtime : 0, 0x561cdff07e70
[       OK ] BinaryEvaluatorTest.Sh3_BinaryEngine_add_msb_test (334 ms)
[----------] 2 tests from BinaryEvaluatorTest (682 ms total)

[----------] Global test environment tear-down
[==========] 2 tests from 1 test suite ran. (683 ms total)
[  PASSED  ] 2 tests.
```
### CircuitTest
Test case only for circuit itself which is not integrated with aby3 aim to check the circuit built is right or not.    
In the root directory of the project:
```
bazel build --config=linux :primitive_test
./bazel-out/k8-fastbuild/bin/primitive_test.runfiles/__main__/primitive_test
```
Then test code output message like:    
```
Running main() from gmock_main.cc
[==========] Running 5 tests from 4 test suites.
[----------] Global test environment set-up.
[----------] 1 test from ShareOTTest
[ RUN      ] ShareOTTest.SharedOT_eval_test
[       OK ] ShareOTTest.SharedOT_eval_test (25 ms)
[----------] 1 test from ShareOTTest (26 ms total)

[----------] 2 tests from CircuitTest
[ RUN      ] CircuitTest.BetaCircuit_int_Sh3Piecewise_Test
[       OK ] CircuitTest.BetaCircuit_int_Sh3Piecewise_Test (10 ms)
[ RUN      ] CircuitTest.garble_Test
d30f8ef52bbfbb59f06f1de916187147
[       OK ] CircuitTest.garble_Test (148 ms)
[----------] 2 tests from CircuitTest (159 ms total)

[----------] 1 test from BetaCircuit_int_Adder_Test
[ RUN      ] BetaCircuit_int_Adder_Test.int_adder
[       OK ] BetaCircuit_int_Adder_Test.int_adder (4 ms)
[----------] 1 test from BetaCircuit_int_Adder_Test (4 ms total)

[----------] 1 test from KoggeStone_int_Adder_test
[ RUN      ] KoggeStone_int_Adder_test.KoggeStone_int_adder
[       OK ] KoggeStone_int_Adder_test.KoggeStone_int_adder (766 ms)
[----------] 1 test from KoggeStone_int_Adder_test (766 ms total)

[----------] Global test environment tear-down
[==========] 5 tests from 4 test suites ran. (956 ms total)
[  PASSED  ] 5 tests.
```