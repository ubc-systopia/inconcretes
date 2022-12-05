# Artifact Evaluation Guide

## Summary

This repository contains the source code for our upcoming RTSS 2022 paper titled *In-ConcReTeS: Interactive Consistency meets Distributed Real-Time Systems, Again!*.

The internal codename for our system is *Achal*, which means immovable or invariable in Hindi, and stands for the predictable performance we seek to achieve in fault-tolerant distributed real-time systems.

We proivde a Docker environment to simplfy artifact evaluation. We also explain how to run the system natively on our testbed (in case reviewers have access to our testbed or similar hardware).

**Disclaimer: Since our evaluation is performance-oriented and results depend on the platform used, the results obtained using Docker may not mirror that in the paper and may also depend on the host machine configurations.**

When cloning this repository, also recursively clone the submodules!

For questions, contact Arpan Gujarati (arpanbg@cs.ubc.ca).

## Quick Start + Docker Setup
Achal is a distributed system. In our experiment setup, we deployed Achal on a cluster of 4 raspberry pis, and control it from a client machine. Note that the client machine is not part of the distributed system, but merely used to orchestrate experiments.

```
            ┌───────────────────────────────┐
            │                               │
            │                               │
            │       Client machine          │
            │                               │
            │                               │
            └──────────────┬────────────────┘
                           │Run experiments!
    ┌───────────────┬──────┴─────────┬───────────────┐
    │               │                │               │
    ▼               ▼                ▼               ▼
┌──────┐         ┌──────┐        ┌──────┐        ┌──────┐
│      │         │      │        │      │        │      │
│ Pi 1 │ ◄─────► │ Pi 2 │ ◄────► │ Pi 3 │ ◄─────►│ Pi 4 │
│      │         │      │        │      │        │      │
└──────┘         └──────┘        └──────┘        └──────┘
```

To make it convenient for new users and artifact evaluators, we have provided a `docker-compose.yml` file which simulates the aforementioned 4 Raspberry Pis with 4 containers, and also deploys a `client` container to control these Pis, as we did while we evaluated the performance of the prototype.

Our code and scripts expect the Raspberry Pis to have fixed hostnames and DNS (`achal0*`), and fixed IPs (`198.162.55.14*`). (Yeah, it is bad coding practice...) In addition, they assume that the client machine can execute command `ssh achal0*` to connect to any of the Pis. These configurations are all set in `docker-compose.yml`, and you do not need to worry about them.

To use `docker-compose.yml`, make sure you have 
`docker` and `docker-compose` installed, and that you can use docker without `sudo`. We recommend that you use `Ubuntu` and your are using an `x86` machine with a CPU that has at least 4 cores, preferably 8. Then, run

```
docker-compose up --build
```

This will bring up 5 containers as described above. Note that the first run of the command will be slow, whereas subsequent commands will be much faster.

1. `docker-compose` will use `Dockerfile` to build environments for the Raspberry Pi containers, and `Dockerfile.client` for the `client` container.
2. `docker-compose` will set the DNS, hostnames, and IPs as configured and expected by our code and scripts.
3. The `pi` containers will launch and start their `openssh-servers` listening for `ssh` connections. The `client` container will attempt to connect to each of the `pi` containers once, to test the connectivity.
4. After you start `docker-compose` you can look straight into your `logging/` directory for the execution logs, as by default, `docker-compose` will launch experiment `A2` in the paper (see the relevant section below for details).
5. Wait for the `client` container to establish its first connetions with the `pi` containers and finish experiment `A2`. When it is done, you should find `et_profile.pdf` under `paper/a2`. Note that the `logging/` and `paper/` is shared between your computer (the host of the docker engine) and the `client` container. So, there is no need to explicitly copy files out of the container.
6. Now, to run other experiments, you can modify `docker-compose.yml`, quit the running `docker-compose` program, and re-run `docker-compose up --build`. So, for instance, change this:
```
...
CONTAINER_MODE=true bash run_ivp_2.sh;
        bash plot_ivp_2.sh;
...
```
to this
```
...
CONTAINER_MODE=true bash run_ivp_1.sh;
        bash plot_ivp_1.sh;
...
```
or this
```
...
CONTAINER_MODE=true bash run_bosch_1.sh;
        bash plot_bosch_1.sh;
...
```

## Running Natively on Raspberry Pis

Like before, we will assume

* a client workstation and four target nodes `achal01`-`achal04`
* that target nodes can be accessed from `client` via SSH without passwords
* and that this repository is cloned at `path/to/achal`

We also assume for now that `redis`, `etcd`, and PTP are installed (see paper for version details)

* TODO: Add details about installing `redis`, `etcd`, and PTP on a vanilla system

### PTP and VLAN config

* Run `ifconfig`, expected output:

