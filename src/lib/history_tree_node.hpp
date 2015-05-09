#pragma once

#include <cassert>
#include <exception>
#include <functional>

extern "C" {
#include <utilities/src/lib/print_debugger.h>
}
#include <utilities/src/lib/memory.h>

#include <lib/tree_node.hpp>
#include <lib/history.hpp>

namespace TreeAndHistoryTraversal {
namespace HistoryTreeNode {
template <typename Value, typename Symbol>
class HistoryTreeNode : public TreeNode::TreeNode<Value> {
public:
  typedef std::function<bool(
      Value *accumulatedValue, const History::History<Symbol> *parentSequence,
      Symbol action, size_t nextActionIndex, size_t nextLegalActionIndex,
      const Value &valueOnSequence)> CombineSuccessorValuesFunction;

  typedef std::function<Value(const HistoryTreeNode<Value, Symbol> *)>
      AccumulatedValueInitializer;

  typedef std::function<bool(const History::History<Symbol> *parentSequence,
                             Symbol action, size_t nextActionIndex,
                             size_t nextLegalActionIndex)>
      BeforeTakingActionFunction;

  typedef std::function<void(
      const HistoryTreeNode<Value, Symbol> *self, const Value &finalValue,
      const std::vector<Value> &childValues)> AfterAllActionsFunction;

  HistoryTreeNode(
      History::History<Symbol> *&&history,
      typename TreeNode::TreeNode<Value>::TerminalValueFunction &&tvf,
      CombineSuccessorValuesFunction &&csvf, AccumulatedValueInitializer &&avi,
      BeforeTakingActionFunction &&btaf = [](
          const History::History<Symbol> * /*parentSequence*/,
          Symbol /*action*/, size_t /*nextActionIndex*/,
          size_t /*nextLegalActionIndex*/
          ) { return false; },
      AfterAllActionsFunction &&afterAllActionsFn = [](
          const HistoryTreeNode<Value, Symbol> * /*self*/,
          const Value & /*finalValue*/,
          const std::vector<Value> & /*childValues*/
          ) { return; })
      : TreeNode::TreeNode<Value>::TreeNode(
            tvf,
            [csvf, avi, btaf, afterAllActionsFn, this]() {
              Value myValue = avi(this);
              size_t legalActionIndex = 0;

              std::vector<Value> newValues;
              newValues.reserve(history_->numSuccessors());

              this->history_->eachSuffix(
                  [&csvf, &myValue, &legalActionIndex, &btaf, &newValues, this](
                      Symbol action, size_t actionIndex) {
                    if (btaf(history_, action, actionIndex, legalActionIndex)) {
                      return false;
                    }
                    history_->push(action);
                    newValues.push_back(this->value());
                    history_->pop();
                    const bool shouldBreak =
                        csvf(&myValue, history_, action, actionIndex,
                             legalActionIndex, newValues.back());
                    ++legalActionIndex;
                    return shouldBreak;
                  });
              afterAllActionsFn(this, myValue, newValues);
              return myValue;
            }),
        history_(history) {
    assert(history_);
  }

  virtual ~HistoryTreeNode() { Utilities::Memory::deletePointer(history_); }
  virtual bool isTerminal() const { return !history_->hasSuccessors(); }
  virtual const History::History<Symbol> *history() const { return history_; };

protected:
  History::History<Symbol> *history_;
};
}
}
