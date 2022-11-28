#ifndef UTILS_KZH15_BENCHMARK_GENERATOR_H
#define UTILS_KZH15_BENCHMARK_GENERATOR_H

#include <cstdint>
#include <cassert>

#include <libconfig.h++>

#include <map>
#include <vector>

#include "logging.h"

namespace Utils {
namespace KZH15 {

const double kMaxUtilPerCPU = 0.5;
//const double kKVSAvgReadTimeUs = 0.5;
//const double kKVSAvgWriteTimeUs = 0.5;

enum class LabelType {kReadOnly = 0, kWriteOnly, kReadWrite};
enum class MessageType {kIntraTaskFwd = 0, kIntraTaskBwd, kInterTask};
enum class MessageVolumeType {kNone = 0, kI, kII, kIII, kIV, kV, kVI};

std::ostream& operator<< (std::ostream& stream, const LabelType& type);
std::ostream& operator<< (std::ostream& stream, const MessageType& mtype);
std::ostream& operator<< (std::ostream& stream, const MessageVolumeType& type);

template <class T>
class Range {
 public:
  T min;
  T max;

  Range() : min(0), max(0) {}
  Range(T min, T max) : min(min), max(max) {}

  friend bool operator<(const Range& l, const Range& r) {
    return l.min < r.min or (l.min == r.min and l.max < r.max);
  }
};

class ExecTimeProfile {
 public:
  double acet_min_us;
  double acet_avg_us;
  double acet_max_us;
  double bcet_f_min;
  double bcet_f_max;
  double wcet_f_min;
  double wcet_f_max;

  ExecTimeProfile()
    : acet_min_us(0), acet_avg_us(0), acet_max_us(0), bcet_f_min(1),
      bcet_f_max(1), wcet_f_min(1), wcet_f_max(1) {}

  ExecTimeProfile(double acet_min_us, double acet_avg_us, double acet_max_us,
                  double bcet_f_min, double bcet_f_max, double wcet_f_min,
                  double wcet_f_max)
    : acet_min_us(acet_min_us), acet_avg_us(acet_avg_us),
      acet_max_us(acet_max_us), bcet_f_min(bcet_f_min),
      bcet_f_max(bcet_f_max), wcet_f_min(wcet_f_min), wcet_f_max(wcet_f_max) {
  }

  operator std::string();
};

class RunnablesConfig {
 public:
  Range<int> count;
  std::map<int, double> period_shares;
  std::map<int, ExecTimeProfile> exec_time_profiles;

  RunnablesConfig() {}
  void parse_config_file(const libconfig::Setting& root);
  operator std::string();
};

class LabelsConfig {
 public:
  Range<int> count;
  std::map<Range<int>, double> size_shares;
  std::map<LabelType, double> type_partition;

  LabelsConfig() {}
  void parse_config_file(const libconfig::Setting& root);
  operator std::string();
};

class InterTaskMessagesConfig {
 public:
  std::map<MessageVolumeType, Range<int>> volume_types;
  std::map<int, std::map<int, MessageVolumeType>> dist;

  InterTaskMessagesConfig() {}
  void parse_config_file(const libconfig::Setting& root);
  operator std::string();
};

class MessagesConfig {
 public:
  std::map<MessageType, double> type_partition;
  InterTaskMessagesConfig inter_task_msgs_cfg;

  MessagesConfig() {}
  void parse_config_file(const libconfig::Setting& root);
  operator std::string();
};

class Config {
 public:
  std::string config_file;
  double scale_factor;
  std::vector<int> periods_ms;
  LabelsConfig labels_cfg;
  MessagesConfig msgs_cfg;
  RunnablesConfig runnables_cfg;

  Config(double scale_factor = 1.0);
  Config(std::string config_file, double scale_factor = 1.0);

  void parse_config_file();
  operator std::string();
};

class Label;

class Runnable {
 public:
  int id;
  int period_ms;
  double acet_us;
  double bcet_us;
  double wcet_us;
  std::vector<Label*> ro_labels_read;
  std::vector<Label*> wo_labels_written;
  std::vector<Label*> rw_intra_task_labels_read;
  std::vector<Label*> rw_intra_task_labels_written;
  std::vector<Label*> rw_inter_task_labels_read;
  std::vector<Label*> rw_inter_task_labels_written;

  Runnable(int id, int period_ms)
    : id(id), period_ms(period_ms), acet_us(0), bcet_us(0), wcet_us(0) {}
  Runnable(int id, int period_ms, double acet_us, double bcet_us,
           double wcet_us)
    : id(id), period_ms(period_ms), acet_us(acet_us), bcet_us(bcet_us),
      wcet_us(wcet_us) {}

  static Range<int> get_angle_sync_period_range();
  static int get_min_period_ms(int period_ms);
  operator std::string();
};

class Task {
 public:
  int id;
  int period_ms;
  double acet_us;
  double bcet_us;
  double wcet_us;
  //double kvs_read_us;
  //double kvs_write_us;
  std::vector<Runnable*> runnables;

  double get_utilization();

