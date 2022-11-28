#include "achal/bft_kvs.h"
#include "catch.hpp"
#include "main.h"
#include "utils/macros.h"
#include "utils/misc.h"

// returns sum of 1, n, n^2, ..., n^r
uint64_t geometric_sum(uint64_t n, uint64_t r) {
    uint64_t result = 0;
    uint64_t n_to_the_i = 1;
    for (int i = 0; i <= r; i++) {
        result += n_to_the_i;
        n_to_the_i *= n;
    }
    return result;
}

Achal::config_t create_config(int num_processes, int max_rounds) {
    assert(num_processes <= 7);
    assert(max_rounds <= 3);

    uint64_t period_s = geometric_sum(num_processes, max_rounds) / 21;
    uint64_t scale_factor = 5;

    Achal::config_t config;
    config.id = 0;
    config.period_ns = SEC_TO_NS(period_s) / scale_factor;
    config.offset_ns = 0;
    config.priority = 1;
    config.cpu = 0;
    config.logger = logger;
    config.port = 8080;
    const char *ports[] = {"8080", "8081", "8082", "8083",
                           "8084", "8085", "8086"};
    for (int i = 0; i < num_processes; i++) {
        Achal::process_t p;
        p.id = i;
        p.ip = "127.0.0.1";
        p.port = ports[i];
        config.peers.push_back(p);
    }
    config.max_network_delay_ns = MS_TO_NS(1000);  // useless config as of now
    config.max_rounds = max_rounds;
    config.my_process_id = 0;
    config.max_jobs = 1;
    return config;
}

std::vector<Achal::BFTKVS<256, 7, 15> *> create_stores(Achal::config_t config) {
    std::vector<Achal::BFTKVS<256, 7, 15> *> stores;

    for (int i = 0; i < config.peers.size(); i++) {
        config.port = 8080 + i;
        config.id = i;
        config.cpu = (i % Utils::get_max_cpus());
        config.my_process_id = i;
        stores.push_back(new Achal::BFTKVS<256, 7, 15>(config));
    }

    for (auto s : stores) {
        s->connect_to_servers();
    }

    for (auto s : stores) {
        s->accept_clients();
    }

    return stores;
}

void run_stores(std::vector<Achal::BFTKVS<256, 7, 15> *> &stores) {
    for (auto s : stores) {
        s->spawn();
    }

    for (auto s : stores) {
        s->join();
    }
}

void cleanup_stores(std::vector<Achal::BFTKVS<256, 7, 15> *> &stores) {
    for (auto s : stores) {
        delete s;
    }
}

TEST_CASE("BFSKVS2 Basic", "[bft_kvs][ningfeng][basic]") {
    int num_processes = 4;
    int max_rounds = 2;
    Achal::config_t config = create_config(num_processes, max_rounds);
    std::vector<Achal::BFTKVS<256, 7, 15> *> stores = create_stores(config);

    for (auto s : stores) {
        uint64_t now = Utils::get_time_now_ns();
        uint64_t publish = now - (now % config.period_ns) + config.period_ns;
        for (int i = 0; i < 256; i++) {
            std::string key = std::to_string(i);
            // node 3 is byzantine, always write bad values
            std::string value =
                s->process_id() == 3 ? "bad!" : std::to_string(i);
            REQUIRE(s->try_write(key, publish, value) == true);
        }
    }

    run_stores(stores);

    for (auto s : stores) {
        uint64_t no_earlier_than = 0;
        std::string value;
        for (int i = 0; i < 256; i++) {
            std::string key = std::to_string(i);
            REQUIRE(s->try_read(key, no_earlier_than, value) == true);
            REQUIRE(value == std::to_string(i));
        }
    }

    cleanup_stores(stores);
}

