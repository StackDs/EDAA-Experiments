#pragma once

// Obtenido y adaptado de:
// https://github.com/boostorg/intrusive/blob/develop/include/boost/intrusive/splaytree_algorithms.hpp

#include <cstddef>
#include <functional>
#include <utility>

template <class Key, class Compare = std::less<Key>> class SplayTree {
public:
  SplayTree() = default;
  explicit SplayTree(Compare compare) : compare_(std::move(compare)) {}

  SplayTree(const SplayTree &) = delete;
  SplayTree &operator=(const SplayTree &) = delete;
  SplayTree(SplayTree &&) = delete;
  SplayTree &operator=(SplayTree &&) = delete;

  ~SplayTree() { clear(); }

  // Inserta una clave. La clave queda en la raíz; retorna false si ya existía.
  bool insert(const Key &key) {
    if (root_ == nullptr) {
      root_ = new Node(key);
      ++size_;
      return true;
    }

    Node *parent = nullptr;
    Node *current = root_;
    bool insert_left = false;
    while (current != nullptr) {
      parent = current;
      if (compare_(key, current->key)) {
        insert_left = true;
        current = current->left;
      } else if (compare_(current->key, key)) {
        insert_left = false;
        current = current->right;
      } else {
        splay(current);
        return false;
      }
    }

    Node *inserted = new Node(key, parent);
    if (insert_left) {
      parent->left = inserted;
    } else {
      parent->right = inserted;
    }
    ++size_;
    splay(inserted);
    return true;
  }

  // Busca la clave. Si existe, la lleva a la raíz; si no, lleva a la raíz el
  // último nodo visitado.
  bool search(const Key &key) {
    Node *current = root_;
    Node *last = nullptr;
    while (current != nullptr) {
      last = current;
      if (compare_(key, current->key)) {
        current = current->left;
      } else if (compare_(current->key, key)) {
        current = current->right;
      } else {
        splay(current);
        return true;
      }
    }

    if (last != nullptr) {
      splay(last);
    }
    return false;
  }

  // Elimina la clave. Si hay subárbol izquierdo, su máximo queda como raíz y
  // recibe el subárbol derecho; si no, la raíz pasa a ser el subárbol derecho.
  bool erase(const Key &key) {
    Node *current = root_;
    Node *last = nullptr;
    while (current != nullptr) {
      last = current;
      if (compare_(key, current->key)) {
        current = current->left;
      } else if (compare_(current->key, key)) {
        current = current->right;
      } else {
        splay(current);
        Node *left = root_->left;
        Node *right = root_->right;
        delete root_;
        --size_;

        if (left == nullptr) {
          root_ = right;
          if (root_ != nullptr) {
            root_->parent = nullptr;
          }
        } else {
          left->parent = nullptr;
          root_ = left;
          Node *maximum = root_;
          while (maximum->right != nullptr) {
            maximum = maximum->right;
          }
          splay(maximum);
          root_->right = right;
          if (right != nullptr) {
            right->parent = root_;
          }
        }
        return true;
      }
    }

    if (last != nullptr) {
      splay(last);
    }
    return false;
  }

  std::size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

private:
  struct Node {
    explicit Node(const Key &value, Node *parent_node = nullptr)
        : key(value), parent(parent_node) {}

    Key key;
    Node *parent = nullptr;
    Node *left = nullptr;
    Node *right = nullptr;
  };

  // Rotaciones y operación splay adaptadas de Boost.Intrusive.

  void rotate_left(Node *node) noexcept {
    Node *pivot = node->right;
    node->right = pivot->left;
    if (pivot->left != nullptr) {
      pivot->left->parent = node;
    }
    pivot->parent = node->parent;
    if (node->parent == nullptr) {
      root_ = pivot;
    } else if (node == node->parent->left) {
      node->parent->left = pivot;
    } else {
      node->parent->right = pivot;
    }
    pivot->left = node;
    node->parent = pivot;
  }

  void rotate_right(Node *node) noexcept {
    Node *pivot = node->left;
    node->left = pivot->right;
    if (pivot->right != nullptr) {
      pivot->right->parent = node;
    }
    pivot->parent = node->parent;
    if (node->parent == nullptr) {
      root_ = pivot;
    } else if (node == node->parent->left) {
      node->parent->left = pivot;
    } else {
      node->parent->right = pivot;
    }
    pivot->right = node;
    node->parent = pivot;
  }

  void splay(Node *node) noexcept {
    while (node->parent != nullptr) {
      Node *parent = node->parent;
      Node *grandparent = parent->parent;

      if (grandparent == nullptr) {
        if (node == parent->left) {
          rotate_right(parent); // Zig
        } else {
          rotate_left(parent); // Zig
        }
      } else if (node == parent->left && parent == grandparent->left) {
        rotate_right(grandparent); // Zig-Zig
        rotate_right(parent);
      } else if (node == parent->right && parent == grandparent->right) {
        rotate_left(grandparent); // Zig-Zig
        rotate_left(parent);
      } else if (node == parent->right && parent == grandparent->left) {
        rotate_left(parent); // Zig-Zag
        rotate_right(grandparent);
      } else {
        rotate_right(parent); // Zig-Zag
        rotate_left(grandparent);
      }
    }
  }

  // Limpieza iterativa para no agotar la pila si el árbol está degenerado.
  void clear() noexcept {
    while (root_ != nullptr) {
      if (root_->left != nullptr) {
        rotate_right(root_);
      } else {
        Node *next = root_->right;
        delete root_;
        root_ = next;
        if (root_ != nullptr) {
          root_->parent = nullptr;
        }
      }
    }
    size_ = 0;
  }

  Node *root_ = nullptr;
  std::size_t size_ = 0;
  Compare compare_{};
};
