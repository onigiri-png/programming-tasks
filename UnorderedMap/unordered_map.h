#include <cmath>
#include <stdexcept>
#include <functional>
#include <memory>
#include <vector>
#include <type_traits>

#include "list_for_map.h"

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
  using NodeType = std::pair<const Key, Value>;
  using AllocTraits = std::allocator_traits<Alloc>;

  using ListNode = typename List<NodeType, Alloc>::Node;
  using ListNodePAlloc = typename AllocTraits::template rebind_alloc<ListNode*>;
  using ListBaseNode = typename List<NodeType, Alloc>::BaseNode;

  using ValueAlloc = typename AllocTraits::template rebind_alloc<Value>;
  using ValAllocTraits = std::allocator_traits<ValueAlloc>;

  using iterator = typename List<NodeType, Alloc>::iterator;
  using const_iterator = typename List<NodeType, Alloc>::const_iterator;

  UnorderedMap(const Alloc& alloc = Alloc(), const Hash& hash = Hash(),
               const Equal& equal = Equal())
    : _alloc(alloc), _hash(hash), _equal_to(equal), _list(alloc), _array(1, alloc) {}

  UnorderedMap(const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other);

  UnorderedMap(UnorderedMap<Key, Value, Hash, Equal, Alloc>&& other) noexcept = default;

  ~UnorderedMap() = default;

  UnorderedMap<Key, Value, Hash, Equal, Alloc>&
    operator=(const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other);

  UnorderedMap<Key, Value, Hash, Equal, Alloc>&
    operator=(UnorderedMap<Key, Value, Hash, Equal, Alloc>&& other) noexcept;

  Value& operator[](const Key& key);

  Value& operator[](Key&& key);

  Value& at(const Key& key);

  const Value& at(const Key& key) const;

  iterator find(const Key& key);

  const_iterator find(const Key& key) const;

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args);

  std::pair<iterator, bool> insert(const NodeType& elem);

  std::pair<iterator, bool> insert(NodeType&& elem);

  template <typename Val>
  std::pair<iterator, bool> insert(Val&& value);

  template <typename InputIt>
  void insert(InputIt first, InputIt last);

  iterator erase(const_iterator pos);

  iterator erase(const_iterator first, const_iterator last);

  void rehash(size_t bucket_count);

  void reserve(size_t count) {
    rehash(std::ceil(count / _max_load_factor));
  }

  float load_factor() const {
    return static_cast<float>(_size) / _array.size();
  }

  size_t bucket_count() const {
    return _array.size();
  }

  float max_load_factor() const {
    return _max_load_factor;   
  }

  void max_load_factor(float mlf) {
    _max_load_factor = mlf;
  }

  size_t size() const {
    return _size;
  }

  iterator begin() {
    return _list.begin();
  }
  
  iterator end() {
    return _list.end();
  }

  const_iterator cbegin() const {
    return _list.cbegin();
  }

  const_iterator cend() const {
    return _list.cend();
  }

  const_iterator end() const {
    return _list.cend();
  }

  const_iterator begin() const {
    return _list.cbegin();
  }