TEST_CASE("BFSKVS2 Garbage Collection", "[bft_kvs][ningfeng]") {
    int num_processes = 4;
    int max_rounds = 2;
    Achal::config_t config = create_config(num_processes, max_rounds);
    std::vector<Achal::BFTKVS<256, 7, 15> *> stores = create_stores(config);

    uint64_t now = Utils::get_time_now_ns();
    uint64_t publish = now - (now % config.period_ns) + config.period_ns;

    // always write 10 more values than the kv stores want to keep
    int keep_n_values = Achal::BFTKVS<256, 7, 15>::keep_n_values;
    int write_n_values = Achal::BFTKVS<256, 7, 15>::keep_n_values + 10;

    for (auto s : stores) {
        for (int i = 1; i <= write_n_values; i++) {
            REQUIRE(s->try_write("key", publish + i,
                                 "value" + std::to_string(i)) == true);
        }
    }

    run_stores(stores);

    for (auto s : stores) {
        uint64_t no_earlier_than = 0;
        std::string value;

        // when you write more values, the store always keep n
        REQUIRE(s->store["key"].size() == keep_n_values);
        REQUIRE(s->try_read("key", no_earlier_than, value) == true);
        REQUIRE(value == "value" + std::to_string(write_n_values));
    }

    cleanup_stores(stores);
}

void test_fuse(int num_processes, int max_rounds, int fault_tolerance, bool read_should_succeed, bool value_should_be_good, Achal::fuse_function_t<256UL, 7UL, 15UL> fuse_func){
    Achal::config_t config = create_config(num_processes, max_rounds);
    std::vector<Achal::BFTKVS<256, 7, 15> *> stores = create_stores(config);

    for (auto s : stores) {
        s->update_fuse_function(fuse_func);
    }

    for (auto s : stores) {
        uint64_t now = Utils::get_time_now_ns();
        uint64_t publish = now - (now % config.period_ns) + config.period_ns;
        for (int i = 0; i < 256; i++) {
            std::string key = std::to_string(i);
            // for every key, there is always F Byzantine
            // node that writes bad value "-1"
            std::string value = (s->process_id() < fault_tolerance)
                                    ? "-1"
                                    : std::to_string(i);
            REQUIRE(s->try_write(key, publish, value) == true);
        }
    }

    run_stores(stores);

    for (auto s : stores) {
        uint64_t no_earlier_than = 0;
        std::string value;
        for (int i = 0; i < 256; i++) {
            std::string key = std::to_string(i);
            if(read_should_succeed){
                REQUIRE(s->try_read(key, no_earlier_than, value) == true);
                if(value_should_be_good){
                    REQUIRE(std::stod(value) == (double)i);
                }
            }
        }
    }

    cleanup_stores(stores);
}

TEST_CASE("BFSKVS2 Custom Fuse Function Simple Median", "[bft_kvs][ningfeng][fuse]") {
    test_fuse(4, 2, 0, true, true, Achal::median_of_doubles);
    test_fuse(4, 2, 1, true, true, Achal::median_of_doubles);
    test_fuse(4, 2, 2, true, false, Achal::median_of_doubles);
}

TEST_CASE("BFSKVS2 7 Nodes Custom Fuse Function Simple Median", "[bft_kvs][fuse]") {
    test_fuse(7, 3, 0, true, true, Achal::median_of_doubles);
    test_fuse(7, 3, 1, true, true, Achal::median_of_doubles);
    test_fuse(7, 3, 2, true, true, Achal::median_of_doubles);
    test_fuse(7, 3, 3, true, true, Achal::median_of_doubles);
    test_fuse(7, 3, 4, true, false, Achal::median_of_doubles);
}

TEST_CASE("BFSKVS2 Custom Fuse Function Strong Simple Majority", "[bft_kvs][ningfeng][strong]") {
    test_fuse(4, 2, 0, true, true, Achal::strong_simple_majority_of_strings);
    test_fuse(4, 2, 1, true, true, Achal::strong_simple_majority_of_strings);
    test_fuse(4, 2, 2, false, false, Achal::strong_simple_majority_of_strings);
}

TEST_CASE("BFSKVS2 7 Nodes Custom Fuse Function Strong Simple Majority", "[bft_kvs][strong]") {
    test_fuse(7, 3, 0, true, true, Achal::strong_simple_majority_of_strings);
    test_fuse(7, 3, 1, true, true, Achal::strong_simple_majority_of_strings);
    test_fuse(7, 3, 2, true, true, Achal::strong_simple_majority_of_strings);
    test_fuse(7, 3, 3, true, true, Achal::strong_simple_majority_of_strings);
    test_fuse(7, 3, 4, false, false, Achal::strong_simple_majority_of_strings);
}

#define MAX_NUM_PROCS 7

uint8_t faulty_data[100 * sizeof(struct Achal::eig_node<256, 7, 15>)];
uint8_t results[MAX_NUM_PROCS][MAX_NUM_PROCS * sizeof(struct Achal::eig_node<256, 7, 15>)];

