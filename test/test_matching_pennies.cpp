#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <string>
#include <vector>
#include <unordered_map>

#include <test_helper.hpp>

#include <lib/history_tree_node.hpp>
#include <lib/history.hpp>

extern "C" {
#include <utilities/src/lib/print_debugger.h>
}

using namespace TreeAndHistoryTraversal;
using namespace HistoryTreeNode;
using namespace History;

template <typename T>
T copyAndReturnAfter(T valueToSaveAndRestore, std::function<void()> doFn) {
  doFn();
  return valueToSaveAndRestore;
}

template <typename Numeric>
static std::vector<Numeric> normalized(const std::vector<Numeric> &v) {
  Numeric sum = 0.0;
  std::vector<Numeric> n(v.size(), 1.0 / v.size());
  for (auto elem : v) {
    sum += elem;
  }
  if (sum > 0) {
    for (size_t i = 0; i < v.size(); ++i) {
      n[i] = v[i] > 0.0 ? exp(log(v[i]) - log(sum)) : 0.0;
    }
  }
  return n;
}

#include <random>
static bool flipCoin(double probTrue, std::mt19937 *randomEngine) {
  assert(randomEngine);
  std::bernoulli_distribution coin(probTrue);
  return coin(*randomEngine);
}

template <typename InformationSet, typename Sequence, typename Regret>
class PolicyGenerator {
protected:
  PolicyGenerator() {}

public:
  virtual ~PolicyGenerator() {}

  virtual std::vector<double> policy(const InformationSet &I) const = 0;
  virtual void update(const Sequence &sequence, Regret regretValue) = 0;
};

typedef double Numeric;

class RegretMatchingTable
    : public PolicyGenerator<size_t, std::pair<size_t, size_t>, Numeric> {
public:
  RegretMatchingTable(size_t numSequences,
                      const std::vector<size_t> &numActionsAtEachInfoSet,
                      const std::vector<size_t> &numSequencesBeforeEachInfoSet)
      : table_(numSequences, 0.0),
        numActionsAtEachInfoSet_(&numActionsAtEachInfoSet),
        numSequencesBeforeEachInfoSet_(&numSequencesBeforeEachInfoSet) {}
  virtual ~RegretMatchingTable() {}

  virtual std::vector<Numeric> policy(const size_t &I) const override {
    Numeric sum = 0.0;
    const auto numActions = (*numActionsAtEachInfoSet_)[I];
    const auto baseIndex = (*numSequencesBeforeEachInfoSet_)[I];

    std::vector<Numeric> policy_(numActions);
    for (size_t i = 0; i < numActions; ++i) {
      sum += table_[baseIndex + i] > 0.0 ? table_[baseIndex + i] : 0.0;
      policy_[i] = 1.0 / numActions;
    }
    if (sum > 0) {
      for (size_t i = 0; i < numActions; ++i) {
        policy_[i] =
            table_[baseIndex + i] > 0.0 ? table_[baseIndex + i] / sum : 0.0;
      }
    }

    DEBUG_VARIABLE("%lg", table_[baseIndex + 0]);
    DEBUG_VARIABLE("%lg", table_[baseIndex + 1]);
    DEBUG_VARIABLE("%lg", policy_[0]);
    DEBUG_VARIABLE("%lg", policy_[1]);

    return policy_;
  }

  virtual void update(const std::pair<size_t, size_t> &sequence,
      Numeric regretValue) override {
    const auto infoSet = sequence.first;
    const auto action = sequence.second;
    const auto index = (*numSequencesBeforeEachInfoSet_)[infoSet] + action;
    table_[index] += regretValue;
  }

protected:
  std::vector<Numeric> table_;
  const std::vector<size_t> *numActionsAtEachInfoSet_;
  const std::vector<size_t> *numSequencesBeforeEachInfoSet_;
};

class RegretMatchingPlusTable : public RegretMatchingTable {
public:
  RegretMatchingPlusTable(
      size_t numSequences, const std::vector<size_t> &numActionsAtEachInfoSet,
      const std::vector<size_t> &numSequencesBeforeEachInfoSet)
      : RegretMatchingTable::RegretMatchingTable(
            numSequences, numActionsAtEachInfoSet,
            numSequencesBeforeEachInfoSet) {}
  virtual ~RegretMatchingPlusTable() {}

