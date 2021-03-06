#include "gbCnf.h"
#include "KNHome.h"

// This function returns the clusters information stored
// in OrderedStruct Class to
// the FCC config. Index i means we use the information stored in the i th row.
Config gbCnf::embedCluster(const Config& cIn, \
                           const pair<string, string>& Elems, \
                           const ODS::OrderedStruct& o256, \
                           const int& i) {
  Config c0 = cIn;

  for (int j = 0; j < o256.mapping[i].size(); ++j) {
    if (o256.mapping[i][j] == 1) {
      c0.atoms[j].tp = Elems.first;
    } else if (o256.mapping[i][j] == 2) {
      c0.atoms[j].tp = Elems.second;
    }
  }
  return c0;
}

// This function creates structure proto config in the root folder and create
// more configs with vac for vasp calculations
int KNHome::createSingle(const int& i, \
                         int index, \
                         gbCnf& cnfModifier, \
                         const vector<int>& dupFactors, \
                         const double& LC, \
                         const string& POT, \
                         const ODS::OrderedStruct& o256, \
                         const pair<string, string>& elemPair) {

  Config c0 = cnfModifier.getFCCConv(LC, "Al", dupFactors);

  const vector<pair<int, int>>& jumpPairsRef = o256.jumpPairs[i];
  int subIndex = 0;

  // create structure proto for visualization
  Config c1 = cnfModifier.embedCluster(c0, elemPair, o256, i);
  cnfModifier.writeCfgData(c1, to_string(index) + ".cfg");

  Config cS;
  for (int j = 0; j < jumpPairsRef.size(); ++j) {
    string tmpType;
    if ((j == 0) || \
         ((j > 0) && (jumpPairsRef[j].first != jumpPairsRef[j - 1].first))) {
      // Create sub folder "s" for initial vac config
      string baseDir = "config" + to_string(index) + "/s";
      string mkBaseDir = "mkdir -p " + baseDir;
      const char* cmkBaseDir = mkBaseDir.c_str();
      const int dir_err = std::system(cmkBaseDir);
      if (-1 == dir_err) {
        cout << "Error creating directory!\n";
        exit(1);
      }
      cS = c0;
      cS = cnfModifier.embedCluster(c1, elemPair, o256, i);

      tmpType = cS.atoms[jumpPairsRef[j].first].tp;
      cS.atoms[jumpPairsRef[j].first].tp = "X";

      cnfModifier.writeCfgData(cS, "config" + to_string(index) + \
                                   "/s/start.cfg");

      std::sort(cS.atoms.begin(), cS.atoms.end());
      cnfModifier.cnvprl2pst(cS);
      cnfModifier.perturb(cS);
      map<string, int> elemName = cnfModifier.writePOSCAR(cS, \
                            "config" + to_string(index) + "/s/POSCAR");
      prepVASPFiles(baseDir, dupFactors, elemName, POT);
      subIndex = 0;
    }

    // Create sub folders for different jump directions
    string subDir = "config" + to_string(index) \
                    + "/e_" + to_string(subIndex);
    const char* csubDir = subDir.c_str();
    const int dir_err = mkdir(csubDir, \
                              S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err) {
      cout << "Error creating directory!\n";
      exit(1);
    }

    Config cF = cnfModifier.swapPair(c0, jumpPairsRef[j]);
    cF = cnfModifier.embedCluster(cF, elemPair, o256, i);
    cF.atoms[jumpPairsRef[j].first].tp = "X";

    string name1 = "config" + to_string(index) \
                   + "/e_" + to_string(subIndex) + "/";
    cnfModifier.writeCfgData(cF, name1 + "end.cfg");

    std::sort(cF.atoms.begin(), cF.atoms.end());
    cnfModifier.cnvprl2pst(cF);
    cnfModifier.perturb(cF);
    map<string, int> elemName = cnfModifier.writePOSCAR(cF, \
                                                name1 + "POSCAR");

    ofstream ofs("log.txt", std::ofstream::app);

    ofs << "config " << index << " end " << subIndex \
        << " pair: " << jumpPairsRef[j].first \
        << " " << jumpPairsRef[j].second << "\n";

    ofs.close();

    prepVASPFiles(name1, dupFactors, elemName, POT);

    ++subIndex;
    if ((j < jumpPairsRef.size()) \
        && (jumpPairsRef[j].first != jumpPairsRef[j+1].first)) {
      ++index;
    }

  }
  return index;
}

