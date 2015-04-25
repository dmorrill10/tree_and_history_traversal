#pragma once

#include <cassert>
#include <exception>
#include <functional>

extern "C" {
#include <print_debugger.h>
}
#include <lib/memory.hpp>
#include <lib/tree_node.hpp>
#include <lib/history.hpp>

namespace NewCfr {
namespace HistoryTreeNode {
  template <typename Value>
  class HistoryTreeNode : public TreeNode::TreeNode<Value> {
  protected:
    HistoryTreeNode(History::History* history)
    :TreeNode::TreeNode<Value>::TreeNode(), history_(history) {
      assert(history_);
    }

  public:
    virtual ~HistoryTreeNode() {}
    virtual bool isTerminal() const { return !history_->hasSuccessors(); }

  protected:
    History::History* history_;
  };
}
}
