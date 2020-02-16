#include "gbCnf.h"
#include "KNHome.h"

#define KB 8.6173303e-5
#define KB_INV 11604.5221105
#define NEI_NUMBER 12

/*  first element in the range [first, last)
 *  that is not less than (i.e. greater or equal to) value */
template<class ForwardIt, class T, class Compare>
inline ForwardIt mylower_bound(ForwardIt first, \
                               ForwardIt last, \
                               const T& value,\
                               Compare comp) {
  ForwardIt it;
  typename std::iterator_traits<ForwardIt>::difference_type count, step;
  count = std::distance(first, last);

  while (count > 0) {
    it = first;
    step = count / 2;
    std::advance(it, step);
    if (comp(*it, value)) {
      first = ++it;
      count -= step + 1;
    }
    else
      count = step;
  }
  return first;
}

void KNHome::buildEmbedding() {
  vector<string> elems = vsparams["elems"];
  for (int i = 1; i <= elems.size(); ++i)
    embedding[elems[i - 1]] = static_cast<double>(i);
}

void KNHome::getVacList() {
  for (int i = 0; i < c0.atoms.size(); ++i) {
    if (c0.atoms[i].tp == "X")
      vacList.push_back(i);
  }
}

void KNHome::KMCInit(gbCnf& cnfModifier) {

  buildEmbedding();
  if (me == 0)
    ofs.open("log.txt", std::ofstream::out | std::ofstream::app);

  E_tot = 0.0;
  string fname = sparams["initconfig"];
  RCut = (dparams["RCut"] == 0.0) ? 3.0 : dparams["RCut"];
  RCut2 = 1.65 * RCut;
  maxIter = iparams["maxIter"];
  nTallyConf = (iparams["nTallyConf"] == 0) ? 1000 : iparams["nTallyConf"];
  nTallyOutput = \
           (iparams["nTallyOutput"] == 0) ? 1000 : iparams["nTallyOutput"];
  EDiff = sparams["EDiff"];
  ECutoff = dparams["ECutoff"];

  trapStep = 0;
  temperature = dparams["temperature"];
  srand(iparams["randSeed"]);
  prefix = dparams["prefix"];
  c0 = cnfModifier.readCfg(fname);

  /* get initial vacancy position in atomList */
  getVacList();

  /* get neibour list of atoms */
  cnfModifier.wrapAtomPrl(c0);

  if (nProcs == 1)
    cnfModifier.getNBL_serial(c0, RCut2);
  else
    cnfModifier.getNBL(c0, RCut2);

#ifdef DEBUG
  if (me == 0)
    for (int i = 0; i < vacList.size(); ++i) {
      cout << vacList[i] << " size: " << c0.atoms[vacList[i]].FNNL.size() \
      << endl;
      for (int j = 0; j < c0.atoms[vacList[i]].FNNL.size(); ++j)
        cout << c0.atoms[vacList[i]].FNNL[j] << " ";
      cout << endl;
    }
#endif

  /* reading the model binary file, initialize the model */
  string modelFname = sparams["kerasModelBarrier"];
  k2pModelB = Model::load(modelFname);
  if (EDiff == "model") {
    modelFname = sparams["kerasModelEDiff"];
    k2pModelD = Model::load(modelFname);
  }

  /* initialize parameters */
  iter = 0;
  if (sparams["method"] == "restart") {

    time = dparams["startingTime"];
    step = iparams["startingStep"];
    E_tot = dparams["startingEnergy"];
    if (me == 0)
      ofs << "#restarting from step " << step << "\n";

  } else {
    time = (dparams["startingTime"] == 0.0) ? 0.0 : dparams["startingTime"];
    E_tot = \
          (dparams["startingEnergy"] == 0.0) ? 0.0 : dparams["startingEnergy"];
    step = (iparams["startingStep"] == 0) ? 0 : iparams["startingStep"];
    if (me == 0)
      cnfModifier.writeCfgData(c0, to_string(step) + ".cfg");
  }
  if (me == 0)
    ofs << "#step     time     Ediff\n";

}

// typedef struct cmp {
//   bool operator() (KMCEvent a, double value) {
//     return (a.getcProb() < value);
//   }
// } comp;

