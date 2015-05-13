#pragma once

#include <cassert>
#include <string>
#include <vector>

#include <lib/utils.hpp>

extern "C" {
#include <utilities/src/lib/print_debugger.h>
}

namespace TreeAndHistoryTraversal {
namespace PolicyGenerator {

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
      const std::vector<size_t> &numSequencesBeforeEachInfoSet, double noise, size_t randomSeed = 63547654)
      : RegretMatchingTable::RegretMatchingTable(
            numSequences, numActionsAtEachInfoSet,
            numSequencesBeforeEachInfoSet),
        randomEngine_(randomSeed), noise_(noise),
        numRegretsSmallerThanNoise_(0) {}
  virtual ~PerturbedPolicyRegretMatchingTable() {}

  virtual std::vector<Numeric> policy(const size_t &I) const override {
    Numeric sum = 0.0;
    const auto numActions = (*numActionsAtEachInfoSet_)[I];
    const auto baseIndex = (*numSequencesBeforeEachInfoSet_)[I];

    std::vector<Numeric> perturbedRegrets(numActions);
    for (size_t i = 0; i < numActions; ++i) {
      if (noise_ > std::abs(table_[baseIndex + i])) {
        ++numRegretsSmallerThanNoise_;
      }

      const int noiseSign = Utils::flipCoin(0.5, &randomEngine_) ? 1 : -1;
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

  size_t numRegretsSmallerThanNoise() const {
    return numRegretsSmallerThanNoise_;
  }

  void clearnumRegretsSmallerThanNoise() {
    numRegretsSmallerThanNoise_ = 0;
  }

protected:
  mutable std::mt19937 randomEngine_;
  const double noise_;
  size_t numRegretsSmallerThanNoise_;
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
    const int noiseSign = Utils::flipCoin(0.5, &randomEngine_) ? 1 : -1;
    RegretMatchingTable::update(sequence, regretValue + noiseSign * noise_);
  }

protected:
  mutable std::mt19937 randomEngine_;
  const double noise_;
};
}
}
