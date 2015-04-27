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
  template <typename Value>
  class HistoryTreeNode : public TreeNode::TreeNode<Value> {
  protected:
    HistoryTreeNode(History::History*&& history)
    :TreeNode::TreeNode<Value>::TreeNode(), history_(history) {
      assert(history_);
    }

  public:
    virtual ~HistoryTreeNode() { DELETE_POINTER(history_); }
    virtual bool isTerminal() const { return !history_->hasSuccessors(); }

  protected:
    History::History* history_;
  };
}
}
