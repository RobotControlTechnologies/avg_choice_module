/* INCLUDE CONFIG */
/* General */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sqlite3.h>

#include <string>
#include <vector>
#include <set>

#ifdef _WIN32
#include "stringC11.h"
#endif

#include "SimpleIni.h"

/* RCML */
#include "module.h"
#include "choice_module.h"
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

#if MODULE_API_VERSION == 102
int AvgChoiceModule::init(initCallback_t& initCallback)
#else
int AvgChoiceModule::init()
#endif
{
  std::string config_path = "config.ini";

  CSimpleIniA ini;
  ini.SetMultiKey(true);

  if (ini.LoadFile(config_path.c_str()) < 0) {
    colorPrintf(ConsoleColor(ConsoleColor::red), "Can't load '%s' file!\n",
                config_path.c_str());
    return 1;
  }

  db_path = ini.GetValue("statisctic", "db_path", "");
  if (db_path.empty()) {
    colorPrintf(ConsoleColor(ConsoleColor::red), "Can't read db_path value\n");
    return 1;
  }

  sqlite3 *db;
  int rc = sqlite3_open(db_path.c_str(), &db);
  if (rc) {
    colorPrintf(ConsoleColor(ConsoleColor::red), "Can't open database: %s\n",
                sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }
  sqlite3_close(db);

  return 0;
}

void AvgChoiceModule::final() {}

int AvgChoiceModule::readPC(int pc_index, void *buffer,
                            unsigned int buffer_length) {
  return 0;
}

int AvgChoiceModule::startProgram(int run_index, int pc_index) { return 0; }

const ChoiceRobotData *AvgChoiceModule::makeChoice(int run_index,
    const ChoiceFunctionData **function_data, uint count_functions,
    const ChoiceRobotData **robots_data, uint count_robots) {

  if (not ((count_functions == 1) && count_robots)) {
     return NULL;
  }

  sqlite3 *db;
  int rc = sqlite3_open(db_path.c_str(), &db);
  if (rc) {
    colorPrintf(ConsoleColor(ConsoleColor::red), "Can't open database: %s\n",
                sqlite3_errmsg(db));
    sqlite3_close(db);
    return NULL;
  }

  string sqlQuery = 
    "select\n"     
    "  s.iid,\n"
    "  ru.uid,\n"
    "avg(fc.end - fc.start) as avg_time,\n"
    "fc.success\n"
    "from\n"
    "  function_calls as fc,\n"
    "  robot_uids as ru,\n"
    "  sources as s,\n"
    "  functions as f,\n"
    "  contexts as c\n"
    "where\n"
    "  fc.function_id = f.id\n"
    "  and f.context_id = c.id\n"
    "  and fc.robot_id = ru.id\n"
    "  and ru.source_id = s.id\n"

    "  /* calls restrict */\n"
    //"  and fc.success = 1\n"
    "  and fc.end is not NULL\n";


  string functionsRestrict = "and ";
  const ChoiceFunctionData *funcData = function_data[0];

  functionsRestrict += "f.name = '" + (string)(funcData->name) +
                        "'\nand " +
                        "c.hash = '" + (string)(funcData->context_hash) +
                        "'\nand " +
                        "f.position = '" + to_string(funcData->position) +
                        "'\n";

  map<string, vector<string>> robotsModules;
  set<string> excludedModules;
  for (uint i = 0; i < count_robots; ++i) {
    const char* checkUid = robots_data[i]->robot_uid;
    const char* checkIid = robots_data[i]->module_data->iid;
    string tmpIid(checkIid);

    if (!robotsModules.count(tmpIid)) {
      robotsModules.emplace(tmpIid, vector<string>());
    }
    
    if (checkUid == NULL) {
      if (!excludedModules.count(tmpIid)){
        excludedModules.insert(tmpIid);
      }
      continue;
    }

    string uid(checkUid);
    if (uid.empty()){
      if (!excludedModules.count(tmpIid)){
        excludedModules.insert(tmpIid);
      }
    } else{
      robotsModules[tmpIid].push_back(uid);
    }
  }

  string robotsRestrict("");
            
  for (auto robotModule = robotsModules.begin(); 
       robotModule != robotsModules.end(); 
       ++robotModule) 
  {
    string robotsUids("");
    if (!excludedModules.count(robotModule->first)) {
      auto robotsNames = robotModule->second;
      for (size_t i = 0; i < robotsNames.size(); ++i) {
        if (!robotsUids.empty()) {
          robotsUids += ",";
        }
        robotsUids += "'" + robotsNames[i] + "'";
      }

      robotsUids = "(" + robotsUids + ")";
    }

    if (!robotsRestrict.empty()) {
      robotsRestrict += "or ";
    }

    robotsRestrict += string("(\n") + 
                       "s.iid = '" + robotModule->first + "'\nand ";
    if (!robotsUids.empty()) {
      robotsRestrict += "ru.uid in" + robotsUids + "\nand ";
    }
    robotsRestrict += "s.type = 2\n)\n";
  }

  if (!robotsRestrict.empty()) {
    robotsRestrict = "and (\n" + robotsRestrict + ")\n";
  }

  sqlQuery += functionsRestrict + robotsRestrict + 
               "group by s.iid,ru.uid order by avg_time ASC";

#ifdef IS_DEBUG
  colorPrintf(ConsoleColor(ConsoleColor::yellow), "SQL statement:\n%s\n\n",
              sqlQuery.c_str());
#endif

  int rowNum = 0;
  int colNum = 0;
  char *errorMessage = 0;
  char **sqlResult = 0;
  if (sqlite3_get_table(db, sqlQuery.c_str(), &sqlResult, &rowNum, &colNum,
                        &errorMessage) != SQLITE_OK) {
    colorPrintf(ConsoleColor(ConsoleColor::red), "SQL error:\n%s\n\n",
                errorMessage);
    sqlite3_free(errorMessage);
    sqlite3_close(db);
    return NULL;
  }

  const ChoiceRobotData *resultRobot = NULL;
  std::vector<ResultData> robotsCandidatesFromSQLQuery;

  for (int i = 0; i < rowNum; ++i) {
    const int iidPos = (i+1)*colNum;
    const int uidPos = iidPos + 1;
    const int avgTimePos = uidPos + 1;
    const int successPos = avgTimePos + 1;

    const string iid(sqlResult[iidPos]);
    const string uid(sqlResult[uidPos]);
    const string averageTime(sqlResult[avgTimePos]);
    const bool success(atof(sqlResult[successPos]));

    robotsCandidatesFromSQLQuery.push_back(ResultData(iid, uid, atof(averageTime.c_str()), success));
  }

  bool isNewDataNeeded = true;
  for (uint i = 0; i < count_robots; i++) {
    const ChoiceRobotData *candidateRobot = robots_data[i];

    const string currentIid(candidateRobot->module_data->iid);
    const string currentUid(candidateRobot->robot_uid);

    isNewDataNeeded = true;
    for (size_t resIndex = 0; resIndex < robotsCandidatesFromSQLQuery.size(); ++resIndex) {
      if (!currentIid.compare(robotsCandidatesFromSQLQuery[resIndex].iid) && 
          !currentUid.compare(robotsCandidatesFromSQLQuery[resIndex].uid)){
        isNewDataNeeded = false;
        break;
      }
    }

    if (isNewDataNeeded){
#ifdef IS_DEBUG
      colorPrintf(ConsoleColor(ConsoleColor::green), 
        "Select robot that don't have statistics\n");
#endif
      resultRobot = candidateRobot;
      break;
    }
  }

  if (!isNewDataNeeded){
    // find first success function
    for (size_t successCandidate = 0; 
         successCandidate < robotsCandidatesFromSQLQuery.size(); 
         ++successCandidate) 
    {
      if (robotsCandidatesFromSQLQuery[successCandidate].success) {
        // find robot data with same uid, iid
        for (uint robotIndex = 0; robotIndex < count_robots; robotIndex++) {
          const ChoiceRobotData *candidateRobot = robots_data[robotIndex];
          const string currentIid(candidateRobot->module_data->iid);
          const string currentUid(candidateRobot->robot_uid);
      
          if (!currentIid.compare(robotsCandidatesFromSQLQuery[successCandidate].iid) &&
              !currentUid.compare(robotsCandidatesFromSQLQuery[successCandidate].uid)){
            resultRobot = candidateRobot;
          break;
          }
        }
        // check is robot available
        if (resultRobot->is_aviable){
          break;
        } else {
          resultRobot = NULL;
        }
      }
    }
  }

#ifdef IS_DEBUG
  string result = "NULL";
  if (resultRobot != NULL) {
    result = (string)(resultRobot->robot_uid);
    result += " : ";
    result += (string)(resultRobot->module_data->iid);
  }
  colorPrintf(ConsoleColor(ConsoleColor::yellow), "MakeChoice result:\n%s\n\n",
              result.c_str());
#endif
  sqlite3_free_table(sqlResult);
  sqlite3_close(db);

  return resultRobot;
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
}
PREFIX_FUNC_DLL ChoiceModule *getChoiceModuleObject() {
  return new AvgChoiceModule();
}
