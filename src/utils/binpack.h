#ifndef UTILS_BINPACK_H
#define UTILS_BINPACK_H

#include "logging.h"

namespace Utils {

class BinPack {
 public:
  std::map<unsigned, double> items; // Item IDs and their respective weights
  unsigned num_bins; // Maximum number of bins
  double max_bin_capacity; // Maximum capacity per bin
  std::map<unsigned, unsigned> mapping; // Item IDs and their assigned bins
  std::set<unsigned> misfits; // Items that did not fit into any bins

  BinPack(std::map<unsigned, double> items, unsigned num_bins,
          double max_bin_capacity);

  bool next_fit();
  bool first_fit();
  bool best_fit();
  bool worst_fit();
  bool any_fit();

  static bool cmp(std::pair<unsigned, double>& a,
                  std::pair<unsigned, double>& b);
  static bool cmp_opp(std::pair<unsigned, double>& a,
                      std::pair<unsigned, double>& b);
};

} // namespace Utils

#endif  // UTILS_BINPACK_H