KMCEvent KNHome::selectEvent(int& dist) {
  double randVal = (double) rand() / (RAND_MAX);
  auto it = mylower_bound(eventList.begin(), eventList.end(), randVal, \
                          [] (KMCEvent a, double value) \
                            {return (a.getcProb() < value);});

  if (it == eventList.cend()) {
    dist = eventList.size() - 1;
    return eventList.back();
  }

  dist = distance(eventList.begin(), it);
#ifdef DEBUGJUMP
  cout << "step " << (step + 1) << " time " << time
       << " prob: " << randVal << " event: " << dist \
       << " event cprob: " << it->getcProb() << " jumpPair: " \
       << it->getJumpPair().first << " " << it->getJumpPair().second \
       << endl;
#endif
  return *it;
}

double KNHome::updateTime() {
  /* update time elapsed */
  double tau = log(rand() / static_cast<double>(RAND_MAX)) \
               / (prefix * kTot);

  time -= tau;
  return -tau;
}

void KNHome::updateEnergy(const int& eventID) {
  E_tot += eventList[eventID].getEnergyChange();
}

void KNHome::buildEventList_serial(gbCnf& cnfModifier) {
  eventList.clear();
  kTot = 0.0;
  int i, j;

  for (i = 0; i < vacList.size(); ++i) {
    for (j = 0; j < c0.atoms[vacList[i]].FNNL.size(); ++j) {
      /* skip Vac jump to Vac in event list */

      int iFirst = vacList[i];
      int iSecond = c0.atoms[vacList[i]].FNNL[j];

      // if (c0.atoms[iFirst].tp == c0.atoms[iSecond].tp)
      //   continue;

      KMCEvent event(make_pair(iFirst, iSecond));
      // string tmpHash = to_string(iFirst) + "_" + to_string(iSecond);
      // eventListMap[tmpHash] = i * jumpList[0].size() + j;

      vector<double> currBarrier = cnfModifier.calBarrierAndEdiff(c0, \
                                temperature, \
                                RCut2, \
                                EDiff, \
                                embedding, \
                                k2pModelB, \
                                k2pModelD, \
                                make_pair(iFirst, iSecond));

      if (c0.atoms[iFirst].tp == c0.atoms[iSecond].tp) {
        event.setRate(0.0);
        event.setEnergyChange(0.0);
        event.setBarrier(0.0);
      }
      else {
        event.setRate(exp(-currBarrier[0] * KB_INV / temperature));
        event.setEnergyChange(currBarrier[1]);
        event.setBarrier(currBarrier[0]);
      }

      kTot += event.getRate();
      eventList.push_back(event);
    }
  }

  /* calculate relative and cumulative probability */
  double curr = 0.0;
  for (int i = 0; i < eventList.size(); ++i) {
    auto&& event = eventList[i];
    event.calProb(kTot);
    curr += event.getProb();
    event.setcProb(curr);
  }
#ifdef DEBUGJUMP
  for (int i = 0; i < eventList.size(); ++i) {
    const auto& event = eventList[i];
    cout << setprecision(12) << i << " rate: " << event.getRate() \
         << " cumulative prob: " << event.getcProb() << endl;
  }
  cout << endl;
#endif
}

void KNHome::buildEventList(gbCnf& cnfModifier) {
  eventList.clear();
  kTot = 0.0;
  int i, j;
  
  #pragma omp parallel shared(eventList, vacList) private(i, j) reduction(+:kTot)
  for (i = 0; i < vacList.size(); ++i) {
    #pragma omp for ordered
    for (j = 0; j < c0.atoms[vacList[i]].FNNL.size(); ++j) {
      /* skip Vac jump to Vac in event list */

      int iFirst = vacList[i];
      int iSecond = c0.atoms[vacList[i]].FNNL[j];

      // if (c0.atoms[iFirst].tp == c0.atoms[iSecond].tp)
      //   continue;

      KMCEvent event(make_pair(iFirst, iSecond));
      // string tmpHash = to_string(iFirst) + "_" + to_string(iSecond);
      // eventListMap[tmpHash] = i * jumpList[0].size() + j;

      vector<double> currBarrier = cnfModifier.calBarrierAndEdiff(c0, \
                                temperature, \
                                RCut2, \
                                EDiff, \
                                embedding, \
                                k2pModelB, \
                                k2pModelD, \
                                make_pair(iFirst, iSecond));

      if (c0.atoms[iFirst].tp == c0.atoms[iSecond].tp) {
        event.setRate(0.0);
        event.setEnergyChange(0.0);
        event.setBarrier(0.0);
      }
      else {
        event.setRate(exp(-currBarrier[0] * KB_INV / temperature));
        event.setEnergyChange(currBarrier[1]);
        event.setBarrier(currBarrier[0]);
      }

      kTot += event.getRate();
      eventList.push_back(event);
    }
  }

  /* calculate relative and cumulative probability */
  double curr = 0.0;
  for (int i = 0; i < eventList.size(); ++i) {
    auto&& event = eventList[i];
    event.calProb(kTot);
    curr += event.getProb();
    event.setcProb(curr);
  }
#ifdef DEBUGJUMP
  for (int i = 0; i < eventList.size(); ++i) {
    const auto& event = eventList[i];
    cout << setprecision(12) << i << " rate: " << event.getRate() \
         << " cumulative prob: " << event.getcProb() << endl;
  }
  cout << endl;
#endif
}

