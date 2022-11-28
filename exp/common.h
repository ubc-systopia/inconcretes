#ifndef EXP_COMMON_H
#define EXP_COMMON_H

char bft_kvs_ports[3][4][5] = {
  {
    "8080", // CPU 0, Node 0
    "8081", // CPU 0, Node 1
    "8082", // CPU 0, Node 2
    "8083", // CPU 0, Node 3
  },
  {
    "8084", // CPU 1, Node 0
    "8085", // CPU 1, Node 1
    "8086", // CPU 1, Node 2
    "8087", // CPU 1, Node 3
  },
  {
    "8088", // CPU 2, Node 0
    "8089", // CPU 2, Node 1
    "8090", // CPU 2, Node 2
    "8091"  // CPU 2, Node 3
  }
};

char node_ips[8][16] = {
  "198.162.55.144", // achal01
  "198.162.55.145", // achal02
  "198.162.55.146", // achal03
  "198.162.55.147", // achal04
  "198.162.55.148", // achal05
  "198.162.55.149", // achal06
  "198.162.55.159", // achal07
  "198.162.55.160"  // achal08
};

#endif // EXP_COMMON_H
