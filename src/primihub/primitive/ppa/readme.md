# Parallel Prefix Adder

The `PPA`(`Parallel Prefix Adder`) circuit built by kogge-stone algorithm has minimun fan-out of 1 at each node which implies faster performance in theory. Spliting some node in circuit built by kogge-stone algorithm will form a new circuit that output `MSB`, see comment in `kogge_stone.cpp` for the detail.

## Build and run test code
### BinaryEvaluatorTest
The test code contian cases for `PPA` and `MSB` integrated with aby3, so more approach to actual use.
In the root directory of the project:
```
bazel build --config=linux_x86_64 //test/primihub/protocol:protocol_aby3_test
./bazel-out/k8-fastbuild/bin/test/primihub/protocol/protocol_aby3_test
```
Then test code output message like:
```
Running main() from gmock_main.cc
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from BinaryEvaluatorTest
[ RUN      ] BinaryEvaluatorTest.Sh3_BinaryEngine_add_msb_test
WARNING: Logging before InitGoogleLogging() is written to STDERR
I20230303 13:46:51.401960 337980 binary_evaluator_test.cc:103] ============> aSize: 64
I20230303 13:46:51.401978 337980 binary_evaluator_test.cc:104] ============> bSize: 64
I20230303 13:46:51.401980 337980 binary_evaluator_test.cc:105] ============> cSize: 1
I20230303 13:46:51.403293 338001 binary_evaluator_test.cc:127] partyID: 0============> a: [(2.09765625)
(12.09765625)
] 537
3097
I20230303 13:46:51.403312 338001 binary_evaluator_test.cc:128] partyID: 0============> b: [(-10.5)
(-10.5)
]-2688
-2688
I20230303 13:46:51.403317 338001 binary_evaluator_test.cc:129] partyID: 0============> ar: [(-36028797018963968.0)
]
I20230303 13:46:51.403321 338001 runtime.h:44] Init Runtime : 0, 0x564d74994b40
I20230303 13:46:51.403328 338001 binary_evaluator_test.cc:135] pIdx: 0==============>C.i64Size: 2
I20230303 13:46:51.403331 338001 binary_evaluator_test.cc:137] pIdx: 0==============>C.rows: 2
I20230303 13:46:51.403332 338001 binary_evaluator_test.cc:138] pIdx: 0==============>C.i64Cols: 1
I20230303 13:46:51.403333 338001 binary_evaluator_test.cc:140] pIdx: 0==============>C.bitCount: 1
I20230303 13:46:51.403344 338002 binary_evaluator_test.cc:127] partyID: 1============> a: [(2.09765625)
(12.09765625)
] 537
3097
I20230303 13:46:51.403357 338002 binary_evaluator_test.cc:128] partyID: 1============> b: [(-10.5)
(-10.5)
]-2688
-2688
I20230303 13:46:51.403363 338002 binary_evaluator_test.cc:129] partyID: 1============> ar: [(-36028797018963968.0)
]
I20230303 13:46:51.403367 338002 runtime.h:44] Init Runtime : 1, 0x564d74994c40
I20230303 13:46:51.403367 338003 binary_evaluator_test.cc:127] partyID: 2============> a: [(2.09765625)
(12.09765625)
] 537
3097
I20230303 13:46:51.403375 338002 binary_evaluator_test.cc:135] pIdx: 1==============>C.i64Size: 2
I20230303 13:46:51.403383 338002 binary_evaluator_test.cc:137] pIdx: 1==============>C.rows: 2
I20230303 13:46:51.403385 338002 binary_evaluator_test.cc:138] pIdx: 1==============>C.i64Cols: 1
I20230303 13:46:51.403388 338002 binary_evaluator_test.cc:140] pIdx: 1==============>C.bitCount: 1
I20230303 13:46:51.403380 338003 binary_evaluator_test.cc:128] partyID: 2============> b: [(-10.5)
(-10.5)
]-2688
-2688
I20230303 13:46:51.403393 338003 binary_evaluator_test.cc:129] partyID: 2============> ar: [(-36028797018963968.0)
]
I20230303 13:46:51.403398 338003 runtime.h:44] Init Runtime : 2, 0x564d74994d40
I20230303 13:46:51.403401 338003 binary_evaluator_test.cc:135] pIdx: 2==============>C.i64Size: 2
I20230303 13:46:51.403403 338003 binary_evaluator_test.cc:137] pIdx: 2==============>C.rows: 2
I20230303 13:46:51.403405 338003 binary_evaluator_test.cc:138] pIdx: 2==============>C.i64Cols: 1
I20230303 13:46:51.403407 338003 binary_evaluator_test.cc:140] pIdx: 2==============>C.bitCount: 1
I20230303 13:46:51.434855 338001 binary_evaluator_test.cc:197] pIdx: 0==============> C[0][0]: 0
I20230303 13:46:51.434862 338001 binary_evaluator_test.cc:197] pIdx: 0==============> C[0][1]: 0
I20230303 13:46:51.434865 338001 binary_evaluator_test.cc:200] pIdx: 0==============> C[1][0]: 0
I20230303 13:46:51.434867 338001 binary_evaluator_test.cc:200] pIdx: 0==============> C[1][1]: 1
I20230303 13:46:51.434882 338002 binary_evaluator_test.cc:197] pIdx: 1==============> C[0][0]: 1
I20230303 13:46:51.434890 338002 binary_evaluator_test.cc:197] pIdx: 1==============> C[0][1]: 1
I20230303 13:46:51.434892 338002 binary_evaluator_test.cc:200] pIdx: 1==============> C[1][0]: 0
I20230303 13:46:51.434895 338002 binary_evaluator_test.cc:200] pIdx: 1==============> C[1][1]: 0
I20230303 13:46:51.434901 338003 binary_evaluator_test.cc:197] pIdx: 2==============> C[0][0]: 0
I20230303 13:46:51.434911 338003 binary_evaluator_test.cc:197] pIdx: 2==============> C[0][1]: 1
I20230303 13:46:51.434914 338003 binary_evaluator_test.cc:200] pIdx: 2==============> C[1][0]: 1
I20230303 13:46:51.434917 338003 binary_evaluator_test.cc:200] pIdx: 2==============> C[1][1]: 1
I20230303 13:46:51.434979 338001 binary_evaluator_test.cc:217] ID: 0------------>mode:0==============> c:1
0
I20230303 13:46:51.458199 338001 binary_evaluator_test.cc:197] pIdx: 0==============> C[0][0]: 0
I20230303 13:46:51.458206 338001 binary_evaluator_test.cc:197] pIdx: 0==============> C[0][1]: 0
I20230303 13:46:51.458209 338001 binary_evaluator_test.cc:200] pIdx: 0==============> C[1][0]: 0
I20230303 13:46:51.458210 338001 binary_evaluator_test.cc:200] pIdx: 0==============> C[1][1]: 1
I20230303 13:46:51.458310 338002 binary_evaluator_test.cc:197] pIdx: 1==============> C[0][0]: 1
I20230303 13:46:51.458317 338002 binary_evaluator_test.cc:197] pIdx: 1==============> C[0][1]: 1
I20230303 13:46:51.458319 338002 binary_evaluator_test.cc:200] pIdx: 1==============> C[1][0]: 0
I20230303 13:46:51.458321 338002 binary_evaluator_test.cc:200] pIdx: 1==============> C[1][1]: 0
I20230303 13:46:51.458320 338003 binary_evaluator_test.cc:197] pIdx: 2==============> C[0][0]: 0
I20230303 13:46:51.458333 338003 binary_evaluator_test.cc:197] pIdx: 2==============> C[0][1]: 1
I20230303 13:46:51.458338 338003 binary_evaluator_test.cc:200] pIdx: 2==============> C[1][0]: 1
I20230303 13:46:51.458339 338003 binary_evaluator_test.cc:200] pIdx: 2==============> C[1][1]: 1
I20230303 13:46:51.458355 338001 binary_evaluator_test.cc:217] ID: 0------------>mode:1==============> c:1
0
[       OK ] BinaryEvaluatorTest.Sh3_BinaryEngine_add_msb_test (60 ms)
[----------] 1 test from BinaryEvaluatorTest (60 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (60 ms total)
[  PASSED  ] 1 test.
```
### CircuitTest
Test case only for circuit itself which is not integrated with aby3 aim to check the circuit built is right or not.
In the root directory of the project:
```
bazel build --config=linux_x86_64 //test/primihub/primitive:primitive_test
./bazel-out/k8-fastbuild/bin/test/primihub/primitive/primitive_test
```
Then test code output message like:
```
Running main() from gmock_main.cc
[==========] Running 5 tests from 4 test suites.
[----------] Global test environment set-up.
[----------] 1 test from ShareOTTest
[ RUN      ] ShareOTTest.SharedOT_eval_test
[       OK ] ShareOTTest.SharedOT_eval_test (19 ms)
[----------] 1 test from ShareOTTest (19 ms total)

[----------] 2 tests from CircuitTest
[ RUN      ] CircuitTest.BetaCircuit_int_Sh3Piecewise_Test
[       OK ] CircuitTest.BetaCircuit_int_Sh3Piecewise_Test (8 ms)
[ RUN      ] CircuitTest.garble_Test
d30f8ef52bbfbb59f06f1de916187147
[       OK ] CircuitTest.garble_Test (120 ms)
[----------] 2 tests from CircuitTest (128 ms total)

[----------] 1 test from BetaCircuit_int_Adder_Test
[ RUN      ] BetaCircuit_int_Adder_Test.int_adder
[       OK ] BetaCircuit_int_Adder_Test.int_adder (2 ms)
[----------] 1 test from BetaCircuit_int_Adder_Test (2 ms total)

[----------] 1 test from KoggeStone_int_Adder_test
[ RUN      ] KoggeStone_int_Adder_test.KoggeStone_int_adder
[       OK ] KoggeStone_int_Adder_test.KoggeStone_int_adder (544 ms)
[----------] 1 test from KoggeStone_int_Adder_test (544 ms total)

[----------] Global test environment tear-down
[==========] 5 tests from 4 test suites ran. (693 ms total)
[  PASSED  ] 5 tests.
```
