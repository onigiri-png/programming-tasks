#include <cmath>
#include <stdexcept>
#include <functional>
#include <memory>
#include <vector>
#include <type_traits>
#include <utility>

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
class UnorderedMap;

template <typename T, typename Allocator = std::allocator<T>>
class List {
 public:
  List(const Allocator& alloc = Allocator()): _alloc(alloc), _node_alloc(alloc) {}

  List(size_t count, const Allocator& alloc = Allocator());

  List(size_t count, const T& value, const Allocator& alloc = Allocator());

  List(const List<T, Allocator>& other);

  List(List<T, Allocator>&& other) noexcept;

  List<T, Allocator>& operator=(const List<T, Allocator>& other);

  List<T, Allocator>& operator=(List<T, Allocator>&& other) noexcept;

  ~List();
  
  Allocator get_allocator() const;

  size_t size() const;

 private:
  class BaseNode;
  class Node;
 
 public:
  template <bool is_const = false>
  class _iterator {
   public:
    using value_type = std::conditional_t<is_const, const T, T>;
    using pointer = std::conditional_t<is_const, const T*, T*>;
    using difference_type = int;
    using reference = std::conditional_t<is_const, const T&, T&>;
    using iterator_category = std::bidirectional_iterator_tag;

    _iterator() = default;

    explicit _iterator(typename List<T, Allocator>::BaseNode* ptr): _current(ptr) {}

    explicit _iterator(typename List<T, Allocator>::Node* ptr): _current(ptr) {}

    _iterator(const _iterator<false>& it): _current(it._current) {}

    BaseNode* get_node() const {
      return _current;
    }

    _iterator<is_const>& operator=(const _iterator<false>& it) {
      _current = it._current;
      return *this;
    }

    _iterator<is_const>& operator++() {
      _current = _current->next;
      return *this;
    }

    _iterator<is_const> operator++(int) {
      _iterator it = *this;
      ++*this;
      return it;
    }

    _iterator<is_const>& operator--() {
      _current = _current->prev;
      return *this;
    }

    _iterator<is_const> operator--(int) {
      _iterator it = *this;
      --*this;
      return it;
    }

    operator _iterator<true>() {
      _iterator<true> it(_current);
      return it;
    }

    reference operator*() const {
      return *static_cast<Node*>(_current)->value_ptr();
    }

    pointer operator->() const {
      return static_cast<Node*>(_current)->value_ptr();
    }

    difference_type operator-(const _iterator<true>& it) const {
      return static_cast<difference_type>(_current - it._current);
    }

    bool operator==(const _iterator<true>& it) const {
      return _current == it._current;
    }

    bool operator!=(const _iterator<true>& it) const {
      return !operator==(it);
    }

    bool operator==(const _iterator<false>& it) const {
      return _current == it._current;
    }

    bool operator!=(const _iterator<false>& it) const {
      return !operator==(it);
    }

   private:
    friend class List<T, Allocator>;

    typename List<T, Allocator>::BaseNode* _current;
  };

  using const_iterator = _iterator<true>;
  using iterator = _iterator<false>;
  using reverse_iterator = std::reverse_iterator<_iterator<false>>;
  using const_reverse_iterator = std::reverse_iterator<_iterator<true>>;

  _iterator<false> begin() {
    _iterator<false> it(_ptr);
    return it;
  }
  
  _iterator<false> end() {
    _iterator<false> it(const_cast<BaseNode*>(&_fake_node));
    return it;
  }

  _iterator<true> cbegin() const {
    _iterator<true> it(_ptr);
    return it;
  }

  _iterator<true> cend() const {
    _iterator<true> it(const_cast<BaseNode*>(&_fake_node));
    return it;
  }

  _iterator<true> end() const {
    return cend();
  }

  _iterator<true> begin() const {
    return cbegin();
  }

  auto rbegin() {
    return std::make_reverse_iterator(end());
  }

  auto rend() {
    return std::make_reverse_iterator(begin());
  }

  auto crbegin() const {
    return std::make_reverse_iterator(cend());
  }

  auto crend() const {
    return std::make_reverse_iterator(cbegin());
  }

  auto rbegin() const {
    return crbegin();
  }

  auto rend() const {
    return crend();
  }

  void splice(_iterator<true> pos, List<T, Allocator>& other);

  void splice(_iterator<true> pos, List<T, Allocator>& other, _iterator<true> it);

  template <typename... Args>
  _iterator<false> emplace(const _iterator<true>& it, size_t hash, Args&&... args);