  virtual void update(const std::pair<size_t, size_t> &sequence,
      Numeric regretValue) override {
    const auto infoSet = sequence.first;
    const auto action = sequence.second;
    const auto index = (*numSequencesBeforeEachInfoSet_)[infoSet] + action;
    if (table_[index] + regretValue > 0.0) {
      table_[index] += regretValue;
    }
    else {
      table_[index] = 0.0;
    }
  }
};

class PerturbedPolicyRegretMatchingTable : public RegretMatchingTable {
public:
  PerturbedPolicyRegretMatchingTable(
      size_t numSequences, const std::vector<size_t> &numActionsAtEachInfoSet,
      const std::vector<size_t> &numSequencesBeforeEachInfoSet, double noise)
      : RegretMatchingTable::RegretMatchingTable(
            numSequences, numActionsAtEachInfoSet,
            numSequencesBeforeEachInfoSet),
        randomEngine_(63547654), noise_(noise) {}
  virtual ~PerturbedPolicyRegretMatchingTable() {}

  virtual std::vector<Numeric> policy(const size_t &I) const override {
    Numeric sum = 0.0;
    const auto numActions = (*numActionsAtEachInfoSet_)[I];
    const auto baseIndex = (*numSequencesBeforeEachInfoSet_)[I];

    std::vector<Numeric> perturbedRegrets(numActions);
    for (size_t i = 0; i < numActions; ++i) {
      const int noiseSign = flipCoin(0.5, &randomEngine_) ? 1 : -1;
      perturbedRegrets[i] = table_[baseIndex + i] + noiseSign * noise_;
    }

    std::vector<Numeric> policy_(numActions);
    for (size_t i = 0; i < numActions; ++i) {
      sum += perturbedRegrets[i] > 0.0 ? perturbedRegrets[i] : 0.0;
      policy_[i] = 1.0 / numActions;
    }
    if (sum > 0) {
      for (size_t i = 0; i < numActions; ++i) {
        policy_[i] =
            perturbedRegrets[i] > 0.0 ? perturbedRegrets[i] / sum : 0.0;
      }
    }

    return policy_;
  }

protected:
  mutable std::mt19937 randomEngine_;
  const double noise_;
};

class PerturbedTableRegretMatchingTable : public RegretMatchingTable {
public:
  PerturbedTableRegretMatchingTable(
      size_t numSequences, const std::vector<size_t> &numActionsAtEachInfoSet,
      const std::vector<size_t> &numSequencesBeforeEachInfoSet, double noise)
      : RegretMatchingTable::RegretMatchingTable(
            numSequences, numActionsAtEachInfoSet,
            numSequencesBeforeEachInfoSet),
        randomEngine_(63547654), noise_(noise) {}
  virtual ~PerturbedTableRegretMatchingTable() {}

  virtual void update(const std::pair<size_t, size_t> &sequence,
      Numeric regretValue) override {
    const int noiseSign = flipCoin(0.5, &randomEngine_) ? 1 : -1;
    RegretMatchingTable::update(sequence, regretValue + noiseSign * noise_);
  }

protected:
  mutable std::mt19937 randomEngine_;
  const double noise_;
};


namespace MatchingPennies {

class MatchingPenniesHistory : public StringHistory {
public:
  MatchingPenniesHistory()
      : StringHistory::StringHistory(
            std::vector<std::string>{"l", "L", "r", "R"}) {}
  virtual ~MatchingPenniesHistory() {}

