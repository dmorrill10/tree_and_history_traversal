#pragma once

#include <vector>
#include <cassert>
#include <functional>

extern "C" {
#include <utilities/src/lib/print_debugger.h>
}
#include <utilities/src/lib/memory.h>

namespace TreeAndHistoryTraversal {
namespace TreeNode {
template <typename Value> class TreeNode {
public:
  using TerminalValueFunction =
      std::function<Value(const TreeNode<Value> *node)>;
  using InteriorValueFunction =
      std::function<Value(const TreeNode<Value> *node)>;

protected:
  TreeNode(TerminalValueFunction tvf, InteriorValueFunction ivf)
      : terminalValueFunction_(tvf), interiorValueFunction_(ivf) {}

public:
  virtual ~TreeNode() {}
  virtual bool isTerminal() const = 0;
  Value value() const {
    if (isTerminal()) {
      return terminalValueFunction_(this);
    } else {
      return interiorValueFunction_(this);
    }
    return Value();
  }

protected:
  TerminalValueFunction terminalValueFunction_;
  InteriorValueFunction interiorValueFunction_;
};

template <typename Value> class TerminalNode : public TreeNode<Value> {
protected:
  TerminalNode(typename TreeNode<Value>::TerminalValueFunction tvf)
      : TreeNode<Value>::TreeNode(
            tvf, [](const TreeNode<Value> *) { return Value(); }) {}

public:
  virtual ~TerminalNode() {}
  virtual bool isTerminal() const { return true; }
};

template <typename Value> class InteriorNode : public TreeNode<Value> {
protected:
  InteriorNode(typename TreeNode<Value>::InteriorValueFunction ivf)
      : TreeNode<Value>::TreeNode(
            [](const TreeNode<Value> *) { return Value(); }, ivf) {}

public:
  virtual ~InteriorNode() {}
  virtual bool isTerminal() const { return false; }
};

template <typename Value>
class StoredTerminalNode : public TerminalNode<Value> {
public:
  StoredTerminalNode(Value value)
      : TerminalNode<Value>::TerminalNode([value](const TreeNode<Value> *) {
          // Move semantics could be used in a generalized lambda, but it's a
          // C++14
          // feature, so it's not necessary for these types used largely for
          // testing
          //    :TerminalNode<Value>::TerminalNode([value =
          //    std::move(value)](const TreeNode<Value>*) {
          return value;
        }) {}
  virtual ~StoredTerminalNode() {}
};

template <typename Value>
class StoredInteriorNode : public InteriorNode<Value> {
public:
  StoredInteriorNode(std::function<Value(Value &&childValue)> f)
      : InteriorNode<Value>::InteriorNode([f](const TreeNode<Value> *self) {
          // Move semantics could be used in a generalized lambda, but it's a
          // C++14
          // feature, so it's not necessary for these types used largely for
          // testing
          //    ):InteriorNode<Value>::InteriorNode([f = std::move(f)](const
          //    TreeNode<Value>* self) {
          Value toReturn;
          assert(static_cast<const StoredInteriorNode<Value> *>(self));
          for (
              const auto child :
              static_cast<const StoredInteriorNode<Value> *>(self)->children_) {
            assert(child);
            toReturn = f(child->value());
          }
          return toReturn;
        }){};
  StoredInteriorNode(std::vector<TreeNode<Value> *> &&children,
                     std::function<Value(Value &&childValue)> &&f)
      : StoredInteriorNode::StoredInteriorNode(f) {
    children_ = children;
  };
  StoredInteriorNode(const std::vector<TreeNode<Value> *> &children,
                     std::function<Value(Value &&childValue)> &&f)
      : StoredInteriorNode::StoredInteriorNode(f) {
    children_ = children;
  };
  virtual ~StoredInteriorNode() {
    for (auto child : children_) {
      Utilities::Memory::deletePointer(child);
    }
  };

  virtual void addChild(TreeNode<Value> *newChild) {
    children_.emplace_back(newChild);
  }

  virtual void setChildren(std::vector<TreeNode<Value> *> &&children) {
    for (auto child : children_) {
      Utilities::Memory::deletePointer(child);
    }
    children_ = children;
  }

  virtual void setChildren(std::vector<TreeNode<Value> *> &children) {
    for (auto child : children_) {
      Utilities::Memory::deletePointer(child);
    }
    children_ = children;
  }

protected:
  std::vector<TreeNode<Value> *> children_;
};
}
}
