# Atomicity-Violation-Detector
Atomicity violation detection tool for C++ programs that use Intel TBB task parallel library.

1. Build a)Task Debug for LLVM+CLANG 3.7 b) Task Debug library c) TBB library

            source build.sh

2. To run all 36 test programs.

            cd av_tests
            python run_tests.py

3. To execute atomicity violation detection tool on traces that capture all possible interleavings that can lead to an atomicity violation

            cd trace_tests
            python trace_generator.py
