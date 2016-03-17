#include <sqlite3.h>
#ifndef TEST_DB_MODULE_H
#define TEST_DB_MODULE_H

using namespace std;

class TestDBModule : public DBModule {
  colorPrintfModuleVA_t *colorPrintf_p;
  ModuleInfo *mi;

  public:
    TestDBModule();
    // init
    const struct ModuleInfo& getModuleInfo();
    void prepare(colorPrintfModule_t *colorPrintf_p, colorPrintfModuleVA_t *colorPrintfVA_p);

    // compiler only
    void *writePC(unsigned int *buffer_length);

    // intepreter - devices
    int init();
    void final();
    
    // db
    sqlite3 *db;

    // intepreter - program & lib
    void readPC(void *buffer, unsigned int buffer_length) {};

    // intepreter - program
    int startProgram(int uniq_index);
    const DBRobotData *makeChoise(const DBFunctionData** function_data, unsigned int count_functions, const DBRobotData** robots_data, unsigned int count_robots);
    int endProgram(int uniq_index);

    // destructor
    void destroy();
    ~TestDBModule() {}

  void colorPrintf(ConsoleColor colors, const char *mask, ...);
};

#endif /* TEST_DB_MODULE_H */
