import enum
import subprocess
import logging
import typing
import os
import datetime
import time
import threading
import argparse

from abc import abstractmethod

parser = argparse.ArgumentParser(description='Process experiment options.')
parser.add_argument('--cid', default=2, type=int,
                    help='Cluster ID (1 or 2)')
parser.add_argument('--exp', default='ivp', type=str,
                    help='Type of experiment (bosch or ivp)')
parser.add_argument('--kvs', default='ivp', type=str,
                    help='Type of KVS (bft, etcd, or redis)')
parser.add_argument('--period', default=100, type=int,
                    help='Task period (ms)')
parser.add_argument('--duration', default=60, type=int,
                    help='Experiment duration (s)')
parser.add_argument('--tasks_per_cpu', default=1, type=int,
                    help='Number of tasks per CPU')
parser.add_argument('--cpus_used', default=1, type=int,
                    help='Number of CPUs used per node')
parser.add_argument('--ivp_task_wcet_ms', default=20, type=int,
                    help='Expected WCET of the IvP task (ms)')
parser.add_argument('--kvs_task_wcet_ms', default=80, type=int,
                    help='Expected WCET of the KVS task (ms)')
parser.add_argument('--output_file', default="ivp.data", type=str,
                    help='Output file name (e.g., name.data)')
parser.add_argument('--fault_mode', default=0, type=int,
                    help='Fault mode (0=OK, 1=DEAD, 2=LIAR)')
parser.add_argument('--num_faulty', default=0, type=int,
                    help='Number of faulty nodes')
parser.add_argument('--config_file', default="kzh15_42pcu_1031l.cfg", type=str,
                    help='COnfig file for Bosch benchmark')

args = parser.parse_args()

container_mode = 'CONTAINER_MODE' in os.environ and os.environ['CONTAINER_MODE'] == "true"

class KVSType(enum.Enum):
    BFT_KVS = "BFTKVS"
    REDIS_KVS1 = "RedisKVS1"
    REDIS_KVS2 = "RedisKVS2"
    ETCD_KVS = "ETCDKVS"


class ExperimentConfig:
    main: str = None
    period_ms: int = None
    exp_duration_s: int = None
    kvs_type: KVSType = None
    output_file: str = None
    host_ids: typing.Dict[str, int] = {}

    def get_main(self) -> str:
        return "./" + self.main

    @abstractmethod
    def get_args(self, hostname: str) -> typing.List[str]:
        pass

class ExperimentConfigBosch(ExperimentConfig):
    benchmark_cfg: str = None

    def get_args(self, hostname: str) -> typing.List[str]:
        return [
            str(self.period_ms),
            self.benchmark_cfg,
            str(self.exp_duration_s),
            self.kvs_type.value,
            self.output_file,
            self.exp_type,
            str(self.host_ids[hostname]),
            str(len(self.host_ids)),
            str(self.cpus_used),
        ]

class ExperimentConfigIvP(ExperimentConfig):
    num_tasks_per_cpu: int = None
    exp_type: str = None
    cpus_used: int = None
    ivp_task_wcet_ms: float = None
    kvs_task_wcet_ms: float = None
    fault_mode: int = None
    num_faulty: int = None

    def get_args(self, hostname: str) -> typing.List[str]:
        return [
            str(self.period_ms),
            str(self.num_tasks_per_cpu),
            str(self.exp_duration_s),
            self.kvs_type.value,
            self.output_file,
            self.exp_type,
            str(self.host_ids[hostname]),
            str(len(self.host_ids)),
            str(self.cpus_used),
            str(self.ivp_task_wcet_ms),
            str(self.kvs_task_wcet_ms),
            str(self.fault_mode),
            str(self.num_faulty),
        ]

