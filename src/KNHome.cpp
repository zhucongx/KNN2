/*
 * Author: 1mingfei
 * Date:   2019-05-27
 * Purpose: functions for KNHome
 * self-explained
 */

#include "gbCnf.h"
#include "KNHome.h"
#include "LSKMC.h"

KNHome::KNHome(int argc, char* argv[]) {
  MPI_Comm_size(MPI_COMM_WORLD, &nProcs);
  MPI_Comm_rank(MPI_COMM_WORLD, &me);

  sparams["mode"] = "generate";
  sparams["initconfig"] = "initconfig.cfg";
  dparams["LC"] = 4.046;
  // dparams["Rcut"] = 6.0; //encode
  dparams["Rcut"] = 3.0; //kmc
  iparams["randSeed"] = 1234567; //kmc

  parseArgs(argc, argv);
  initParam();

  gbCnf cnfModifier(sparams);

  if ((sparams["mode"]) == "generate") {
    // srand(time(NULL) + me);
    srand(iparams["randSeed"] + me);
    createPreNEB();
  } else if (sparams["mode"] == "encode") {
    KNEncode();
  } else if (sparams["mode"] == "BondCount") {
    KNBondCount();
  } else if (sparams["mode"] == "kmc") {
    KMCSimulation(cnfModifier);
  } else if (sparams["mode"] == "lskmc") {
    LSKMCSimulation(cnfModifier);
  } else if (sparams["mode"] == "test") {
    // testK2P();
    /* test encoding */
    // gbCnf cnfModifier(*this);
    // Config c1 = cnfModifier.readCfg("in.cfg");
    // cnfModifier.writePOSCARVis(c1, "POSCAR", "");
    // cnfModifier.writeCfgData(c1, "test_out.cfg");
    /* test LSKMC */
    // LSKMCSimulation(cnfModifier);

    /* test mat transfer */
    LS::LSKMC::test_vvd2mat();


  }


}

KNHome::~KNHome() {}
