#define CATCH_CONFIG_MAIN
#include <string>
#include "libs/catch.hpp"
#include "../include/gateway.h"

TEST_CASE("Creating a Gateway", "[gateway]") {
    string tmp = "low";
    gateway gt(1, 100, tmp);
    REQUIRE(gt.simTime == 100);
    REQUIRE(gt.count == -1);
}

// TEST_CASE("Creating and Setting Values to a Point", "[point]") {
//     Point p;
//     p.setX(2);
//     p.setY(3);

//     REQUIRE(p.getX() == 2);
//     REQUIRE(p.getY() == 3);
// }