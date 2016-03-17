#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sqlite3.h>
//#include <sqlite3.c>
#include <iostream>
#include <string>

#include "module.h"
#include "db_module.h"

#include "test_db_module.h"

/* GLOBALS CONFIG */

#define IID "RCT.Test_db_module_v101"
typedef unsigned int uint;

TestDBModule::TestDBModule() {
  mi = new ModuleInfo;
  mi->uid = IID;
  mi->mode = ModuleInfo::Modes::PROD;
  mi->version = BUILD_NUMBER;
  mi->digest = NULL;
}

const struct ModuleInfo &TestDBModule::getModuleInfo() { return *mi; }

void TestDBModule::prepare(colorPrintfModule_t *colorPrintf_p,
                              colorPrintfModuleVA_t *colorPrintfVA_p) {
  this->colorPrintf_p = colorPrintfVA_p;
}

void *TestDBModule::writePC(unsigned int *buffer_length) {
  (*buffer_length) = 0;
  return NULL;
}

int TestDBModule::init() {
  //connect to data base here
  int rc;  
  rc = sqlite3_open("stats.db", &db);
  if( rc ){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return(1);
  }  
  return 0;
}

void TestDBModule::final() {
  //disconnect from data base here
  sqlite3_close(db);  
}

int TestDBModule::startProgram(int uniq_index) { return 0; }

const DBRobotData *TestDBModule::makeChoise(const DBFunctionData** function_data, uint count_functions,
                                            const DBRobotData** robots_data, uint count_robots) {
  char *zErrMsg = 0;    
  int rc;
  int nRow;
  int nCol;
  bool havenulluid;
  char **pRes;
  string strIn;  
  string *psqlText = new string[4];
  string sTmp;
  string sRes;
  const char *psqlText2;  
      
    // main + funcs
    psqlText[0] = 
    "select uid from robot_uids ru\n"
    "join\n"
    "(select fc.robot_id,\n"
    " --fc.function_id,\n"
    " avg(fc.end-fc.start) as avg_time\n"
    "from function_calls fc\n"
    "where exists(select 1 from functions f\n"
    "               where f.id = fc.function_id\n"
    "		and f.name in (%IN_CLAUSE%/*'debug','test'*/))\n";
      
    sTmp = (string)"";
    for (uint i = 0; i <= count_functions - 1; i++) {
        sTmp = (function_data[i] -> name);
        strIn += (string)"'" + sTmp + "',";
    }    
    strIn = strIn.substr(0,strIn.length()-1);    
    psqlText[0].replace(psqlText[0].find("%IN_CLAUSE%"),11,strIn);    
    
    // robots
    psqlText[1] = (string)"";
    sTmp = (string)"";
    strIn = (string)"";
    if (count_robots) {
        for (uint i = 0; i <= count_robots - 1; i++) {
            if (*robots_data[i] -> robot_uid == 0) {
                havenulluid = true;
                break;
            } else {
                sTmp = (robots_data[i] -> robot_uid);
                strIn += (string)"'" + sTmp + "',";                
            }            
        }
        if (not havenulluid) {
            psqlText[1] =
            "and exists (select 1 from robot_uids ru\n"
            "		where ru.id = fc.robot_id\n"
            "		and ru.uid in (%IN_CLAUSE%/*'robot-3','robot-1'*/))\n";
            strIn = strIn.substr(0,strIn.length()-1);
            psqlText[1].replace(psqlText[1].find("%IN_CLAUSE%"),11,strIn);    
        }        
    }    
            
    // group by
    psqlText[2] =        
    "group by fc.robot_id\n"
    "--,fc.function_id\n"
    "order by avg_time asc\n"
    "limit 1\n"    
    ") b\n"
    "on (ru.id = b.robot_id)\n";
    
    // concat
    for (uint i = 0; i <= 2; i++) {
        psqlText[3] += psqlText[i];
    }    
        
    // string to char[]    
    psqlText2 = psqlText[3].c_str();         
    
    rc = sqlite3_get_table(db, psqlText2, &pRes, &nRow, &nCol, &zErrMsg);
    if( rc!=SQLITE_OK ){
       fprintf(stderr, "SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
    }    
    
    sRes = pRes[1];    
    for (uint i = 0; i <= count_robots - 1; i++) {
        sTmp = robots_data[i] -> robot_uid;
        if (sTmp == sRes) {
            sqlite3_free_table(pRes);
            return robots_data[i];
            }
        }   
   
    sqlite3_free_table(pRes);
    return NULL; 
    
  /*colorPrintf(ConsoleColor(ConsoleColor::gray), "Total functions: %d\n", count_functions);
  for (unsigned int i = 0; i < count_functions; ++i) {
    colorPrintf(ConsoleColor(ConsoleColor::gray), "  function: %d\n", i + 1);
    const DBFunctionData *db_fd = function_data[i];
      colorPrintf(ConsoleColor(ConsoleColor::gray), "    name: %s\n", db_fd->name);
      colorPrintf(ConsoleColor(ConsoleColor::gray), "    context_hash: %s\n", db_fd->context_hash);
      colorPrintf(ConsoleColor(ConsoleColor::gray), "    position: %d\n", db_fd->position);
      colorPrintf(ConsoleColor(ConsoleColor::gray), "    call_type: %d\n", db_fd->call_type);
  }
  colorPrintf(ConsoleColor(ConsoleColor::gray), "Total robots: %d\n", count_robots);
  for (unsigned int i = 0; i < count_robots; ++i) {
    const DBRobotData *db_rd = robots_data[i];
    colorPrintf(ConsoleColor(ConsoleColor::gray), "  %d. %s\n", i + 1, db_rd->robot_uid);
  }*/ 
}

int TestDBModule::endProgram(int uniq_index) { return 0; }

void TestDBModule::destroy() {
  delete mi;
  delete this;
}

void TestDBModule::colorPrintf(ConsoleColor colors, const char *mask, ...) {
  va_list args;
  va_start(args, mask);
  (*colorPrintf_p)(this, colors, mask, args);
  va_end(args);
}

PREFIX_FUNC_DLL unsigned short getDBModuleApiVersion() {
  return MODULE_API_VERSION;
};
PREFIX_FUNC_DLL DBModule *getDBModuleObject() {
  return new TestDBModule();
}
