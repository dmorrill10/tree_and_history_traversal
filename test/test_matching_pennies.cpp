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

class MatchingPenniesTree : public HistoryTreeNode<double, std::string> {
public:
  MatchingPenniesTree(
      const std::unordered_map<std::string, int> &utilsForPlayer1,
      HistoryTreeNode::CombineSuccessorValuesFunction &&csvf,
      HistoryTreeNode::AccumulatedValueInitializer &&avi,
      HistoryTreeNode::BeforeTakingActionFunction &&btaf = [](
          const History<std::string> * /*parentSequence*/,
          std::string /*action*/, size_t /*nextActionIndex*/,
          size_t /*nextLegalActionIndex*/
          ) { return false; },
      HistoryTreeNode::AfterAllActionsFunction &&afterAllActionsFn =
          [](const HistoryTreeNode * /*self*/, const double & /*finalValue*/,
             const std::vector<double> &) { return; },
      std::function<double(double terminalValue)> &&valueAtTerminal =
          [](double v) { return v; })
      : HistoryTreeNode(
            new StringHistory(
                std::vector<std::string>{"l", "L", "r", "R"},
                [](const StringHistory &prefix,
                   const std::string &candidateString) {
                  return (
                      (prefix.isEmpty() &&
                       (candidateString == "l" || candidateString == "r")) ||
                      ((prefix.last() == "l") && ((candidateString == "R") ||
                                                  (candidateString == "L"))) ||
                      ((prefix.last() == "r") &&
                       ((candidateString == "R") || (candidateString == "L"))));
                }),
            [this, valueAtTerminal]() {
              return valueAtTerminal(
                  utilsForPlayer1_[static_cast<StringHistory *>(this->history_)
                                       ->toString()]);
            },
            std::move(csvf), std::move(avi), std::move(btaf),
            std::move(afterAllActionsFn)),
        utilsForPlayer1_(utilsForPlayer1) {}
  virtual ~MatchingPenniesTree() {}

protected:
  std::unordered_map<std::string, int> utilsForPlayer1_;
};

class MatchingPennies : public MatchingPenniesTree {
public:
  MatchingPennies(const std::unordered_map<std::string, int> &utilsForPlayer1,
                  double player1Strat, double player2Strat)
      : MatchingPenniesTree::MatchingPenniesTree(
            utilsForPlayer1,
            [this](double *accumulatedValue,
                   const History<std::string> *parentSequence,
                   std::string /*action*/, size_t /*nextActionIndex*/,
                   size_t nextLegalActionIndex, const double &valueOnSequence) {
              if (parentSequence->isEmpty()) {
                const auto probAction = nextLegalActionIndex == 0
                                            ? player1Strat_
                                            : 1 - player1Strat_;
                (*accumulatedValue) += probAction * valueOnSequence;
              } else {
                const auto probAction = nextLegalActionIndex == 0
                                            ? player2Strat_
                                            : 1 - player2Strat_;
                (*accumulatedValue) += probAction * valueOnSequence;
              }
              return false;
            },
            [](const HistoryTreeNode *) { return 0.0; }),
        player1Strat_(player1Strat), player2Strat_(player2Strat) {}

protected:
  double player1Strat_;
  double player2Strat_;
};
}

using namespace MatchingPennies;