  virtual bool
  suffixIsLegal(const std::string &candidateString) const override {
    return ((isEmpty() && (candidateString == "l" || candidateString == "r")) ||
            ((last() == "l" || last() == "r") &&
             ((candidateString == "R") || (candidateString == "L"))));
  }
  virtual size_t actor() const { return isEmpty() ? 0 : 1; }
  virtual size_t lastActor() const {
    return last() == "l" || last() == "r" ? 0 : 1;
  }
  virtual std::string action(size_t player) const { return state_[player]; }
  virtual size_t legalActionIndex(size_t player) const {
    return ((action(player) == "l" || action(player) == "L") ? 0 : 1);
  }
};

class MatchingPennies : public HistoryTreeNode<Numeric, std::string> {
public:
  MatchingPennies(const std::unordered_map<std::string, int> &utilsForPlayer1,
                  Numeric player1Strat, Numeric player2Strat)
      : HistoryTreeNode<Numeric, std::string>::HistoryTreeNode(
            static_cast<History<std::string> *>(new MatchingPenniesHistory())),
        strats_({player1Strat, player2Strat}),
        utilsForPlayer1_(utilsForPlayer1) {}
  virtual ~MatchingPennies() {}

protected:
  virtual Numeric terminalValue() override {
    return utilsForPlayer1_.at(
        static_cast<const StringHistory *>(history())->toString());
  }
  virtual Numeric interiorValue() override {
    Numeric ev = 0.0;
    history_->eachSuccessor([&](size_t, size_t legalSuccessorIndex) {
      const auto actor =
          static_cast<const MatchingPenniesHistory *>(history())->lastActor();
      const auto probAction =
          legalSuccessorIndex == 0 ? strats_[actor] : 1 - strats_[actor];
      ev += probAction * value();
      return false;
    });
    return ev;
  }

protected:
  std::vector<Numeric> strats_;
  std::unordered_map<std::string, int> utilsForPlayer1_;
};

class BrForMatchingPennies
    : public HistoryTreeNode<std::vector<Numeric>, std::string> {
public:
  BrForMatchingPennies(const std::vector<std::vector<int>> &utilsForPlayer1,
                       const std::vector<std::vector<Numeric>> &stratProfile)
      : HistoryTreeNode<std::vector<Numeric>, std::string>::HistoryTreeNode(
            static_cast<History<std::string> *>(new MatchingPenniesHistory())),
        reachProbProfile_({{1.0, 1.0}, {1.0, 1.0}}),
        strategyProfile_(&stratProfile), brProfile_({{1.0, 0.0}, {1.0, 0.0}}),
        utilsForPlayer1_(utilsForPlayer1), i_(0) {}
  virtual ~BrForMatchingPennies() {}

  virtual std::vector<Numeric> valueProfile() {
    const auto u1Vec = value();
    i_ = (i_ + 1) % brProfile_.size();
    const auto u2Vec = value();
    i_ = (i_ + 1) % brProfile_.size();

    std::vector<Numeric> brValues(brProfile_.size());
    for (size_t i = 0; i < u1Vec.size(); ++i) {
      if (i == 0 || u1Vec[i] > brValues[0]) {
        brValues[0] = u1Vec[i];
      }
      if (i == 0 || u2Vec[i] > brValues[1]) {
        brValues[1] = u2Vec[i];
      }
    }
    return brValues;
  }

  virtual double averageExploitability() {
    const auto brValues = valueProfile();
    DEBUG_VARIABLE("%lg", brValues[0]);
    DEBUG_VARIABLE("%lg", brValues[1]);
    return (brValues[0] + brValues[1]) / 2.0;
  }

  virtual const std::vector<std::vector<Numeric>> &strategyProfile() const {
    return brProfile_;
  }

protected:
  virtual std::vector<Numeric> terminalValue() override {
    int sign = 1;
    size_t not_i = 1;
    const auto myChoice = static_cast<const MatchingPenniesHistory *>(history())
                              ->legalActionIndex(i_);
    std::function<Numeric(size_t)> u =
        [this, &myChoice](size_t opponentChoice) {
          return utilsForPlayer1_.at(myChoice).at(opponentChoice);
        };
    if (i_ == 1) {
      sign = -1;
      not_i = 0;
      u = [this, &myChoice](size_t opponentChoice) {
        return utilsForPlayer1_.at(opponentChoice).at(myChoice);
      };
    }
    std::vector<Numeric> values(reachProbProfile_[not_i].size(), 0.0);
    for (size_t opponentChoice = 0;
         opponentChoice < reachProbProfile_[not_i].size(); ++opponentChoice) {
      if (reachProbProfile_[not_i][opponentChoice] > 0.0) {
        values[opponentChoice] =
            (sign * reachProbProfile_[not_i][opponentChoice] *
             u(opponentChoice));
      }
    }
    return values;
  }
  virtual std::vector<Numeric> interiorValue() override {
    const auto actor =
        static_cast<const MatchingPenniesHistory *>(history())->actor();
    const auto &sigma_I = (*strategyProfile_)[actor];
    return (actor != i_) ? opponentValue(actor, sigma_I)
                         : myValue(actor, sigma_I);
  }

  virtual std::vector<Numeric>
  opponentValue(size_t actor, const std::vector<Numeric> &sigma_I) {
    std::vector<Numeric> counterfactualValue;
    reachProbProfile_[actor] =
        copyAndReturnAfter(reachProbProfile_[actor], [&]() {
          history_->eachSuccessor([&](size_t, size_t legalSuccessorIndex) {
            reachProbProfile_[actor][legalSuccessorIndex] =
                reachProbProfile_[actor][legalSuccessorIndex] *
                sigma_I[legalSuccessorIndex];
            if (legalSuccessorIndex == (sigma_I.size() - 1)) {
              counterfactualValue = value();
            }
            return false;
          });
        });
    return counterfactualValue;
  }

  virtual std::vector<Numeric> myValue(size_t actor,
                                       const std::vector<Numeric> &sigma_I) {
    std::vector<Numeric> actionVals(sigma_I.size(), 0.0);
    history_->eachSuccessor([&](size_t, size_t legalSuccessorIndex) {
      const std::vector<Numeric> valsForAllOpponentChoices = value();
      for (size_t i = 0; i < valsForAllOpponentChoices.size(); ++i) {
        actionVals[legalSuccessorIndex] += valsForAllOpponentChoices[i];
      }
      return false;
    });

    Numeric bestValueSoFar;
    history_->eachLegalSuffix(
        [&](std::string, size_t, size_t legalSuccessorIndex) {
          if (legalSuccessorIndex == 0 ||
              actionVals[legalSuccessorIndex] > bestValueSoFar) {
            bestValueSoFar = actionVals[legalSuccessorIndex];
            brProfile_[actor][legalSuccessorIndex] = 1.0;
          } else {
            brProfile_[actor][legalSuccessorIndex] = 0.0;
          }
          return false;
        });
    return actionVals;
  }

protected:
  // Player / action
  std::vector<std::vector<Numeric>> reachProbProfile_;
  const std::vector<std::vector<Numeric>> *strategyProfile_;
  std::vector<std::vector<Numeric>> brProfile_;
  const std::vector<std::vector<int>> utilsForPlayer1_;
  size_t i_;
};

#include <unistd.h>

template <typename InformationSet, typename Sequence, typename Regret>
class CfrForMatchingPennies : public HistoryTreeNode<Numeric, std::string> {
public:
  CfrForMatchingPennies(
      const std::vector<std::vector<int>> &utilsForPlayer1,
      std::vector<PolicyGenerator<InformationSet, Sequence, Regret> *>
          &&policyGeneratorProfile)
      : HistoryTreeNode<Numeric, std::string>::HistoryTreeNode(
            static_cast<History<std::string> *>(new MatchingPenniesHistory())),
        reachProbProfile_({{1.0, 1.0}, {1.0, 1.0}}),
        policyGeneratorProfile_(std::move(policyGeneratorProfile)),
        cumulativeAverageStrategyProfile_({{0, 0}, {0, 0}}),
        utilsForPlayer1_(utilsForPlayer1), i_(0) {}
  virtual ~CfrForMatchingPennies() {
    for (auto &policyGenerator : policyGeneratorProfile_) {
      if (policyGenerator) {
        delete policyGenerator;
      }
    }
  }

