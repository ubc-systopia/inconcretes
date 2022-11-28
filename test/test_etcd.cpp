#include <chrono>
#include <etcd/SyncClient.hpp>
#include <iostream>
#include <thread>
#include <vector>

#include "achal/etcd_kvs.h"
#include "catch.hpp"
#include "main.h"
#include "utils/misc.h"

using namespace std::chrono_literals;

TEST_CASE("ETCD Basic put multi-level", "[etcd]") {
    etcd::SyncClient etcd("http://localhost:2379");
    REQUIRE(etcd.head().is_ok() == true);

    etcd.rmdir("/key", true);
    REQUIRE(etcd.ls("/key").values().size() == 0);

    etcd.put("/key/t1", "v1");
    REQUIRE(etcd.get("/key/t1").value().as_string() == "v1");
    REQUIRE(etcd.ls("/key").is_ok() == true);
    REQUIRE(etcd.ls("/key").values().size() == 1);
    REQUIRE(etcd.ls("/key").values()[0].key() == "/key/t1");

    etcd.put("/key/t2", "v2");
    REQUIRE(etcd.get("/key/t2").value().as_string() == "v2");
    REQUIRE(etcd.ls("/key").is_ok() == true);
    REQUIRE(etcd.ls("/key").values().size() == 2);
    REQUIRE(etcd.ls("/key").values()[0].key() == "/key/t1");
    REQUIRE(etcd.ls("/key").values()[1].key() == "/key/t2");

    etcd.rmdir("/key", true);
}

TEST_CASE("ETCD Basic test TTL", "[etcd]") {
    etcd::SyncClient etcd("http://localhost:2379");
    etcd.rm("test");

    REQUIRE(etcd.head().is_ok() == true);

    etcd.set("test", "value", 3);
    REQUIRE(etcd.get("test").is_ok() == true);
    REQUIRE(etcd.get("test").value().as_string() == "value");

    std::this_thread::sleep_for(20s);
    REQUIRE(etcd.get("test").is_ok() == false);

    etcd.rm("test");
}

TEST_CASE("EtcdKVS Basic read write", "[etcd]") {
    std::vector<Achal::EtcdKVS *> kvs;
    for (int me = 0; me < 4; me++) {
        std::vector<Achal::TCP::Node> peers_minus_self;
        for (int peer = 0; peer < 4; peer++) {
            if (peer == me) {
                continue;
            }
            peers_minus_self.push_back(
                Achal::TCP::Node{"127.0.0.1", std::to_string(8080 + peer)});
        }
        auto temp = new Achal::EtcdKVS(me, SEC_TO_NS(1), 0, 1, me, 10, 8080 + me,
                                   peers_minus_self, logger);
        temp->update_fuse_function();
        kvs.push_back(temp);
    }

    uint64_t now = Utils::get_time_now_ns();
    uint64_t last_sec = now - (now % SEC_TO_NS(1));

    uint64_t no_earlier_than = last_sec - SEC_TO_NS(1);
    uint64_t pt1 = last_sec + SEC_TO_NS(2);
    uint64_t pt2 = last_sec + SEC_TO_NS(6);

    std::string value;

    for (int me = 0; me < 4; me++) {
        // write 2 version of values, one after 2 sec and one after 6 sec
        REQUIRE(kvs[me]->try_write("kvs_test_key", pt1, "10.0") == true);
        REQUIRE(kvs[me]->try_write("kvs_test_key", pt2, "20.0") == true);

        // immediate read should fail
        REQUIRE(kvs[me]->try_read("kvs_test_key", no_earlier_than, value) ==
                false);
    }

    // after 4 seconds, read should succeed and should get 10 (published after 2
    // sec)
    std::this_thread::sleep_for(4s);

    for (int me = 0; me < 4; me++) {
        REQUIRE(kvs[me]->try_read("kvs_test_key", no_earlier_than, value) ==
                true);
        REQUIRE(std::stod(value) == 10.0);
    }

    // after 4 + 4 = 8 seconds, read should succeed and should get 20 (published
    // after 6 sec)
    std::this_thread::sleep_for(4s);

    for (int me = 0; me < 4; me++) {
        REQUIRE(kvs[me]->try_read("kvs_test_key", no_earlier_than, value) == true);
        REQUIRE(std::stod(value) == 20.0);
    }

    for(auto k : kvs){
        delete k;
    }
}
