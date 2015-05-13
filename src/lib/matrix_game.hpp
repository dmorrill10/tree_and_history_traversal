#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <algorithm>

#include <lib/history_tree_node.hpp>
#include <lib/history.hpp>
#include <lib/utils.hpp>
#include <lib/policy_generator.hpp>

extern "C" {
#include <utilities/src/lib/print_debugger.h>
}
namespace TreeAndHistoryTraversal {
namespace MatrixGame {

class MatrixGameHistory : public History::StringHistory {
public:
//  @todo Provide player 1 and player 2 actions
  MatrixGameHistory(std::vector<std::string>&& actions = {"l", "L", "r", "R"})
      : StringHistory::StringHistory(std::move(actions)) {}
  virtual ~MatrixGameHistory() {}

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

class BestResponse
    : public HistoryTreeNode::HistoryTreeNode<std::vector<Utils::Numeric>, std::string> {
public:
  BestResponse(const std::vector<std::vector<int>> &utilsForPlayer1,
                       const std::vector<std::vector<Utils::Numeric>> &stratProfile)
      : HistoryTreeNode<std::vector<Utils::Numeric>, std::string>::HistoryTreeNode(
            static_cast<History::History<std::string> *>(new MatrixGameHistory())),
        reachProbProfile_({{1.0, 1.0}, {1.0, 1.0}}),
        strategyProfile_(&stratProfile), brProfile_({{1.0, 0.0}, {1.0, 0.0}}),
        utilsForPlayer1_(&utilsForPlayer1), i_(0) {}
  virtual ~BestResponse() {}

  virtual std::vector<Utils::Numeric> valueProfile() {
    const auto u1Vec = value();
    i_ = (i_ + 1) % brProfile_.size();
    const auto u2Vec = value();
    i_ = (i_ + 1) % brProfile_.size();

    std::vector<Utils::Numeric> brValues(brProfile_.size());
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

  virtual const std::vector<std::vector<Utils::Numeric>> &strategyProfile() const {
    return brProfile_;
  }

protected:
  virtual std::vector<Utils::Numeric> terminalValue() override {
    int sign = 1;
    size_t not_i = 1;
    const auto myChoice = static_cast<const MatrixGameHistory *>(history())
                              ->legalActionIndex(i_);
    std::function<Utils::Numeric(size_t)> u =
        [this, &myChoice](size_t opponentChoice) {
          return utilsForPlayer1_->at(myChoice).at(opponentChoice);
        };
    if (i_ == 1) {
      sign = -1;
      not_i = 0;
      u = [this, &myChoice](size_t opponentChoice) {
        return utilsForPlayer1_->at(opponentChoice).at(myChoice);
      };
    }
    std::vector<Utils::Numeric> values(reachProbProfile_[not_i].size(), 0.0);
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
  virtual std::vector<Utils::Numeric> interiorValue() override {
    const auto actor =
        static_cast<const MatrixGameHistory *>(history())->actor();
    const auto &sigma_I = (*strategyProfile_)[actor];
    return (actor != i_) ? opponentValue(actor, sigma_I)
                         : myValue(actor, sigma_I);
  }

  virtual std::vector<Utils::Numeric>
  opponentValue(size_t actor, const std::vector<Utils::Numeric> &sigma_I) {
    std::vector<Utils::Numeric> counterfactualValue;
    reachProbProfile_[actor] =
        Utils::copyAndReturnAfter(reachProbProfile_[actor], [&]() {
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

  virtual std::vector<Utils::Numeric> myValue(size_t actor,
                                       const std::vector<Utils::Numeric> &sigma_I) {
    std::vector<Utils::Numeric> actionVals(sigma_I.size(), 0.0);
    history_->eachSuccessor([&](size_t, size_t legalSuccessorIndex) {
      const std::vector<Utils::Numeric> valsForAllOpponentChoices = value();
      for (size_t i = 0; i < valsForAllOpponentChoices.size(); ++i) {
        actionVals[legalSuccessorIndex] += valsForAllOpponentChoices[i];
      }
      return false;
    });

    Utils::Numeric bestValueSoFar;
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
  std::vector<std::vector<Utils::Numeric>> reachProbProfile_;
  const std::vector<std::vector<Utils::Numeric>> *strategyProfile_;
  std::vector<std::vector<Utils::Numeric>> brProfile_;
  const std::vector<std::vector<int>>* utilsForPlayer1_;
  size_t i_;
};

template <typename InformationSet, typename Sequence, typename Regret>
class Cfr : public HistoryTreeNode::HistoryTreeNode<Utils::Numeric, std::string> {
public:
  // @todo Assumes the matrix game has two actions, which should be generalized
  Cfr(
      const std::vector<std::vector<int>> &utilsForPlayer1,
      std::vector<PolicyGenerator::PolicyGenerator<InformationSet, Sequence, Regret> *>
          &&policyGeneratorProfile)
      : HistoryTreeNode<Utils::Numeric, std::string>::HistoryTreeNode(
            static_cast<History::History<std::string> *>(new MatrixGameHistory())),
        reachProbProfile_({{1.0, 1.0}, {1.0, 1.0}}),
        policyGeneratorProfile_(std::move(policyGeneratorProfile)),
        cumulativeAverageStrategyProfile_({{0, 0}, {0, 0}}),
        utilsForPlayer1_(&utilsForPlayer1),
        i_(0),
        averageStrategyProfile_({{1.0, 0}, {1.0, 0}}) {}
  virtual ~Cfr() {
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
    auto br = BestResponse(*utilsForPlayer1_, avgStrat);
    return br.averageExploitability();
  }

  virtual const std::vector<std::vector<Utils::Numeric>>& strategyProfile() const {
    for (size_t i = 0; i < cumulativeAverageStrategyProfile_.size(); ++i) {
      Utils::normalize(cumulativeAverageStrategyProfile_[i], &(averageStrategyProfile_[i]));
    }
    return averageStrategyProfile_;
  }

protected:
  virtual Utils::Numeric terminalValue() override {
    int sign = 1;
    size_t not_i = 1;
    const auto myChoice = static_cast<const MatrixGameHistory *>(history())
                              ->legalActionIndex(i_);
    std::function<Utils::Numeric(size_t)> u =
        [this, &myChoice](size_t opponentChoice) {
          return utilsForPlayer1_->at(myChoice).at(opponentChoice);
        };
    DEBUG_VARIABLE("%zu", i_);
    if (i_ == 1) {
      sign = -1;
      not_i = 0;
      u = [this, &myChoice](size_t opponentChoice) {
        return utilsForPlayer1_->at(opponentChoice).at(myChoice);
      };
    }
    Utils::Numeric value = 0.0;
    for (size_t opponentChoice = 0; opponentChoice < reachProbProfile_[not_i].size(); ++opponentChoice) {
      DEBUG_VARIABLE("%lg", reachProbProfile_[not_i][opponentChoice]);
      DEBUG_VARIABLE("%lg", u(opponentChoice));
      value += (sign * reachProbProfile_[not_i][opponentChoice] * u(opponentChoice));
      DEBUG_VARIABLE("%lg", values[opponentChoice]);
    }
    return value;
  }
  virtual Utils::Numeric interiorValue() override {
    const auto actor = static_cast<const MatrixGameHistory *>(history())->actor();
    const auto sigma_I = policyGeneratorProfile_[actor]->policy(0);
    return (actor != i_) ? opponentValue(actor, sigma_I)
                         : myValue(actor, sigma_I);
  }

  virtual Utils::Numeric opponentValue(size_t actor, const std::vector<Utils::Numeric> &sigma_I) {
    Utils::Numeric counterfactualValue;
    DEBUG_VARIABLE("%zu", i_);
    reachProbProfile_[actor] =
        Utils::copyAndReturnAfter(reachProbProfile_[actor], [&]() {
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

  virtual Utils::Numeric myValue(size_t actor, const std::vector<Utils::Numeric> &sigma_I) {
    std::vector<Utils::Numeric> actionVals;
    actionVals.reserve(sigma_I.size());
    Utils::Numeric counterfactualValue = 0.0;
    reachProbProfile_[actor] =
        Utils::copyAndReturnAfter(reachProbProfile_[actor], [&]() {
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
  std::vector<std::vector<Utils::Numeric>> reachProbProfile_;
  std::vector<PolicyGenerator::PolicyGenerator<InformationSet, Sequence, Regret> *>
      policyGeneratorProfile_;
  std::vector<std::vector<Utils::Numeric>> cumulativeAverageStrategyProfile_;
  const std::vector<std::vector<int>>* utilsForPlayer1_;
  size_t i_;
  mutable std::vector<std::vector<Utils::Numeric>> averageStrategyProfile_;
};

const size_t NUM_SEQUENCES = 2;
const std::vector<size_t> NUM_SEQUENCES_BEFORE_EACH_INFO_SET{0};
}
}
