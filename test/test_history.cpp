#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <string>

#include <test_helper.hpp>

#include <lib/history.hpp>

extern "C" {
#include <utilities/src/lib/print_debugger.h>
}

using namespace TreeAndHistoryTraversal;
using namespace History;

SCENARIO("Walking a string history") {
  GIVEN("A set of legal characters and legal suffix check") {
    StringHistory patient({"a", "b", "c"},
                          [](const StringHistory &prefix,
                             const std::string &candidate) { return true; });
    REQUIRE(patient.isEmpty());
    REQUIRE(patient.hasSuccessors());
    REQUIRE("" == patient.toString());
    REQUIRE("" == patient.last());
    THEN("#push and #pop work") {
      patient.push("a");
      REQUIRE("a" == patient.toString());
      REQUIRE("a" == patient.last());
      REQUIRE(!patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
      patient.push("b");
      REQUIRE("a -> b" == patient.toString());
      REQUIRE("b" == patient.last());
      REQUIRE(!patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
      patient.pop();
      REQUIRE("a" == patient.toString());
      REQUIRE("a" == patient.last());
      REQUIRE(!patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
      patient.pop();
      REQUIRE("" == patient.toString());
      REQUIRE(patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
    }
  }
  GIVEN("A set of legal characters and more complicated legal suffix check") {
    StringHistory patient({"a", "b", "c"}, [](const StringHistory &prefix,
                                              const std::string &candidate) {
      return (prefix.isEmpty() || (prefix.last() == "b" && candidate == "c") ||
              (prefix.last() == "a" || prefix.last() == "c"));
    });
    REQUIRE(patient.isEmpty());
    REQUIRE(patient.hasSuccessors());
    REQUIRE("" == patient.toString());
    REQUIRE("" == patient.last());
    THEN("#push and #pop work") {
      patient.push("a");
      REQUIRE("a" == patient.toString());
      REQUIRE("a" == patient.last());
      REQUIRE(!patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
      patient.push("b");
      REQUIRE("a -> b" == patient.toString());
      REQUIRE("b" == patient.last());
      REQUIRE(!patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
      patient.push("c");
      REQUIRE("a -> b -> c" == patient.toString());
      REQUIRE("c" == patient.last());
      REQUIRE(!patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
      patient.pop();
      REQUIRE("a -> b" == patient.toString());
      REQUIRE("b" == patient.last());
      REQUIRE(!patient.isEmpty());
      REQUIRE(patient.hasSuccessors());

      REQUIRE_THROWS(patient.push("a"));

      patient.pop();
      REQUIRE("a" == patient.toString());
      REQUIRE("a" == patient.last());
      REQUIRE(!patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
      patient.pop();
      REQUIRE("" == patient.toString());
      REQUIRE(patient.isEmpty());
      REQUIRE(patient.hasSuccessors());
    }
    THEN("It creates and destroys sequences properly") {
      const std::vector<std::string> xStrings{
          "",       "a", "a -> a", "a -> b", "a -> c", "b",
          "b -> c", "c", "c -> a", "c -> b", "c -> c"};
      const std::vector<size_t> xIndices{0, // Placeholder
                                         0, 0, 1, 2, 1, 2, 2, 0, 1, 2};
      size_t i = 0;
      REQUIRE(xStrings[i] == patient.toString());
      ++i;
      patient.eachSuccessor([&xStrings, &i, &xIndices](
          History<std::string> *successor, size_t index) {
        REQUIRE(xIndices[i] == index);
        REQUIRE(xStrings[i] ==
                static_cast<StringHistory *>(successor)->toString());
        ++i;
        successor->eachSuccessor([&xStrings, &i, &xIndices](
            History<std::string> *nextSuccessor, size_t subIndex) {
          REQUIRE(xIndices[i] == subIndex);
          REQUIRE(xStrings[i] ==
                  static_cast<StringHistory *>(nextSuccessor)->toString());
          ++i;
          return false;
        });
        return false;
      });
    }
  }
}