  virtual void doIterations(size_t numIterations) {
    for (size_t t = 0; t < numIterations; ++t) {
      doIteration();
    }
  }

  virtual void doIteration() {
    value();
    i_ = (i_ + 1) % cumulativeAverageStrategyProfile_.size();
  }

  virtual double averageExploitability() const {
    const auto avgStrat = strategyProfile();
    auto br = BrForMatchingPennies(utilsForPlayer1_, avgStrat);
    return br.averageExploitability();
  }

  virtual std::vector<std::vector<Numeric>> strategyProfile() const {
    std::vector<std::vector<Numeric>> s;
    s.reserve(cumulativeAverageStrategyProfile_.size());
    for (const auto &elem : cumulativeAverageStrategyProfile_) {
      s.push_back(normalized(elem));
    }
    return s;
  }

protected:
  virtual Numeric terminalValue() override {
    int sign = 1;
    size_t not_i = 1;
    const auto myChoice = static_cast<const MatchingPenniesHistory *>(history())
                              ->legalActionIndex(i_);
    std::function<Numeric(size_t)> u =
        [this, &myChoice](size_t opponentChoice) {
          return utilsForPlayer1_.at(myChoice).at(opponentChoice);
        };
    DEBUG_VARIABLE("%zu", i_);
    if (i_ == 1) {
      sign = -1;
      not_i = 0;
      u = [this, &myChoice](size_t opponentChoice) {
        return utilsForPlayer1_.at(opponentChoice).at(myChoice);
      };
    }
    Numeric value = 0.0;
    for (size_t opponentChoice = 0; opponentChoice < reachProbProfile_[not_i].size(); ++opponentChoice) {
      DEBUG_VARIABLE("%lg", reachProbProfile_[not_i][opponentChoice]);
      DEBUG_VARIABLE("%lg", u(opponentChoice));
      value += (sign * reachProbProfile_[not_i][opponentChoice] * u(opponentChoice));
      DEBUG_VARIABLE("%lg", values[opponentChoice]);
    }
    return value;
  }
  virtual Numeric interiorValue() override {
    const auto actor = static_cast<const MatchingPenniesHistory *>(history())->actor();
    const auto sigma_I = policyGeneratorProfile_[actor]->policy(0);
    return (actor != i_) ? opponentValue(actor, sigma_I)
                         : myValue(actor, sigma_I);
  }

