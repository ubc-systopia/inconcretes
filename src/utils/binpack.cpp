#include "binpack.h"

#include <algorithm>

#include "macros.h"

namespace Utils {

// Adopted from https://github.com/brandenburg/schedcat/blob/master/schedcat/mapping/binpack.py

BinPack::BinPack(std::map<unsigned, double> items, unsigned num_bins,
                 double max_bin_capacity)
  : items(items), num_bins(num_bins), max_bin_capacity(max_bin_capacity) {
}

bool BinPack::next_fit() {
  mapping.clear();
  misfits.clear();

  unsigned bid = 0;
  double sum = 0.0;

  for (std::pair<const unsigned, double>& p : items) {
    while (sum + p.second > max_bin_capacity) {
      sum = 0.0;
      bid += 1;
      if (bid == num_bins) {
        break;
      }
    }

    if (bid == num_bins) {
      misfits.insert(p.first);
      break;
    }

    mapping[p.first] = bid;
    sum += p.second;
  }

  return misfits.empty();
}

bool BinPack::first_fit() {
  mapping.clear();
  misfits.clear();

  std::vector<std::pair<unsigned, double>> bins;
  for (int i = 0; i < num_bins; i++) {
    bins.push_back(std::make_pair(i, 0));
  }

  for (std::pair<const unsigned, double>& i : items) {

    for (std::pair<unsigned, double>& b : bins) {
      unsigned bid = b.first;
      double bin_capacity = b.second;
      if (i.second + b.second <= max_bin_capacity) {
        mapping[i.first] = b.first;
        b.second += i.second;
        break;
      }
    }

    if (mapping.find(i.first) == mapping.end()) {
      misfits.insert(i.first);
    }
  }

  return misfits.empty();
}

bool BinPack::best_fit() {
  mapping.clear();
  misfits.clear();

  std::vector<std::pair<unsigned, double>> bins;
  for (int i = 0; i < num_bins; i++) {
    bins.push_back(std::make_pair(i, 0));
  }

  for (std::pair<const unsigned, double>& i : items) {

    std::sort(bins.begin(), bins.end(), BinPack::cmp_opp);
    for (std::pair<unsigned, double>& b : bins) {
      if (i.second + b.second <= max_bin_capacity) {
        mapping[i.first] = b.first;
        b.second += i.second;
        break;
      }
    }

    if (mapping.find(i.first) == mapping.end()) {
      misfits.insert(i.first);
    }
  }

  return misfits.empty();
}

bool BinPack::worst_fit() {
  mapping.clear();
  misfits.clear();

  std::vector<std::pair<unsigned, double>> bins;
  for (int i = 0; i < num_bins; i++) {
    bins.push_back(std::make_pair(i, 0));
  }

  for (std::pair<const unsigned, double>& i : items) {

    std::sort(bins.begin(), bins.end(), BinPack::cmp);
    for (std::pair<unsigned, double>& b : bins) {
      if (i.second + b.second <= max_bin_capacity) {
        mapping[i.first] = b.first;
        b.second += i.second;
        break;
      }
    }

    if (mapping.find(i.first) == mapping.end()) {
      misfits.insert(i.first);
    }
  }

  return misfits.empty();
}

bool BinPack::any_fit() {
  if (next_fit() or first_fit() or best_fit() or worst_fit()) {
    return true;
  }
  return false;
}

bool BinPack::cmp(std::pair<unsigned, double>& a,
                  std::pair<unsigned, double>& b) {
  return a.second < b.second;
}

bool BinPack::cmp_opp(std::pair<unsigned, double>& a,
                      std::pair<unsigned, double>& b) {
  return a.second > b.second;
}

} // namespace Utils
