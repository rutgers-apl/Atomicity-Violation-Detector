#! /usr/bin/python

import sys, string, os, popen2, shutil, platform
import util

#clean up the obj directories
do_clean = True

#build the configs
do_build = True

#run the benchmarks
do_run = True

#collect data to plot
do_collect_data = True

if do_clean and not do_build:
    print "Clean - true and build - false not allowed"
    exit

#set paths
#TASK_DEBUG_ROOT="/home/yoga/apl-svn-repo/apl-projects/task_debug"
TBBROOT=os.environ['TBBROOT']
print "TBBROOT = " + TBBROOT
TD_ROOT=os.environ['TD_ROOT']
print "TDROOT = " + TD_ROOT

configs = [[0,0,0],
           [0,0,1],
           [0,1,0],
           [0,1,1],
           [1,1,1],
           [1,1,0],
           [1,0,1],
           [1,0,0]
]

av_result = [0,
             0,
             1,
             1,
             1,
             1,
             1,
             0
]

global total_count
total_count = 0
global failed_count
failed_count = 0
global failed_tests
failed_tests = []
global passed_count
passed_count = 0
#crashed_count = 0
#crashed_tests = []

def gen_task_start(f):
    f.write("tg_CaptureSetTaskId-0,1,true\n")
    f.write("tg_CaptureExecute-1,1\n")
    f.write("CaptureExecute-1\n")
    f.write("tg_CaptureSetTaskId-0,2,true\n")
    f.write("tg_CaptureExecute-2,2\n")
    f.write("CaptureExecute-2\n")

def gen_task_return(f):
    f.write("tg_CaptureReturn-1\n")
    f.write("CaptureReturn-1\n")
    f.write("tg_CaptureReturn-2\n")
    f.write("CaptureReturn-2\n")

def execute():
    run_string = "./trace_executor trace.txt"
    util.run_command(run_string, verbose=False)

def check_av(i_config, i_interleaving):
    global total_count
    global failed_count
    global failed_tests
    global passed_count
    
    f = open('violations.out', 'r')
    file_line = f.readline()
    f.close()
    
    total_count = total_count + 1
    
    output_res = 0
    if "Atomicity violation detected" in file_line:
        output_res = 1
    if av_result[i_config] == output_res:
        util.log_message("*** TEST PASSED ***")
        passed_count = passed_count + 1
    else:
        util.log_message("*** TEST FAILED ***")
        failed_count = failed_count + 1
        f_test = []
        f_test.append(str(configs[i_config]))
        f_test.append(str(i_interleaving))
        failed_tests.append(f_test)
        if av_result[i_config] == 1 and output_res == 0:
            util.log_message("Test has an Atomicty Violation but was not reported by tool")
        else:
            util.log_message("Test has no Atomicty Violation but tool reported atomicity violation")
    

if do_clean:
    util.run_command("make clean", verbose=False)
    
if do_build:
    util.run_command("make", verbose=False)

for config in configs:
    t11 = ""
    t12 = ""
    t21 = ""
    if config[0] == 0:
        t11 = "RecordMem-1,x,READ\n"
    else:
        t11 = "RecordMem-1,x,WRITE\n"

    if config[1] == 0:
        t21 = "RecordMem-2,x,READ\n"
    else:
        t21 = "RecordMem-2,x,WRITE\n"
        
    if config[2] == 0:
        t12 = "RecordMem-1,x,READ\n"
    else:
        t12 = "RecordMem-1,x,WRITE\n"
    
    f = open('trace.txt', 'w')
    
    gen_task_start(f)

    #generate recordmem - 1->2->1        
    f.write(t11)
    f.write(t21)
    f.write(t12)
    
    gen_task_return(f)
    
    f.close()

    execute()
    check_av(configs.index(config), 1)

    f = open('trace.txt', 'w')

    gen_task_start(f)

    #generate recordmem - 2->1->1
    f.write(t21)
    f.write(t11)
    f.write(t12)    
    
    gen_task_return(f)

    f.close()

    execute()
    check_av(configs.index(config), 2)

    f = open('trace.txt', 'w')

    gen_task_start(f)

    #generate recordmem - 1->1->2
    f.write(t11)
    f.write(t12)
    f.write(t21)    
    
    gen_task_return(f)

    f.close()

    execute()
    check_av(configs.index(config), 3)

print ""
print "********* TEST REPORT **********"
print "Total testcases run: " + str(total_count)
print "Total testcases PASSED: " + str(passed_count)
print "Total testcases FAILED: " + str(failed_count)
if failed_count != 0:
    print "FAILED testcases:"
    print failed_tests
#    print "Total testcases CRASHED: " + str(crashed_count)
#if crashed_count != 0:
#    print "CRASHED testcases:"
#    print crashed_tests
print "********* TEST REPORT ENDS **********"    
