#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <string>

#include <test_helper.hpp>

#include <lib/tree_node.hpp>

extern "C" {
#include <cpp_utilities/src/lib/print_debugger.h>
}

using namespace TreeAndHistoryTraversal;
using namespace TreeNode;

SCENARIO("Traversing a tree") {
  GIVEN("A terminal node") {
    const int xVal = 5;
    StoredTerminalNode<int> patient(xVal);
    THEN("It returns its value") { REQUIRE(patient.value() == xVal); }
    THEN("It reports that it is a terminal node") {
      REQUIRE(patient.isTerminal());
    }
  }
  GIVEN("An internal node") {
    const int xLeftVal = 5;
    const int xRightVal = -2;
    int sum = 0;
    StoredInteriorNode<int> patient(
        {new StoredTerminalNode<int>(xLeftVal),
         new StoredTerminalNode<int>(xRightVal)},
        [&sum](int childValue) { return sum += childValue; });

    THEN("It returns its value") {
      REQUIRE(patient.value() == (xLeftVal + xRightVal));
    }
    THEN("It reports that it is not a terminal node") {
      REQUIRE(!patient.isTerminal());
    }
  }
  GIVEN("A larger tree") {
    double prod = 1.0;
    std::function<double(double)> combiner =
        [&prod](double childValue) { return prod *= childValue; };

    std::vector<TreeNode<double>*> terminals1(5);
    std::vector<TreeNode<double>*> terminals2(2);
    std::vector<TreeNode<double>*> terminals3(3);

    for (auto& t : terminals1) {
      t = new StoredTerminalNode<double>(0.2);
    }
    for (auto& t : terminals2) {
      t = new StoredTerminalNode<double>(0.57);
    }
    for (auto& t : terminals3) {
      t = new StoredTerminalNode<double>(0.94);
    }

    std::vector<TreeNode<double>*> immediateChildren(3);
    for (auto& c : immediateChildren) {
      c = new StoredInteriorNode<double>(combiner);
    }
    static_cast<StoredInteriorNode<double>*>(immediateChildren[0])
        ->setChildren(terminals1);
    static_cast<StoredInteriorNode<double>*>(immediateChildren[1])
        ->setChildren(terminals2);
    static_cast<StoredInteriorNode<double>*>(immediateChildren[2])
        ->setChildren(terminals3);

    double sum = 0.0;
    StoredInteriorNode<double> patient(
        immediateChildren,
        [&sum](double childValue) { return sum += childValue; });

    THEN("It returns its value") {
      REQUIRE(patient.value() == (pow(0.2, 5) + pow(0.2, 5) * pow(0.57, 2) +
                                  pow(0.2, 5) * pow(0.57, 2) * pow(0.94, 3)));
    }
    THEN("It reports that it is not a terminal node") {
      REQUIRE(!patient.isTerminal());
    }
  }
}
