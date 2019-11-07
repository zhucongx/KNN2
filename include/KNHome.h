#ifndef _KNHome_H_
#define _KNHome_H_

#include <math.h>
#include <mpi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <set>
#include <algorithm> 
#include "armadillo"
#include "gbDef.h"
// #include "gbCnf.h"
#include "KMCEvent.h"
#include "gbUtl.h"

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::map;
using std::move;
using std::ofstream;
using std::setprecision;
using std::string;
using std::stringstream;
using std::to_string;
using std::unordered_map;
using std::vector;
using std::set;
using std::pair;
using std::min;
using arma::vec;
using arma::mat;

class KNHome {
private:
  class gbCnf;

  Config c0;
  vector<KMCEvent> eventList;
  vector<int> vacList;
  double RCut;
  long long maxIter;

public:
  int me, nProcs;

  unordered_map<string, double> dparams;
  unordered_map<string, int> iparams;
  unordered_map<string, string> sparams;
  unordered_map<string, vector<string>> vsparams;
  unordered_map<string, vector<int>> viparams;


  KNHome(int argc, char* argv[]);
  ~KNHome();

  /* KNParam.cpp */
  void parseArgs(int argc, char* argv[]);
  void initParam();
  void readParam();
  
  /* KNSolidSol.cpp */
  void createPreNEB();
  /* KNvasp.cpp */
  void prepVASPFiles(const string, const vector<int>&,
                     const set<string>&);
  /* KNEncode.cpp */
  void KNEncode();
  vector<vector<int>> readPairs(const string&);

  /* KNBondCount.cpp */
  void KNBondCount();

  /* KMCSimulation.cpp */
  void KMCInit();
  void getVacList();
  void KMCSimulation();
  void buildEventList();
  
};

#endif
