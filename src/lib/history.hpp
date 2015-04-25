#pragma once

#include <vector>
#include <cassert>
#include <exception>
#include <functional>

extern "C" {
#include <utilities/src/lib/print_debugger.h>
}
#include <utilities/src/lib/memory.h>

namespace TreeAndHistoryTraversal {
namespace History {
  /**
   * A sequence of symbols that, given any prefix,
   * knows the legal alphabet for the
   * next symbol, and can enumerate these, as well as successive histories.
   */
  template<typename Symbol>
  class History {
  protected:
    History() {}

  public:
    virtual ~History() {}
    /**
     * Breaks when true is returned from the closure and returns true itself
     * in this case. false otherwise.
     */
    virtual bool eachSuffix(std::function<bool(Symbol&& suffix, size_t suffixIndex)> doFn) const = 0;
    virtual void push(Symbol suffix) = 0;
    virtual void pop() = 0;
    virtual bool suffixIsLegal(const Symbol& candidate) const = 0;
    virtual Symbol last() const = 0;
    virtual bool isEmpty() const = 0;
    virtual bool hasSuccessors() const = 0;

    /**
     * Breaks when true is returned from the closure and returns true itself
     * in this case. false otherwise.
     */
    template<typename ConcreteHistory>
    bool eachSuccessor(std::function<bool(ConcreteHistory* successor, size_t successorIndex)> doFn) {
      return eachSuffix([&doFn, this](Symbol&& suffix, size_t suffixIndex) {
        push(std::move(suffix));
        auto concreteThis = static_cast<ConcreteHistory*>(this);
        assert(concreteThis);
        const bool shouldBreak = doFn(concreteThis, suffixIndex);
        pop();
        if (shouldBreak) { return true; }
        return false;
      });
    }
  };

  class StringHistory : public History<std::string> {
  public:
    StringHistory(
        std::vector<std::string>&& allLegalStrings,
        std::function<bool(
            const StringHistory& prefix,
            const std::string& candidateString)>&& isLegalSuffix
    ):state_(),
      allLegalStrings_(allLegalStrings),
      suffixIsLegal_(isLegalSuffix) {}
    virtual ~StringHistory() {}

    virtual bool eachSuffix(std::function<bool(std::string&& suffix, size_t suffixIndex)> doFn) const {
      for (size_t candidateIndex = 0; candidateIndex < allLegalStrings_.size(); ++candidateIndex) {
        const auto& candidate = allLegalStrings_[candidateIndex];
        if (!suffixIsLegal(candidate)) { continue; }
        const bool shouldBreak = doFn(candidate.substr(), candidateIndex);
        if (shouldBreak) { return true; }
      }
      return false;
    }
    virtual void push(std::string suffix) {
      if (!suffixIsLegal(suffix)) {
        throw std::runtime_error(
            "Illegal StringHistory suffix, \"" + suffix +
            "\", for prefix, \"" + toString() + "\""
        );
      }
      state_.emplace_back(suffix);
    }
    virtual void pop() { state_.pop_back(); }
    virtual bool suffixIsLegal(const std::string& candidate) const {
      return suffixIsLegal_((*this), candidate);
    }
    virtual std::string last() const {
      return isEmpty() ? "" : state_.back();
    }
    virtual bool isEmpty() const { return state_.empty(); }
    virtual bool hasSuccessors() const {
      for (const auto& s : allLegalStrings_) {
        if (suffixIsLegal(s)) { return true; }
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
    std::function<bool(
        const StringHistory& prefix,
        const std::string& candidateString)> suffixIsLegal_;
  };
}
}
