#pragma once

#include <cassert>
#include <exception>
#include <functional>

#include <cpp_utilities/src/lib/memory.h>

#include "tree_node.hpp"
#include "history.hpp"

namespace TreeAndHistoryTraversal {
namespace HistoryTreeNode {
template <typename Value, typename Symbol>
class HistoryTreeNode : public TreeNode::TreeNode<Value> {
 public:
  HistoryTreeNode(History::History<Symbol>*&& history)
      : TreeNode::TreeNode<Value>::TreeNode(), history_(std::move(history)) {
    assert(history_);
  }

  virtual ~HistoryTreeNode() { Utilities::Memory::deletePointer(history_); }
  virtual bool isTerminal() const { return !history_->hasSuccessors(); }
  virtual const History::History<Symbol>* history() const { return history_; };

 protected:
  History::History<Symbol>* history_;
};

template <typename Symbol>
class NoReturnHistoryTreeNode : public TreeNode::NoReturnTreeNode {
 public:
  NoReturnHistoryTreeNode(History::History<Symbol>*&& history)
      : NoReturnTreeNode(), history_(std::move(history)) {
    assert(history_);
  }

  virtual ~NoReturnHistoryTreeNode() {
    Utilities::Memory::deletePointer(history_);
  }
  virtual bool isTerminal() const { return !history_->hasSuccessors(); }
  virtual const History::History<Symbol>* history() const { return history_; };

 protected:
  History::History<Symbol>* history_;
};

template <class Symbol>
class PreorderHistoryTreeTraversal : public NoReturnHistoryTreeNode<Symbol> {
 public:
  using NoReturnHistoryTreeNode<Symbol>::NoReturnHistoryTreeNode;

 protected:
  virtual void computeTerminalValue() override {}
  virtual void computeInteriorValue() override {
    this->history_->eachSuccessor([this](size_t, size_t) {
      this->computeValue();
      return false;
    });
  }
};
}
}