void KNHome::createOrdered(gbCnf& cnfModifier, \
                           const vector<int>& dupFactors, \
                           const double& LC, \
                           const string& POT) {
  vector<pair<string, string>> elemPairs = {{"Al", "Mg"}, \
                                            {"Al", "Zn"}, \
                                            {"Mg", "Zn"}};
  int index = 0;
  ODS::OrderedStruct o256;
  o256.generateAuFeOccupInfo();
  for (int i = 0; i < o256.mapping.size(); ++i)
    for (const auto& elemPair : elemPairs) {
      index = createSingle(i, index, cnfModifier, dupFactors, \
                           LC, POT, o256, elemPair);
    }
}

void KNHome::createOrderedRandom(gbCnf& cnfModifier, \
                                 const vector<int>& dupFactors, \
                                 const double& LC, \
                                 const string& POT, \
                                 const int& dupTimes) {
  pair<string, string> elemPair = {"Zn", "Mg"};
  ODS::OrderedStruct oRef;
  oRef.generateAuFeOccupInfo();
  int index = 0;
  for (int i = 0; i < oRef.mapping.size(); ++i) {
    for (int j = 0; j < dupTimes; ++j) {
      for (int k = 1; k <= 2; ++k) {
        ODS::OrderedStruct o256(oRef);
        o256.omit(i, k);
        o256.makeRandom(i);
        index = createSingle(i, index, cnfModifier, dupFactors, \
                             LC, POT, o256, elemPair);
      }
    }
  }
}
void KNHome::createOrderedDiffCon(gbCnf& cnfModifier, \
                                  const vector<int>& dupFactors, \
                                  const double& LC, \
                                  const string& POT, \
                                  const int& numDataset) {
  vector<double> concentrationFracList;
  concentrationFracList.push_back(0.0);
  for (int i = 0; i < numDataset; ++i) {
    auto num = static_cast<double>(i);
    concentrationFracList.push_back((num + 1) / (numDataset + 1));
  }
  concentrationFracList.push_back(1.0);

  pair<string, string> elemPair = {"Zn", "Mg"};
  ODS::OrderedStruct oRef;
  oRef.generateAuFeOccupInfo();
  int index = 0;
  for (int i = 0; i < oRef.mapping.size(); ++i) {
    for (int j = 1; j <= 2; ++j) {
      for (const auto& k : concentrationFracList) {
        ODS::OrderedStruct o256(oRef);
        o256.omit(i, j);
        o256.makeShuffleFraction(i, k);
        index = createSingle(i, index, cnfModifier, dupFactors, \
                             LC, POT, o256, elemPair);
      }
    }
  }
}
void KNHome::createOrderedAntiPhase(gbCnf& cnfModifier, \
                                  const vector<int>& dupFactors, \
                                  const double& LC, \
                                  const string& POT, \
                                  const int& dupTimes) {

  pair<string, string> elemPair = {"Mg", "Zn"};
  int index = 0;
  ODS::OrderedStruct o256;
  o256.generateCuAuOccupInfo();
  for (int i = 0; i < o256.mapping.size(); ++i) {
    for (int j = 0; j < dupTimes; ++j) {
      ODS::OrderedStruct o256;
      o256.generateCuAuOccupInfo();
      index = createSingle(i, index, cnfModifier, dupFactors, \
                           LC, POT, o256, elemPair);
    }
  }
}