  template <typename... Args>
  void emplace_back(size_t hash, Args&&... args);

  template <typename... Args>
  void emplace_front(size_t hash, Args&&... args);

  _iterator<false> insert(const _iterator<true>& it, const T& value, size_t hash);

  _iterator<false> insert(const _iterator<true>& it, T&& value, size_t hash);

  void push_back(const T& value, size_t hash);

  void push_back(T&& value, size_t hash);

  void push_front(const T& value, size_t hash);

  void push_front(T&& value, size_t hash);

  _iterator<false> erase(const _iterator<true>& it);

  _iterator<false> erase(_iterator<true> first, _iterator<true> last);

  void pop_back();

  void pop_front();

 private:
  struct BaseNode {
    BaseNode* next = this;
    BaseNode* prev = this;
  };

  /*
  struct Node : BaseNode {
    using AllocatorTraits = std::allocator_traits<Allocator>;

    Node(const Node& other): hash(other.hash) {
      alloc = AllocatorTraits::select_on_container_copy_construction(other.alloc);
      AllocatorTraits::construct(alloc, reinterpret_cast<T*>(value_), *reinterpret_cast<T*>(other.value_));
    }

    Node(Node&& other): alloc(std::move(alloc)), hash(std::exchange(other.hash, 0)) {
      AllocatorTraits::construct(alloc, reinterpret_cast<T*>(value_), std::move(*reinterpret_cast<T*>(other.value_)));
    }

    template <typename... Args>
    Node(size_t hash, Args&&... args): hash(hash) {
      AllocatorTraits::construct(alloc, reinterpret_cast<T*>(value_), std::forward<Args>(args)...);
    }

    T* value_ptr() const {
      return reinterpret_cast<T*>(value_);
    }

    ~Node() {
      AllocatorTraits::destroy(alloc, reinterpret_cast<T*>(value_));
    }

    [[no_unique_address]] Allocator alloc;

    size_t hash;

   private:
    alignas(T) mutable char value_[sizeof(T)];
  };
  */

  struct Node : BaseNode {
    using AllocatorTraits = std::allocator_traits<Allocator>;

    Node(const Node& other): hash(other.hash) {
      alloc = AllocatorTraits::select_on_container_copy_construction(other.alloc);
      AllocatorTraits::construct(alloc, reinterpret_cast<T*>(value_), *reinterpret_cast<T*>(other.value_));
    }

    Node(Node&& other): alloc(std::move(alloc)), hash(std::exchange(other.hash, 0)) {
      AllocatorTraits::construct(alloc, reinterpret_cast<T*>(value_), std::move(*reinterpret_cast<T*>(other.value_)));
    }

    template <typename... Args>
    Node(size_t hash, Args&&... args): hash(hash) {
      AllocatorTraits::construct(alloc, reinterpret_cast<T*>(value_), std::forward<Args>(args)...);
    }

    T* value_ptr() const {
      return reinterpret_cast<T*>(value_);
    }

    ~Node() {
      AllocatorTraits::destroy(alloc, reinterpret_cast<T*>(value_));
    }

    [[no_unique_address]] Allocator alloc;

    size_t hash;

   private:
    alignas(T) mutable char value_[sizeof(T)];
  };

  using NodeAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using AllocTraits = std::allocator_traits<NodeAlloc>;

  friend class _iterator<false>;
  friend class _iterator<true>;
  
  template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
  friend class UnorderedMap;

  void _init_fake_node(Node* last_el);

  void _init_node(Node* cur, Node* prev);

  void _clear_mem(size_t i, Node* cur, Node* prev);

  void _init_ptr(size_t& i, Node* prev);

  [[no_unique_address]] Allocator _alloc;
  [[no_unique_address]] NodeAlloc _node_alloc;
  BaseNode _fake_node;
  size_t _size = 0;
  Node* _ptr = reinterpret_cast<Node*>(&_fake_node);
};

template <typename T, typename Allocator>
void List<T, Allocator>::_init_fake_node(Node* last_el) {
  _fake_node.next = _ptr;
  _fake_node.prev = last_el;
  last_el->next = &_fake_node;
}

template <typename T, typename Allocator>
void List<T, Allocator>::_init_node(Node* cur, Node* prev) {
  cur->prev = prev;
  prev->next = cur;
}

template <typename T, typename Allocator>
void List<T, Allocator>::_clear_mem(size_t i, Node* cur, Node* prev) {
  cur = prev;
  prev = static_cast<Node*>(prev->prev);
  for (size_t j = i; j--; ) {
    AllocTraits::destroy(_node_alloc, cur);
    AllocTraits::deallocate(_node_alloc, cur, 1);
    cur = prev;
    prev = static_cast<Node*>(cur->prev);
  }
}