class RaspberryPiController:
    logger: logging.Logger = None
    hostname: str = None
    logging_id: str = None
    logfile: typing.TextIO = None
    logfile2: typing.TextIO = None

    def __init__(self, hostname: str, logging_id: str) -> None:
        self.hostname = hostname
        self.logging_id = logging_id
        self.__setup_logging()

    def __setup_logging(self):
        self.logfile = open(
            f"logging/{self.logging_id}/{self.hostname}.log", "w")
        self.logfile2 = open(
            f"logging/{self.logging_id}/{self.hostname}_external_kvs.log", "w")

    def __run(self, command: typing.List[str], logfile: typing.TextIO = None) -> subprocess.Popen:
        if not logfile:
            logfile = self.logfile
        logfile.write("RaspberryPiController: " + " ".join(command) + "\n")
        logfile.flush()
        return subprocess.Popen(command, stdout=logfile, stderr=logfile)

    def ssh(self, command: typing.List, logfile: typing.TextIO = None) -> subprocess.Popen:
        prefix = ["ssh", self.hostname, "--", ]
        return self.__run(prefix + command, logfile)

    def compile(self, branch: str = "main") -> subprocess.Popen:
        print(self.hostname, "compile")
        command = [
            "cd", "achal/build", "&&",
        ]
        if not container_mode:
            command = command + [
                "git", "fetch", "&&",
                "git", "checkout", branch, "&&",
                "git", "pull", "origin", branch, "&&",
            ]
        command = command + [
            "cmake", "-GNinja", "..", "&&",
            "ninja", "-j", "4"
        ]
        return self.ssh(command)

    def run_external_kvs(self, config: ExperimentConfig) -> subprocess.Popen:
        print(self.hostname, "run_external_kvs")
        if config.kvs_type == KVSType.ETCD_KVS:
            command = [
                "cd", "achal/scripts", "&&"]
            if container_mode:
                command = command + ["sudo"]
            command = command + ["bash", "etcd.sh", "start"]
            return self.ssh(command, self.logfile2)
        elif config.kvs_type == KVSType.REDIS_KVS1:
            return self.ssh([
                "cd", "achal/scripts", "&&",
                "bash", "redis.sh", "start", "1",
            ], self.logfile2)
        elif config.kvs_type == KVSType.REDIS_KVS2:
            return self.ssh([
                "cd", "achal/scripts", "&&",
                "bash", "redis.sh", "start", "2", self.hostname
            ], self.logfile2)
        else:
            return None

    def run_experiment(self, config: ExperimentConfig) -> subprocess.Popen:
        print(self.hostname, "run_experiment")
        command = ["cd", "achal/build", "&&" ]
        if container_mode:
            command = command + ["sudo"]
        command = command + [config.get_main()]
        return self.ssh(command + config.get_args(self.hostname))

    def download_output_files(self, config: ExperimentConfig):
        print(self.hostname, "download_output_files")
        self.__run([
            "scp", "-r",
            f"{self.hostname}:achal/data/{config.output_file}",
            f"logging/{self.logging_id}/{self.hostname}_{config.output_file}",
        ]).wait()
        self.__run([
            "scp", "-r",
            f"{self.hostname}:achal/data/{config.output_file}.et",
            f"logging/{self.logging_id}/{self.hostname}_{config.output_file}.et",
        ]).wait()
        return

    def cleanup_experiments(self):
        print(self.hostname, "cleanup_experiments")
        self.ssh(["sudo", "killall", "redis-server",
                 "etcd", "exp", "exp2", "exp_ivp_1", "bosch1"], self.logfile2).wait()

    def __del__(self):
        self.logfile.close()
        self.logfile2.close()


def get_local_branch():
    return subprocess.run(["git", "branch", "--show-current"], capture_output=True, text=True).stdout.strip()


def run_experiment_on_all_pis(machines: typing.List[RaspberryPiController], config: ExperimentConfig):
    print("run_experiment_on_all_pis")

    external_kvs_processes = [m.run_external_kvs(config) for m in machines]

    time.sleep(5)

    experiment_processes = [m.run_experiment(config) for m in machines]

    print("waiting ...")
    for p in experiment_processes:
        p.wait()

    print("Did any experiment fail?")
    for p in experiment_processes:
        if p.returncode != 0:
            raise Exception("Experiment failed, check logs for more detail.")

    print("Killing external KVS processes")
    for p in external_kvs_processes:
        if p is not None:
            p.kill()

    for m in machines:
        m.cleanup_experiments()


