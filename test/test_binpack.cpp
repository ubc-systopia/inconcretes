#include "catch.hpp"

#include "main.h"
#include "utils/logging.h"
#include "utils/binpack.h"

TEST_CASE("Test next-fit heuristic for bin-packing", "[BinPack]") {
  std::map<unsigned, double> items;
  items[0] = 0.23;
  items[1] = 0.39;
  items[2] = 0.41;
  items[3] = 0.83;
  items[4] = 0.03;
  items[5] = 0.01;
  items[6] = 0.30;

  unsigned num_bins = 4;
  double max_bin_capacity = 1.0;

  Utils::BinPack bp1(items, num_bins, max_bin_capacity);
  REQUIRE(bp1.next_fit());
  REQUIRE(bp1.misfits.empty());
  REQUIRE(bp1.mapping[0] == 0);
  REQUIRE(bp1.mapping[1] == 0);
  REQUIRE(bp1.mapping[2] == 1);
  REQUIRE(bp1.mapping[3] == 2);
  REQUIRE(bp1.mapping[4] == 2);
  REQUIRE(bp1.mapping[5] == 2);
  REQUIRE(bp1.mapping[6] == 3);

  items[7] = 0.75;

  Utils::BinPack bp2(items, num_bins, max_bin_capacity);
  REQUIRE(!bp2.next_fit());
  REQUIRE(bp2.misfits.size() == 1);
  REQUIRE(bp2.misfits.find(7) != bp2.misfits.end());
}

TEST_CASE("Test first-fit heuristic for bin-packing", "[BinPack]") {
  std::map<unsigned, double> items;
  items[0] = 0.23;
  items[1] = 0.39;
  items[2] = 0.41;
  items[3] = 0.83;
  items[4] = 0.03;
  items[5] = 0.01;
  items[6] = 0.30;

  unsigned num_bins = 4;
  double max_bin_capacity = 1.0;

  Utils::BinPack bp1(items, num_bins, max_bin_capacity);
  REQUIRE(bp1.first_fit());
  REQUIRE(bp1.misfits.empty());
  REQUIRE(bp1.mapping[0] == 0);
  REQUIRE(bp1.mapping[1] == 0);
  REQUIRE(bp1.mapping[2] == 1);
  REQUIRE(bp1.mapping[3] == 2);
  REQUIRE(bp1.mapping[4] == 0);
  REQUIRE(bp1.mapping[5] == 0);
  REQUIRE(bp1.mapping[6] == 0);

  items[7] = 0.75;
  items[8] = 0.75;

  Utils::BinPack bp2(items, num_bins, max_bin_capacity);
  REQUIRE(!bp2.first_fit());
  REQUIRE(bp2.misfits.size() == 1);
  REQUIRE(bp2.misfits.find(8) != bp2.misfits.end());
}

TEST_CASE("Test best-fit heuristic for bin-packing", "[BinPack]") {
  std::map<unsigned, double> items;
  items[0] = 0.23;
  items[1] = 0.39;
  items[2] = 0.41;
  items[3] = 0.83;
  items[4] = 0.03;
  items[5] = 0.01;
  items[6] = 0.30;

  unsigned num_bins = 4;
  double max_bin_capacity = 1.0;

  Utils::BinPack bp1(items, num_bins, max_bin_capacity);
  REQUIRE(bp1.best_fit());
  REQUIRE(bp1.misfits.empty());
  REQUIRE(bp1.mapping[0] == 0);
  REQUIRE(bp1.mapping[1] == 0);
  REQUIRE(bp1.mapping[2] == 1);
  REQUIRE(bp1.mapping[3] == 2);
  REQUIRE(bp1.mapping[4] == 2);
  REQUIRE(bp1.mapping[5] == 2);
  REQUIRE(bp1.mapping[6] == 0);

  items[7] = 0.75;
  items[8] = 0.75;

  Utils::BinPack bp2(items, num_bins, max_bin_capacity);
  REQUIRE(!bp2.best_fit());
  REQUIRE(bp2.misfits.size() == 1);
  REQUIRE(bp2.misfits.find(8) != bp2.misfits.end());
}

TEST_CASE("Test worst-fit heuristic for bin-packing", "[BinPack]") {
  std::map<unsigned, double> items;
  items[0] = 0.23;
  items[1] = 0.39;
  items[2] = 0.41;
  items[3] = 0.83;
  items[4] = 0.03;
  items[5] = 0.01;
  items[6] = 0.30;

  unsigned num_bins = 4;
  double max_bin_capacity = 1.0;

  Utils::BinPack bp1(items, num_bins, max_bin_capacity);
  REQUIRE(bp1.worst_fit());
  REQUIRE(bp1.misfits.empty());
  REQUIRE(bp1.mapping[0] == 0);
  REQUIRE(bp1.mapping[1] == 1);
  REQUIRE(bp1.mapping[2] == 2);
  REQUIRE(bp1.mapping[3] == 3);
  REQUIRE(bp1.mapping[4] == 0);
  REQUIRE(bp1.mapping[5] == 0);
  REQUIRE(bp1.mapping[6] == 0);

  items[7] = 0.75;
  items[8] = 0.75;

  Utils::BinPack bp2(items, num_bins, max_bin_capacity);
  REQUIRE(!bp2.worst_fit());
  REQUIRE(bp2.misfits.size() == 2);
  REQUIRE(bp2.misfits.find(7) != bp2.misfits.end());
  REQUIRE(bp2.misfits.find(8) != bp2.misfits.end());
}

TEST_CASE("Test any-fit heuristic for bin-packing", "[BinPack]") {
  std::map<unsigned, double> items;
  items[0] = 0.3;
  items[1] = 0.5;
  items[2] = 0.7;
  items[3] = 0.5;

  unsigned num_bins = 2;
  double max_bin_capacity = 1.0;

  Utils::BinPack bp1(items, num_bins, max_bin_capacity);
  REQUIRE(!bp1.next_fit());
  REQUIRE(!bp1.first_fit());
  REQUIRE(!bp1.best_fit());
  REQUIRE(bp1.worst_fit());
  REQUIRE(bp1.mapping[0] == 0);
  REQUIRE(bp1.mapping[1] == 1);
  REQUIRE(bp1.mapping[2] == 0);
  REQUIRE(bp1.mapping[3] == 1);
  REQUIRE(bp1.any_fit());
  REQUIRE(bp1.mapping[0] == 0);
  REQUIRE(bp1.mapping[1] == 1);
  REQUIRE(bp1.mapping[2] == 0);
  REQUIRE(bp1.mapping[3] == 1);
}
