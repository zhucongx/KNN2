#include "gbCnf.h"
#include "KNHome.h"
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// random generator function:
inline int myRandom (int i) {
  return std::rand()%i;
}

void KNHome::gbCnf::getRandConf(Config& cnf,\
                                const vector<string>& elems,\
                                const vector<int>& nums) {
  assert(("input nums does not sum up to configuration sites",\
        cnf.natoms == std::accumulate(nums.begin(), nums.end(), 0)));
  vector<int> TPArr(nums[0], 0);
  for (unsigned int i = 1; i < nums.size(); ++i) {
    for (unsigned int j = 0; j < nums[i]; ++j) {
      TPArr.push_back(i);
    }
  }

  std::random_shuffle(TPArr.begin(), TPArr.end(), myRandom);

  for (unsigned int i = 0; i < TPArr.size(); ++i) {
    for (unsigned int j = 0; j < elems.size(); ++j) {
      if (TPArr[i] == j) {
        cnf.atoms[i].tp = elems[j];
      }
      if (cnf.atoms[i].tp == "X") {
        cnf.vacList.push_back(i);
      }
    }
  }
}

vector<pair<int, int>> KNHome::gbCnf::getPairToSwap(Config& cnf) {
  vector<pair<int, int>> res;
  getNBL(cnf, rcut);
  for (unsigned int i = 0; i < cnf.vacList.size(); ++i) {
    for (unsigned int j = 0; j < cnf.atoms[cnf.vacList[i]].NBL.size(); ++j) {
      if (cnf.atoms[j].tp != cnf.atoms[cnf.vacList[i]].tp) {
        res.push_back( std::make_pair(cnf.vacList[i], \
              cnf.atoms[cnf.vacList[i]].NBL[j]) );
      }
    }
  }
  return res;
}

Config KNHome::gbCnf::swapPair(const Config& c0, pair<int, int> atomPair) {
  int idx0 = atomPair.first;
  int idx1 = atomPair.second;
#ifdef DEBUG
  cout << idx0 << " " << idx1 << endl;
#endif
  Config c1 = c0;
  //c1.atoms[idx0].tp.swap( c1.atoms[idx1].tp );

  string tmpType = c1.atoms[idx0].tp;
  c1.atoms[idx0].tp = c1.atoms[idx1].tp;
  c1.atoms[idx1].tp = tmpType;

  return c1;
}

void KNHome::createPreNEB(){
  gbCnf cnfModifier(*this);
  vector<int> dupFactors = viparams["factors"];
  double LC = dparams["LC"];
  vector<string> elems = vsparams["elems"];
  vector<int> nums = viparams["nums"]; 
  int NConfigs = iparams["NConfigs"];
  int NBars = iparams["NBarriers"];

  MPI_Barrier(MPI_COMM_WORLD);
  if (me == 0)
    std::cout << "generating inital and final structures\n";
  int quotient = NConfigs / nProcs;
  int remainder = NConfigs % nProcs;
  int nCycle = remainder ? (quotient + 1) : quotient;
  /*calculate total boltzmann energy Z*/
  Config c0 = cnfModifier.getFCCConv(LC, elems[0], dupFactors);
  for (int j = 0; j < nCycle; ++j) {
    for (int i = (j * nProcs); i < ((j + 1) * nProcs); ++i) {
      if ((i % nProcs != me) || (i >= NConfigs)) continue;

      cnfModifier.getRandConf(c0, elems, nums);
      Config c0copy = c0;

      string baseDir = "mkdir -p config" + to_string(i) + "/start";
      const char *cbaseDir = baseDir.c_str();
      const int dir_err = std::system(cbaseDir);
      if (-1 == dir_err) {
        cout << "Error creating directory!\n";
        exit(1);
      }

      cnfModifier.writePOSCAR(c0, "config" + to_string(i) + "/start/POSCAR");
      cnfModifier.writePOSCARVis(c0, "config" + to_string(i) + "/start/POSCAR.vis");
      cnfModifier.writeCfgData(c0, "config" + to_string(i) + "/start/0.cfg");
      vector<pair<int, int>> pairs = cnfModifier.getPairToSwap(c0copy);
      std::random_shuffle(pairs.begin(), pairs.end(), myRandom);
      int end = MIN(NBars, pairs.size());
      for (unsigned int k = 0; k < end; ++k) {

        string subDir = "config" + to_string(i) + "/end_" + to_string(k);
        const char *csubDir = subDir.c_str();
        const int dir_err = mkdir(csubDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (-1 == dir_err) {
          cout << "Error creating directory!\n";
          exit(1);
        }

        Config c1 = cnfModifier.swapPair(c0copy, pairs[k]);
        string name1 = "config" + to_string(i) + "/end_" + to_string(k) + "/";
        cnfModifier.writePOSCAR(c1, name1 + "POSCAR");
        cnfModifier.writePOSCARVis(c1, name1 + "POSCAR.vis");
        cnfModifier.writeCfgData(c0, name1 + "end.cfg");
      }
    }
  }
}
