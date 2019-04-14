#define CATCH_CONFIG_MAIN
#include <string>
#include "libs/catch.hpp"
#include "../include/gateway.h"

TEST_CASE("Creating a Gateway", "[gateway]") {
    string tmp = "low";
    gateway gt(1, 100, tmp, "././samples/RED/topology/topology-gateway.txt");
    REQUIRE(gt.simTime == 100);
    REQUIRE(gt.count == -1);
}