 public:
  Task(int id, int period_ms)
    : id(id), period_ms(period_ms), acet_us(0), bcet_us(0), wcet_us(0) {}

  bool does_runnable_fit(Runnable* r);
  void add_runnable(Runnable* r);
  operator std::string();
};

class Label {
 public:
  int id;
  int size_bytes;
  LabelType type;
  MessageType mtype;
  MessageVolumeType mvtype;
  std::vector<Runnable*> readers;
  std::vector<Runnable*> writers;

  Label(int id, int size_bytes, LabelType type, MessageType mtype)
    : id(id), size_bytes(size_bytes), type(type), mtype(mtype),
      mvtype(MessageVolumeType::kNone) {}
  //Label(int id, int size_bytes, LabelType type, std::vector<Runnable*> readers,
  //  std::vector<Runnable*> writers)
  //  : id(id), size_bytes(size_bytes), type(type), readers(readers),
  //    writers(writers) {}

  operator std::string();
};

class BenchmarkInstance {
 public:
  Config& config;
  bool merge_runnables;
  std::vector<Runnable*> runnables;
  std::vector<Task*> tasks;
  std::vector<Label*> labels;
  log4cpp::Category* logger;

  int estimate_total_number_of_runnables();
  void generate_runnable_instances(int max);
  void generate_runnable_acets_bcets_and_wcets();

  void generate_task_instances_from_runnables();

  int estimate_total_number_of_labels();
  void generate_label_instances(int max);
  void map_labels_to_runnables();
  void map_inter_task_msgs_to_runnables();

  unsigned int get_num_tasks(int period_ms);
  double get_avg_runnables_per_task(int period_ms);
  double get_avg_utilization_per_task(int period_ms);

  Range<double> get_bcet_ms_range(int period_ms);
  Range<double> get_acet_ms_range(int period_ms);
  Range<double> get_wcet_ms_range(int period_ms);

  std::map<int, int> get_label_sizes();
  std::map<int, unsigned int> get_label_size_dist();
  std::map<LabelType, unsigned int> get_label_type_dist();
  std::map<MessageType, unsigned int> get_msg_type_dist();

  unsigned int get_avg_ro_labels_read_per_task();
  unsigned int get_avg_rw_labels_read_per_task();
  unsigned int get_avg_wo_labels_written_per_task();
  unsigned int get_avg_rw_labels_written_per_task();

 public:
  //BenchmarkInstance(std::string filename, log4cpp::Category* logger);
  BenchmarkInstance(Config& config, log4cpp::Category* logger, bool
                    merge_runnables = true);

  std::vector<Runnable*>& get_runnables() {
    return runnables;
  }

  std::vector<Task*>& get_tasks() {
    return tasks;
  }

  std::vector<Label*>& get_labels() {
    return labels;
  }

  double get_total_utilization();
  void print_summary();

  std::string export_to_file();
};

class RunnableBasic {
 public:
  int id;
  int period_ms;
  double acet_us;
  double bcet_us;
  double wcet_us;
  std::vector<int> ro_labels_read;
  std::vector<int> wo_labels_written;
  std::vector<int> rw_intra_task_labels_read;
  std::vector<int> rw_intra_task_labels_written;
  std::vector<int> rw_inter_task_labels_read;
  std::vector<int> rw_inter_task_labels_written;
  
  RunnableBasic(int id, int period_ms, double acet_us, double bcet_us,
                double wcet_us)
    : id(id), period_ms(period_ms), acet_us(acet_us), bcet_us(bcet_us),
      wcet_us(wcet_us) {}
  operator std::string();
};

class TaskBasic {
 public:
  int id;
  int period_ms;
  double acet_us;
  double bcet_us;
  double wcet_us;
  std::vector<RunnableBasic*> runnables;

 public:
  TaskBasic(int id, int period_ms, double acet_us, double bcet_us,
            double wcet_us)
    : id(id), period_ms(period_ms), acet_us(acet_us), bcet_us(bcet_us),
      wcet_us(wcet_us) {}
  operator std::string();
};

class BenchmarkInstanceBasic {
 public:
  std::vector<TaskBasic*> tasks;
  std::vector<RunnableBasic*> runnables;
  std::map<int, int> label_sizes;
  log4cpp::Category* logger;

 public:
  BenchmarkInstanceBasic(std::string config_file, log4cpp::Category* logger);

  double get_total_utilization();
  double get_avg_label_size();
  unsigned get_total_labels();
  unsigned get_total_tasks();

  void print_summary();
};

void resolve_label_dependencies(Utils::KZH15::BenchmarkInstanceBasic& instance,
                                uint64_t smallest_period_ms);
void trim_benchmark(Utils::KZH15::BenchmarkInstanceBasic& instance,
                    uint64_t smallest_period_ms);
void shrink_label_sizes(Utils::KZH15::BenchmarkInstanceBasic& instance,
                        unsigned max_label_size);

} // namespace KZH15
} // namespace Utils

#endif  // UTILS_KZH15_BENCHMARK_GENERATOR_H