void KNHome::KMCSimulation(gbCnf& cnfModifier) {

  KMCInit(cnfModifier);
  MPI_Barrier(MPI_COMM_WORLD);

  while (iter < maxIter) {

    // if (nProcs == 1)
    //   buildEventList_serial(cnfModifier);
    if (me == 0)
      buildEventList(cnfModifier);

    if (me == 0) {
      int eventID = 0;
      auto&& event = selectEvent(eventID);
      event.exeEvent(c0, RCut); // event updated
      double oneStepTime = updateTime();
      updateEnergy(eventID);
    }

    ++step;
    ++iter;

    if (me == 0) {
      if (step % nTallyOutput == 0)
        ofs << std::setprecision(7) << step << " " << time << " " \
            << E_tot << " " << endl;

      if (step % nTallyConf == 0)
        cnfModifier.writeCfgData(c0, to_string(step) + ".cfg");
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

}

vector<double> gbCnf::calBarrierAndEdiff(Config& c0, \
                              const double& T, \
                              const double& RCut2, \
                              const string& EDiff, \
                              unordered_map<string, double>& embedding, \
                              Model& k2pModelB, \
                              Model& k2pModelD, \
                              const pair<int, int>& jumpPair) {

  int first = jumpPair.first;
  int second = jumpPair.second;

  // if (c0.atoms[first].tp == c0.atoms[second].tp)
  //   return {0.0, 0.0};

  vector<string> codes; // atom location in original atom list
  //RCut2 for 2NN encoding needed
  vector<vector<string>> encodes = encodeConfig(c0, \
                                                {first, second}, \
                                                RCut2, \
                                                codes, \
                                                {first, second}, \
                                                false);
  vector<vector<double>> input(encodes.size(), \
                               vector<double>(encodes[0].size(), 0.0));

  for (int i = 0; i < encodes.size(); ++i)
    for (int j = 0; j < encodes[i].size(); ++j)
      input[i][j] = embedding[encodes[i][j]];

  int nRow = input.size(); // encodings for one jump pair considering symmetry
  int nCol = nRow ? input[0].size() : 0;
  Tensor in{ nRow, nCol };

  for (int i = 0; i < nRow; ++i)
    for (int j = 0; j < nCol; ++j)
      in.data_[i * nCol + j] = input[i][j];

  if (EDiff == "model") {
    Tensor outB = k2pModelB(in);
    Tensor outD = k2pModelD(in);

    double deltaE = 0.0;
    double tmpEdiff = 0.0;

    for (int i = 0; i < nRow; ++i) {
      deltaE += static_cast<double>(outB(i, 0));
      tmpEdiff += static_cast<double>(outD(i, 0));
    }
    deltaE /= static_cast<double>(nRow);
    tmpEdiff /= static_cast<double>(nRow);

    return {deltaE, tmpEdiff};
  }  else if (EDiff == "barrier"){
    Tensor outB = k2pModelB(in);

    double deltaE = 0.0;

    for (int i = 0; i < nRow; ++i) {
      deltaE += static_cast<double>(outB(i, 0));
    }
    deltaE /= static_cast<double>(nRow);

    Tensor inBack{ nRow, nCol };

    for (int i = 0; i < nRow; ++i) {
      inBack.data_[i * nCol] = input[i][0];
      for (int j = 1; j < nCol; ++j)
        inBack.data_[i * nCol + j] = input[i][nCol - j];
    }
    Tensor outBBack = k2pModelB(inBack);

    double deltaEBack = 0.0;

    for (int i = 0; i < nRow; ++i) {
      deltaEBack += static_cast<double>(outBBack(i, 0));
    }
    deltaEBack /= static_cast<double>(nRow);

    return {deltaE, deltaE - deltaEBack};
  } else {
    Tensor outB = k2pModelB(in);

    double deltaE = 0.0;

    for (int i = 0; i < nRow; ++i) {
      deltaE += static_cast<double>(outB(i, 0));
    }
    deltaE /= static_cast<double>(nRow);

    return {deltaE, 0.0};
  }
}