template <typename Value, typename Symbol> class Cfr {
public:
  static std::vector<double> regretMatching(const std::vector<double> regrets) {
    std::vector<double> probs(regrets.size(), 1.0 / regrets.size());
    auto sum = 0.0;
    for (auto r : regrets) {
      sum += r > 0 ? r : 0.0;
    }
    if (sum > 0.0) {
      for (size_t r = 0; r < regrets.size(); ++r) {
        probs[r] = regrets[r] > 0.0 ? regrets[r] / sum : 0.0;
      }
    }
    return probs;
  }
  static double regretMatchingProb(const std::vector<double> regrets,
                                   size_t index) {
    auto sum = 0.0;
    for (auto r : regrets) {
      sum += r > 0 ? r : 0.0;
    }
    if (sum > 0.0) {
      return regrets[index] > 0.0 ? regrets[index] / sum : 0.0;
    } else {
      return 1.0 / regrets.size();
    }
  }

  typedef HistoryTreeNode<Value, Symbol> Game;
  typedef std::function<
    Game*(
      std::function<Value(const HistoryTreeNode<Value, Symbol> *)> &&avi,
      std::function<bool(const History<Symbol> *parentSequence,
                             Symbol action, size_t nextActionIndex,
                             size_t nextLegalActionIndex)> &&btaf,
      std::function<bool(
      Value *accumulatedValue, const History<Symbol> *parentSequence,
      Symbol action, size_t nextActionIndex, size_t nextLegalActionIndex,
      const Value &valueOnSequence)> &&csvf,
      std::function<void(
      const HistoryTreeNode<Value, Symbol> *self, const Value &finalValue,
      const std::vector<Value> &childValues)> &&afterAllActionsFn,
      std::function<Value(Value terminalValue)> &&terminalValueFunction)> GameFactory;
  Cfr(GameFactory &&gameFactory, GameInformation *&&gameInfo,
      // @todo Sampling info
      size_t numIterations)
      : actionProbabilityProfile_(gameInfo->numPlayers()),
        cumulativeRegretProfile_(gameInfo->numPlayers()),
        averageStrategyProfile_(gameInfo->numPlayers()), currentPlayer_(0),
        gameInfo_(gameInfo) {
    for (size_t player = 0; player < gameInfo_->numPlayers(); ++player) {
      // @todo Use sampling width instead
      actionProbabilityProfile_[player].push_back(1.0);
      for (size_t infoSet = 0; infoSet < gameInfo_->numInfoSets(player);
           ++infoSet) {
        cumulativeRegretProfile_[player].emplace_back(0.0);
        averageStrategyProfile_[player].emplace_back(0.0);
      }
    }
    // @todo This is matching pennies specific right now
    game_ = gameFactory(
        [](const Game *) { return 0.0; },
        [this](const History<Symbol> *parentSequence, Symbol /*action*/,
               size_t /*nextActionIndex*/, size_t nextLegalActionIndex) {
          if (parentSequence->isEmpty()) {
            const auto probAction = regretMatchingProb(
                cumulativeRegretProfile_[0], nextLegalActionIndex);
            actionProbabilityProfile_[0][0] *= probAction;
          } else {
            const auto probAction = regretMatchingProb(
                cumulativeRegretProfile_[1], nextLegalActionIndex);
            actionProbabilityProfile_[1][0] *= probAction;
          }
          return false;
        },
        [this](Value *accumulatedValue, const History<Symbol> *parentSequence,
               Symbol /*action*/, size_t /*nextActionIndex*/,
               size_t nextLegalActionIndex, const Value &valueOnSequence) {
          if (parentSequence->isEmpty()) {
            const auto probAction = regretMatchingProb(
                cumulativeRegretProfile_[0], nextLegalActionIndex);
            (*accumulatedValue) += probAction * valueOnSequence;
            actionProbabilityProfile_[0][0] /= probAction;
          } else {
            const auto probAction = regretMatchingProb(
                cumulativeRegretProfile_[1], nextLegalActionIndex);
            (*accumulatedValue) += probAction * valueOnSequence;
            actionProbabilityProfile_[1][0] /= probAction;
          }
          return false;
        },
        [this](const Game *self, const Value &finalValue,
               const std::vector<Value> &childValues) {
          if ((self->history()->isEmpty() && currentPlayer_ == 0) ||
              (!self->history()->isEmpty() && currentPlayer_ == 1)) {
            std::vector<double> probs =
                regretMatching(cumulativeRegretProfile_[currentPlayer_]);
            auto sum = 0.0;
            for (size_t a = 0; a < childValues.size(); ++a) {
              cumulativeRegretProfile_[currentPlayer_][a] +=
                  (childValues[a] - finalValue);
              averageStrategyProfile_[currentPlayer_][a] +=
                  actionProbabilityProfile_[currentPlayer_][0] * probs[a];
              sum += averageStrategyProfile_[currentPlayer_][a];
            }
            if (sum > 0.0) {
              for (auto &s : averageStrategyProfile_[currentPlayer_]) {
                s /= sum;
              }
            }
            else {
              for (auto &s : averageStrategyProfile_[currentPlayer_]) {
                s = 1.0 / averageStrategyProfile_[currentPlayer_].size();
              }
            }
          }
        },
        [this](Value rawUtil) {
          auto prob = 1.0;
          for (size_t player = 0; player < actionProbabilityProfile_.size();
               ++player) {
            if (player != currentPlayer_) {
              prob *= actionProbabilityProfile_[player][0];
            }
          }
          return prob * rawUtil;
        });
    for (size_t t = 0; t < numIterations; ++t) {
      game_->value();
      currentPlayer_ = (currentPlayer_ + 1) % gameInfo_->numPlayers();
    }
  }
  virtual ~Cfr() {
    delete gameInfo_;
    delete game_;
  }

  const std::vector<std::vector<double>> profile() const { return averageStrategyProfile_; }

protected:
  // @todo Make these contiguous?
  std::vector<std::vector<double>> actionProbabilityProfile_;
  std::vector<std::vector<double>> cumulativeRegretProfile_;
  std::vector<std::vector<double>> averageStrategyProfile_;
  size_t currentPlayer_;
  HistoryTreeNode<Value, Symbol> *game_;
  GameInformation *gameInfo_;
};