template <typename T, typename Allocator>
void List<T, Allocator>::_init_ptr(size_t& i, Node* prev) {
  _ptr = prev;
  _ptr->prev = &_fake_node;
  ++i;
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t count, const Allocator& alloc): _alloc(alloc), _node_alloc(alloc) {
  size_t i = 0;
  Node* cur = nullptr;
  Node* prev = AllocTraits::allocate(_node_alloc, 1);
  try {
    AllocTraits::construct(_node_alloc, prev);
    _init_ptr(i, prev);

    if (i == count) {
      cur = prev;
    }

    for (; i < count; ++i) {
      cur = AllocTraits::allocate(_node_alloc, 1);
      AllocTraits::construct(_node_alloc, cur);
      _init_node(cur, prev);
      prev = cur;
    }
  } catch (...) {
    _clear_mem(i, cur, prev);
    throw;
  }
  _size = count;

  if (count != 0) {
    _init_fake_node(cur);
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t count, const T& value, const Allocator& alloc): _alloc(alloc),
  _node_alloc(alloc) {
  size_t i = 0;
  Node* cur = nullptr;
  Node* prev = AllocTraits::allocate(_node_alloc, 1);
  try {
    AllocTraits::construct(_node_alloc, prev, value);
    _init_ptr(i, prev);
    if (i == count) {
      cur = prev;
    }

    for (; i < count; ++i) {
      cur = AllocTraits::allocate(_node_alloc, 1);
      AllocTraits::construct(_node_alloc, cur, value);
      _init_node(cur, prev);
      prev = cur;
    }
  } catch (...) {
    _clear_mem(i, cur, prev);
    throw;
  }

  _size = count;
  if (count != 0) {
    _init_fake_node(cur);
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const List<T, Allocator>& other): _size(other._size) {
  _alloc = AllocTraits::select_on_container_copy_construction(other._alloc);
  _node_alloc = AllocTraits::select_on_container_copy_construction(other._node_alloc);

  Node* cur = nullptr;
  Node* prev = AllocTraits::allocate(_node_alloc, 1);
  _iterator<true> it = other.begin();
  size_t i = 0;
  try {
    if (it != other.end()) {
      AllocTraits::construct(_node_alloc, prev, *static_cast<Node*>(it._current));
      _init_ptr(i, prev);
      ++it;
    } else {
      AllocTraits::deallocate(_node_alloc, prev, 1);
    }

    if (it == other.end()) {
      cur = prev;
    }

    for (; it != other.end(); ++it, ++i) {
      cur = AllocTraits::allocate(_node_alloc, 1);
      AllocTraits::construct(_node_alloc, cur, *static_cast<Node*>(it._current));
      _init_node(cur, prev);
      prev = cur;
    }
  } catch (...) {
    _clear_mem(i, cur, prev);
    throw;
  }

  if (_size == 1) {
    cur = prev;
  }

  if (_size != 0) {
    _init_fake_node(cur);
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(List<T, Allocator>&& other) noexcept: 
  _alloc(std::move(other._alloc)), _node_alloc(std::move(other._node_alloc)),
  _size(other._size) {
  if (other._size != 0) {
    _ptr = other._ptr;
    _fake_node.prev = _ptr->prev->prev;
    _fake_node.next = _ptr;
    _ptr->prev->prev->next = &_fake_node;
    _ptr->prev = &_fake_node;
    other._fake_node.next = other._fake_node.prev = &other._fake_node;
    other._ptr = static_cast<Node*>(&other._fake_node);
  } else {
    _ptr = static_cast<Node*>(&_fake_node);
    _ptr->next = &_fake_node;
    _ptr->prev = &_fake_node;
  }
  other._size = 0;
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List<T, Allocator>& other) {
  if (this == &other) {
    return *this;
  }

  Node* old_ptr = _ptr;
  if (AllocTraits::propagate_on_container_copy_assignment::value) {
    Node* cur = nullptr;
    Node* prev = AllocTraits::allocate(const_cast<NodeAlloc&>(other._node_alloc), 1);
    _iterator<true> it = other.begin();
    size_t i = 0;
    try {
      if (it != other.end()) {
        AllocTraits::construct(const_cast<NodeAlloc&>(other._node_alloc),
          prev, *static_cast<Node*>(it._current));
        _init_ptr(i, prev);
        ++it;
      } else {
        AllocTraits::deallocate(const_cast<NodeAlloc&>(other._node_alloc), prev, 1);
      }

      if (it == other.end()) {
        cur = prev;
      }

      for (; it != other.end(); ++it, ++i) {
        cur = AllocTraits::allocate(const_cast<NodeAlloc&>(other._node_alloc), 1);
        AllocTraits::construct(const_cast<NodeAlloc&>(other._node_alloc),
            cur, *static_cast<Node*>(it._current));
        _init_node(cur, prev);
        prev = cur;
      }
    } catch (...) {
      _clear_mem(i, cur, prev);
      _ptr = old_ptr;
      throw;
    }

    Node* finish = reinterpret_cast<Node*>(old_ptr->prev);
    for (_iterator<false> it = _iterator<false>(old_ptr); it != _iterator<false>(finish);) {
      _iterator<false> cur = it++;
      AllocTraits::destroy(_node_alloc, static_cast<Node*>(cur._current));
      AllocTraits::deallocate(_node_alloc, static_cast<Node*>(cur._current), 1);
    }

    if (_size != 0) {
      _init_fake_node(cur);
    }
    _node_alloc = const_cast<NodeAlloc&>(other._node_alloc);
    _alloc = const_cast<Allocator&>(other._alloc);
  } else {
    Node* cur = nullptr;
    Node* prev = AllocTraits::allocate(_node_alloc, 1);
    _iterator<true> it = other.begin();
    size_t i = 0;
    try {
      if (it != other.end()) {
        AllocTraits::construct(const_cast<NodeAlloc&>(other._node_alloc),
          prev, *static_cast<Node*>(it._current));
        _init_ptr(i, prev);
        ++it;
      } else {
        AllocTraits::deallocate(const_cast<NodeAlloc&>(other._node_alloc), prev, 1);
      }

      if (it == other.end()) {
        cur = prev;
      }

      for (; it != other.end(); ++it, ++i) {
        cur = AllocTraits::allocate(_node_alloc, 1);
        AllocTraits::construct(_node_alloc, cur, *static_cast<Node*>(it._current));
        _init_node(cur, prev);
        prev = cur;
      }
    } catch (...) {
      _clear_mem(i, cur, prev);
      _ptr = old_ptr;
      throw;
    }

    Node* finish = reinterpret_cast<Node*>(old_ptr->prev);
    for (_iterator<false> it = _iterator<false>(old_ptr); it != _iterator<false>(finish);) {
      _iterator<false> cur = it++;
      AllocTraits::destroy(_node_alloc, static_cast<Node*>(cur._current));
      AllocTraits::deallocate(_node_alloc, static_cast<Node*>(cur._current), 1);
    }

    if (_size != 0) {
      _init_fake_node(cur);
    }
  }
  _size = other._size;
  return *this;
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(List<T, Allocator>&& other) noexcept {
  Node* old_ptr = _ptr;
  Node* finish = reinterpret_cast<Node*>(old_ptr->prev);
  for (_iterator<false> it = _iterator<false>(old_ptr); it != _iterator<false>(finish);) {
   _iterator<false> cur = it++;
   AllocTraits::destroy(_node_alloc, static_cast<Node*>(cur._current));
   AllocTraits::deallocate(_node_alloc, static_cast<Node*>(cur._current), 1);
  }

  if (AllocTraits::propagate_on_container_move_assignment::value) {
    _alloc = std::move(other._alloc);
    _node_alloc = std::move(other._node_alloc);
  }

  _size = other._size;
  if (other._size != 0) {
    _ptr = other._ptr;
    _fake_node.prev = _ptr->prev->prev;
    _fake_node.next = _ptr;
    _ptr->prev->prev->next = &_fake_node;
    _ptr->prev = &_fake_node;
    other._fake_node.next = other._fake_node.prev = &other._fake_node;
    other._ptr = static_cast<Node*>(&other._fake_node);
  } else {
    _ptr = static_cast<Node*>(&_fake_node);
    _ptr->next = &_fake_node;
    _ptr->prev = &_fake_node;
  }
  other._size = 0;

  return *this;
}

template <typename T, typename Allocator>
List<T, Allocator>::~List() {
  for (_iterator<true> it = begin(); it != end();) {
    _iterator<true> cur = it++;
    AllocTraits::destroy(_node_alloc, static_cast<Node*>(cur._current));
    AllocTraits::deallocate(_node_alloc, static_cast<Node*>(cur._current), 1);
  }
}

template <typename T, typename Allocator>
Allocator List<T, Allocator>::get_allocator() const {
  return _alloc;
}

template <typename T, typename Allocator>
size_t List<T, Allocator>::size() const {
  return _size;
}

template <typename T, typename Allocator>
template <typename... Args>
typename List<T, Allocator>::template _iterator<false>
List<T, Allocator>::emplace(const _iterator<true>& it, size_t hash, Args&&... args) {
  Node* node = AllocTraits::allocate(_node_alloc, 1);
  try {
    AllocTraits::construct(_node_alloc, node, hash, std::forward<Args>(args)...);
  } catch (...) {
    AllocTraits::deallocate(_node_alloc, node, 1);
    throw;
  }
  node->next = it._current;
  node->prev = it._current->prev;
  it._current->prev->next = node;
  it._current->prev = node;
  if (_size == 0 || it._current == _ptr) {
    _ptr = node;
  }
  ++_size;

  return _iterator<false>(node);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::template _iterator<false>
List<T, Allocator>::insert(const _iterator<true>& it, const T& value, size_t hash) {
  return emplace(it, hash, value);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::template _iterator<false>
List<T, Allocator>::insert(const _iterator<true>& it, T&& value, size_t hash) {
  return emplace(it, hash, std::move(value));
}

template <typename T, typename Allocator>
template <typename... Args>
void List<T, Allocator>::emplace_back(size_t hash, Args&&... args) {
  emplace(end(), hash, std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
template <typename... Args>
void List<T, Allocator>::emplace_front(size_t hash, Args&&... args) {
  emplace(begin(), hash, std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value, size_t hash) {
  emplace_back(hash, value);
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_back(T&& value, size_t hash) {
  emplace_back(hash, std::move(value));
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value, size_t hash) {
  emplace_front(hash, value);
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_front(T&& value, size_t hash) {
  emplace_front(hash, std::move(value));
}

template <typename T, typename Allocator>
typename List<T, Allocator>::template _iterator<false>
List<T, Allocator>::erase(const _iterator<true>& it) {
  bool equal_to_begin = (it._current == _ptr);

  BaseNode* node = it._current;
  BaseNode* ans = node->next;
  node->prev->next = node->next;
  node->next->prev = node->prev;

  try {
    AllocTraits::destroy(_node_alloc, static_cast<Node*>(node));
  } catch (...) {
    node->prev->next = node;
    node->next->prev = node;
    throw;
  }

  AllocTraits::deallocate(_node_alloc, static_cast<Node*>(node), 1);
  if (equal_to_begin) {
    _ptr = static_cast<Node*>(ans);
  }
  --_size;
  return _iterator<false>(ans);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::template _iterator<false>
List<T, Allocator>::erase(_iterator<true> first, _iterator<true> last) {
  if (first == last) {
    return _iterator<false>(last._current);
  }

  for (; first != last;) {
    first = erase(first);
  }

  return first;
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
  _iterator<false> it = end();
  --it;
  erase(it);
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
  _iterator<false> it = begin();
  erase(it);
}

template <typename T, typename Allocator>
void List<T, Allocator>::splice(_iterator<true> pos, List<T, Allocator>& other) {
  if (other._size == 0) {
    return;
  }

  BaseNode* node = pos._current;
  BaseNode* prev_node = node->prev;

  BaseNode* first = other._ptr;
  BaseNode* last = other.end()._current->prev;

  prev_node->next = first;
  first->prev = prev_node;
  last->next = node;
  node->prev = last;
  if (node == _ptr) {
    _ptr = static_cast<Node*>(first);
  }
  other._fake_node.next = other._fake_node.prev = &other._fake_node;
  other._ptr = static_cast<Node*>(&other._fake_node);
  _size += other._size;
  other._size = 0;
}

template <typename T, typename Allocator>
void List<T, Allocator>::splice(_iterator<true> pos, List<T, Allocator>& other,
                                _iterator<true> it) {
  if (other._size == 0) {
    return;
  }

  BaseNode* node = pos._current;
  BaseNode* prev_node = node->prev;

  BaseNode* grabbed = it._current;

  grabbed->prev->next = grabbed->next;
  grabbed->next->prev = grabbed->prev;
  --other._size;
  
  if (grabbed == other._ptr) {
    other._ptr = static_cast<Node*>(grabbed->next);
  }

  grabbed->next = node;
  node->prev = grabbed;
  grabbed->prev = prev_node;
  prev_node->next = grabbed;

  if (node == _ptr) {
    _ptr = static_cast<Node*>(_ptr->prev);
  }
  ++_size;
}
