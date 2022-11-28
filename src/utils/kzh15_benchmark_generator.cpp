#include "kzh15_benchmark_generator.h"

#include <iostream>
#include <set>
#include <chrono>

#include <boost/math/distributions/beta.hpp>

#include "utils/macros.h"
#include "utils/misc.h"

namespace Utils {
namespace KZH15 {

std::ostream& operator<< (std::ostream& stream, const LabelType& type) {
  if (type == LabelType::kReadOnly) {
    stream << "R-";
  } else if (type == LabelType::kWriteOnly) {
    stream << "-W";
  } else if (type == LabelType::kReadWrite) {
    stream << "RW";
  } else {
    stream << "?";
  }
  return stream;
}

std::ostream& operator<< (std::ostream& stream, const MessageType& mtype) {
  if (mtype == MessageType::kIntraTaskFwd) {
    stream << "IntraTaskFwd";
  } else if (mtype == MessageType::kIntraTaskBwd) {
    stream << "IntraTaskBwd";
  } else if (mtype == MessageType::kInterTask) {
    stream << "InterTask";
  } else {
    stream << "?";
  }
  return stream;
}

std::ostream& operator<< (std::ostream& stream, const MessageVolumeType& type) {
  if (type == MessageVolumeType::kNone) {
    stream << "-";
  } else if (type == MessageVolumeType::kI) {
    stream << "I";
  } else if (type == MessageVolumeType::kII) {
    stream << "II";
  } else if (type == MessageVolumeType::kIII) {
    stream << "III";
  } else if (type == MessageVolumeType::kIV) {
    stream << "IV";
  } else if (type == MessageVolumeType::kV) {
    stream << "V";
  } else if (type == MessageVolumeType::kVI) {
    stream << "VI";
  } else {
    stream << "?";
  }
  return stream;
}

ExecTimeProfile::operator std::string() {
  std::stringstream ss;
  ss << "ExecTimeProfile: ACET tuple = (" << acet_min_us << ", "
     << acet_avg_us << ", " << acet_max_us << "), BCET factors = ("
     << bcet_f_min << ", " << bcet_f_max << "), WCET factors = ("
     << wcet_f_min << ", " << wcet_f_max << ")";
  return ss.str();
}

void RunnablesConfig::parse_config_file(const libconfig::Setting& root) {
  const libconfig::Setting& cfg = root.lookup("KZH15Config.RunnablesConfig");

  count.min = cfg.lookup("count.min");
  count.max = cfg.lookup("count.max");

  const libconfig::Setting& s = cfg.lookup("period_shares");
  for (int i = 0; i < s.getLength(); i++) {
    period_shares[s[i]["period_ms"]] = s[i]["share_pc"];
  }

  const libconfig::Setting& p = cfg.lookup("exec_time_profiles");
  for (int i = 0; i < p.getLength(); i++) {
    ExecTimeProfile temp(p[i]["acet_min_us"], p[i]["acet_avg_us"],
                         p[i]["acet_max_us"], p[i]["bcet_f_min"], p[i]["bcet_f_max"],
                         p[i]["wcet_f_min"], p[i]["wcet_f_max"]);
    exec_time_profiles[p[i]["period_ms"]] = temp;
  }
}

RunnablesConfig::operator std::string() {
  std::stringstream ss;
  ss << "Section IV.C." << std::endl;
  ss << "Table III: Runnable Distribution Among Periods" << std::endl;
  ss << "Period, Share" << std::endl;
  for (auto it = period_shares.begin(); it != period_shares.end(); it++) {
    if (it->first == 0) {
      continue;
    }
    ss << it->first << " ms, " << (int) it->second << " %" << std::endl;
  }
  ss << "angle-synchronous, " << (int) period_shares[0] << " %" << std::endl;
  ss << std::endl;

  ss << "Section IV.D." << std::endl;
  ss << "Table IV: Runnable Average Execurion Times" << std::endl;
  ss << "Period, Average Execution Times in us" << std::endl;
  ss << "Period, Min., Avg., Max." << std::endl;
  for (auto it = exec_time_profiles.begin(); it != exec_time_profiles.end();
       it++) {
    if (it->first == 0) {
      continue;
    }
    ss << it->first << " ms, " << it->second.acet_min_us << ", "
       << it->second.acet_avg_us << ", " << it->second.acet_max_us;
  }
  ss << "angle-synchronous, " << exec_time_profiles[0].acet_min_us << ", "
     << exec_time_profiles[0].acet_avg_us << ", "
     << exec_time_profiles[0].acet_max_us << std::endl;
  ss << std::endl;

  ss << "Section IV.D." << std::endl;
  ss << "Table V: Factors For Determining Runnable Best- And Worst- "
     << "Case Execution Times" << std::endl;
  ss << "Period, Best, Worst" << std::endl;
  ss << "Period, f_min, f_max, f_min, f_max" << std::endl;
  for (auto it = exec_time_profiles.begin(); it != exec_time_profiles.end();
       it++) {
    if (it->first == 0) {
      continue;
    }
    ss << it->first << " ms, " << it->second.bcet_f_min << ", "
       << it->second.bcet_f_max << ", " << it->second.wcet_f_min << ", "
       << it->second.wcet_f_max;
  }
  ss << "angle-synchronous, " << exec_time_profiles[0].bcet_f_min << ", "
     << exec_time_profiles[0].bcet_f_max << ", "
     << exec_time_profiles[0].wcet_f_min << ", "
     << exec_time_profiles[0].wcet_f_max << std::endl;
  ss << std::endl;

  return ss.str();
}

void LabelsConfig::parse_config_file(const libconfig::Setting& root) {
  const libconfig::Setting& cfg = root.lookup("KZH15Config.LabelsConfig");

  count.min = cfg.lookup("count.min");
  count.max = cfg.lookup("count.max");

  const libconfig::Setting& s = cfg.lookup("size_shares");
  for (int i = 0; i < s.getLength(); i++) {
    size_shares[Range<int>(s[i]["size_bytes_min"], s[i]["size_bytes_max"])] =
      s[i]["share_pc"];
  }

  const libconfig::Setting& p = cfg.lookup("type_partition");
  type_partition[LabelType::kReadOnly] = p.lookup("read_only");
  type_partition[LabelType::kWriteOnly] = p.lookup("write_only");
  type_partition[LabelType::kReadWrite] = p.lookup("read_write");
}

LabelsConfig::operator std::string() {
  std::stringstream ss;
  ss << "Section IV.A." << std::endl;
  ss << "Table I: Distribution of Label Sizes" << std::endl;
  ss << "Size (bytes), Share" << std::endl;
  for (auto it = size_shares.begin(); it != size_shares.end(); it++) {
    if (it->first.min == it->first.max) {
      ss << it->first.min << ", " << it->second << " %" << std::endl;
    } else {
      ss << it->first.min << " - " << it->first.max << ", " << it->second
         << " %" << std::endl;
    }
  }
  ss;

  ss << "Section IV.A." << std::endl;
  ss << "The partitioning of all accesses is as follows:" << std::endl;
  ss << "* Read-only : " << type_partition[LabelType::kReadOnly] << "%"
     << std::endl;
  ss << "* Write-only : " << type_partition[LabelType::kWriteOnly] << "%"
     << std::endl;
  ss << "* Read-Write : " << type_partition[LabelType::kReadWrite] << "%"
     << std::endl;
  ss << std::endl;
  return ss.str();
}

void InterTaskMessagesConfig::parse_config_file(const libconfig::Setting&
    root) {
  const libconfig::Setting& cfg =
    root.lookup("KZH15Config.MessagesConfig.InterTaskMessagesConfig");

  const libconfig::Setting& t = cfg.lookup("volume_types");
  for (int i = 0; i < t.getLength(); i++) {
    volume_types[static_cast<MessageVolumeType>((int)t[i]["type"])] =
      Range<int>(t[i]["min"], t[i]["max"]);
  }

  const libconfig::Setting& d = cfg.lookup("dist");
  for (int i = 0; i < d.getLength(); i++) {
    if (dist.find(d[i]["send_period_ms"]) == dist.end()) {
      std::map<int, MessageVolumeType> empty_map;
      dist[d[i]["send_period_ms"]] = empty_map;
    }
    dist[d[i]["send_period_ms"]][d[i]["recv_period_ms"]] =
      static_cast<MessageVolumeType>((int) d[i]["volume_type"]);
  }
}

InterTaskMessagesConfig::operator std::string() {
  std::stringstream ss;
  ss << "Section IV.B." << std::endl;
  ss << "The color codes the amount of communications with the following"
     << " legend:" << std::endl;
  ss << "I, II, III, IV, V, VI" << std::endl;
  ss << volume_types[MessageVolumeType::kI].min << "-"
     << volume_types[MessageVolumeType::kI].max << ", "
     << volume_types[MessageVolumeType::kII].min << "-"
     << volume_types[MessageVolumeType::kII].max << ", "
     << volume_types[MessageVolumeType::kIII].min << "-"
     << volume_types[MessageVolumeType::kIII].max << ", "
     << volume_types[MessageVolumeType::kIV].min << "-"
     << volume_types[MessageVolumeType::kIV].max << ", "
     << volume_types[MessageVolumeType::kV].min << "-"
     << volume_types[MessageVolumeType::kV].max << ", "
     << volume_types[MessageVolumeType::kVI].min << "-"
     << volume_types[MessageVolumeType::kVI].max << std::endl;
  ss << std::endl;

  ss << "Section IV.B." << std::endl;
  ss << "Table II: Inter-Task Communication" << std::endl;
  ss << "Period" << std::endl;
  for (auto it = dist.begin(); it != dist.end(); it++) {
    if (it->first == 0) {
      continue;
    }
    ss << ", " << it->first << " ms";
  }
  ss << ", Angle-sync";
  for (auto it = dist.begin(); it != dist.end(); it++) {
    if (it->first == 0) {
      continue;
    }
    ss << it->first << " ms";
    std::map<int, MessageVolumeType>& dist_nested = it->second;
    for (auto it2 = dist_nested.begin(); it2 != dist_nested.end(); it2++) {
      if (it2->first == 0) {
        continue;
      }
      ss << ", " << it2->second;
    }
    ss << ", " << dist_nested[0];
  }
  ss << "Angle-sync";
  std::map<int, MessageVolumeType>& dist_nested = dist[0];
  for (auto it2 = dist_nested.begin(); it2 != dist_nested.end(); it2++) {
    if (it2->first == 0) {
      continue;
    }
    ss << ", " << it2->second;
  }
  ss << ", " << dist_nested[0];
  ss << std::endl;
  return ss.str();
}

void MessagesConfig::parse_config_file(const libconfig::Setting& root) {
  const libconfig::Setting& cfg = root.lookup("KZH15Config.MessagesConfig");

  const libconfig::Setting& p = cfg.lookup("type_partition");
  type_partition[MessageType::kIntraTaskFwd] = p.lookup("intra_task_fwd");
  type_partition[MessageType::kIntraTaskBwd] = p.lookup("intra_task_bwd");
  type_partition[MessageType::kInterTask] = p.lookup("inter_task");

  inter_task_msgs_cfg.parse_config_file(root);
}

MessagesConfig::operator std::string() {
  std::stringstream ss;
  ss << "Section IV.B." << std::endl;
  ss << "Forward and backward intra-task communication occurs in"
     << std::endl << type_partition[MessageType::kIntraTaskFwd] << "% and "
     << type_partition[MessageType::kIntraTaskBwd] << "% of all cases, "
     << "respectively. Inter-task" << std::endl << "communication is the most "
     << "frequent type with a share around" << std::endl
     << type_partition[MessageType::kInterTask] << "%." << std::endl;
  ss << std::endl;
  ss << std::string(inter_task_msgs_cfg);
  return ss.str();
}

Config::Config(double scale_factor)
  : scale_factor(scale_factor),
    config_file(get_project_directory() + "/config/benchmarks/kzh15.cfg") {
  parse_config_file();
}


Config::Config(std::string config_file, double scale_factor)
  : config_file(config_file), scale_factor(scale_factor) {
  parse_config_file();
}

void Config::parse_config_file() {
  libconfig::Config cfg;

  try {
    cfg.readFile(config_file.c_str());
  } catch(const libconfig::FileIOException &fioex) {
    std::cerr << "I/O error while reading file " << config_file;
  } catch(const libconfig::ParseException &pex) {
    std::cerr << "Parse error in " << config_file << " at " << pex.getFile()
              << ":" << pex.getLine() << " - " << pex.getError();
  }

  const libconfig::Setting& root = cfg.getRoot();

  const libconfig::Setting& s = root["KZH15Config"]["periods_ms"];
  for (int i = 0; i < s.getLength(); i++) {
    periods_ms.push_back(s[i]);
  }

  labels_cfg.parse_config_file(root);
  msgs_cfg.parse_config_file(root);
  runnables_cfg.parse_config_file(root);
}

Config::operator std::string() {
  std::stringstream ss;
  ss << std::string(labels_cfg);
  ss << std::string(msgs_cfg);
  ss << std::string(runnables_cfg);
  return ss.str();
}

Runnable::operator std::string() {
  std::stringstream ss;
  ss << "RID = " << id << ", P = " << period_ms << "ms, " << "ETs = ("
     << bcet_us << ", " << acet_us << ", " << wcet_us << ")" << std::endl;;
  return ss.str();
}

Range<int> Runnable::get_angle_sync_period_range() {
  // According to this source:
  // https://www.arnoldclark.com/newsroom/2331-everything-you-need-to-know-about-engine-cylinders
  // there are 4, 6, or 8 cylnders in a typical car.
  int min_num_cylinders = 4;
  int max_num_cylinders = 8;

  // According to Wikipedia (https://en.wikipedia.org/wiki/Revolutions_per_minute),
  // modern automobile engines are typically operated around 2,000–3,000 RPM
  // when cruising, with a minimum (idle) speed around 750–900 RPM, and an upper
  // limit anywhere from 4500 to 10,000 RPM for a road car.
  int min_rpm = 750;
  int max_rpm = 10000;
  int typical_min_rpm = 2000;
  int typical_max_rpm = 3000;

  int max_period_ms = 120 * 1000 / (typical_min_rpm * min_num_cylinders);
  int min_period_ms = 120 * 1000 / (typical_max_rpm * max_num_cylinders);
  return Range<int>(min_period_ms, max_period_ms);
}

int Runnable::get_min_period_ms(int period_ms) {
  if (period_ms > 0) {
    return period_ms;
  }
  Range<int> range = Runnable::get_angle_sync_period_range();
  return range.min;
}

double Task::get_utilization() {
  //return US_TO_MS(wcet_us + kvs_read_us + kvs_write_us) /
  //       (double) Runnable::get_min_period_ms();
  return US_TO_MS(wcet_us) / (double) Runnable::get_min_period_ms(period_ms);
}

// TODO Need to sort out this function, a bit hacky right now
bool Task::does_runnable_fit(Runnable* r) {
  if (runnables.empty()) {
    return true;
  } else {
    //double new_wcet_us = wcet_us + r->wcet_us;
    //double new_kvs_access_time_us = kvs_read_us + kvs_write_us +
    //                                (r->labels_read.size() * kKVSAvgReadTimeUs) +
    //                                (r->labels_written.size() * kKVSAvgWriteTimeUs);
    //return US_TO_MS(new_wcet_us + new_kvs_access_time_us)
    //       <= (kMaxUtilPerCPU * Runnable::get_min_period_ms());
    return US_TO_MS(wcet_us + r->wcet_us)
           <= (kMaxUtilPerCPU * Runnable::get_min_period_ms(period_ms));
  }
}

void Task::add_runnable(Runnable* r) {
  runnables.push_back(r);
  acet_us += r->acet_us;
  bcet_us += r->bcet_us;
  wcet_us += r->wcet_us;
  //kvs_read_us += r->labels_read.size() * kKVSAvgReadTimeUs;
  //kvs_write_us += r->labels_written.size() * kKVSAvgWriteTimeUs;
}

Task::operator std::string() {
  std::stringstream ss;
  ss << "TID = " << id << ", P = " << period_ms << "ms, U = "
     << 100 * get_utilization() << "%, ETs = (" << bcet_us << ", " << acet_us << ", "
     << wcet_us << "), RIDs = (";
  for (int i = 0; i < runnables.size(); i++) {
    if (i == 0) {
      ss << runnables[i]->id;
    } else if (i < 3) {
      ss << ", " << runnables[i]->id;
    } else if (i == 3) {
      ss << ", ...";
    } else if (i < runnables.size() - 1) {
      continue;
    } else {
      ss << ", " << runnables[i]->id;
    }
  }
  ss << ")" << std::endl;
  return ss.str();
}

Label::operator std::string() {
  std::stringstream ss;
  ss << "LID = " << id << ", Size = " << size_bytes << "B, Type = "
     << type;
  if (type == LabelType::kReadWrite) {
    ss << "." << mtype << "." << mvtype;
  }
  if (!readers.empty()) {
    ss << ", Reader=R" << readers[0]->id;
  }
  if (!writers.empty()) {
    ss << ", Writer=R" << writers[0]->id;
  }
  return ss.str();
}

int BenchmarkInstance::estimate_total_number_of_runnables() {
  RunnablesConfig& runnables_cfg = config.runnables_cfg;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 gen(seed);
  UniIntDist uni_int_dist(runnables_cfg.count.min, runnables_cfg.count.max);
  return uni_int_dist(gen) * config.scale_factor;
}

void BenchmarkInstance::generate_runnable_instances(int max) {
  RunnablesConfig& runnables_cfg = config.runnables_cfg;
  int idx = 0;
  double counter = 0;
  for (int i = 0; i < max; i++) {
    int p = config.periods_ms[idx];
    runnables.push_back(new Runnable(i, p));
    counter++;
    if (counter * 100.0 / max >= runnables_cfg.period_shares[p]) {
      idx++;
      counter = 0;
    }
  }
}

void BenchmarkInstance::generate_runnable_acets_bcets_and_wcets() {
  RunnablesConfig& runnables_cfg = config.runnables_cfg;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 gen(seed);
  std::map<int, boost::math::beta_distribution<>> beta_dists_for_acet;
  std::map<int, UniRealDist> uni_real_dists_for_bcet;
  std::map<int, UniRealDist> uni_real_dists_for_wcet;

  for (int p : config.periods_ms) {
    ExecTimeProfile& et = runnables_cfg.exec_time_profiles[p];
    double alpha = 2;
    double beta = (et.acet_max_us - et.acet_avg_us) /
                  (et.acet_avg_us - et.acet_min_us);
    beta = beta * alpha;
    beta_dists_for_acet[p] = boost::math::beta_distribution<>(alpha, beta);
    uni_real_dists_for_bcet[p] = UniRealDist(et.bcet_f_min, et.bcet_f_max);
    uni_real_dists_for_wcet[p] = UniRealDist(et.wcet_f_min, et.wcet_f_max);
  }

  UniRealDist uni_real_dist(0, 1);
  for (Runnable* r : runnables) {
    int p = r->period_ms;
    ExecTimeProfile& et = runnables_cfg.exec_time_profiles[p];
    double rand = uni_real_dist(gen);
    r->acet_us = boost::math::quantile(beta_dists_for_acet[p], rand);
    r->acet_us *= et.acet_max_us - et.acet_min_us;
    r->acet_us += et.acet_min_us;
    r->bcet_us = r->acet_us * uni_real_dists_for_bcet[p](gen);
    r->wcet_us = r->acet_us * uni_real_dists_for_wcet[p](gen);
  }
}

void BenchmarkInstance::generate_task_instances_from_runnables() {
  int id = 0;

  if (!merge_runnables) {
    for (Runnable* r : runnables) {
      Task* t = new Task(id++, r->period_ms);
      t->add_runnable(r);
      tasks.push_back(t);
    }
  } else {
    for (int p : config.periods_ms) {
      Task* t = NULL;
      for (Runnable* r : runnables) {
        if (r->period_ms != p) {
          continue;
        }
        if (t == NULL or !t->does_runnable_fit(r)) {
          t = new Task(id++, p);
          assert(t->does_runnable_fit(r));
          tasks.push_back(t);
        }
        t->add_runnable(r);
      }
    }
  }
}

int BenchmarkInstance::estimate_total_number_of_labels() {
  LabelsConfig& labels_cfg = config.labels_cfg;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 gen(seed);
  UniIntDist uni_int_dist(labels_cfg.count.min, labels_cfg.count.max);
  return uni_int_dist(gen) * config.scale_factor;
}

void BenchmarkInstance::generate_label_instances(int max) {
  LabelsConfig& labels_cfg = config.labels_cfg;
  MessagesConfig& msgs_cfg = config.msgs_cfg;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 gen(seed);
  UniRealDist uni_real_dist(0, 100);
  UniRealDist uni_real_dist2(0, 100);

  double rand;
  double rand2;
  double cumulative_share;
  int size_bytes;
  LabelType type;
  MessageType mtype;
  MessageVolumeType mvtype;

  for (int i = 0; i < max; i++) {
    rand = uni_real_dist(gen);

    cumulative_share = 0;
    std::map<Range<int>, double>::iterator it1;
    for (it1 = labels_cfg.size_shares.begin();
         it1 != labels_cfg.size_shares.end(); it1++) {
      cumulative_share += it1->second;
      if (rand < cumulative_share) {
        break;
      }
    }
    size_bytes = it1->first.max;

    cumulative_share = 0;
    std::map<LabelType, double>::iterator it2;
    for (it2 = labels_cfg.type_partition.begin();
         it2 != labels_cfg.type_partition.end(); it2++) {
      cumulative_share += it2->second;
      if (rand < cumulative_share) {
        break;
      }
    }
    type = it2->first;

    if (type == LabelType::kReadWrite) {
      rand2 = uni_real_dist2(gen);
      cumulative_share = 0;
      std::map<MessageType, double>::iterator it3;
      for (it3 = msgs_cfg.type_partition.begin();
           it3 != msgs_cfg.type_partition.end(); it3++) {
        cumulative_share += it3->second;
        if (rand2 < cumulative_share) {
          break;
        }
      }
      mtype = it3->first;
    }

    labels.push_back(new Label(i, size_bytes, type, mtype));
  }
}

void BenchmarkInstance::map_labels_to_runnables() {
  LDEBUG << "BenchmarkInstance::map_labels_to_runnables";
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 gen(seed);
  UniIntDist uni_int_dist(0, runnables.size() - 1);
  UniIntDist uni_int_dist2(0, tasks.size() - 1);

  for (Label* l : labels) {
    if (l->type == LabelType::kReadOnly) {
      Runnable* r = runnables[uni_int_dist(gen)];
      l->readers.push_back(r);
      r->ro_labels_read.push_back(l);
      //LDEBUG << "Mapping RO label " << l->id << " to " << " runnable " << r->id;
    } else if (l->type == LabelType::kWriteOnly) {
      Runnable* r = runnables[uni_int_dist(gen)];
      l->writers.push_back(r);
      r->wo_labels_written.push_back(l);
      //LDEBUG << "Mapping WO label " << l->id << " to " << " runnable " << r->id;
    } else if (l->type == LabelType::kReadWrite and
               (l->mtype == MessageType::kIntraTaskFwd or
                l->mtype == MessageType::kIntraTaskBwd)) {
      Task* t = tasks[uni_int_dist2(gen)];
      while (t->runnables.size() < 2) {
        t = tasks[uni_int_dist2(gen)];
      }
      //LDEBUG << "Mapping RW intra-task label " << l->id << " to "
      //       << " task " << t->id;
      UniIntDist uni_int_dist3(0, t->runnables.size() - 1);
      Runnable* r1 = t->runnables[uni_int_dist3(gen)];
      Runnable* r2 = t->runnables[uni_int_dist3(gen)];
      while (r1->id == r2->id) {
        r2 = t->runnables[uni_int_dist3(gen)];
      }
      if (r2->id < r1->id) {
        Runnable* temp = r2;
        r2 = r1;
        r1 = temp;
      }
      if (l->mtype == MessageType::kIntraTaskFwd) {
        l->writers.push_back(r1);
        l->readers.push_back(r2);
        r1->rw_intra_task_labels_written.push_back(l);
        r2->rw_intra_task_labels_read.push_back(l);
        //LDEBUG << "Mapping RW intra-task label " << l->id << " to "
        //       << " runnables " << r1->id << " and " << r2->id;
      } else {
        l->readers.push_back(r1);
        l->writers.push_back(r2);
        r1->rw_intra_task_labels_read.push_back(l);
        r2->rw_intra_task_labels_written.push_back(l);
        //LDEBUG << "Mapping RW intra-task label " << l->id << " to "
        //       << " runnables " << r2->id << " and " << r1->id;
      }
    } else if (l->type == LabelType::kReadWrite and
               l->mtype == MessageType::kInterTask) {
      continue; // Let's do this in a separate loop!
    }
  }
}

void BenchmarkInstance::map_inter_task_msgs_to_runnables() {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 gen(seed);

  // Map each label to a unique MessageVolumeType
  InterTaskMessagesConfig& inter_task_msgs_cfg =
    config.msgs_cfg.inter_task_msgs_cfg;
  UniIntDist uni_int_dist3(
    inter_task_msgs_cfg.volume_types[MessageVolumeType::kI].min,
    inter_task_msgs_cfg.volume_types[MessageVolumeType::kVI].max);
  int rand;
  for (Label *l : labels) {
    if (l->type != LabelType::kReadWrite or
        l->mtype != MessageType::kInterTask) {
      continue;
    }
    rand = uni_int_dist3(gen);
    std::map<MessageVolumeType, Range<int>>::iterator it;
    for (it = inter_task_msgs_cfg.volume_types.begin();
         it != inter_task_msgs_cfg.volume_types.end(); it++) {
      if (rand >= it->second.min and rand <= it->second.max) {
        break;
      }
    }
    l->mvtype = it->first;
  }

  // InterTaskMessagesConfig.dist maps each pair of periods p1, p2,
  // (p1 and p2 are the periods of sending and receiving task, respectively)
  // to a MessageVolumeType
  // Here, we reverse this dictionary, i.e., we map each MessageVolumeType
  // to a list of pairs (p1, p2).
  // TODO: Why can't we do this InterTaskMessagesConfig::parse_config_file
  // where we configure InterTaskMessagesConfig.dist?
  std::map<MessageVolumeType, std::vector<Range<int>>> reverse_dist;
  for (int p1 : config.periods_ms) {
    for (int p2 : config.periods_ms) {
      std::map<int, std::map<int, MessageVolumeType>>& dist =
            inter_task_msgs_cfg.dist;
      assert(dist.find(p1) != dist.end());
      assert(dist[p1].find(p2) != dist[p1].end());
      MessageVolumeType mvtype = dist[p1][p2];
      if (reverse_dist.find(mvtype) == reverse_dist.end()) {
        std::vector<Range<int>> empty_list;
        reverse_dist[mvtype] = empty_list;
      }
      reverse_dist[mvtype].push_back(Range<int>(p1, p2));
    }
  }

  // Map from period to a list of runnables with that period
  std::map<int, std::vector<Runnable*>> p_to_r;
  for (Runnable* r : runnables) {
    if (p_to_r.find(r->period_ms) == p_to_r.end()) {
      std::vector<Runnable*> empty_list;
      p_to_r[r->period_ms] = empty_list;
    }
    p_to_r[r->period_ms].push_back(r);
  }

  std::set<int> labels_to_be_deleted;

  for (Label *l : labels) {
    if (l->type == LabelType::kReadWrite and
        l->mtype == MessageType::kInterTask) {
      UniIntDist uni_int_dist4(0, reverse_dist[l->mvtype].size() - 1);
      int rand4 = uni_int_dist4(gen);
      int p1 = reverse_dist[l->mvtype][rand4].min;
      int p2 = reverse_dist[l->mvtype][rand4].max;
      if (p_to_r.find(p1) == p_to_r.end() or p_to_r.find(p2) == p_to_r.end()) {
        // Apparently, there's no runnable generated with period p1 and/or with
        // period p2. Let's skip assigning this label to any runnable for now,
        // and instead, delete it later from the list of all labels.
        labels_to_be_deleted.insert(l->id);
        continue;
      }
      if (p1 == p2 and p_to_r[p1].size() < 2) {
        // In this case, there aren't even two runnables with period p1 = p2,
        // such that one can become a sender and another can become a receiver
        // for the label. Thus, we once again plan to delete the label.
        labels_to_be_deleted.insert(l->id);
        continue;
      }
      UniIntDist uni_int_dist5(0, p_to_r[p1].size() - 1);
      UniIntDist uni_int_dist6(0, p_to_r[p2].size() - 1);
      Runnable* r1 = p_to_r[p1][uni_int_dist5(gen)];
      Runnable* r2 = p_to_r[p2][uni_int_dist6(gen)];
      while (r1->id == r2->id) {
        r2 = p_to_r[p2][uni_int_dist6(gen)];
      }
      l->writers.push_back(r1);
      l->readers.push_back(r2);
      r1->rw_inter_task_labels_written.push_back(l);
      r2->rw_inter_task_labels_read.push_back(l);
      //LDEBUG << "Mapping RW inter-task label " << l->id << " to "
      //       << " runnables " << r2->id << " and " << r1->id;
    }
  }

  for (auto it = labels.begin(); it != labels.end();) {
    if (labels_to_be_deleted.find((*it)->id) != labels_to_be_deleted.end()) {
      it = labels.erase(it);
    } else {
      it++;
    }
  }
}

unsigned int BenchmarkInstance::get_num_tasks(int period_ms) {
  unsigned int count = 0;
  for (Task* t : tasks) {
    if (t->period_ms != period_ms) {
      continue;
    }
    count++;
  }
  return count;
}

double BenchmarkInstance::get_avg_runnables_per_task(int period_ms) {
  unsigned int task_count = 0;
  unsigned int runnable_count = 0;
  for (Task* t : tasks) {
    if (t->period_ms != period_ms) {
      continue;
    }
    task_count++;
    runnable_count += t->runnables.size();
  }
  return runnable_count / (double) task_count;
}

double BenchmarkInstance::get_avg_utilization_per_task(int period_ms) {
  unsigned int task_count = 0;
  double utilization = 0;
  for (Task* t : tasks) {
    if (t->period_ms != period_ms) {
      continue;
    }
    task_count++;
    utilization += t->get_utilization();
  }
  return utilization / (double) task_count;
}

Range<double> BenchmarkInstance::get_bcet_ms_range(int period_ms) {
  Range<double> range = Range<double>(std::numeric_limits<double>::max(),
                                      std::numeric_limits<double>::min());
  for (Task* t : tasks) {
    if (t->period_ms != period_ms) {
      continue;
    }
    if (t->bcet_us < range.min) {
      range.min = US_TO_MS(t->bcet_us);
    }
    if (t->bcet_us > range.max) {
      range.max = US_TO_MS(t->bcet_us);
    }
  }
  return range;
}

Range<double> BenchmarkInstance::get_acet_ms_range(int period_ms) {
  Range<double> range = Range<double>(std::numeric_limits<double>::max(),
                                      std::numeric_limits<double>::min());
  for (Task* t : tasks) {
    if (t->period_ms != period_ms) {
      continue;
    }
    if (t->acet_us < range.min) {
      range.min = US_TO_MS(t->acet_us);
    }
    if (t->acet_us > range.max) {
      range.max = US_TO_MS(t->acet_us);
    }
  }
  return range;
}

Range<double> BenchmarkInstance::get_wcet_ms_range(int period_ms) {
  Range<double> range = Range<double>(std::numeric_limits<double>::max(),
                                      std::numeric_limits<double>::min());
  for (Task* t : tasks) {
    if (t->period_ms != period_ms) {
      continue;
    }
    if (t->wcet_us < range.min) {
      range.min = US_TO_MS(t->wcet_us);
    }
    if (t->wcet_us > range.max) {
      range.max = US_TO_MS(t->wcet_us);
    }
  }
  return range;
}

std::map<int, unsigned int> BenchmarkInstance::get_label_size_dist() {
  std::map<int, unsigned int> sizes;
  for (Label* l : labels) {
    if (sizes.find(l->size_bytes) == sizes.end()) {
      sizes[l->size_bytes] = 0;
    }
    sizes[l->size_bytes]++;
  }
  return sizes;
}

std::map<LabelType, unsigned int> BenchmarkInstance::get_label_type_dist() {
  std::map<LabelType, unsigned int> counts;
  for (Label* l : labels) {
    if (counts.find(l->type) == counts.end()) {
      counts[l->type] = 0;
    }
    counts[l->type]++;
  }
  return counts;
}

std::map<MessageType, unsigned int> BenchmarkInstance::get_msg_type_dist() {
  std::map<MessageType, unsigned int> counts;
  for (Label* l : labels) {
    if (l->type != LabelType::kReadWrite) {
      continue;
    }
    if (counts.find(l->mtype) == counts.end()) {
      counts[l->mtype] = 0;
    }
    counts[l->mtype]++;
  }
  return counts;
}


unsigned int BenchmarkInstance::get_avg_ro_labels_read_per_task() {
  unsigned int count = 0;
  for (Task* t : tasks) {
    for (Runnable* r : t->runnables) {
      count += r->ro_labels_read.size();
    }
  }
  return count / (double) tasks.size();
}

unsigned int BenchmarkInstance::get_avg_rw_labels_read_per_task() {
  unsigned int count = 0;
  for (Task* t : tasks) {
    for (Runnable* r : t->runnables) {
      count += r->rw_intra_task_labels_read.size() +
               r->rw_inter_task_labels_read.size();
    }
  }
  return count / (double) tasks.size();
}

unsigned int BenchmarkInstance::get_avg_wo_labels_written_per_task() {
  unsigned int count = 0;
  for (Task* t : tasks) {
    for (Runnable* r : t->runnables) {
      count += r->wo_labels_written.size();
    }
  }
  return count / (double) tasks.size();
}

unsigned int BenchmarkInstance::get_avg_rw_labels_written_per_task() {
  unsigned int count = 0;
  for (Task* t : tasks) {
    for (Runnable* r : t->runnables) {
      count += r->rw_intra_task_labels_written.size() +
               r->rw_inter_task_labels_written.size();
    }
  }
  return count / (double) tasks.size();
}

//BenchmarkInstance::BenchmarkInstance(std::string filename,
//                                     log4cpp::Category* logger)
//  : logger(logger) {
//}

BenchmarkInstance::BenchmarkInstance(Config& config, log4cpp::Category* logger,
                                     bool merge_runnables)
  : config(config), merge_runnables(merge_runnables), logger(logger) {
  // Generate all Runnable instances first
  int num_runnables = estimate_total_number_of_runnables();
  generate_runnable_instances(num_runnables);
  generate_runnable_acets_bcets_and_wcets();

  // Generate all Task instances next
  generate_task_instances_from_runnables();

  // Generate all Label instances finally
  int num_labels = estimate_total_number_of_labels();
  generate_label_instances(num_labels);
  map_labels_to_runnables();
  map_inter_task_msgs_to_runnables();

  // Print and check
  //for (Runnable* r : runnables) { r->print(); }
  //LDEBUG;
  //for (Task* t : tasks) { t->print(); }
  //LDEBUG;
  //LDEBUG << "Utotal = " << 100 * get_total_utilization() << "%";
  //LDEBUG;
  //for (Label* l : labels) { l->print(); }
  //LDEBUG;
}

double BenchmarkInstance::get_total_utilization() {
  double total_utilization = 0;
  for (Task* t : tasks) {
    total_utilization += t->get_utilization();
  }
  return total_utilization;
}

void BenchmarkInstance::print_summary() {
  LDEBUG << "KZH15 Benchmark Instance Summary";

  LDEBUG << "\t" << tasks.size() << " tasks consisting of " << runnables.size()
         << " runnables";
  LDEBUG << "\t(since runnables with the same activation pattern can be " <<
         "grouped into one task)";
  for (int p : config.periods_ms) {
    Range<int> range = Runnable::get_angle_sync_period_range();
    if (p == 0) {
      LDEBUG << "\t\t" << get_num_tasks(p) <<
             " sporadic tasks with minimum inter-arrival time of " << range.min << "ms";
    } else {
      LDEBUG << "\t\t" << get_num_tasks(p) << " periodic tasks with period " << p
             << "ms";
    }
    if (get_num_tasks(p) == 0) {
      continue;
    }
    LDEBUG << "\t\t\tAverage # runnables per task " <<
           get_avg_runnables_per_task(p);
    LDEBUG << "\t\t\tAverage utilization per task " <<
           get_avg_utilization_per_task(p);
    LDEBUG << "\t\t\tExecution times across tasks";
    Range<double> rb = get_bcet_ms_range(p);
    Range<double> ra = get_acet_ms_range(p);
    Range<double> rw = get_wcet_ms_range(p);
    LDEBUG << "\t\t\t\tBest-case execution times (BCET) in    [" << rb.min <<
           "ms, " << rb.max << "ms]";
    LDEBUG << "\t\t\t\tAverage-case execution times (ACET) in [" << ra.min <<
           "ms, " << ra.max << "ms]";
    LDEBUG << "\t\t\t\tWorst-case execution times (WCET) in   [" << rw.min <<
           "ms, " << rw.max << "ms]";
  }

  LDEBUG << "\t" << labels.size() <<
         " labels (at most one read and write access per label)";

  LDEBUG << "\t\tDistribution of labels based on read/write access";
  std::map<LabelType, unsigned int> d1 = get_label_type_dist();
  double pc_ro = d1[LabelType::kReadOnly]  * 100.0 / labels.size();
  double pc_wo = d1[LabelType::kWriteOnly] * 100.0 / labels.size();
  double pc_rw = d1[LabelType::kReadWrite] * 100.0 / labels.size();
  LDEBUG << "\t\t\t" << pc_ro << "% read-only  (RO) labels";
  LDEBUG << "\t\t\t" << pc_wo << "% write-only (WO) labels";
  LDEBUG << "\t\t\t" << pc_rw << "% read-write (RW) labels, out of which";
  std::map<MessageType, unsigned int> d2 = get_msg_type_dist();
  unsigned int num_rw_itf = d2[MessageType::kIntraTaskFwd];
  unsigned int num_rw_itb = d2[MessageType::kIntraTaskBwd];
  unsigned int num_rw_it  = d2[MessageType::kInterTask];
  double pc_rw_itf = num_rw_itf * 100.0 / d1[LabelType::kReadWrite];
  double pc_rw_itb = num_rw_itb * 100.0 / d1[LabelType::kReadWrite];
  double pc_rw_it  = num_rw_it  * 100.0 / d1[LabelType::kReadWrite];
  LDEBUG << "\t\t\t\t" << pc_rw_itf <<
         "% labels with read after write in the same task " << "(intra-task-forward)";
  LDEBUG << "\t\t\t\t" << pc_rw_itb <<
         "% labels with write after read in the same task " << "(intra-task-backward)";
  LDEBUG << "\t\t\t\t" << pc_rw_it  <<
         "% labels with read and write in different tasks " << "(inter-task)";

  LDEBUG << "\t\tDistribution of labels based on data size";
  std::map<int, unsigned int> d3 = get_label_size_dist();
  std::stringstream ss;
  ss << "\t\t\t";
  for (auto it = d3.begin(); it != d3.end(); it++) {
    if (it != d3.begin()) {
      ss << ", ";
    }
    ss << it->first << "B (" << it->second * 100.0 / labels.size() << "%)";
  }
  LDEBUG << ss.str();

  LDEBUG << "\t\tDistribution of labels based on task mapping";
  LDEBUG << "\t\t\tAverage # RO labels read per task    = " <<
         get_avg_ro_labels_read_per_task();
  LDEBUG << "\t\t\tAverage # RW labels read per task    = " <<
         get_avg_rw_labels_read_per_task();
  LDEBUG << "\t\t\tAverage # WO labels written per task = " <<
         get_avg_wo_labels_written_per_task();
  LDEBUG << "\t\t\tAverage # RW labels written per task = " <<
         get_avg_rw_labels_written_per_task();

  LDEBUG;

}

std::string BenchmarkInstance::export_to_file() {

  //std::string config_file = get_project_directory() +
  //                          "/config/benchmarks/kzh15_instance.cfg";
  std::string config_file = get_project_directory() +
                            "/config/benchmarks/kzh15_" +
                            std::to_string(int(get_total_utilization() * 100)) +
                            "pcu_" + std::to_string(get_labels().size()) + "l.cfg";
  LDEBUG << "Exporting benchmark instance to file " << config_file;

  libconfig::Config cfg;
  libconfig::Setting& root = cfg.getRoot();

  libconfig::Setting& benchmark_instance =
    root.add("KZH15BenchmarkInstance", libconfig::Setting::TypeGroup);

  libconfig::Setting& cfg_summary =
    benchmark_instance.add("Summary", libconfig::Setting::TypeGroup);
  cfg_summary.add("utilization", libconfig::Setting::TypeFloat) =
    get_total_utilization();
  cfg_summary.add("num_labels",
                  libconfig::Setting::TypeInt) = (int) labels.size();

  libconfig::Setting& cfg_tasks =
    benchmark_instance.add("Tasks", libconfig::Setting::TypeList);
  libconfig::Setting& cfg_runnables =
    benchmark_instance.add("Runnables", libconfig::Setting::TypeList);
  libconfig::Setting& cfg_labels =
    benchmark_instance.add("Labels", libconfig::Setting::TypeList);

  for (Task* t : tasks) {
    libconfig::Setting& s = cfg_tasks.add(libconfig::Setting::TypeGroup);
    s.add("id", libconfig::Setting::TypeInt) = t->id;
    s.add("period_ms", libconfig::Setting::TypeInt)
      = Runnable::get_min_period_ms(t->period_ms);
    s.add("acet_us", libconfig::Setting::TypeFloat) = t->acet_us;
    s.add("bcet_us", libconfig::Setting::TypeFloat) = t->bcet_us;
    s.add("wcet_us", libconfig::Setting::TypeFloat) = t->wcet_us;
    libconfig::Setting& s_runnables =
      s.add("runnables", libconfig::Setting::TypeArray);
    for (Runnable* r : t->runnables) {
      s_runnables.add(libconfig::Setting::TypeInt) = r->id;
    }
  }

  for (Runnable* r : runnables) {
    libconfig::Setting& s = cfg_runnables.add(libconfig::Setting::TypeGroup);
    s.add("id", libconfig::Setting::TypeInt) = r->id;
    s.add("period_ms", libconfig::Setting::TypeInt)
      = Runnable::get_min_period_ms(r->period_ms);
    s.add("acet_us", libconfig::Setting::TypeFloat) = r->acet_us;
    s.add("bcet_us", libconfig::Setting::TypeFloat) = r->bcet_us;
    s.add("wcet_us", libconfig::Setting::TypeFloat) = r->wcet_us;
    libconfig::Setting& s_ro_labels_read =
      s.add("ro_labels_read", libconfig::Setting::TypeArray);
    libconfig::Setting& s_wo_labels_written =
      s.add("wo_labels_written", libconfig::Setting::TypeArray);
    libconfig::Setting& s_rw_intra_task_labels_read =
      s.add("rw_intra_task_labels_read", libconfig::Setting::TypeArray);
    libconfig::Setting& s_rw_intra_task_labels_written =
      s.add("rw_intra_task_labels_written", libconfig::Setting::TypeArray);
    libconfig::Setting& s_rw_inter_task_labels_read =
      s.add("rw_inter_task_labels_read", libconfig::Setting::TypeArray);
    libconfig::Setting& s_rw_inter_task_labels_written =
      s.add("rw_inter_task_labels_written", libconfig::Setting::TypeArray);
    for (Label* l : r->ro_labels_read) {
      s_ro_labels_read.add(libconfig::Setting::TypeInt) = l->id;
    }
    for (Label* l : r->wo_labels_written) {
      s_wo_labels_written.add(libconfig::Setting::TypeInt) = l->id;
    }
    for (Label* l : r->rw_intra_task_labels_read) {
      s_rw_intra_task_labels_read.add(libconfig::Setting::TypeInt) = l->id;
    }
    for (Label* l : r->rw_inter_task_labels_read) {
      s_rw_inter_task_labels_read.add(libconfig::Setting::TypeInt) = l->id;
    }
    for (Label* l : r->rw_intra_task_labels_written) {
      s_rw_intra_task_labels_written.add(libconfig::Setting::TypeInt) = l->id;
    }
    for (Label* l : r->rw_inter_task_labels_written) {
      s_rw_inter_task_labels_written.add(libconfig::Setting::TypeInt) = l->id;
    }
  }

  LDEBUG << "Adding node root.KZH15BenchmarkInstance.Labels.[id|size_bytes]";
  for (Label* l : labels) {
    libconfig::Setting& s = cfg_labels.add(libconfig::Setting::TypeGroup);
    s.add("id", libconfig::Setting::TypeInt) = l->id;
    s.add("size_bytes", libconfig::Setting::TypeInt) = l->size_bytes;
  }

  try {
    cfg.writeFile(config_file);
    LDEBUG << "New configuration successfully written to: " << config_file;
  } catch(const libconfig::FileIOException &fioex) {
    LDEBUG << "I/O error while writing file " << config_file;
  }

  return config_file;
}

TaskBasic::operator std::string() {
  std::stringstream ss;
  ss << "TID = " << id << ", P = " << period_ms << "ms, U = "
     << (100 * US_TO_MS(wcet_us) / (double) period_ms) << "%, ETs = ("
     << US_TO_MS(bcet_us) << "ms, " << US_TO_MS(acet_us) << "ms, "
     << US_TO_MS(wcet_us) << "ms)";
  return ss.str();
}

BenchmarkInstanceBasic::BenchmarkInstanceBasic(
  std::string config_file, log4cpp::Category* logger)
  : logger(logger) {
  libconfig::Config cfg;

  try {
    cfg.readFile(config_file.c_str());
  } catch(const libconfig::FileIOException &fioex) {
    std::cerr << "I/O error while reading file " << config_file;
  } catch(const libconfig::ParseException &pex) {
    std::cerr << "Parse error in " << config_file << " at " << pex.getFile()
              << ":" << pex.getLine() << " - " << pex.getError();
  }

  const libconfig::Setting& root = cfg.getRoot();

  const libconfig::Setting& s_tasks = root.lookup("KZH15BenchmarkInstance.Tasks");
  const libconfig::Setting& s_runnables =
    root.lookup("KZH15BenchmarkInstance.Runnables");
  const libconfig::Setting& s_labels =
    root.lookup("KZH15BenchmarkInstance.Labels");

  std::map<int, RunnableBasic*> rid_to_obj_map;
  for (int i = 0; i < s_runnables.getLength(); i++) {
    RunnableBasic* runnable = new RunnableBasic(s_runnables[i]["id"],
        s_runnables[i]["period_ms"],
        s_runnables[i]["acet_us"],
        s_runnables[i]["bcet_us"],
        s_runnables[i]["wcet_us"]);
    rid_to_obj_map[runnable->id] = runnable;
    const libconfig::Setting& s_ro_labels_read =
      s_runnables[i]["ro_labels_read"];
    const libconfig::Setting& s_wo_labels_written =
      s_runnables[i]["wo_labels_written"];
    const libconfig::Setting& s_rw_intra_task_labels_read =
      s_runnables[i]["rw_intra_task_labels_read"];
    const libconfig::Setting& s_rw_intra_task_labels_written =
      s_runnables[i]["rw_intra_task_labels_written"];
    const libconfig::Setting& s_rw_inter_task_labels_read =
      s_runnables[i]["rw_inter_task_labels_read"];
    const libconfig::Setting& s_rw_inter_task_labels_written =
      s_runnables[i]["rw_inter_task_labels_written"];
    for (int j = 0; j < s_ro_labels_read.getLength(); j++) {
      runnable->ro_labels_read.push_back(s_ro_labels_read[j]);
    }
    for (int j = 0; j < s_wo_labels_written.getLength(); j++) {
      runnable->wo_labels_written.push_back(s_wo_labels_written[j]);
    }
    for (int j = 0; j < s_rw_intra_task_labels_read.getLength(); j++) {
      runnable->rw_intra_task_labels_read.push_back(s_rw_intra_task_labels_read[j]);
    }
    for (int j = 0; j < s_rw_intra_task_labels_written.getLength(); j++) {
      runnable->rw_intra_task_labels_written.push_back(
        s_rw_intra_task_labels_written[j]);
    }
    for (int j = 0; j < s_rw_inter_task_labels_read.getLength(); j++) {
      runnable->rw_inter_task_labels_read.push_back(s_rw_inter_task_labels_read[j]);
    }
    for (int j = 0; j < s_rw_inter_task_labels_written.getLength(); j++) {
      runnable->rw_inter_task_labels_written.push_back(
        s_rw_inter_task_labels_written[j]);
    }
    runnables.push_back(runnable);
  }

  for (int i = 0; i < s_tasks.getLength(); i++) {
    TaskBasic* task = new TaskBasic(s_tasks[i]["id"],
                                    s_tasks[i]["period_ms"],
                                    s_tasks[i]["acet_us"],
                                    s_tasks[i]["bcet_us"],
                                    s_tasks[i]["wcet_us"]);
    const libconfig::Setting& s_runnables = s_tasks[i]["runnables"];
    for (int j = 0; j < s_runnables.getLength(); j++) {
      task->runnables.push_back(rid_to_obj_map[s_runnables[j]]);
    }
    tasks.push_back(task);
  }

  for (int i = 0; i < s_labels.getLength(); i++) {
    label_sizes[s_labels[i]["id"]] = s_labels[i]["size_bytes"];
  }

}

double BenchmarkInstanceBasic::get_total_utilization() {
  double total_utilization = 0;
  for (TaskBasic* t : tasks) {
    total_utilization += US_TO_MS(t->wcet_us) / (double) t->period_ms;
  }
  total_utilization *= 100;
  return total_utilization;
}

double BenchmarkInstanceBasic::get_avg_label_size() {
  double avg_label_size = 0;
  for (auto& ls : label_sizes) {
    avg_label_size += ls.second;
  }
  avg_label_size /= label_sizes.size();
  return avg_label_size;
}

unsigned BenchmarkInstanceBasic::get_total_labels() {
  //std::stringstream ss;
  //ss << label_sizes.size() << " labels: ";
  //for (auto it = label_sizes.begin(); it != label_sizes.end(); it++) {
  //  ss << (*it).first << " ";
  //}
  //LDEBUG << ss.str();
  return label_sizes.size();
}

unsigned BenchmarkInstanceBasic::get_total_tasks() {
  return tasks.size();
}

void BenchmarkInstanceBasic::print_summary() {
  LDEBUG << "KZH15 Benchmark Instance (Basic) Summary";
  LDEBUG << "\tTotal labels " << get_total_labels();
  //for (auto& ls : label_sizes) {
  //  LDEBUG << "\t\tLID " << ls.first;
  //}
  LDEBUG << "\tAverage label size " << get_avg_label_size();
  LDEBUG << "\tTotal utilization " << get_total_utilization() << "%";
  LDEBUG << "\tTotal tasks " << get_total_tasks() << ":";
  for (TaskBasic* t : tasks) {
    LDEBUG << "\t\t" << std::string(*t);
  }
  LDEBUG;
}

void resolve_label_dependencies(Utils::KZH15::BenchmarkInstanceBasic& instance,
                                uint64_t smallest_period_ms) {
  //LDEBUG << "Resolving label dependencies";

  std::set<int> delete_set;
  for (Utils::KZH15::RunnableBasic* r : instance.runnables) {
    if (r->period_ms < smallest_period_ms) {
      for (int lid : r->ro_labels_read) {
        delete_set.insert(lid);
      }
      for (int lid : r->wo_labels_written) {
        delete_set.insert(lid);
      }
      for (int lid : r->rw_intra_task_labels_read) {
        delete_set.insert(lid);
      }
      for (int lid : r->rw_intra_task_labels_written) {
        delete_set.insert(lid);
      }
      for (int lid : r->rw_inter_task_labels_read) {
        delete_set.insert(lid);
      }
      for (int lid : r->rw_inter_task_labels_written) {
        delete_set.insert(lid);
      }
    }
  }

  for (Utils::KZH15::RunnableBasic* r : instance.runnables) {
    for (auto it = r->rw_inter_task_labels_read.begin();
         it != r->rw_inter_task_labels_read.end();) {
      int lid = *it;
      if (delete_set.find(lid) != delete_set.end()) {
        it = r->rw_inter_task_labels_read.erase(it);
        //LDEBUG << "Erase label " << lid << " from runnable " << r->id << "'s "
        //       << "rw_inter_task_labels_read list";
      } else {
        it++;
      }
    }
    for (auto it = r->rw_inter_task_labels_written.begin();
         it != r->rw_inter_task_labels_written.end();) {
      int lid = *it;
      if (delete_set.find(lid) != delete_set.end()) {
        it = r->rw_inter_task_labels_written.erase(it);
        //LDEBUG << "Erase label " << lid << " from runnable " << r->id << "'s "
        //       << "rw_inter_task_labels_written list";
      } else {
        it++;
      }
    }
  }

  for (int lid : delete_set) {
    instance.label_sizes.erase(lid);
  }

  //LDEBUG << "Deleted " << delete_set.size()
  //       << " RW labels from their readers' lists";
}

void trim_benchmark(Utils::KZH15::BenchmarkInstanceBasic& instance,
                    uint64_t smallest_period_ms) {
  //LDEBUG << "Trim benchmark so that the smallest period is "
  //       << smallest_period_ms << "ms";

  unsigned tasks_removed = instance.tasks.size();
  unsigned runnables_removed = instance.runnables.size();

  for (auto it = instance.tasks.begin(); it != instance.tasks.end();) {
    if ((*it)->period_ms < smallest_period_ms) {
      it = instance.tasks.erase(it);
    } else {
      it++;
    }
  }

  for (auto it = instance.runnables.begin(); it != instance.runnables.end();) {
    if ((*it)->period_ms < smallest_period_ms) {
      it = instance.runnables.erase(it);
    } else {
      it++;
    }
  }

  tasks_removed = tasks_removed - instance.tasks.size();
  runnables_removed = runnables_removed - instance.runnables.size();

  //LDEBUG << "Removed " << runnables_removed << " runnables and "
  //       << tasks_removed << " tasks from the benchmark";
}

void shrink_label_sizes(Utils::KZH15::BenchmarkInstanceBasic& instance,
                        unsigned max_label_size) {
  //LDEBUG << "Shrink all label sizes under " << max_label_size << " bytes";

  unsigned bytes_reduced = 0;
  unsigned labels_modified = 0;

  for (auto& item : instance.label_sizes) {
    if (item.second > max_label_size) {
      bytes_reduced += item.second - max_label_size;
      labels_modified++;
      item.second = max_label_size;
    }
  }

  //LDEBUG << "Reduced label sizes by an average of "
  //       << bytes_reduced / (double)labels_modified << " across "
  //       << labels_modified << " labels";
}

} // namespace KZH15
} // namespace Utils