  virtual Numeric opponentValue(size_t actor, const std::vector<Numeric> &sigma_I) {
    Numeric counterfactualValue;
    DEBUG_VARIABLE("%zu", i_);
    reachProbProfile_[actor] =
        copyAndReturnAfter(reachProbProfile_[actor], [&]() {
          history_->eachSuccessor([&](size_t, size_t legalSuccessorIndex) {
            DEBUG_VARIABLE("%lg", sigma_I[legalSuccessorIndex]);
            assert(sigma_I[legalSuccessorIndex] >= 0.0);
            reachProbProfile_[actor][legalSuccessorIndex] *= sigma_I[legalSuccessorIndex];
            cumulativeAverageStrategyProfile_[actor][legalSuccessorIndex] += reachProbProfile_[actor][legalSuccessorIndex];
            if (legalSuccessorIndex == reachProbProfile_.size() - 1) {
              counterfactualValue = value();
            }
            return false;
          });
        });
    return counterfactualValue;
  }

  virtual Numeric myValue(size_t actor, const std::vector<Numeric> &sigma_I) {
    std::vector<Numeric> actionVals;
    actionVals.reserve(sigma_I.size());
    Numeric counterfactualValue = 0.0;
    reachProbProfile_[actor] =
        copyAndReturnAfter(reachProbProfile_[actor], [&]() {
          history_->eachSuccessor([&](size_t, size_t legalSuccessorIndex) {
            reachProbProfile_[actor][legalSuccessorIndex] *= sigma_I[legalSuccessorIndex];
            actionVals.push_back(value());
            counterfactualValue += actionVals.back() * sigma_I[legalSuccessorIndex];
            return false;
          });
        });
    history_->eachLegalSuffix(
        [&](std::string, size_t, size_t legalSuccessorIndex) {
          policyGeneratorProfile_[actor]->update(
              std::make_pair(0, legalSuccessorIndex),
              actionVals[legalSuccessorIndex] - counterfactualValue);
          return false;
        });
    return counterfactualValue;
  }

protected:
  // Player / action
  std::vector<std::vector<Numeric>> reachProbProfile_;
  std::vector<PolicyGenerator<InformationSet, Sequence, Regret> *>
      policyGeneratorProfile_;
  std::vector<std::vector<Numeric>> cumulativeAverageStrategyProfile_;
  const std::vector<std::vector<int>> utilsForPlayer1_;
  size_t i_;
};

const size_t numSequences = 2;
const std::vector<size_t> numActionsAtEachInfoSet{2};
const std::vector<size_t> numSequencesBeforeEachInfoSet{0};
}

using namespace MatchingPennies;

