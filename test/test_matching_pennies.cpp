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

typedef double Numeric;

class GameInformation {
protected:
  GameInformation() {}

public:
  virtual ~GameInformation() {}

  virtual size_t numPlayers() const = 0;
  virtual size_t numSequences(size_t player) const = 0;
  virtual size_t numInfoSets(size_t player) const = 0;
  virtual size_t numActions(size_t player, size_t infoSetIndex) const = 0;
};

namespace MatchingPennies {

class MatchingPenniesInformation : public GameInformation {
public:
  virtual size_t numPlayers() const { return 2; }
  virtual size_t numSequences(size_t /*player*/) const { return 2; }
  virtual size_t numInfoSets(size_t /*player*/) const { return 1; }
  virtual size_t numActions(size_t /*player*/, size_t /*infoSetIndex*/) const {
    return 2;
  }
};

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
  virtual Numeric terminalValueFunction() override {
    return utilsForPlayer1_.at(
        static_cast<const StringHistory *>(history())->toString());
  }
  virtual Numeric interiorValueFunction() override {
    Numeric ev = 0.0;
    history_->eachSuccessor([this, &ev](size_t, size_t legalSuccessorIndex) {
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

class CfrForMatchingPennies : public HistoryTreeNode<Numeric, std::string> {
public:
  static std::vector<Numeric> regretMatching(std::vector<Numeric> regrets) {
    std::vector<Numeric> probs(regrets.size(), 1.0 / regrets.size());
    Numeric sum = 0.0;
    for (auto r : regrets) {
      sum += r > 0.0 ? r : 0.0;
    }
    if (sum > 0.0) {
      DEBUG_VARIABLE("%lg", sum);
      for (size_t rIndex = 0; rIndex < regrets.size(); ++rIndex) {
        DEBUG_VARIABLE("%lg", regrets[rIndex]);
        probs[rIndex] =
            regrets[rIndex] > 0.0 ? exp(log(regrets[rIndex]) - log(sum)) : 0.0;
        DEBUG_VARIABLE("%lg", probs[rIndex]);
      }
    }
    return probs;
  }
  static std::vector<Numeric> normalized(const std::vector<Numeric> &v) {
    Numeric sum = 0.0;
    std::vector<Numeric> n(v.size(), 1.0 / v.size());
    for (auto elem : v) {
      DEBUG_VARIABLE("%lg", elem);
      sum += elem;
      DEBUG_VARIABLE("%lg", sum);
    }
    if (sum > 0) {
      DEBUG_VARIABLE("%lg", sum);
      for (size_t i = 0; i < v.size(); ++i) {
        n[i] = v[i] > 0.0 ? exp(log(v[i]) - log(sum)) : 0.0;
        DEBUG_VARIABLE("%lg", n[i]);
      }
    }
    return n;
  }
  CfrForMatchingPennies(
      const std::unordered_map<std::string, int> &utilsForPlayer1)
      : HistoryTreeNode<Numeric, std::string>::HistoryTreeNode(
            static_cast<History<std::string> *>(new MatchingPenniesHistory())),
        piSigma_({1.0, 1.0}), regretProfile_({{0, 0}, {0, 0}}),
        regretDataProfile_({{0, 0}, {0, 0}}),
        cumulativeAverageStrategyProfile_({{0, 0}, {0, 0}}),
        utilsForPlayer1_(utilsForPlayer1), i_(0) {}
  virtual ~CfrForMatchingPennies() {}

  virtual void doIterations(size_t numIterations) {
    for (size_t t = 0; t < numIterations; ++t) {
      DEBUG_HEADER2("");
      value();
      for (size_t j = 0; j < regretDataProfile_[i_].size(); ++j) {
        regretProfile_[i_][j] += regretDataProfile_[i_][j];
        regretDataProfile_[i_][j] = 0.0;
      }
      DEBUG_VARIABLE("%zu", t);
      DEBUG_VARIABLE("%zu", i_);
      DEBUG_VARIABLE("%lg", regretProfile_[0][0]);
      DEBUG_VARIABLE("%lg", regretProfile_[0][1]);
      DEBUG_VARIABLE("%lg", regretProfile_[1][0]);
      DEBUG_VARIABLE("%lg", regretProfile_[1][1]);
      DEBUG_VARIABLE("%lg", cumulativeAverageStrategyProfile_[0][0]);
      DEBUG_VARIABLE("%lg", cumulativeAverageStrategyProfile_[0][1]);
      DEBUG_VARIABLE("%lg", cumulativeAverageStrategyProfile_[1][0]);
      DEBUG_VARIABLE("%lg", cumulativeAverageStrategyProfile_[1][1]);
      i_ = (i_ + 1) % cumulativeAverageStrategyProfile_.size();
    }
  }

  virtual std::vector<std::vector<Numeric>> strategyProfile() const {
    std::vector<std::vector<Numeric>> s;
    s.reserve(cumulativeAverageStrategyProfile_.size());
    for (const auto &elem : cumulativeAverageStrategyProfile_) {
      DEBUG_VARIABLE("%lg", elem[0]);
      DEBUG_VARIABLE("%lg", elem[1]);
      s.push_back(normalized(elem));
    }
    return s;
  }

protected:
  virtual Numeric terminalValueFunction() override {
    const auto not_i = (i_ + 1) % cumulativeAverageStrategyProfile_.size();
    return (
        (piSigma_[not_i] > 0.0)
            ? (piSigma_[not_i] *
               utilsForPlayer1_.at(
                   static_cast<const StringHistory *>(history())->toString()) *
               (i_ == 0 ? 1 : -1))
            : 0.0);
  }
  virtual Numeric interiorValueFunction() override {
    const auto actor =
        static_cast<const MatchingPenniesHistory *>(history())->actor();
    const auto sigma_I = regretMatching(regretProfile_[actor]);
    Numeric counterfactualValue = 0.0;
    if (actor != i_) {
      history_->eachSuccessor([this, &actor, &counterfactualValue, &sigma_I](
          size_t, size_t legalSuccessorIndex) {
        const auto savedPiSigma = piSigma_[actor];
        if (sigma_I[legalSuccessorIndex] > 0.0) {
          piSigma_[actor] =
              exp(log(piSigma_[actor]) + log(sigma_I[legalSuccessorIndex]));
          cumulativeAverageStrategyProfile_[actor][legalSuccessorIndex] +=
              piSigma_[actor];
        } else {
          piSigma_[actor] = 0.0;
        }
        counterfactualValue += value();
        piSigma_[actor] = savedPiSigma;
        return false;
      });
    } else {
      std::vector<Numeric> actionVals(sigma_I.size());
      history_->eachSuccessor(
          [this, &actor, &counterfactualValue, &actionVals, &sigma_I](
              size_t, size_t legalSuccessorIndex) {
            const auto savedPiSigma = piSigma_[actor];
            if (sigma_I[legalSuccessorIndex] > 0.0) {
              piSigma_[actor] =
                  exp(log(piSigma_[actor]) + log(sigma_I[legalSuccessorIndex]));
            } else {
              piSigma_[actor] = 0.0;
            }
            actionVals[legalSuccessorIndex] = value();
            DEBUG_VARIABLE("%lg", actionVals.back());
            DEBUG_VARIABLE("%lg", sigma_I[legalSuccessorIndex]);
            counterfactualValue +=
                actionVals.back() * sigma_I[legalSuccessorIndex];
            piSigma_[actor] = savedPiSigma;
            return false;
          });
      history_->eachLegalSuffix(
          [this, &actor, &counterfactualValue, &actionVals](
              std::string, size_t, size_t legalSuccessorIndex) {
        DEBUG_VARIABLE("%zu", actor);
        DEBUG_VARIABLE("%zu", legalSuccessorIndex);
        DEBUG_VARIABLE("%lg", actionVals[legalSuccessorIndex]);
        DEBUG_VARIABLE("%lg", counterfactualValue);
            regretDataProfile_[actor][legalSuccessorIndex] +=
                actionVals[legalSuccessorIndex] - counterfactualValue;
            DEBUG_VARIABLE("%lg", regretDataProfile_[actor][legalSuccessorIndex]);
            return false;
          });
    }
    return counterfactualValue;
  }

protected:
  // Player / action
  std::vector<double> piSigma_;
  std::vector<std::vector<Numeric>> regretProfile_;
  std::vector<std::vector<Numeric>> regretDataProfile_;
  std::vector<std::vector<Numeric>> cumulativeAverageStrategyProfile_;
  std::unordered_map<std::string, int> utilsForPlayer1_;
  size_t i_;
};

#include <random>

class PerturbedCfrForMatchingPennies : public CfrForMatchingPennies {
public:
  static bool flipCoin(double probTrue, std::mt19937 *randomEngine) {
    assert(randomEngine);
    std::bernoulli_distribution coin(probTrue);
    return coin(*randomEngine);
  }

  PerturbedCfrForMatchingPennies(
      const std::unordered_map<std::string, int> &utilsForPlayer1,
      const double noise)
      : CfrForMatchingPennies::CfrForMatchingPennies(utilsForPlayer1),
        noise_(noise) {
    bool initialized = false;
    for (auto& imap : utilsForPlayer1_) {
      DEBUG_VARIABLE("%d", abs(imap.second));
      if (!initialized || abs(imap.second) > maxCfr_) {
        maxCfr_ = double(abs(imap.second));
        initialized = true;
      }
    }
    maxCfr_ *= 2;
    assert(maxCfr_ > 0.0);
  }
  virtual ~PerturbedCfrForMatchingPennies() {}

  virtual void doIterations(size_t numIterations) override {
    std::mt19937 randomEngine(63547654);
    for (size_t t = 0; t < numIterations; ++t) {
      DEBUG_HEADER2("");
      value();
      for (size_t j = 0; j < regretDataProfile_[i_].size(); ++j) {
        int noiseSign = flipCoin(0.5, &randomEngine) ? 1 : -1;
        regretProfile_[i_][j] += (regretDataProfile_[i_][j] / maxCfr_) + noiseSign * noise_;

        DEBUG_VARIABLE("%lg", (regretDataProfile_[i_][j]));
        assert(std::abs(regretDataProfile_[i_][j] / maxCfr_) <= 1.0);
        DEBUG_VARIABLE("%lg", noiseSign * noise_);

        regretDataProfile_[i_][j] = 0.0;
      }
      DEBUG_VARIABLE("%zu", t);
      DEBUG_VARIABLE("%zu", i_);
      DEBUG_VARIABLE("%lg", regretProfile_[0][0]);
      DEBUG_VARIABLE("%lg", regretProfile_[0][1]);
      DEBUG_VARIABLE("%lg", regretProfile_[1][0]);
      DEBUG_VARIABLE("%lg", regretProfile_[1][1]);
      DEBUG_VARIABLE("%lg", cumulativeAverageStrategyProfile_[0][0]);
      DEBUG_VARIABLE("%lg", cumulativeAverageStrategyProfile_[0][1]);
      DEBUG_VARIABLE("%lg", cumulativeAverageStrategyProfile_[1][0]);
      DEBUG_VARIABLE("%lg", cumulativeAverageStrategyProfile_[1][1]);
      i_ = (i_ + 1) % cumulativeAverageStrategyProfile_.size();
    }
  }

protected:
  const double noise_;
  double maxCfr_;
};
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
  GIVEN("Traditional terminal values") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 1}, {"l -> R", -1}, {"r -> L", -1}, {"r -> R", 1}};
    THEN("CFR finds the equilibrium properly") {
      CfrForMatchingPennies patient(utilsForPlayer1);
      patient.doIterations(10);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5));
    }
  }
  GIVEN("Alternative terminal values #1") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 1}, {"l -> R", -2}, {"r -> L", -1}, {"r -> R", 2}};
    THEN("CFR finds the equilibrium properly") {
      CfrForMatchingPennies patient(utilsForPlayer1);
      patient.doIterations(1000);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5).epsilon(0.01));
      CHECK(patient.strategyProfile()[1][0] == Approx(2.0 / 3).epsilon(0.001));
    }
  }
  GIVEN("Alternative terminal values #2") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 2}, {"l -> R", -2}, {"r -> L", -1}, {"r -> R", 1}};
    THEN("CFR finds the equilibrium properly") {
      CfrForMatchingPennies patient(utilsForPlayer1);
      patient.doIterations(1000);
      CHECK(patient.strategyProfile()[0][0] == Approx(1.0 / 3).epsilon(0.01));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5).epsilon(0.01));
    }
  }
  GIVEN("Alternative terminal values #3") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 2}, {"l -> R", -2}, {"r -> L", -4}, {"r -> R", 3}};
    THEN("CFR finds the equilibrium properly") {
      CfrForMatchingPennies patient(utilsForPlayer1);
      patient.doIterations(1000);
      CHECK(patient.strategyProfile()[0][0] == Approx(7.0 / 11).epsilon(0.01));
      CHECK(patient.strategyProfile()[1][0] == Approx(5 / 11.0).epsilon(0.01));
    }
  }
}