template<int N>
const uint8_t * test_lie(uint8_t process_id, uint64_t num_jobs, size_t length) {
    assert(N <= MAX_NUM_PROCS);
    assert(process_id < N);
    return faulty_data;
}

template<int N>
void test_record_result(uint8_t process_id, uint64_t num_jobs, const struct Achal::eig_node<256, 7, 15> *result, size_t length) {
    assert(N <= MAX_NUM_PROCS);
    assert(process_id < N);
    assert(length == N);
    std::memcpy((void *)results[process_id], (void *)result, length * sizeof(struct Achal::eig_node<256, 7, 15>));
}

template<int NUM_PROCESSES, int MAX_ROUNDS, int NUM_BAD>
void test_bft(bool read_should_succeed, Achal::fuse_function_t<256, 7, 15> fuse_func) {
    Achal::config_t config = create_config(NUM_PROCESSES, MAX_ROUNDS);
    std::vector<Achal::BFTKVS<256, 7, 15> *> stores = create_stores(config);

    std::memset(faulty_data, 1, sizeof(faulty_data));
    *((uint32_t *)faulty_data) = 256;

    for (auto s : stores) {
        if(s->process_id() < NUM_BAD){
            s->set_lie(test_lie<NUM_PROCESSES>);
        }
        s->set_record_result(test_record_result<NUM_PROCESSES>);
        s->update_fuse_function(fuse_func);
    }

    for (auto s : stores) {
        uint64_t now = Utils::get_time_now_ns();
        uint64_t publish = now - (now % config.period_ns) + config.period_ns;
        for (int i = 0; i < 256; i++) {
            std::string key = std::to_string(i);
            std::string value = std::to_string(i);
            REQUIRE(s->try_write(key, publish, value) == true);
        }
    }

    run_stores(stores);

    for (auto s : stores) {
        uint64_t no_earlier_than = 0;
        std::string value;
        for (int i = 0; i < 256; i++) {
            std::string key = std::to_string(i);
            if(read_should_succeed){
                REQUIRE(s->try_read(key, no_earlier_than, value) == true);
                REQUIRE(std::stod(value) == (double) i);
            }else{
                REQUIRE(s->try_read(key, no_earlier_than, value) == false);
            }
        }
    }

    cleanup_stores(stores);

    for(int pid = 1; pid < NUM_PROCESSES; pid++){
        if(!read_should_succeed){
            if(pid - 1 < NUM_BAD){
                continue;
            }
            if(pid < NUM_BAD){
                continue;
            }
        }
        REQUIRE(std::memcmp(results[pid - 1], results[pid], NUM_PROCESSES * sizeof(struct Achal::eig_node<256, 7, 15>)) == 0);
    }
}

TEST_CASE("BFSKVS2 Inject Fault", "[bft_kvs][ningfeng][inject]") {
    // simple majority
    test_bft<4, 2, 0>(true, NULL);
    test_bft<4, 2, 1>(true, NULL);
    test_bft<4, 2, 2>(false, NULL);
    // simple median
    test_bft<4, 2, 0>(true, Achal::median_of_doubles);
    test_bft<4, 2, 1>(true, Achal::median_of_doubles);
    test_bft<4, 2, 2>(false, Achal::median_of_doubles);
    // strong simple majority
    test_bft<4, 2, 0>(true, Achal::strong_simple_majority_of_strings);
    test_bft<4, 2, 1>(true, Achal::strong_simple_majority_of_strings);
    test_bft<4, 2, 2>(false, Achal::strong_simple_majority_of_strings);
}

TEST_CASE("BFSKVS2 7 Nodes Inject Fault", "[bft_kvs][ningfeng][inject]") {
    // simple majority
    test_bft<7, 3, 0>(true, NULL);
    test_bft<7, 3, 1>(true, NULL);
    test_bft<7, 3, 2>(true, NULL);
    test_bft<7, 3, 3>(false, NULL);
    // simple median
    test_bft<7, 3, 0>(true, Achal::median_of_doubles);
    test_bft<7, 3, 1>(true, Achal::median_of_doubles);
    test_bft<7, 3, 2>(true, Achal::median_of_doubles);
    test_bft<7, 3, 3>(false, Achal::median_of_doubles);
    // strong simple majority
    test_bft<7, 3, 0>(true, Achal::strong_simple_majority_of_strings);
    test_bft<7, 3, 1>(true, Achal::strong_simple_majority_of_strings);
    test_bft<7, 3, 2>(true, Achal::strong_simple_majority_of_strings);
    test_bft<7, 3, 3>(false, Achal::strong_simple_majority_of_strings);
}