def main():
    machines = []
    branch = get_local_branch() if not container_mode else "main"
    try:
        # setup global logging config
        print("setup global logging config")
        log_id = datetime.datetime.now()
        if not os.path.exists(f"logging/{log_id}"):
            os.mkdir(f"logging/{log_id}")

        # prepare the controllers/machines
        print("prepare the controllers/machines")
        if args.cid == 1:
            hostnames = ["achal01", "achal02", "achal03", "achal04"]
        elif args.cid == 2:
            hostnames = ["achal05", "achal06", "achal07", "achal08"]
        else:
            raise Exception("Error: Please provide a valid cluster ID")

        machines = [RaspberryPiController(hn, log_id) for hn in hostnames]

        # cleanup after last experiment
        print("cleanup after last experiment")
        for m in machines:
            m.cleanup_experiments()

        # git pull, compile cpp code, assert success
        print("git pull, compile cpp code, assert success")
        compile_processes = [m.compile(branch) for m in machines]
        for p in compile_processes:
            if p.wait() != 0:
                raise Exception("Compile failed, check logs for more detail.")

        # prepare and run experiments (this part of code can be copy and pasted)
        print("prepare and run experiments")
        if args.exp == 'bosch':
            config = ExperimentConfigBosch()
        elif args.exp == "ivp":
            config = ExperimentConfigIvP()
        else:
            raise Exception("Error: Please provide a valid experiment type")

        config.period_ms = args.period
        config.exp_duration_s = args.duration
        if args.kvs == 'bft':
          config.kvs_type = KVSType.BFT_KVS
        elif args.kvs == 'redis':
          config.kvs_type = KVSType.REDIS_KVS2
        elif args.kvs == "etcd":
          config.kvs_type = KVSType.ETCD_KVS
        else:
            raise Exception("Error: Please provide a valid KVS type")
        config.output_file = args.output_file 
        config.host_ids = {}
        for host_id in range(0, len(hostnames)):
            config.host_ids[hostnames[host_id]] = int(hostnames[host_id][-1])
        
        print("main", config.main)
        print("period_ms", config.period_ms)
        print("exp_dutation_s", config.exp_duration_s)
        print("kvs_type", config.kvs_type)
        print("output_file", config.output_file)
        print("host_ids", config.host_ids)

        if args.exp == 'bosch':
            config.main = "bosch1"
            config.benchmark_cfg = args.config_file
            config.exp_type = 'distributed'
            config.cpus_used = args.cpus_used
        elif args.exp == "ivp":
            config.main = "exp_ivp_1"
            config.num_tasks_per_cpu = args.tasks_per_cpu
            config.exp_type = 'distributed'
            config.cpus_used = args.cpus_used
            config.ivp_task_wcet_ms = args.ivp_task_wcet_ms
            config.kvs_task_wcet_ms = args.kvs_task_wcet_ms
            if args.fault_mode not in [0, 1, 2]:
                raise Exception(f"Bad argument fault_mode {args.fault_mode}, should be in [0, 1, 2]")
            if args.num_faulty not in [0, 1, 2]:
                raise Exception(f"Bad argument num_faulty {args.fault_mode}, should be in [0, 1, 2]")
            config.fault_mode = args.fault_mode
            config.num_faulty = args.num_faulty

            workload = config.num_tasks_per_cpu * config.ivp_task_wcet_ms
            workload += config.kvs_task_wcet_ms
            if workload > config.period_ms:
                raise Exception("Error: Overloaded workload configuration")

        else:
            raise Exception("Error: Please provide a valid experiment type")
        
        run_experiment_on_all_pis(machines, config)

        # download the final output file given an experiment config
        for m in machines:
            m.download_output_files(config)

    except Exception as e:
        print("Exception:", e)
        for m in machines:
            m.cleanup_experiments()
    except KeyboardInterrupt:
        print("Ctrl+C detected, terminating experiments.")
        for m in machines:
            m.cleanup_experiments()


if __name__ == "__main__":
    main()
