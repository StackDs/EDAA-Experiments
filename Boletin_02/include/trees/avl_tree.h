#pragma once

// Obtenido y adaptado de:
// https://github.com/Aditya-A-garwal/AgAVLTree/blob/main/src/AgAVLTree.h

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

template <class Key, class Compare = std::less<Key>> class AVLTree {
public:
  AVLTree() = default;
  explicit AVLTree(Compare compare) : compare_(std::move(compare)) {}

  AVLTree(const AVLTree &) = delete;
  AVLTree &operator=(const AVLTree &) = delete;
  AVLTree(AVLTree &&) = delete;
  AVLTree &operator=(AVLTree &&) = delete;

  // Inserta una clave. Retorna true si se agregó; ignora claves equivalentes.
  bool insert(const Key &key) {
    bool inserted = false;
    insert(root_, key, inserted);
    if (inserted) {
      ++size_;
    }
    return inserted;
  }

  // Busca una clave y retorna true si está presente.
  bool search(const Key &key) const {
    const Node *current = root_.get();
    while (current != nullptr) {
      if (compare_(key, current->key)) {
        current = current->left.get();
      } else if (compare_(current->key, key)) {
        current = current->right.get();
      } else {
        return true;
      }
    }
    return false;
  }

  // Elimina una clave y retorna true si estaba presente.
  bool erase(const Key &key) {
    bool erased = false;
    erase(root_, key, erased);
    if (erased) {
      --size_;
    }
    return erased;
  }

  std::size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

private:
  struct Node {
    explicit Node(const Key &value) : key(value) {}

    Key key;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    int height = 1;
  };

  // Calcula la altura de un nodo.
  static int height(const std::unique_ptr<Node> &node) noexcept {
    return node == nullptr ? 0 : node->height;
  }

  // Actualiza la altura del nodo a partir de las alturas de sus hijos.
  static void update_height(Node &node) noexcept {
    node.height = 1 + std::max(height(node.left), height(node.right));
  }

  // Calcula el factor de balance de un nodo.
  static int balance_factor(const Node &node) noexcept {
    return height(node.left) - height(node.right);
  }

  // Rebalancea el subárbol con raíz en root, si es necesario.
  static void rebalance(std::unique_ptr<Node> &root) noexcept {
    if (root == nullptr) {
      return;
    }

    update_height(*root);
    const int balance = balance_factor(*root);

    if (balance > 1) {
      if (balance_factor(*root->left) < 0) {
        balance_lr(root);
      } else {
        balance_ll(root);
      }
    } else if (balance < -1) {
      if (balance_factor(*root->right) > 0) {
        balance_rl(root);
      } else {
        balance_rr(root);
      }
    }
  }

  // Rotaciones LL, LR, RL y RR adaptadas de AgAVLTree.
  static void balance_ll(std::unique_ptr<Node> &root) noexcept {
    auto top = std::move(root);
    auto bottom = std::move(top->left);
    top->left = std::move(bottom->right);
    update_height(*top);
    bottom->right = std::move(top);
    update_height(*bottom);
    root = std::move(bottom);
  }

  static void balance_lr(std::unique_ptr<Node> &root) noexcept {
    auto top = std::move(root);
    auto middle = std::move(top->left);
    auto bottom = std::move(middle->right);
    middle->right = std::move(bottom->left);
    update_height(*middle);
    bottom->left = std::move(middle);
    top->left = std::move(bottom->right);
    update_height(*top);
    bottom->right = std::move(top);
    update_height(*bottom);
    root = std::move(bottom);
  }

  static void balance_rl(std::unique_ptr<Node> &root) noexcept {
    auto top = std::move(root);
    auto middle = std::move(top->right);
    auto bottom = std::move(middle->left);
    middle->left = std::move(bottom->right);
    update_height(*middle);
    bottom->right = std::move(middle);
    top->right = std::move(bottom->left);
    update_height(*top);
    bottom->left = std::move(top);
    update_height(*bottom);
    root = std::move(bottom);
  }

  static void balance_rr(std::unique_ptr<Node> &root) noexcept {
    auto top = std::move(root);
    auto bottom = std::move(top->right);
    top->right = std::move(bottom->left);
    update_height(*top);
    bottom->left = std::move(top);
    update_height(*bottom);
    root = std::move(bottom);
  }

  void insert(std::unique_ptr<Node> &node, const Key &key, bool &inserted) {
    if (node == nullptr) {
      node = std::make_unique<Node>(key);
      inserted = true;
      return;
    }

    if (compare_(key, node->key)) {
      insert(node->left, key, inserted);
    } else if (compare_(node->key, key)) {
      insert(node->right, key, inserted);
    } else {
      return;
    }

    if (inserted) {
      rebalance(node);
    }
  }

  void erase(std::unique_ptr<Node> &node, const Key &key, bool &erased) {
    if (node == nullptr) {
      return;
    }

    if (compare_(key, node->key)) {
      erase(node->left, key, erased);
    } else if (compare_(node->key, key)) {
      erase(node->right, key, erased);
    } else {
      erased = true;
      if (node->left == nullptr) {
        node = std::move(node->right);
      } else if (node->right == nullptr) {
        node = std::move(node->left);
      } else {
        Node *successor = node->right.get();
        while (successor->left != nullptr) {
          successor = successor->left.get();
        }
        node->key = successor->key;
        bool successor_erased = false;
        erase(node->right, successor->key, successor_erased);
      }
    }

    if (node != nullptr) {
      rebalance(node);
    }
  }

  std::unique_ptr<Node> root_;
  std::size_t size_ = 0;
  Compare compare_{};
};
