/* INCLUDE CONFIG */
/* General */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sqlite3.h>
#include <string>

#ifdef _WIN32
#include "stringC11.h"
#endif

/* RCML */
#include <module.h>
#include <choice_module.h>
#include <avg_choice_module.h>

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

int AvgChoiceModule::readPC(int pc_index, void *buffer, unsigned int buffer_length) { return 0; }

int AvgChoiceModule::startProgram(int run_index, int pc_index) { return 0; }

const ChoiceRobotData *AvgChoiceModule::makeChoice(const ChoiceFunctionData** function_data, uint count_functions,
                                            const ChoiceRobotData** robots_data, uint count_robots) {

string sSqlText =
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
"                        and ( %FUNCS_CLAUSE%"
"                             )\n"
"                     ) %ROBOTS_CLAUSE%"
"        group by fc.robot_id\n"
"        order by avg_time asc\n"
"        limit 1\n"
"       ) b\n"
"    on (ru.id = b.robot_id)\n";

string sFuncs = "";
for (uint i = 0; i < count_functions; i++) {
    if (i >= 1) {
        sFuncs += "or ";
    }
     sFuncs += "(f.name = '" + (string)(function_data[i] -> name) + "'\n"
               " and f.position = " + to_string(function_data[i] -> position) + "\n"
               " and c.hash = '" + (string)(function_data[i] -> context_hash) + "')\n";
}
sSqlText.replace(sSqlText.find("%FUNCS_CLAUSE%"),14,sFuncs);

if (count_robots) {
    string sRobotsClause =
    "\n and exists (select 1\n"
    "                 from robot_uids ru\n"
    "                 join sources s\n"
    "                   on (ru.source_id = s.id)\n"
    "	             where ru.id = fc.robot_id\n"
    "		       and (   %ROBOT_UIDS%\n"
    "                       or %SOURCE_HASHES%\n"
    "                       ))\n";
string sUids = "";
string sHashes = "";
    for (uint i = 0; i < count_robots; i++) {
        if (   robots_data[i] -> robot_uid == 0
            or robots_data[i] -> robot_uid == NULL
            or (string)(robots_data[i] -> robot_uid) == (string)"") {
           sHashes += "'" + (string)(robots_data[i] -> module_data -> hash) + "',";
        } else {
          sUids += "'" + (string)(robots_data[i] -> robot_uid) + "',";
        }
    }
    if (sUids == (string)"") {
        sUids = "1=0";
    } else {    
      sUids = "ru.uid in (" + sUids.substr(0,sUids.length()-1) + ")";
    }
    if (sHashes == (string)"") {
        sHashes = "1=0";
    } else {    
      sHashes = "s.hash in (" + sHashes.substr(0,sHashes.length()-1) + ")";
    }
    sRobotsClause.replace(sRobotsClause.find("%ROBOT_UIDS%"),12,sUids);
    sRobotsClause.replace(sRobotsClause.find("%SOURCE_HASHES%"),15,sHashes);
    sSqlText.replace(sSqlText.find("%ROBOTS_CLAUSE%"),15,sRobotsClause);
} else {
  sSqlText.replace(sSqlText.find("%ROBOTS_CLAUSE%"),15,"");
}
#ifdef IS_DEBUG
    colorPrintf(ConsoleColor(ConsoleColor::yellow),"SQL statement:\n%s\n\n", sSqlText.c_str());
#endif

int nRow = 0;
int nCol = 0;
char *zErrMsg = 0;
char **pResSQL = 0;
if( sqlite3_get_table(db, sSqlText.c_str(), &pResSQL, &nRow, &nCol, &zErrMsg) != SQLITE_OK ){
   colorPrintf(ConsoleColor(ConsoleColor::red),"SQL error:\n%s\n\n", zErrMsg);
   sqlite3_free(zErrMsg);
   return NULL;
}
#ifdef IS_DEBUG
    colorPrintf(ConsoleColor(ConsoleColor::yellow),"SQL result:\n%s\n\n", nCol > 0 ? pResSQL[1] : "NULL");
#endif

const ChoiceRobotData *pRes = NULL;
if (nCol > 0) {
for (uint i = 0; i < count_robots; i++) {
    if ( (string)(robots_data[i] -> robot_uid) == (string)pResSQL[1]) {
        pRes = robots_data[i];
        }
    }
}

sqlite3_free_table(pResSQL);
#ifdef IS_DEBUG
string Res = "NULL";
if (pRes != NULL) {
    Res = (string)(pRes -> robot_uid);
}
colorPrintf(ConsoleColor(ConsoleColor::yellow),"MakeChoice result:\n%s\n\n", Res.c_str());
#endif
return pRes;
}

int AvgChoiceModule::endProgram(int run_index) { return 0; }

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

PREFIX_FUNC_DLL unsigned short getChoiceModuleApiVersion() {
  return MODULE_API_VERSION;
};
PREFIX_FUNC_DLL ChoiceModule *getChoiceModuleObject() {
  return new AvgChoiceModule();
}