SCENARIO("Playing matching pennies") {
  GIVEN("Terminal values") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 1}, {"l -> R", -1}, {"r -> L", -1}, {"r -> R", 1}};
    GIVEN("0.5 strategy for player 1") {
      const Numeric strat1 = 0.5;
      GIVEN("0.5 strategy for player 2") {
        const Numeric strat2 = 0.5;
        THEN("The value of matching pennies for player 1 is computed "
             "correctly") {
          class MatchingPennies patient(utilsForPlayer1, strat1, strat2);
          REQUIRE(patient.value() == 0.0);
        }
      }
      GIVEN("0.0 strategy for player 2") {
        const Numeric strat2 = 0.0;
        THEN("The value of matching pennies for player 1 is computed "
             "correctly") {
          class MatchingPennies patient(utilsForPlayer1, strat1, strat2);
          REQUIRE(patient.value() == 0.0);
        }
      }
    }
    GIVEN("0.1 strategy for player 1") {
      const Numeric strat1 = 0.1;
      GIVEN("0.1 strategy for player 2") {
        const Numeric strat2 = 0.1;
        THEN("The value of matching pennies for player 1 is computed "
             "correctly") {
          class MatchingPennies patient(utilsForPlayer1, strat1, strat2);
          REQUIRE(patient.value() == Approx(0.1 * 0.1 * 1 + 0.1 * 0.9 * -1 +
                                            0.9 * 0.9 * 1 + 0.9 * 0.1 * -1));
        }
      }
      GIVEN("0.6 strategy for player 2") {
        const Numeric strat2 = 0.6;
        THEN("The value of matching pennies for player 1 is computed "
             "correctly") {
          class MatchingPennies patient(utilsForPlayer1, strat1, strat2);
          REQUIRE(patient.value() == Approx(0.1 * 0.6 * 1 + 0.1 * 0.4 * -1 +
                                            0.9 * 0.4 * 1 + 0.9 * 0.6 * -1));
        }
      }
    }
  }
}

SCENARIO("CFR on matching pennies") {
  const auto policyGeneratorProfileFactory = [&]() {
    return std::vector<
        PolicyGenerator<size_t, std::pair<size_t, size_t>, Numeric> *>{
        new RegretMatchingTable(numSequences, numActionsAtEachInfoSet,
                                       numSequencesBeforeEachInfoSet),
        new RegretMatchingTable(numSequences, numActionsAtEachInfoSet,
                                       numSequencesBeforeEachInfoSet)};
  };
  GIVEN("Traditional terminal values") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -1}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
        new RegretMatchingPlusTable(numSequences, numActionsAtEachInfoSet,
                                       numSequencesBeforeEachInfoSet),
        new RegretMatchingPlusTable(numSequences, numActionsAtEachInfoSet,
                                       numSequencesBeforeEachInfoSet)};
  };
  GIVEN("Traditional terminal values") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -1}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
        new PerturbedPolicyRegretMatchingTable(numSequences, numActionsAtEachInfoSet,
                                         numSequencesBeforeEachInfoSet, 0.1),
        new PerturbedPolicyRegretMatchingTable(numSequences, numActionsAtEachInfoSet,
                                         numSequencesBeforeEachInfoSet, 0.1)};
  };
  GIVEN("Traditional terminal values") {
    std::vector<std::vector<int>> utilsForPlayer1{{1, -1}, {-1, 1}};
    THEN("CFR finds the equilibrium properly") {
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
      CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric> patient(
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
                  numSequences, numActionsAtEachInfoSet,
                  numSequencesBeforeEachInfoSet, 1),
              new PerturbedPolicyRegretMatchingTable(
                  numSequences, numActionsAtEachInfoSet,
                  numSequencesBeforeEachInfoSet, 1)};
        };
        CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric>
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

void runPerturbedCfrWithNoiseAndExploitabilityThreshold(double noise, double exploitability = 5e-5) {
  std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-4, 3}};
  const auto altPolicyGeneratorProfileFactory = [&]() {
    return std::vector<
        PolicyGenerator<size_t, std::pair<size_t, size_t>, Numeric> *>{
      new PerturbedTableRegretMatchingTable(
          numSequences, numActionsAtEachInfoSet,
          numSequencesBeforeEachInfoSet, noise),
              new PerturbedTableRegretMatchingTable(
                  numSequences, numActionsAtEachInfoSet,
                  numSequencesBeforeEachInfoSet, noise)};
  };
  CfrForMatchingPennies<size_t, std::pair<size_t, size_t>, Numeric>
              patient(utilsForPlayer1, altPolicyGeneratorProfileFactory());
  size_t t = 1;
  while (true) {
    patient.doIteration();
    if (patient.averageExploitability() < exploitability || t % 100000 == 0) {
      printf("%lg      %zu    %lg\n", noise, t, patient.averageExploitability());

      if (patient.averageExploitability() < exploitability) {
        return;
      }
    }
    ++t;
  }
}

//SCENARIO("Show impact of noise on CFR convergence") {
//  runPerturbedCfrWithNoiseAndExploitabilityThreshold(0);
//  double noise = 0.001;
//  for (size_t i = 0; i < 6; ++i) {
//    runPerturbedCfrWithNoiseAndExploitabilityThreshold(noise);
//    noise *= 10;
//  }
//}
