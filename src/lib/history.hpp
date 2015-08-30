#pragma once

#include <vector>
#include <cassert>
#include <exception>
#include <functional>

#include <cpp_utilities/src/lib/memory.h>

namespace TreeAndHistoryTraversal {
namespace History {

template <typename Symbol>
class ActionSet {
 protected:
  ActionSet() {}

 public:
  virtual ~ActionSet() {}
  /**
   * Breaks when true is returned from the closure and returns true itself
   * in this case. false otherwise.
   */
  virtual bool eachSuffix(
      std::function<bool(Symbol&& suffix, size_t suffixIndex)> doFn) const = 0;
  virtual bool suffixIsLegal(const Symbol& candidate) const = 0;
  virtual bool isEmpty() const = 0;
  virtual bool hasSuccessors() const = 0;
  virtual bool eachLegalSuffix(std::function<
      bool(Symbol&& suffix, size_t suffixIndex, size_t legalSuffixIndex)> doFn)
      const {
    size_t legalIndex = 0;
    return eachSuffix([this, &legalIndex, &doFn](Symbol&& suffix, size_t i) {
      bool val = false;
      if (suffixIsLegal(suffix)) {
        val = doFn(std::move(suffix), i, legalIndex);
        ++legalIndex;
      }
      return val;
    });
  }
  virtual size_t numSuccessors() const {
    if (!hasSuccessors()) {
      return 0;
    }
    size_t n = 0;
    eachLegalSuffix([this, &n](Symbol&&, size_t, size_t) {
      ++n;
      return false;
    });
    return n;
  }
};

/**
 * A sequence of symbols that, given any prefix,
 * knows the legal alphabet for the
 * next symbol, and can enumerate these, as well as successive histories.
 */
template <typename Symbol>
class History : public ActionSet<Symbol> {
 protected:
  History() : ActionSet<Symbol>() {}

 public:
  virtual ~History() {}

  virtual void push(Symbol suffix) = 0;
  virtual void pop() = 0;
  virtual Symbol last() const = 0;

  /**
   * Breaks when true is returned from the closure and returns true itself
   * in this case. false otherwise.
   */
  bool eachSuccessor(std::function<bool(size_t successorIndex,
                                        size_t legalSuffixIndex)> doFn) {
    return this->eachLegalSuffix([&doFn, this](
        Symbol&& suffix, size_t suffixIndex, size_t legalSuffixIndex) {
      push(std::move(suffix));
      const bool shouldBreak = doFn(suffixIndex, legalSuffixIndex);
      pop();
      return shouldBreak;
    });
  }
};

class StringHistory : public History<std::string> {
 protected:
  StringHistory(std::vector<std::string>&& allLegalStrings)
      : state_(), allLegalStrings_(allLegalStrings) {}

 public:
  virtual ~StringHistory() {}

  virtual bool eachSuffix(std::function<bool(std::string&& suffix,
                                             size_t suffixIndex)> doFn) const {
    for (size_t candidateIndex = 0; candidateIndex < allLegalStrings_.size();
         ++candidateIndex) {
      const auto& candidate = allLegalStrings_[candidateIndex];
      if (!suffixIsLegal(candidate)) {
        continue;
      }
      const bool shouldBreak = doFn(candidate.substr(), candidateIndex);
      if (shouldBreak) {
        return true;
      }
    }
    return false;
  }
  virtual void push(std::string suffix) {
    if (!suffixIsLegal(suffix)) {
      throw std::runtime_error("Illegal StringHistory suffix, \"" + suffix +
                               "\", for prefix, \"" + toString() + "\"");
    }
    state_.emplace_back(suffix);
  }
  virtual void pop() { state_.pop_back(); }
  virtual std::string last() const { return isEmpty() ? "" : state_.back(); }
  virtual bool isEmpty() const { return state_.empty(); }
  virtual bool hasSuccessors() const {
    for (const auto& s : allLegalStrings_) {
      if (suffixIsLegal(s)) {
        return true;
      }
    }
    return false;
  }

  virtual std::string toString() const {
    std::string str = "";
    for (size_t i = 0; i < state_.size(); ++i) {
      if (i > 0) {
        str += " -> ";
      }
      str += state_[i];
    }
    return str;
  }

 protected:
  std::vector<std::string> state_;
  std::vector<std::string> allLegalStrings_;
};
}
}
