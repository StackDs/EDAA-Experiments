#pragma once

// Adaptador propio que implementa insert, search y erase usando std::set.

#include <cstddef>
#include <functional>
#include <set>
#include <utility>

template <class Key, class Compare = std::less<Key>> class RedBlackTree {
public:
  RedBlackTree() = default;
  explicit RedBlackTree(Compare compare) : values_(std::move(compare)) {}

  // Inserta una clave. Retorna true solo si no existía.
  bool insert(const Key &key) { return values_.insert(key).second; }

  // Busca una clave y retorna true si está presente.
  bool search(const Key &key) const {
    return values_.find(key) != values_.end();
  }

  // Elimina una clave y retorna true si estaba presente.
  bool erase(const Key &key) { return values_.erase(key) != 0; }

  // Retorna el número de claves almacenadas.
  std::size_t size() const noexcept { return values_.size(); }
  bool empty() const noexcept { return values_.empty(); }

private:
  std::set<Key, Compare> values_;
};
