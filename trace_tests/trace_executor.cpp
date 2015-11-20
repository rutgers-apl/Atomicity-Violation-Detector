#include "t_debug_task.h"
#include<cstdlib>
#include<fstream>
#include<string>

using namespace std;

int x = 0;

void insert_call(string fn_name, vector<string> args) {
  if (fn_name.compare("tg_CaptureSetTaskId") == 0) {
    taskGraph->CaptureSetTaskId(std::stoul (args[0], NULL),
				std::stoul (args[1], NULL),
				args[2].compare("true") == 0 ? true:false);
  } else if (fn_name.compare("tg_CaptureExecute") == 0) {
    taskGraph->CaptureExecute(std::stoul (args[0], NULL),
			      std::stoul (args[1], NULL));
  } else if (fn_name.compare("CaptureExecute") == 0) {
    CaptureExecute(std::stoul (args[0], NULL));
  } else if (fn_name.compare("tg_CaptureReturn") == 0) {
    taskGraph->CaptureReturn(std::stoul (args[0], NULL));
  } else if (fn_name.compare("CaptureReturn") == 0) {
    CaptureReturn(std::stoul (args[0], NULL));
  }  else if (fn_name.compare("RecordMem") == 0) {
    RecordMem(std::stoul (args[0], NULL), &x, args[2].compare("READ") == 0 ? READ:WRITE);
  }
}

void parse_trace(const char* filename) {
  ifstream file;
  file.open(filename);
  for(string line; getline( file, line ); ) {
    //cout << line << endl;
    unsigned int pos = line.find("-");
    string fn_name = line.substr (0, pos);

    vector<string> args;
    string rest;
    rest = line.substr(pos + 1);
    while(!rest.empty()) {
      unsigned int arg_end;
      string arg;
      if (rest.find(",") == string::npos) {
	arg_end = rest.length();
	arg = rest.substr(0, arg_end);
	rest = rest.substr(arg_end);
      } else {
	arg_end = rest.find(",");
	arg = rest.substr(0, arg_end);
	rest = rest.substr(arg_end + 1);
      }
      args.push_back(arg);
    };
    
    insert_call(fn_name, args);
  }
}
 
int main(int argc, const char *argv[]) {

  if (argc != 2) {
    printf("Format:./trace_executer <trace_filename>\n");
    return -1;
  }
  
  TD_Activate();

  parse_trace(argv[1]);

  // taskGraph->CaptureSetTaskId(0, 1, true);
  // taskGraph->CaptureExecute(1, 1);
  // CaptureExecute(1);
  // RecordMem(1, &x, READ);
  // RecordMem(1, &x, READ);
  // taskGraph->CaptureReturn(1);
  // CaptureReturn(1);

  // taskGraph->CaptureSetTaskId(0, 2, true);
  // taskGraph->CaptureExecute(2, 2);
  // CaptureExecute(2);
  // RecordMem(2, &x, WRITE);
  // taskGraph->CaptureReturn(2);
  // CaptureReturn(2);

  Fini();
}
