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
protected:
  HistoryTreeNode(History::History<Symbol> *&&history,
                  typename TreeNode::TreeNode<Value>::TerminalValueFunction tvf,
                  typename TreeNode::TreeNode<Value>::InteriorValueFunction ivf)
      : TreeNode::TreeNode<Value>::TreeNode(tvf, ivf), history_(history) {
    assert(history_);
  }

public:
  virtual ~HistoryTreeNode() { Utilities::Memory::deletePointer(history_); }
  virtual bool isTerminal() const { return !history_->hasSuccessors(); }

protected:
  History::History<Symbol> *history_;
};
}
}
