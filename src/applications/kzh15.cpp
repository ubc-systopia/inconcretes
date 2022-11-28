#include "kzh15.h"

#include <sstream>
#include <algorithm>

#include "utils/timespec.h"
#include "utils/periodic_task.h"

namespace Applications {

uint64_t KZH15Task::get_random_et_ns() {
  return boost::math::quantile((*beta_dist), (*uni_real_dist)((*gen)));
}

//void KZH15Task::profile_kvs_rw(unsigned int max_iterations) {
//  unsigned curr_iteration = 0;
//  TimeSpec start_ts, end_ts, diff_ts;
//  std::vector<uint64_t> job_exec_times;
//
//  while ((++curr_iteration) <= max_iterations) {
//    Utils::flush_cache();
//
//    clock_gettime(CLOCK_ID, &start_ts);
//    kvs_read_ss_batch(label_keys, label_values);
//    clock_gettime(CLOCK_ID, &end_ts);
//    timespecsub(&end_ts, &start_ts, &diff_ts);
//    job_exec_times.push_back(SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec);
//  }
//
//  kvs_read_profile = new Utils::ExecTimeProfile(job_exec_times);
//  kvs_read_profile->print("KZH15Task" + std::to_string(id) + " " +
//                          std::to_string(label_keys.size()) +
//                          " KVS reads: ");
//
//  job_exec_times.clear();
//  curr_iteration = 0;
//  while ((++curr_iteration) <= max_iterations) {
//    Utils::flush_cache();
//
//    clock_gettime(CLOCK_ID, &start_ts);
//    kvs_write_ss_batch(label_keys, label_values);
//    clock_gettime(CLOCK_ID, &end_ts);
//    timespecsub(&end_ts, &start_ts, &diff_ts);
//    job_exec_times.push_back(SEC_TO_NS(diff_ts.tv_sec) + diff_ts.tv_nsec);
//  }
//
//  kvs_write_profile = new Utils::ExecTimeProfile(job_exec_times);
//  kvs_write_profile->print("2KZH15Task" + std::to_string(id) + " " +
//                           std::to_string(label_keys.size()) +
//                           " KVS writes: ");
//}

//KZH15Task::KZH15Task(unsigned id, uint64_t period_ns, uint64_t offset_ns,
//                     int prio, int cpu, Achal::KVSInterface* kvs,
//                     double acet_us, double bcet_us, double wcet_us,
//                     uint64_t max_jobs, log4cpp::Category *logger)
//  : Utils::PeriodicTask(id, period_ns, offset_ns, prio, cpu, logger),
//    kvs(kvs),
//    acet_ns(US_TO_NS(acet_us)),
//    bcet_ns(US_TO_NS(bcet_us)),
//    wcet_ns(US_TO_NS(wcet_us)),
//    max_jobs(max_jobs),
//    logger(logger) {
//
//  gen = new std::minstd_rand0(
//    std::chrono::system_clock::now().time_since_epoch().count());
//  uni_real_dist = new UniRealDist(0, 1);
//  double beta = (wcet_ns - acet_ns) / (double) (acet_ns - bcet_ns);
//  beta_dist = new boost::math::beta_distribution<>(1, beta);
//}

//KZH15Task::update_labels(Utils::KZH15::TaskBasic* task,
//                         std::map<int, int>& label_sizes) {
//  for (int lid : task->labels_read) {
//    label_map[std::to_string(lid)] = std::string(label_sizes[lid], 'a');
//  }
//  for (int lid : task->labels_written) {
//    label_map[std::to_string(lid)] = std::string(label_sizes[lid], 'a');
//
//  std::stringstream ss;
//  ss << "Task " << id << ": period " << NS_TO_MS(period_ns) <<
//     "ms, # labels: " << label_map.size();
//  for (const auto &item : label_map) {
//    label_keys.push_back(item.first);
//    label_values.push_back(item.first);
//    //ss << item.first << " ";
//  }
//  LDEBUG << ss.str();
//}

KZH15Task::KZH15Task(unsigned id, uint64_t period_ns, uint64_t offset_ns,
                     int prio, int cpu, Achal::KVSInterface* kvs, uint64_t max_jobs,
                     Utils::KZH15::TaskBasic* task,
                     std::map<int, int>& label_sizes, int max_label_size,
                     log4cpp::Category *logger)
  : Utils::PeriodicTask(id, period_ns, offset_ns, prio, cpu, logger),
    kvs(kvs),
    acet_ns(US_TO_NS(task->acet_us)),
    bcet_ns(US_TO_NS(task->bcet_us)),
    wcet_ns(US_TO_NS(task->wcet_us)),
    max_jobs(max_jobs),
    num_jobs_to_ignore(SEC_TO_NS(2)/period_ns),
    logger(logger) {
  gen = new std::minstd_rand0(
    std::chrono::system_clock::now().time_since_epoch().count());
  uni_real_dist = new UniRealDist(0, 1);
  double alpha = 2;
  double beta = alpha * ((wcet_ns - acet_ns) / (double) (acet_ns - bcet_ns));
  beta_dist = new boost::math::beta_distribution<>(alpha, beta);

  std::stringstream ss_ro_labels_read;
  std::stringstream ss_ro_labels_written;
  std::stringstream ss_rw_inter_task_labels_read;
  std::stringstream ss_rw_inter_task_labels_written;
  std::stringstream ss_rw_intra_task_labels_read;
  std::stringstream ss_rw_intra_task_labels_written;

  for (Utils::KZH15::RunnableBasic* r : task->runnables) {
    for (int lid : r->ro_labels_read) {
      int label_size = std::max(std::min(max_label_size, label_sizes[lid]) - 1, 1);
      ro_labels_read_keys.push_back(std::to_string(lid));
      ro_labels_read_values.push_back(std::string(label_size, 'a'));
      ss_ro_labels_read << lid << " ";
    }
    for (int lid : r->wo_labels_written) {
      int label_size = std::max(std::min(max_label_size, label_sizes[lid]) - 1, 1);
      wo_labels_written_keys.push_back(std::to_string(lid));
      wo_labels_written_values.push_back(std::string(label_size, 'a'));
      ss_ro_labels_written << lid << " ";
    }
    for (int lid : r->rw_inter_task_labels_read) {
      int label_size = std::max(std::min(max_label_size, label_sizes[lid]) - 1, 1);
      rw_inter_task_labels_read_keys.push_back(std::to_string(lid));
      rw_inter_task_labels_read_values.push_back(std::string(label_size, 'a'));
      ss_rw_inter_task_labels_read << lid << " ";
    }
    for (int lid : r->rw_inter_task_labels_written) {
      int label_size = std::max(std::min(max_label_size, label_sizes[lid]) - 1, 1);
      rw_inter_task_labels_written_keys.push_back(std::to_string(lid));
      rw_inter_task_labels_written_values.push_back(std::string(label_size, 'a'));
      ss_rw_inter_task_labels_written << lid << " ";
    }
    for (int lid : r->rw_intra_task_labels_read) {
      int label_size = std::max(std::min(max_label_size, label_sizes[lid]) - 1, 1);
      rw_intra_task_labels_read_keys.push_back(std::to_string(lid));
      rw_intra_task_labels_read_values.push_back(std::string(label_size, 'a'));
      ss_rw_intra_task_labels_read << lid << " ";
    }
    for (int lid : r->rw_intra_task_labels_written) {
      int label_size = std::max(std::min(max_label_size, label_sizes[lid]) - 1, 1);
      rw_intra_task_labels_written_keys.push_back(std::to_string(lid));
      rw_intra_task_labels_written_values.push_back(std::string(label_size, 'a'));
      ss_rw_intra_task_labels_written << lid << " ";
    }
  }

  LDEBUG << "Task " << id << ": period " << NS_TO_MS(period_ns)
         << "ms, ignoring reads during the first " << num_jobs_to_ignore
         << " jobs";
  LDEBUG << "Task " << id << ": " << ro_labels_read_keys.size()
         << " RO labels read: " << ss_ro_labels_read.str();
  LDEBUG << "Task " << id << ": " << wo_labels_written_keys.size()
         << " WO labels written: " << ss_ro_labels_written.str();
  LDEBUG << "Task " << id << ": " << rw_inter_task_labels_read_keys.size()
         << " RW inter-task labels read: " << ss_rw_inter_task_labels_read.str();
  LDEBUG << "Task " << id << ": " << rw_inter_task_labels_written_keys.size()
         << " RW inter-task labels written: " << ss_rw_inter_task_labels_written.str();
  LDEBUG << "Task " << id << ": " << rw_intra_task_labels_read_keys.size()
         << " RW intra-task labels read: " << ss_rw_intra_task_labels_read.str();
  LDEBUG << "Task " << id << ": " << rw_intra_task_labels_written_keys.size()
         << " RW intra-task labels written: " << ss_rw_intra_task_labels_written.str();
}

KZH15Task::KZH15Task(unsigned id, uint64_t period_ns, uint64_t offset_ns,
                     int prio, int cpu, Achal::KVSInterface* kvs, uint64_t max_jobs,
                     Utils::KZH15::Task* task, log4cpp::Category *logger)
  : Utils::PeriodicTask(id, period_ns, offset_ns, prio, cpu, logger),
    kvs(kvs),
    acet_ns(US_TO_NS(task->acet_us)),
    bcet_ns(US_TO_NS(task->bcet_us)),
    wcet_ns(US_TO_NS(task->wcet_us)),
    max_jobs(max_jobs),
    logger(logger) {
  gen = new std::minstd_rand0(
    std::chrono::system_clock::now().time_since_epoch().count());
  uni_real_dist = new UniRealDist(0, 1);
  double beta = (wcet_ns - acet_ns) / (double) (acet_ns - bcet_ns);
  beta_dist = new boost::math::beta_distribution<>(1, beta);

  for (Utils::KZH15::Runnable* r : task->runnables) {
    for (Utils::KZH15::Label* l : r->wo_labels_written) {
      wo_labels_written_keys.push_back(std::to_string(l->id));
      wo_labels_written_values.push_back(std::string(l->size_bytes - 1, 'a'));
    }
    for (Utils::KZH15::Label* l : r->rw_inter_task_labels_read) {
      rw_inter_task_labels_read_keys.push_back(std::to_string(l->id));
      rw_inter_task_labels_read_values.push_back(std::string(l->size_bytes - 1, 'a'));
    }
    for (Utils::KZH15::Label* l : r->rw_inter_task_labels_written) {
      rw_inter_task_labels_written_keys.push_back(std::to_string(l->id));
      rw_inter_task_labels_written_values.push_back(std::string(l->size_bytes - 1, 'a'));
    }
  }

  std::stringstream ss;
  ss << "Task " << id << ": period " << NS_TO_MS(period_ns)
     << "ms, # WO labels written: " << wo_labels_written_keys.size()
     << ", # RW labels read: " << rw_inter_task_labels_read_keys.size()
     << ", # RW labels written: " << rw_inter_task_labels_written_keys.size()
     << ", ignoring reads during the first " << num_jobs_to_ignore << " jobs";
  LDEBUG << ss.str();
}

KZH15Task::~KZH15Task() {
  delete gen;
  delete uni_real_dist;
  delete beta_dist;
}

void KZH15Task::job() {
  // Start clock
  TimeSpec start_ts;
  clock_gettime(CLOCK_ID, &start_ts);

  uint64_t release_time = my_release_time_without_offset();

  // Read all labels in ro_labels_read
  read_status_1 = kvs->try_read_batch(
    ro_labels_read_keys,
    release_time, // - time_period_ns,
    ro_labels_read_values);
  // Read all labels in rw_intra_task_labels_read
  read_status_3 = kvs->try_read_batch(
    rw_intra_task_labels_read_keys,
    release_time, // - time_period_ns,
    rw_intra_task_labels_read_values);

  // Read all labels in rw_inter_task_labels_read
  uint64_t freshness_constraint;
  for (unsigned i = 0; i < rw_inter_task_labels_read_keys.size(); i++) {
    freshness_constraint = release_time;
    if (rw_inter_task_labels_read_freshness_constraints[i] > time_period_ns) {
      freshness_constraint = others_release_time_without_offset(
        rw_inter_task_labels_read_freshness_constraints[i]);
    }
    if (kvs->try_read(rw_inter_task_labels_read_keys[i],
                      freshness_constraint,
                      rw_inter_task_labels_read_values[i])) {
      INC_SUCCESS(read_status_2);
    } else {
      INC_FAILURE(read_status_2);
    }
  }

  if (num_jobs <= num_jobs_to_ignore) {
    read_status_1.first = ro_labels_read_keys.size();
    read_status_1.second = 0;
    read_status_2.first = rw_inter_task_labels_read_keys.size();
    read_status_2.second = 0;
    read_status_3.first = rw_intra_task_labels_read_keys.size();
    read_status_3.second = 0;
  }

  successful_reads += GET_SUCCESS(read_status_1) +
                      GET_SUCCESS(read_status_2) +
                      GET_SUCCESS(read_status_3);
  failed_reads += GET_FAILURE(read_status_1) +
                  GET_FAILURE(read_status_2) +
                  GET_FAILURE(read_status_3);

  if (GET_FAILURE(read_status_1) == 0 and
      GET_FAILURE(read_status_2) == 0 and
      GET_FAILURE(read_status_3) == 0) {
    successful_batch_reads++;
  } else {
    failed_batch_reads++;
  }

  read_status_1 = std::make_pair(0, 0);
  read_status_2 = std::make_pair(0, 0);
  read_status_3 = std::make_pair(0, 0);

  //// Taint all labels in wo_label_written and rw_inter_task_labels_written
  //char taint = (char)((num_jobs % 94) + 33);
  //std::string dummy = std::to_string(num_jobs);
  //for (std::string& label_value : wo_labels_written_values) {
  //  label_value.back() = taint;
  //}
  //for (std::string& label_value : rw_inter_task_labels_written_values) {
  //  label_value.back() = taint;
  //}
  //for (std::string& label_value : rw_intra_task_labels_written_values) {
  //  label_value.back() = taint;
  //}

  // Spin
  //Utils::spin2(get_random_et_ns(), start_ts);
  Utils::spin(get_random_et_ns());

  // Write all labels in wo_labels_written and rw_inter_task_labels_written
  uint64_t publish_time = release_time + time_period_ns;
  write_status_1 = kvs->try_write_batch(
    wo_labels_written_keys,
    publish_time,
    wo_labels_written_values);
  write_status_2 = kvs->try_write_batch(
    rw_inter_task_labels_written_keys,
    publish_time,
    rw_inter_task_labels_written_values);
  write_status_3 = kvs->try_write_batch(
    rw_intra_task_labels_written_keys,
    publish_time,
    rw_intra_task_labels_written_values);
  write_status_4 = kvs->try_write_batch(
    ro_labels_read_keys,
    publish_time,
    ro_labels_read_values);

  successful_writes += GET_SUCCESS(write_status_1) +
                       GET_SUCCESS(write_status_2) +
                       GET_SUCCESS(write_status_3) +
                       GET_SUCCESS(write_status_4);
  failed_writes += GET_FAILURE(write_status_1) +
                   GET_FAILURE(write_status_2) +
                   GET_FAILURE(write_status_3) +
                   GET_FAILURE(write_status_4);

  if (GET_FAILURE(write_status_1) == 0 and
      GET_FAILURE(write_status_2) == 0 and
      GET_FAILURE(write_status_3) == 0 and
      GET_FAILURE(write_status_4) == 0) {
    successful_batch_writes++;
  } else {
    failed_batch_writes++;
  }

  write_status_1 = std::make_pair(0, 0);
  write_status_2 = std::make_pair(0, 0);
  write_status_3 = std::make_pair(0, 0);
  write_status_4 = std::make_pair(0, 0);

  num_jobs++;
}

void KZH15Task::job_for_profiler() {
  job();
}

bool KZH15Task::terminate() {
  return num_jobs >= max_jobs;
}

bool KZH15Task::initialize_writes(uint64_t release_ns) {
  uint64_t publish_time = release_ns + offset_ns;
  bool initial_write_status = false;

  write_status_1 = std::make_pair(0, 0);
  write_status_2 = std::make_pair(0, 0);
  write_status_3 = std::make_pair(0, 0);
  write_status_4 = std::make_pair(0, 0);

  write_status_1 = kvs->try_write_batch(
    wo_labels_written_keys,
    publish_time,
    wo_labels_written_values);
  write_status_2 = kvs->try_write_batch(
    rw_inter_task_labels_written_keys,
    publish_time,
    rw_inter_task_labels_written_values);
  write_status_3 = kvs->try_write_batch(
    rw_intra_task_labels_written_keys,
    publish_time,
    rw_intra_task_labels_written_values);
  write_status_4 = kvs->try_write_batch(
    ro_labels_read_keys,
    publish_time,
    ro_labels_read_values);

  if (GET_FAILURE(write_status_1) == 0 and
      GET_FAILURE(write_status_2) == 0 and
      GET_FAILURE(write_status_3) == 0 and
      GET_FAILURE(write_status_4) == 0) {
    initial_write_status = true;
  }

  write_status_1 = std::make_pair(0, 0);
  write_status_2 = std::make_pair(0, 0);
  write_status_3 = std::make_pair(0, 0);
  write_status_4 = std::make_pair(0, 0);

  return initial_write_status;
}

void KZH15Task::print_stats() {
  TimeSpec diff_ts;
  timespecsub(&task_finish_ts, &task_start_ts, &diff_ts);
  LDEBUG << "Task " << id << " actual (expected) duration: "
         << TS_TO_MS(&diff_ts) << "ms (" << max_jobs * NS_TO_MS(time_period_ns)
         << "ms), BCET (measured): " << bcet_measured_ms
         << "ms, ACET (measured): " << acet_measured_ms
         << "ms, WCET (measured): " << wcet_measured_ms << "ms";
  LDEBUG << "Task " << id << " stats: Reads: " << successful_reads
         << "S, " << failed_reads << "F, " << successful_batch_reads << "BS, "
         << failed_batch_reads << "BF; Writes: " << successful_writes
         << "S, " << failed_writes << "F, " << successful_batch_writes << "BS, "
         << failed_batch_writes << "BF";
  //for (unsigned int i = 0; i < read_stats.size(); i++) {
  //  if (read_stats[i]) {
  //    std::cout << "_";
  //  } else {
  //    std::cout << "#";
  //  }
  //  if (write_stats[i]) {
  //    std::cout << ".";
  //  } else {
  //    std::cout << "$";
  //  }
  //}
}

} // namespace Aplications
