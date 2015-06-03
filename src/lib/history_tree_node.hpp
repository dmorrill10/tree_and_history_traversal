#pragma once

#include <cassert>
#include <exception>
#include <functional>

#include <utilities/src/lib/print_debugger.h>
#include <utilities/src/lib/memory.h>

#include "tree_node.hpp"
#include "history.hpp"

namespace TreeAndHistoryTraversal {
namespace HistoryTreeNode {
template <typename Value, typename Symbol>
class HistoryTreeNode : public TreeNode::TreeNode<Value> {
 public:
  HistoryTreeNode(History::History<Symbol>*&& history)
      : TreeNode::TreeNode<Value>::TreeNode(), history_(history) {
    assert(history_);
  }

  virtual ~HistoryTreeNode() { Utilities::Memory::deletePointer(history_); }
  virtual bool isTerminal() const { return !history_->hasSuccessors(); }
  virtual const History::History<Symbol>* history() const { return history_; };

 protected:
  History::History<Symbol>* history_;
};
}
}
