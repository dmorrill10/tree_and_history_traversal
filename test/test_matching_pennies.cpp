#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <string>
#include <vector>
#include <unordered_map>

#include <test_helper.hpp>

#include <lib/history_tree_node.hpp>
#include <lib/history.hpp>
#include <lib/utils.hpp>
#include <lib/matrix_game.hpp>
#include <lib/policy_generator.hpp>

extern "C" {
#include <utilities/src/lib/print_debugger.h>
}

using namespace TreeAndHistoryTraversal;
using namespace HistoryTreeNode;
using namespace History;
using namespace PolicyGenerator;
using namespace MatrixGame;

const std::vector<size_t> numActionsAtEachInfoSet{2};

SCENARIO("CFR on matching pennies") {
  const auto policyGeneratorProfileFactory = [&]() {
    return std::vector<
        PolicyGenerator<size_t, std::pair<size_t, size_t>, Numeric> *>{
        new RegretMatchingTable(NUM_SEQUENCES, numActionsAtEachInfoSet,
                                       NUM_SEQUENCES_BEFORE_EACH_INFO_SET),
        new RegretMatchingTable(NUM_SEQUENCES, numActionsAtEachInfoSet,
                                       NUM_SEQUENCES_BEFORE_EACH_INFO_SET)};
  };
  GIVEN("Traditional terminal values") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -1}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(10);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #1") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -2}, {-1, 2}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(2.0 / 3).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #2") {
    std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(1.0 / 3).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #3") {
    std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-4, 3}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(5e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(7.0 / 11).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(5 / 11.0).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #4") {
    std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-4, 1}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(5.0 / 9.0).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(1.0 / 3.0).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
}

SCENARIO("CFR+ on matching pennies") {
  const auto policyGeneratorProfileFactory = [&]() {
    return std::vector<
        PolicyGenerator<size_t, std::pair<size_t, size_t>, Numeric> *>{
        new RegretMatchingPlusTable(NUM_SEQUENCES, numActionsAtEachInfoSet,
                                       NUM_SEQUENCES_BEFORE_EACH_INFO_SET),
        new RegretMatchingPlusTable(NUM_SEQUENCES, numActionsAtEachInfoSet,
                                       NUM_SEQUENCES_BEFORE_EACH_INFO_SET)};
  };
  GIVEN("Traditional terminal values") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -1}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(10);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #1") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -2}, {-1, 2}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(2.0 / 3).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #2") {
    std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(1.0 / 3).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #3") {
    std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-4, 3}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(5e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(7.0 / 11).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(5 / 11.0).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #4") {
    std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-4, 1}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(5.0 / 9.0).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(1.0 / 3.0).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
}

SCENARIO("perturbed CFR on matching pennies") {
  const auto policyGeneratorProfileFactory = [&]() {
    return std::vector<
        PolicyGenerator<size_t, std::pair<size_t, size_t>, Numeric> *>{
        new PerturbedPolicyRegretMatchingTable(NUM_SEQUENCES, numActionsAtEachInfoSet,
                                         NUM_SEQUENCES_BEFORE_EACH_INFO_SET, 0.1),
        new PerturbedPolicyRegretMatchingTable(NUM_SEQUENCES, numActionsAtEachInfoSet,
                                         NUM_SEQUENCES_BEFORE_EACH_INFO_SET, 0.1)};
  };
  GIVEN("Traditional terminal values") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -1}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #1") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -2}, {-1, 2}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(2.0 / 3).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #2") {
    std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      patient.doIterations(1e4);
      CHECK(patient.strategyProfile()[0][0] == Approx(1.0 / 3).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
  }
  GIVEN("Alternative terminal values #3") {
    std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-4, 3}};
    THEN("CFR finds the equilibrium properly") {
      Cfr<size_t, std::pair<size_t, size_t>, Numeric> patient(
          utilsForPlayer1, policyGeneratorProfileFactory());
      for (size_t i = 0; i < 5e4; ++i) {
        patient.doIteration();
      }

      CHECK(patient.strategyProfile()[0][0] == Approx(7.0 / 11).epsilon(0.001));
      CHECK(patient.strategyProfile()[1][0] == Approx(5.0 / 11.0).epsilon(0.001));
      CHECK(patient.averageExploitability() < 1e-3);
    }
    GIVEN("Very noisy regret updaters") {
      THEN("CFR finds the equilibrium properly") {
        const auto altPolicyGeneratorProfileFactory = [&]() {
          return std::vector<
              PolicyGenerator<size_t, std::pair<size_t, size_t>, Numeric> *>{
              new PerturbedPolicyRegretMatchingTable(
                  NUM_SEQUENCES, numActionsAtEachInfoSet,
                  NUM_SEQUENCES_BEFORE_EACH_INFO_SET, 1),
              new PerturbedPolicyRegretMatchingTable(
                  NUM_SEQUENCES, numActionsAtEachInfoSet,
                  NUM_SEQUENCES_BEFORE_EACH_INFO_SET, 1)};
        };
        Cfr<size_t, std::pair<size_t, size_t>, Numeric>
            patient(utilsForPlayer1, altPolicyGeneratorProfileFactory());
        for (size_t i = 0; i < 1e5; ++i) {
          patient.doIteration();
        }
        CHECK(patient.strategyProfile()[0][0] ==
              Approx(7.0 / 11).epsilon(0.01));
        CHECK(patient.strategyProfile()[1][0] ==
              Approx(5 / 11.0).epsilon(0.01));
        CHECK(patient.averageExploitability() < 1e-3);
      }
    }
  }
}