```
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 198.162.55.144  netmask 255.255.254.0  broadcast 198.162.55.255
        inet6 fe80::3a6:5deb:2c59:cd7f  prefixlen 64  scopeid 0x20<link>
        ether dc:a6:32:cd:ce:77  txqueuelen 1000  (Ethernet)
        RX packets 130046388  bytes 273546776 (260.8 MiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 131685298  bytes 406542628 (387.7 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1  netmask 255.0.0.0
        inet6 ::1  prefixlen 128  scopeid 0x10<host>
        loop  txqueuelen 1000  (Local Loopback)
        RX packets 76902811  bytes 8946060175 (8.3 GiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 76902811  bytes 8946060175 (8.3 GiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

* Run the following commands:

  * `sudo ip link add link eth0 name eth0.11 type vlan id 1 egress 0:2 1:2 2:2 3:2 4:2 5:2 6:2 7:2`
  * `sudo ip link add link eth0 name eth0.12 type vlan id 2 egress 0:4 1:4 2:4 3:4 4:4 5:4 6:4 7:4`

* Again run `ifconfig`, expected output:

```
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 198.162.55.144  netmask 255.255.254.0  broadcast 198.162.55.255
        inet6 fe80::3a6:5deb:2c59:cd7f  prefixlen 64  scopeid 0x20<link>
        ether dc:a6:32:cd:ce:77  txqueuelen 1000  (Ethernet)
        RX packets 130046461  bytes 273556620 (260.8 MiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 131685371  bytes 406551468 (387.7 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

eth0.11: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 fe80::6584:daac:a387:87c4  prefixlen 64  scopeid 0x20<link>
        ether dc:a6:32:cd:ce:77  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 4  bytes 686 (686.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

eth0.12: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 fe80::824d:a435:2f9b:ca3a  prefixlen 64  scopeid 0x20<link>
        ether dc:a6:32:cd:ce:77  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 7  bytes 976 (976.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1  netmask 255.0.0.0
        inet6 ::1  prefixlen 128  scopeid 0x10<host>
        loop  txqueuelen 1000  (Local Loopback)
        RX packets 76902811  bytes 8946060175 (8.3 GiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 76902811  bytes 8946060175 (8.3 GiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

* Check the PTP status using `systemctl status ptp4l` and find the path of the ptp4l service
* For example, the output will contain a line `Loaded: loaded ( ; enabled; vendor preset: enabled)`
* Open file using ` sudo vim /lib/systemd/system/ptp4l.service` and check the ethernet interface used by PTP
* By default, it is `eth0`, replace it with `eth0.11`
* Run the following `systemctl` commands:

  * `sudo systemctl daemon-reload`
  * `sudo systemctl restart ptp4l`

## Experiment Details

This applies mainly to native runs. Comments regarding Docker runs are added at the end of each experiment description.

### Experiment A1

This relates to Figure 3 in the paper.

* First, go to the project working directory `cd path/to/achal`
* Empty the logging directory `rm -rf logging/*`
* From the workstation, execute the bash script `bash run_ivp_1.sh`

  * This initiates the python script `run.py` for 20 different configurations (sequentially)
  * The experiment may take about 8 to 9 hours in total
  * All logs are stored inside the `logging` directory
  * For each run, a new folder with a unique timestamp `2022-09-08 04:22:01.107544` is created
  * This folder contains the logs and data files for that run collected from each node
  * We care about the latest folder since the data files contain cumulative history

* Afterwards, execute the plotting script `bash plot_ivp_1.sh`
  
  * This picks the latest folder folder inside `logging`, say, `2022-09-08 04:22:01.107544`
  * All data files of the form `achal03_20220907192353.data` are then moved to the plotting directory `/paper/a1`
  * The plotting script `plot_a1.py` is then executed
  * Resulting plots `s.pdf` and `et.pdf` are stored in `paper/a1` as well

* The plots are intepreted as follows.

  * `s.pdf` corresponds to Fig 3(a) in the paper that measures the number of successful iterations for each configuration
  * `et.pdf` corresponds to Fig 3(b) in the paper that measures the execution times for each configuration

* Shortening the experiment

  * We currently do not support producing a graph for a shorter version of the experiment.
  * However, feel free to simplify the configuration space in `run_ivp_1.sh` (i.e., the big nested `for` loop inside)
  * You continue with the above steps now, but the plotting script may fail
  * However, the plotting script copies the relevant node-specific raw data to the `/paper/a1`, which you can peruse for a quick overview of results

* Docker
  * As mentioned above, the `docker-compose.yml` file can be modified to run the `run_ivp_1.sh` and `plot_ivp_1.sh` scripts instead, and the results can be found in the `/paper/a1` folder


### Experiment A2

This relates to Figure 4 in the paper.

* Again, go to the project working directory `cd path/to/achal`
* Empty the logging directory `rm -rf logging/*`
* From the workstation, execute the bash script `bash run_ivp_2.sh`

  * This initiates the python script `run.py` for 6 different configurations (sequentially)
  * The experiment may take about 1 hour in total
  * All logs are stored inside the `logging` directory
  * For each run, a new folder with a unique timestamp `2022-09-08 04:22:01.107544` is created
  * This folder contains the logs and data files for that run collected from each node
  * We care about the latest folder since the data files contain cumulative history

* Afterwards, execute the plotting script `bash plot_ivp_2.sh`
  
  * This picks the latest folder folder inside `logging`, say, `2022-09-08 04:22:01.107544`
  * All data files containing the execution times of the form `achal03_20220907192353.data.et` are then moved to the plotting directory `/paper/a2`
  * The plotting script `plot_a2.py` is then executed
  * Resulting plots `et_profile.pdf` is stored in `paper/a2` as well

* The plot is intepreted as follows.

  * `et_profile.pdf` corresponds to Fig 4 in the paper that measures the execution times for different faulty scenarios

* Docker
  * As mentioned above, the `docker-compose.yml` file can be modified to run the `run_ivp_2.sh` and `plot_ivp_2.sh` scripts instead, and the results can be found in the `/paper/a2` folder


### Experiment B1

This relates to Table I in the paper.

* First, go to the project working directory `cd path/to/achal`
* Empty the logging directory `rm -rf logging/*`
* From the workstation, execute the bash script `bash run_bosch_1.sh`

  * This initiates the python script `run.py` for 8 different configurations (sequentially)
  * The experiment may take about 2 hours in total
  * All logs are stored inside the `logging` directory
  * For each run, a new folder with a unique timestamp `2022-09-08 04:22:01.107544` is created
  * This folder contains the logs and data files for that run collected from each node
  * We care about the latest folder since the data files contain cumulative history

* Afterwards, execute the plotting script `bash plot_bosch_1.sh`
  
  * This picks the latest folder folder inside `logging`, say, `2022-09-08 04:22:01.107544`
  * All data files of the form `achal03_20220907192353.data` are then moved to the plotting directory `/paper/b1`
  * The plotting script `plot_b1.py` is then executed
  * The script does not plot anything but output a file `table.txt` containing a table that we show in the paper as Table I

* Shortening the experiment

  * In the `run_bosch_1.sh` file, you can remove some of the entries from the `config_files0` array to shorten the runtime

* Docker
  * As mentioned above, the `docker-compose.yml` file can be modified to run the `run_bosch_1.sh` and `plot_bosch_1.sh` scripts instead, and the results can be found in the `/paper/b1` folder


### Experiments B2 and B3

These relate to Table II (upper and lower halves) in the paper (respectively).

* These steps are identical to the previous experiment B1, only the workload configurations used in each `run_bosch_*.sh` script are different
* The plotting scripts are actually identical, we have separated them just for naming convenience
* Hence, follow the same steps as B1, but replace `b1` with `b2` or `b3` everywhere
* B2 generates data for the first 9 rows of table II in the paper and B3 for the remaining rows

<!---
## Compilation

The build process is managed using [CMake](https://cmake.org/cmake/help/latest/guide/tutorial/) (currently, version 3.16.3).

## Organization

### config

Currently, there is only one `default.cfg` configuration file. It summarizes:

1. all distributed nodes in the system, including their respective hostnames, IP addresses, and ports, which are used for networking, and 
2. all applications deployed in the system, including their respective names, periods, and replication factors.

Achal relies on [libconfig](https://hyperrealm.github.io/libconfig/) for parsing all configuration files. The parsing logic is implemented in files `src/achal/config.h` and `src/achal/config.cpp`.

### src/utils

### test

For unit-testing, we use the stable version of [Catch2 (v2.x)](https://github.com/catchorg/Catch2/tree/v2.x). Specifically, the automatically generated single header file that we use in this regard corresponds to version 2.13.0 (see `tests/catch.hpp` for more details).

The tests take a long time to run. In order to see progress, run `./tests -s` to see the outputof successful tests as well.

## Workloads

Inverted pendulum simulation based on Gonçalo Morgado's implementation:
* Author webpage: http://gmagno.users.sourceforge.net/InvertedPendulum.htm
* Original source: https://sourceforge.net/projects/ivpendulum/

https://www.ecrts.org/forum/viewtopic.php?f=32&t=85

## Styleguide

We try to follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

In order to use a tool like [Artistic Style](http://astyle.sourceforge.net) for auto-formatting:
```
./astyle --style=google --indent=spaces=2 --suffix=none --max-code-length=80 --recursive "$HOME/achal/src/*.cpp" "$HOME/achal/src/*.h" "$HOME/achal/test/*.cpp" "$HOME/achal/test/*.h"
```
-->