template<int NUM_PROCESSES>
void test_should_die(int max_rounds, int num_dead_processes, bool read_should_succeed, Achal::fuse_function_t<256, 7, 15> fuse_func) {
    Achal::config_t config = create_config(NUM_PROCESSES, max_rounds);
    std::vector<Achal::BFTKVS<256, 7, 15> *> stores = create_stores(config);

    for (auto s : stores) {
        if(s->process_id() < num_dead_processes) {
            s->set_should_die([](uint8_t, uint64_t){return true;});
        }
        s->set_record_result(test_record_result<NUM_PROCESSES>);
        s->update_fuse_function(fuse_func);
    }

    for (auto s : stores) {
        uint64_t now = Utils::get_time_now_ns();
        uint64_t publish = now - (now % config.period_ns) + config.period_ns;
        for (int i = 0; i < 256; i++) {
            std::string key = std::to_string(i);
            std::string value = std::to_string(i);
            REQUIRE(s->try_write(key, publish, value) == true);
        }
    }

    run_stores(stores);

    for (auto s : stores) {
        if(s->process_id() < num_dead_processes){
            continue;
        }
        uint64_t no_earlier_than = 0;
        std::string value;
        for (int i = 0; i < 256; i++) {
            std::string key = std::to_string(i);
            if(read_should_succeed){
                REQUIRE(s->try_read(key, no_earlier_than, value) == true);
                REQUIRE(std::stod(value) == (double) i);
            }else{
                REQUIRE(s->try_read(key, no_earlier_than, value) == false);
            }
        }
    }

    cleanup_stores(stores);

    for(int pid = 1; pid < NUM_PROCESSES; pid++){
        if(pid - 1 < num_dead_processes){
            continue;
        }
        if(pid < num_dead_processes){
            continue;
        }
        REQUIRE(std::memcmp(results[pid - 1], results[pid], NUM_PROCESSES * sizeof(struct Achal::eig_node<256, 7, 15>)) == 0);
    }
}

TEST_CASE("BFSKVS2 Pretend Dead", "[bft_kvs][ningfeng][dead]") {
    // simple majority
    test_should_die<4>(2, 0, true, NULL);
    test_should_die<4>(2, 1, true, NULL);
    test_should_die<4>(2, 2, false, NULL);

    // simple median
    test_should_die<4>(2, 0, true, Achal::median_of_doubles);
    test_should_die<4>(2, 1, true, Achal::median_of_doubles);
    test_should_die<4>(2, 2, false, Achal::median_of_doubles);

    // strong simple majority
    test_should_die<4>(2, 0, true, Achal::strong_simple_majority_of_strings);
    test_should_die<4>(2, 1, true, Achal::strong_simple_majority_of_strings);
    test_should_die<4>(2, 2, false, Achal::strong_simple_majority_of_strings);
}

TEST_CASE("BFSKVS2 7 Nodes Pretend Dead", "[bft_kvs][ningfeng][dead]") {
    // simple majority
    test_should_die<7>(3, 0, true, NULL);
    test_should_die<7>(3, 1, true, NULL);
    test_should_die<7>(3, 2, true, NULL);
    test_should_die<7>(3, 3, false, NULL);

    // simple median
    test_should_die<7>(3, 0, true, Achal::median_of_doubles);
    test_should_die<7>(3, 1, true, Achal::median_of_doubles);
    test_should_die<7>(3, 2, true, Achal::median_of_doubles);
    test_should_die<7>(3, 3, false, Achal::median_of_doubles);

    // strong simple majority
    test_should_die<7>(3, 0, true, Achal::strong_simple_majority_of_strings);
    test_should_die<7>(3, 1, true, Achal::strong_simple_majority_of_strings);
    test_should_die<7>(3, 2, true, Achal::strong_simple_majority_of_strings);
    test_should_die<7>(3, 3, false, Achal::strong_simple_majority_of_strings);
}