SCENARIO("perturbed CFR on matching pennies") {
  GIVEN("Traditional terminal values") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 1}, {"l -> R", -1}, {"r -> L", -1}, {"r -> R", 1}};
    THEN("CFR finds the equilibrium properly") {
      PerturbedCfrForMatchingPennies patient(utilsForPlayer1, 0.1);
      patient.doIterations(100);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5).epsilon(0.01));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5).epsilon(0.1));
    }
  }
  GIVEN("Alternative terminal values #1") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 1}, {"l -> R", -2}, {"r -> L", -1}, {"r -> R", 2}};
    THEN("CFR finds the equilibrium properly") {
      PerturbedCfrForMatchingPennies patient(utilsForPlayer1, 0.1);
      patient.doIterations(100);
      CHECK(patient.strategyProfile()[0][0] == Approx(0.5).epsilon(0.1));
      CHECK(patient.strategyProfile()[1][0] == Approx(2.0 / 3).epsilon(0.1));
    }
  }
  GIVEN("Alternative terminal values #2") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 2}, {"l -> R", -2}, {"r -> L", -1}, {"r -> R", 1}};
    THEN("CFR finds the equilibrium properly") {
      PerturbedCfrForMatchingPennies patient(utilsForPlayer1, 0.1);
      patient.doIterations(100);
      CHECK(patient.strategyProfile()[0][0] == Approx(1.0 / 3).epsilon(0.1));
      CHECK(patient.strategyProfile()[1][0] == Approx(0.5).epsilon(0.1));
    }
  }
  GIVEN("Alternative terminal values #3") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 2}, {"l -> R", -2}, {"r -> L", -4}, {"r -> R", 3}};
    THEN("CFR finds the equilibrium properly") {
      PerturbedCfrForMatchingPennies patient(utilsForPlayer1, 0.1);
      patient.doIterations(100);
      CHECK(patient.strategyProfile()[0][0] == Approx(7.0 / 11).epsilon(0.1));
      CHECK(patient.strategyProfile()[1][0] == Approx(5 / 11.0).epsilon(0.1));
    }
  }
}
