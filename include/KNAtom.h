#ifndef _KNATOM_
#define _KNATOM_

#define FNN 12

#include <array>
using std::vector;
using std::string;
using std::array;

class KNAtom {
private:
public:
  int id;
  int clusterID;
  string tp;
  double pst[3], prl[3];
  vector<int> NBL;
  array<int, FNN> FNNL;

  KNAtom() : id(0), tp("H"), clusterID(-1) {};
  KNAtom(int n) : id(n), tp("H"), clusterID(-1) {};
  KNAtom(int _id, double x, double y, double z)
   : id(_id),
     tp("H"),
     clusterID(-1) {
    prl[0] = x, prl[1] = y, prl[2] = z;
    FNNL.fill(-1);
  };

  KNAtom(int _id, string _tp, double x, double y, double z):
    id(_id), tp(_tp), clusterID(-1) {
    prl[0] = x, prl[1] = y, prl[2] = z;
    FNNL.fill(-1);
  }

  ~KNAtom(){};

  bool operator<(const KNAtom& b) const { return tp < b.tp; }
};

#endif
