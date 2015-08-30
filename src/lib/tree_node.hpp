#pragma once

#include <vector>
#include <cassert>
#include <functional>

#include <cpp_utilities/src/lib/memory.h>

namespace TreeAndHistoryTraversal {
namespace TreeNode {
template <typename Value>
class TreeNode {
 protected:
  TreeNode() {}

 public:
  virtual ~TreeNode() {}
  virtual bool isTerminal() const = 0;
  virtual Value value() {
    return isTerminal() ? terminalValue() : interiorValue();
  }

 protected:
  virtual Value terminalValue() = 0;
  virtual Value interiorValue() = 0;
};

template <typename Value>
class TerminalNode : public TreeNode<Value> {
 protected:
  TerminalNode() : TreeNode<Value>::TreeNode() {}

 public:
  virtual ~TerminalNode() {}
  virtual bool isTerminal() const override { return true; }

 protected:
  virtual Value interiorValue() override final { return Value(); }
};

template <typename Value>
class InteriorNode : public TreeNode<Value> {
 protected:
  InteriorNode() : TreeNode<Value>::TreeNode() {}

 public:
  virtual ~InteriorNode() {}
  virtual bool isTerminal() const override { return false; }

 protected:
  virtual Value terminalValue() override final { return Value(); }
};

template <typename Value>
class StoredTerminalNode : public TerminalNode<Value> {
 public:
  StoredTerminalNode(Value value)
      : TerminalNode<Value>::TerminalNode(), value_(value) {}
  virtual ~StoredTerminalNode() {}

 protected:
  virtual Value terminalValue() override final { return value_; }

 protected:
  Value value_;
};

template <typename Value>
class StoredInteriorNode : public InteriorNode<Value> {
 public:
  StoredInteriorNode(std::function<Value(Value&& childValue)> f)
      : InteriorNode<Value>::InteriorNode(), f_(f){};
  StoredInteriorNode(std::vector<TreeNode<Value>*>&& children,
                     std::function<Value(Value&& childValue)>&& f)
      : StoredInteriorNode::StoredInteriorNode(f) {
    children_ = children;
  };
  StoredInteriorNode(const std::vector<TreeNode<Value>*>& children,
                     std::function<Value(Value&& childValue)>&& f)
      : StoredInteriorNode::StoredInteriorNode(f) {
    children_ = children;
  };
  virtual ~StoredInteriorNode() {
    for (auto child : children_) {
      Utilities::Memory::deletePointer(child);
    }
  };

  virtual void addChild(TreeNode<Value>* newChild) {
    children_.emplace_back(newChild);
  }

  virtual void setChildren(std::vector<TreeNode<Value>*>&& children) {
    for (auto child : children_) {
      Utilities::Memory::deletePointer(child);
    }
    children_ = children;
  }

  virtual void setChildren(std::vector<TreeNode<Value>*>& children) {
    for (auto child : children_) {
      Utilities::Memory::deletePointer(child);
    }
    children_ = children;
  }

 protected:
  virtual Value interiorValue() override final {
    Value toReturn;
    for (const auto child : children_) {
      assert(child);
      toReturn = f_(child->value());
    }
    return toReturn;
  }

 protected:
  std::vector<TreeNode<Value>*> children_;
  std::function<Value(Value&& childValue)> f_;
};

class NoReturnTreeNode {
 protected:
  NoReturnTreeNode() {}

 public:
  virtual ~NoReturnTreeNode() {}
  virtual bool isTerminal() const = 0;
  virtual void computeValue() {
    if (isTerminal()) {
      computeTerminalValue();
    } else {
      computeInteriorValue();
    }
  }

 protected:
  virtual void computeTerminalValue() = 0;
  virtual void computeInteriorValue() = 0;
};
}
}
