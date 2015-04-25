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
  template <typename Value>
  class TreeNode {
  protected:
    TreeNode() {}

  public:
    virtual ~TreeNode() {}
    virtual bool isTerminal() const = 0;
    virtual Value value() const = 0;
  };

  template <typename Value>
  class TerminalNode : public TreeNode<Value> {
  protected:
    TerminalNode():TreeNode<Value>::TreeNode() {}

  public:
    virtual ~TerminalNode() {}
    virtual bool isTerminal() const { return true; }
  };

  template <typename Value>
  class InteriorNode : public TreeNode<Value> {
  protected:
    InteriorNode():TreeNode<Value>::TreeNode() {}

  public:
    virtual ~InteriorNode() {}
    virtual bool isTerminal() const { return false; }
  };

  template <typename Value>
  class StoredTerminalNode : public TerminalNode<Value> {
  public:
    StoredTerminalNode(Value&& value)
    :TerminalNode<Value>::TerminalNode(), value_(value) {}
    StoredTerminalNode(const Value& value)
    :TerminalNode<Value>::TerminalNode(), value_(value) {}
    virtual ~StoredTerminalNode() {}

    virtual Value value() const { return value_; };
  protected:
    Value value_;
  };

  template <typename Value>
  class StoredInteriorNode : public InteriorNode<Value> {
  public:
    StoredInteriorNode(
        std::function<Value(Value&& childValue)>&& f
    ):InteriorNode<Value>::InteriorNode(),
      combiner_(f) {};
    StoredInteriorNode(
        std::vector<TreeNode<Value>*>&& children,
        std::function<Value(Value&& childValue)>&& f
    ):InteriorNode<Value>::InteriorNode(),
      children_(children),
      combiner_(f) {};
    StoredInteriorNode(
        const std::vector<TreeNode<Value>*>& children,
        std::function<Value(Value&& childValue)>&& f
    ):InteriorNode<Value>::InteriorNode(),
      children_(children),
      combiner_(f) {};
    virtual ~StoredInteriorNode() {
      for (auto child : children_) {
        Utilities::Memory::deletePointer(child);
      }
    };

    virtual Value value() const {
      Value toReturn;
      for (const auto child : children_) {
        toReturn = combiner_(child->value());
      }
      return toReturn;
    }

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
    std::vector<TreeNode<Value>*> children_;
    std::function<Value(Value&& childValue)> combiner_;
  };
}
}
