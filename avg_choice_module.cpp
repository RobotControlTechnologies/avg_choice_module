/* INCLUDE CONFIG */
/* General */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sqlite3.h>
#include <string>
#include "stringC11.h"

/* RCML */
#include "module.h"
#include "db_module.h"
#include "avg_choice_module.h"

/* GLOBALS CONFIG */
#define IID "RCT.AVG_choise_module_v101"
typedef unsigned int uint;

AvgChoiceModule::AvgChoiceModule() {
  mi = new ModuleInfo;
  mi->uid = IID;
  mi->mode = ModuleInfo::Modes::PROD;
  mi->version = BUILD_NUMBER;
  mi->digest = NULL;
}

const struct ModuleInfo &AvgChoiceModule::getModuleInfo() { return *mi; }

void AvgChoiceModule::prepare(colorPrintfModule_t *colorPrintf_p,
                              colorPrintfModuleVA_t *colorPrintfVA_p) {
  this->colorPrintf_p = colorPrintfVA_p;
}

void *AvgChoiceModule::writePC(unsigned int *buffer_length) {
  (*buffer_length) = 0;
  return NULL;
}

int AvgChoiceModule::init() {
  //connect to data base here
  int rc = sqlite3_open("stats.db", &db);
  if( rc ){
    colorPrintf(ConsoleColor(ConsoleColor::red),"Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return(1);
  }  
  return 0;
}

void AvgChoiceModule::final() {
  //disconnect from data base here
  sqlite3_close(db);  
}

int AvgChoiceModule::startProgram(int uniq_index) { return 0; }

const DBRobotData *AvgChoiceModule::makeChoise(const DBFunctionData** function_data, uint count_functions,
                                            const DBRobotData** robots_data, uint count_robots) {

string psqlText =
"select uid\n"
"  from robot_uids ru\n"
"  join\n"
"      (select fc.robot_id,\n"
"              avg(fc.end-fc.start) as avg_time\n"
"         from function_calls fc\n"
"        where exists(select 1\n"
"                       from functions f\n"
"                       join contexts c\n"
"                         on f.context_id = c.id\n"
"                      where f.id = fc.function_id\n"
"                        and ( %FUNCS_CLAUSE%\n"
"                             )\n"
"                     ) %ROBOTS_CLAUSE%\n"
"        group by fc.robot_id\n"
"        order by avg_time asc\n"
"        limit 1\n"
"       ) b\n"
"    on (ru.id = b.robot_id)\n";

string sTmp = "";
for (uint i = 0; i <= count_functions-1; i++) {
    if (i >= 1) {
        sTmp += "or ";
    }
     sTmp += "(f.name = '" + (string)(function_data[i] -> name) + "'\n"
             " and f.position = " + to_string(function_data[i] -> position) + "\n"
             " and c.hash = '" + (string)(function_data[i] -> context_hash) + "')\n";
}
sTmp = sTmp.substr(0, sTmp.length()-1);
psqlText.replace(psqlText.find("%FUNCS_CLAUSE%"),14,sTmp);

sTmp = (string)"";
if (count_robots) {
    bool havenulluid = false;
    for (uint i = 0; i < count_robots; i++) {
        if (   *robots_data[i] -> robot_uid == 0
            or *robots_data[i] -> robot_uid == NULL
            or (string)(robots_data[i] -> robot_uid) == (string)"") {
            havenulluid = true;
            break;
        } else {
          sTmp += (string)"'" + (string)(robots_data[i] -> robot_uid) + "',";
        }
    }
    if (not havenulluid) {
        string sTmp2 =
        "\n and exists (select 1 from robot_uids ru\n"
        "		where ru.id = fc.robot_id\n"
        "		and ru.uid in (%IN_CLAUSE%))\n";
        sTmp = sTmp.substr(0,sTmp.length()-1);
        sTmp2.replace(sTmp2.find("%IN_CLAUSE%"),11,sTmp);
        sTmp2 = sTmp2.substr(0, sTmp2.length()-1);
        psqlText.replace(psqlText.find("%ROBOTS_CLAUSE%"),15,sTmp2);
    } else {
      psqlText.replace(psqlText.find("%ROBOTS_CLAUSE%"),15,"");
    }
} else {
  psqlText.replace(psqlText.find("%ROBOTS_CLAUSE%"),15,"");
}
#ifdef IS_DEBUG
    colorPrintf(ConsoleColor(ConsoleColor::yellow),"SQL statement:\n%s\n\n", psqlText.c_str());
#endif

int nRow = 0;
int nCol = 0;
char *zErrMsg = 0;
char **pResSQL = 0;
if( sqlite3_get_table(db, psqlText.c_str(), &pResSQL, &nRow, &nCol, &zErrMsg) != SQLITE_OK ){
   colorPrintf(ConsoleColor(ConsoleColor::red),"SQL error:\n%s\n\n", zErrMsg);
   sqlite3_free(zErrMsg);
   return NULL;
}
#ifdef IS_DEBUG
    colorPrintf(ConsoleColor(ConsoleColor::yellow),"SQL result:\n%s\n\n", nCol > 0 ? pResSQL[1] : "NULL");
#endif

const DBRobotData *pRes = NULL;
if (nCol > 0) {
for (uint i = 0; i < count_robots; i++) {
    if ( (string)(robots_data[i] -> robot_uid) == (string)pResSQL[1]) {
        pRes = robots_data[i];
        }
    }
}

sqlite3_free_table(pResSQL);
#ifdef IS_DEBUG
    colorPrintf(ConsoleColor(ConsoleColor::yellow),"MakeChoice result:\n%s\n\n", *pRes != NULL ? pRes -> robot_uid : "NULL");
#endif
return pRes;
}

int AvgChoiceModule::endProgram(int unique_index) { return 0; }

void AvgChoiceModule::destroy() {
  delete mi;
  delete this;
}

void AvgChoiceModule::colorPrintf(ConsoleColor colors, const char *mask, ...) {
  va_list args;
  va_start(args, mask);
  (*colorPrintf_p)(this, colors, mask, args);
  va_end(args);
}

PREFIX_FUNC_DLL unsigned short getDBModuleApiVersion() {
  return MODULE_API_VERSION;
};
PREFIX_FUNC_DLL DBModule *getDBModuleObject() {
  return new AvgChoiceModule();
}