SCENARIO("Playing matching pennies") {
  GIVEN("Terminal values") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 1}, {"l -> R", -1}, {"r -> L", -1}, {"r -> R", 1}};
    GIVEN("0.5 strategy for player 1") {
      const double strat1 = 0.5;
      GIVEN("0.5 strategy for player 2") {
        const double strat2 = 0.5;
        THEN("The value of matching pennies for player 1 is computed "
             "correctly") {
          class MatchingPennies patient(utilsForPlayer1, strat1, strat2);
          REQUIRE(patient.value() == 0.0);
        }
      }
      GIVEN("0.0 strategy for player 2") {
        const double strat2 = 0.0;
        THEN("The value of matching pennies for player 1 is computed "
             "correctly") {
          class MatchingPennies patient(utilsForPlayer1, strat1, strat2);
          REQUIRE(patient.value() == 0.0);
        }
      }
    }
    GIVEN("0.1 strategy for player 1") {
      const double strat1 = 0.1;
      GIVEN("0.1 strategy for player 2") {
        const double strat2 = 0.1;
        THEN("The value of matching pennies for player 1 is computed "
             "correctly") {
          class MatchingPennies patient(utilsForPlayer1, strat1, strat2);
          REQUIRE(patient.value() == Approx(0.1 * 0.1 * 1 + 0.1 * 0.9 * -1 +
                                            0.9 * 0.9 * 1 + 0.9 * 0.1 * -1));
        }
      }
      GIVEN("0.6 strategy for player 2") {
        const double strat2 = 0.6;
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
  GIVEN("Terminal values") {
    std::unordered_map<std::string, int> utilsForPlayer1{
        {"l -> L", 1}, {"l -> R", -1}, {"r -> L", -1}, {"r -> R", 1}};
    THEN("CFR finds the equilibrium properly after 100 iterations") {
      Cfr<double, std::string> patient(
          [&utilsForPlayer1](
              HistoryTreeNode<double, std::string>::AccumulatedValueInitializer
                  &&avi,
              HistoryTreeNode<double, std::string>::BeforeTakingActionFunction
                  &&btaf,
              HistoryTreeNode<
                  double, std::string>::CombineSuccessorValuesFunction &&csvf,
              HistoryTreeNode<double, std::string>::AfterAllActionsFunction
                  &&afterAllActionsFn,
              std::function<double(double terminalValue)>
                  &&terminalValueFunction) {
            return static_cast<HistoryTreeNode<double, std::string> *>(
                new MatchingPenniesTree(utilsForPlayer1, std::move(csvf),
                                        std::move(avi), std::move(btaf),
                                        std::move(afterAllActionsFn),
                                        std::move(terminalValueFunction)));
          },
          new MatchingPenniesInformation(), 100);
      REQUIRE(patient.profile()[0][0] == 0.5);
      REQUIRE(patient.profile()[1][0] == 0.5);
    }
  }
}
