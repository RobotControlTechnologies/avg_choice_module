#include <sqlite3.h>
#ifndef AVG_CHOICE_MODULE_H
#define AVG_CHOICE_MODULE_H

using namespace std;

struct ResultData {
  std::string iid;
  std::string uid;
  double averageTime;
  bool success;
  ResultData(std::string iid, std::string uid, double averageTime, bool success)
    : iid(iid), uid(uid), averageTime(averageTime), success(success) {};
};

class AvgChoiceModule : public ChoiceModule {
  colorPrintfModuleVA_t *colorPrintf_p;
  ModuleInfo *mi;

 public:
  AvgChoiceModule();
  // init
  const struct ModuleInfo &getModuleInfo();
  void prepare(colorPrintfModule_t *colorPrintf_p,
               colorPrintfModuleVA_t *colorPrintfVA_p);

  // compiler only
  void *writePC(unsigned int *buffer_length);

  // intepreter - devices
  #if MODULE_API_VERSION == 102
    virtual int init(initCallback_t& initCallback);
  #else
    int init();
  #endif
  void final();

  // db
  std::string db_path;

  // intepreter - program & lib
  int readPC(int pc_index, void *buffer, unsigned int buffer_length);

  // intepreter - program
  int startProgram(int run_index, int pc_index);
  const ChoiceRobotData *makeChoice(int run_index, const ChoiceFunctionData **function_data,
                                    unsigned int count_functions,
                                    const ChoiceRobotData **robots_data,
                                    unsigned int count_robots);
  int endProgram(int run_index);

  // destructor
  void destroy();
  ~AvgChoiceModule() {}

  void colorPrintf(ConsoleColor colors, const char *mask, ...);
};

#endif /* AVG_CHOICE_MODULE_H */
