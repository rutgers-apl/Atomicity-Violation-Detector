#! /usr/bin/python

import sys, string, os, popen2, shutil, platform
import util

#clean up the obj directories
do_clean = False

#build the configs
do_build = False

#run the tests
do_run = True

if do_clean and not do_build:
    print "Clean - true and build - false not allowed"
    exit

#set paths
TBBROOT=os.environ['TBBROOT']
print "TBBROOT = " + TBBROOT
TD_ROOT=os.environ['TD_ROOT']
print "TD_ROOT = " + TD_ROOT

configs = []

entry = { "NAME" : "av_detector",
          "BUILD_LINE" : "make",
          "CLEAN_LINE" : " make clean ",
          "BUILD_ARCH" : "x86_64",
          "RUN_ARCH" : "x86_64",
          "ADDITIONAL_FLAGS" : "",
          "RUN_LINE" : "./",
          "ARGS" : "",
          }

configs.append(entry);

benchmarks=[
    "simple_Yes_AV",
    "simple_No_AV",
    "simple_nested_Yes_AV",
    "simple_nested_No_AV",
    "nested_Yes_AV",
    "RRR_No_AV",
    "RRR_No_AV_ver1",
    "RRW_No_AV",
    "RRW_No_AV_ver1",
    "RWR_Yes_AV",
    "RWR_Yes_AV_fixed1",
    "RWR_Yes_AV_fixed2",
    "RWR_Yes_AV_ver1",
    "RWR_diff_var_No_AV",
    "RWR_diff_var_No_AV_ver1",
    "RWW_Yes_AV",
    "RWW_Yes_AV_fixed1",
    "RWW_Yes_AV_fixed2",
    "RWW_Yes_AV_ver1",
    "WRR_No_AV",
    "WRR_No_AV_ver1",
    "WRW_Yes_AV",
    "WRW_Yes_AV_fixed1",
    "WRW_Yes_AV_fixed2",
    "WRW_Yes_AV_ver1",
    "WWR_Yes_AV",
    "WWR_Yes_AV_fixed1",    
    "WWR_Yes_AV_fixed2",
    "WWR_Yes_AV_ver1",
    "WWW_Yes_AV",
    "WWW_Yes_AV_fixed1",
    "WWW_Yes_AV_fixed2",
    "WWW_Yes_AV_ver1",
    "WWW_multivar_Yes_AV",
    "par_for_Yes_test",
    "par_for_No_test",
]

bm_results = []
total_count = 0
failed_count = 0
failed_tests = []
passed_count = 0
crashed_count = 0
crashed_tests = []

ref_cwd = os.getcwd();
arch = platform.machine()
full_hostname = platform.node()
hostname=full_hostname

for config in configs:
    util.log_heading(config["NAME"], character="-")
    if do_clean:
        util.chdir(TD_ROOT)
        util.run_command("make clean", verbose=False)
        util.chdir(TBBROOT)
        util.run_command("make clean", verbose=False)
    if do_build:
        util.chdir(TD_ROOT)
        util.run_command("make", verbose=False)
        util.chdir(TBBROOT)
        util.run_command("make", verbose=False)
    util.chdir(ref_cwd)
    
    try:
        clean_string = config["CLEAN_LINE"]
        util.run_command(clean_string, verbose=False)
    except:
        print "Clean failed"
        
    build_string = config["BUILD_LINE"]
    util.run_command(build_string, verbose=False)

    for benchmark in benchmarks:
        total_count = total_count + 1
        try:
            util.log_heading(benchmark, character="=")
            if do_run:
                
                run_string = config["RUN_LINE"] + benchmark
                util.run_command(run_string, verbose=False)
                
                test_res = 0
                if "Yes" in benchmark and "fixed" not in benchmark: 
                    test_res = 1
                
                f = open('violations.out', 'r')
                file_line = f.readline()
                f.close()
                output_res = 0
                if "Atomicity violation detected" in file_line: output_res = 1
                if test_res == output_res:
                    util.log_message("*** TEST PASSED ***")
                    passed_count = passed_count + 1
                else:
                    util.log_message("*** TEST FAILED ***")
                    failed_count = failed_count + 1
                    f_test = []
                    f_test.append(config["NAME"])
                    f_test.append(benchmark)
                    failed_tests.append(f_test)
                    if test_res == 1 and output_res == 0:
                        util.log_message("Test has an Atomicty Violation but was not reported by tool")
                    else:
                        util.log_message("Test has no Atomicty Violation but tool reported atomicity violation")

        except util.ExperimentError, e:
            print "Error: %s" % e
            print "-----------"
            print "%s" % e.output
            print "-----------"
            crashed_count = crashed_count + 1
            c_test = []
            c_test.append(config["NAME"])
            c_test.append(benchmark)
            crashed_tests.append(c_test)
            continue

print ""
print "********* TEST REPORT **********"
print "Total testcases run: " + str(total_count)
print "Total testcases PASSED: " + str(passed_count)
print "Total testcases FAILED: " + str(failed_count)
if failed_count != 0:
    print "FAILED testcases:"
    print failed_tests
    print "Total testcases CRASHED: " + str(crashed_count)
if crashed_count != 0:
    print "CRASHED testcases:"
    print crashed_tests
print "********* TEST REPORT ENDS **********"