private:
  template <typename Arg>
  Value& _elem_by_index(Arg&& key);

  iterator _find(const Key& key) const;

  [[no_unique_address]] Alloc _alloc;
  [[no_unique_address]] ValueAlloc _val_alloc;
  [[no_unique_address]] Hash _hash;
  [[no_unique_address]] Equal _equal_to;

  List<NodeType, Alloc> _list;
  std::vector<ListNode*, ListNodePAlloc> _array;
  size_t _size = 0;
  float _max_load_factor = 1.0;
};

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template <typename Arg>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::_elem_by_index(Arg&& key) {
  size_t hash = _hash(key);
  size_t hash_index = hash % _array.size();
  Value* val;

  if (_array[hash_index] == nullptr) {
    if constexpr (std::is_default_constructible<Value>::value) {
      val = ValAllocTraits::allocate(_val_alloc, 1);
      try {
        ValAllocTraits::construct(_val_alloc, val); 
      } catch (...) {
        ValAllocTraits::deallocate(_val_alloc, val, 1);
        throw;
      }
      
      Value& ans = emplace(std::forward<Arg>(key),
        std::move(*val)).first->second;
      ValAllocTraits::destroy(_val_alloc, val);
      ValAllocTraits::deallocate(_val_alloc, val, 1);
      return ans;
    }
  }

  if (_array[hash_index] != nullptr) {
    for (auto it = iterator(_array[hash_index]); it != end(); ++it) {
      ListNode* node = static_cast<ListNode*>(it.get_node());

      if (_equal_to(node->value_ptr()->first, key)) {
        return node->value_ptr()->second;
      }

      if constexpr (std::is_default_constructible<Value>::value) {
        if (node->hash % _array.size() != hash_index) {
          val = ValAllocTraits::allocate(_val_alloc, 1);
          try {
            ValAllocTraits::construct(_val_alloc, val); 
          } catch (...) {
            ValAllocTraits::deallocate(_val_alloc, val, 1);
            throw;
          }
          Value& ans = emplace(std::forward<Arg>(key),
            std::move(*val)).first->second;
          ValAllocTraits::destroy(_val_alloc, val);
          ValAllocTraits::deallocate(_val_alloc, val, 1);
          return ans;
        }
      }
    }
  }

  if constexpr (std::is_default_constructible<Value>::value) {
    val = ValAllocTraits::allocate(_val_alloc, 1);
    try {
      ValAllocTraits::construct(_val_alloc, val); 
    } catch (...) {
      ValAllocTraits::deallocate(_val_alloc, val, 1);
      throw;
    }
    Value& ans = emplace(std::forward<Arg>(key),
      std::move(*val)).first->second;
    ValAllocTraits::destroy(_val_alloc, val);
    ValAllocTraits::deallocate(_val_alloc, val, 1);
    return ans;
  }

  throw std::logic_error("type must be default constructible");

}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::_find(const Key& key) const {
  size_t hash = _hash(key);
  size_t hash_index = hash % _array.size();

  if (_array[hash_index] == nullptr) {
    return iterator(end().get_node());
  } else {
    for (auto it = const_iterator(_array[hash_index]); it != cend(); ++it) {
      auto node = static_cast<ListNode*>(it.get_node());

      if (_equal_to(node->value_ptr()->first, key)) {
        return iterator(it.get_node());
      }

      if (node->hash % _array.size() != hash_index) {
        return iterator(end().get_node());
      }
    }
  }

  return iterator(end().get_node());
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(
  const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other)
  : _hash(other._hash), _equal_to(other._equal_to), _list(other._list),
  _array(other._array), _size(other._size), _max_load_factor(other._max_load_factor) {

  _alloc = AllocTraits::select_on_container_copy_construction(other._alloc);
  _val_alloc = AllocTraits::select_on_container_copy_construction(other._val_alloc);
  size_t prev_hash_index = 0;
  for (auto it = _list.begin(); it != _list.end(); ++it) {
    size_t hash_index = static_cast<ListNode*>(it.get_node())->hash % _array.size();
    if ((it == _list.begin()) || (hash_index != prev_hash_index)) {
      _array[hash_index] = static_cast<ListNode*>(it.get_node());
    }
    prev_hash_index = hash_index;
  }
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(
  const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other) {
  if (AllocTraits::propagate_on_container_copy_assignment::value) {
    _alloc = const_cast<Alloc&>(other._alloc);
  }

  if (ValAllocTraits::propagate_on_container_copy_assignment::value) {
    _val_alloc = const_cast<ValueAlloc&>(other._val_alloc);
  }

  _hash = other._hash;
  _equal_to = other._equal_to;
  _list = other._list;
  _array = other._array;
  _size = other._size;
  _max_load_factor = other._max_load_factor;
  size_t prev_hash_index = 0;
  for (auto it = _list.begin(); it != _list.end(); ++it) {
    size_t hash_index = static_cast<ListNode*>(it.get_node())->hash % _array.size();
    if ((it == _list.begin()) || (hash_index != prev_hash_index)) {
      _array[hash_index] = static_cast<ListNode*>(it.get_node());
    }
    prev_hash_index = hash_index;
  }

  return *this;
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(
  UnorderedMap<Key, Value, Hash, Equal, Alloc>&& other) noexcept {
  if (ValAllocTraits::propagate_on_container_move_assignment::value) {
    _val_alloc = std::move(other._val_alloc);
  }
  if (AllocTraits::propagate_on_container_move_assignment::value) {
    _alloc = std::move(other._alloc);
  }
  _hash = std::move(other._hash);
  _equal_to = std::move(other._equal_to);
  _array = std::move(other._array);
  other._array.resize(1);
  _list = std::move(other._list);
  _max_load_factor = other._max_load_factor;
  _size = other._size;
  other._size = 0;
  other._max_load_factor = 1.0;

  return *this;
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::rehash(size_t bucket_count) {
  if (bucket_count < _size / _max_load_factor) {
    bucket_count = static_cast<size_t>(std::ceil(_size / _max_load_factor));
  }

  List<NodeType, Alloc> new_list;
  std::vector<ListNode*, ListNodePAlloc> new_array(bucket_count);

  for (auto it = _list.begin(); it != _list.end(); ) {
    auto cur = it++;
    ListNode* node = static_cast<ListNode*>(cur.get_node());
    size_t new_hash_index = node->hash % new_array.size();

    if (new_array[new_hash_index] == nullptr) {
      new_list.splice(new_list.cend(), _list, cur);
      new_array[new_hash_index] = static_cast<ListNode*>(new_list.end().get_node()->prev);
    } else {
      auto iter = typename List<NodeType, Alloc>::iterator(new_array[new_hash_index]);
      for (; iter != new_list.end(); ++iter) {
        ListNode* new_node = static_cast<ListNode*>(iter.get_node());
        if (new_node->hash % new_array.size() != new_hash_index) {
          new_list.splice(iter, _list, cur);
          new_array[new_hash_index] = static_cast<ListNode*>(iter.get_node()->prev);
          break;
        }
      }
      
      if (iter == new_list.end()) {
        new_list.splice(new_list.cend(), _list, cur);
        new_array[new_hash_index] = static_cast<ListNode*>(new_list.end().get_node()->prev);
      }
    }
  }

  _array = std::move(new_array);
  _list = std::move(new_list);
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template <typename... Args>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::emplace(Args&&... args) {
  if (_size + 1 > _max_load_factor * _array.size()) {
    rehash(2 * _array.size());
  }

  List<NodeType, Alloc> temp_list;
  temp_list.emplace_front(0, std::forward<Args>(args)...);

  ListNode* node = static_cast<ListNode*>(temp_list.begin().get_node());
  size_t hash = _hash(node->value_ptr()->first);
  node->hash = hash;
  size_t hash_index = hash % _array.size();

  if (_array[hash_index] == nullptr) {
    _list.splice(_list.cbegin(), temp_list);
    std::pair<iterator, bool> ans(_list.begin(), true); 
    _array[hash_index] = static_cast<ListNode*>(ans.first.get_node());
    ++_size;
    return ans;
  } else {
    for (auto it = iterator(_array[hash_index]); it != end(); ++it) {
      auto node = static_cast<ListNode*>(it.get_node());

      if (_equal_to(node->value_ptr()->first,
          static_cast<ListNode*>(temp_list.begin().get_node())->value_ptr()->first)) {
        return std::pair<iterator, bool>(it, false);
      }

      if (node->hash % _array.size() != hash_index) {
        _list.splice(it, temp_list);
        std::pair<iterator, bool> ans(iterator(it.get_node()->prev), true);
        ++_size;
        return ans;
      }
    }
  }

  _list.splice(_list.cend(), temp_list);
  ++_size;
  return std::pair<iterator, bool>(iterator(_list.end().get_node()->prev), true);
}


template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(const NodeType& elem) {
  return emplace(elem);
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(NodeType&& elem) {
  return emplace(std::move(elem));
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template <typename Val>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(Val&& value) {
  return emplace(std::forward<Val>(value));
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template <typename InputIt>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(InputIt first, InputIt last) {
  for (; first != last; ++first) {
    insert(std::forward<decltype(*first)>(*first));
  }
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(const Key& key) {
  return iterator(_find(key));
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(const Key& key) const {
  return _find(key);
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator[](const Key& key) {
  return _elem_by_index(key);
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator[](Key&& key) {
  return _elem_by_index(std::move(key));
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(const Key& key) {
  if (find(key) == end()) {
    throw std::out_of_range("the container does not have an element with the specified key");
  }

  return operator[](key);
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
const Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(const Key& key) const {
  if (find(key) == end()) {
    throw std::out_of_range("the container does not have an element with the specified key");
  }

  size_t hash = _hash(key);
  size_t hash_index = hash % _array.size();

  for (auto it = iterator(_array[hash_index]); it != end(); ++it) {
    ListNode* node = static_cast<ListNode*>(it.get_node());

    if (_equal_to(node->value_ptr()->first, key)) {
      return node->value_ptr()->second;
    }
  }

  return _array[hash_index]->value_ptr()->second;
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(const_iterator pos) {
  size_t hash_index = static_cast<ListNode*>(pos.get_node())->hash % _array.size();
  --_size;
  if ((_array[hash_index] == pos.get_node()) &&
      (pos.get_node()->next == _list.end().get_node() ||
       static_cast<ListNode*>(pos.get_node()->next)->hash % _array.size() != hash_index)) {
    _array[hash_index] = nullptr;
    return _list.erase(pos);
  } else if (_array[hash_index] == pos.get_node()) {
    _array[hash_index] = static_cast<ListNode*>(pos.get_node()->next);
    return _list.erase(pos);
  }
  return _list.erase(pos);
}

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(const_iterator first, const_iterator last) {
  if (first == last) {
    return iterator(static_cast<ListNode*>(first.get_node()));
  }

  for (; first != last; ) {
    first = erase(first);
  }

  return iterator(first.get_node());
}
