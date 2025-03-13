#include <string>
#include <cmath>
#include <iterator>

template <typename T>
class Deque {
 public:
  ~Deque() noexcept;

  explicit Deque();

  explicit Deque(int size);

  Deque(int size, const T& value);

  Deque(const Deque<T>& deque);

  Deque<T>& operator=(const Deque<T>& deque);

  size_t size() const;

  void push_back(const T& value);

  void push_front(const T& value);

  void pop_back();

  void pop_front();

  T& operator[](size_t index);

  const T& operator[](size_t index) const;

  T& at(size_t index);

  const T& at(size_t index) const;

  template <bool is_const = false>
  class _iterator {
   public:
    using value_type = std::conditional_t<is_const, const T, T>;
    using pointer = std::conditional_t<is_const, const T*, T*>;
    using difference_type = int;
    using reference = std::conditional_t<is_const, const T&, T&>;
    using iterator_category = std::random_access_iterator_tag;

    _iterator() = default;

    explicit _iterator(size_t index, const Deque<T>* deque)
      : _outer(deque->_outer),
        _index(index) {
      if ((deque->_first_alloc_index == deque->_last_alloc_index) &&
          (deque->_inner_first_index + index <= deque->_inner_last_index)) {
        _outer_index = deque->_first_alloc_index;
        _inner_index = deque->_inner_first_index + index;
      } else {
        if (deque->_inner_first_index + index < _inner_size) {
          _outer_index = deque->_first_alloc_index;
          _inner_index = deque->_inner_first_index + index;
        } else {
          index -= (_inner_size - deque->_inner_first_index);
          _outer_index = index / _inner_size + deque->_first_alloc_index + 1;
          _inner_index = index % _inner_size;
        }
      }

      if (_outer_index < deque->_outer_size) {
        _current = _outer[_outer_index] + _inner_index;
      }
    }

    _iterator<is_const>& operator++() {
      if (_inner_index + 1 == _inner_size) {
        _inner_index = 0;
        ++_outer_index;
        _current = _outer[_outer_index] + _inner_index;
      } else {
        ++_inner_index;
        ++_current;
      }
      ++_index;
      return *this;
    }

    _iterator<is_const> operator++(int) {
      _iterator it = *this;
      ++*this;
      return it;
    }

    _iterator<is_const>& operator+=(difference_type n) {
      if (n < 0) {
        return *this -= -n;
      }
      if (n == 1) {
        return operator++();
      }
      _index += n;

      if (_inner_index + n < _inner_size) {
        _inner_index += n;
        if (_current != nullptr) {
          _current += n;
        } else {
          _current = _outer[_outer_index] + _inner_index + n;
        }
      } else {
        n -= static_cast<difference_type>(_inner_size - _inner_index);
        size_t add = static_cast<size_t>(n) / _inner_size;
        _inner_index = static_cast<size_t>(n) % _inner_size;
        _outer_index += add + 1;
        _current = _outer[_outer_index] + _inner_index;
      }
      return *this;
    }

    _iterator<is_const> operator+(difference_type n) const {
      _iterator<is_const> it;
      it._index = _index;
      it._outer_index = _outer_index;
      it._inner_index = _inner_index;
      it._outer = _outer;
      it._current = _current;

      return it += n;
    }

    _iterator<is_const> operator-(difference_type n) const {
      _iterator<is_const> it;
      it._index = _index;
      it._outer_index = _outer_index;
      it._inner_index = _inner_index;
      it._outer = _outer;
      it._current = _current;

      return it -= n;
    }

    _iterator<is_const>& operator--() {
      if (_inner_index >= 1) {
        --_inner_index;
        --_current;
      } else {
        _inner_index = _inner_size - 1;
        --_outer_index;
        _current = _outer[_outer_index] + _inner_index;
      }
      --_index;
      _is_end = false;
      return *this;
    }

    _iterator<is_const> operator--(int) {
      _iterator it = *this;
      --*this;
      return it;
    }

    _iterator<is_const>& operator-=(difference_type n) {
      if (n < 0) {
        return *this += -n;
      }

      if (n == 1) {
        return operator--();
      }

      _index -= n;
      if (static_cast<size_t>(n) <= _inner_index) {
        _inner_index -= static_cast<size_t>(n);
        if (_current != nullptr) {
          _current -= n;
        } else {
          _current = _outer[_outer_index] + _inner_index - n;
        }
      } else {
        n -= static_cast<difference_type>(_inner_index);
        size_t add = static_cast<size_t>(n) / _inner_size;
        _inner_index = _inner_size - (static_cast<size_t>(n) % _inner_size);
        _outer_index -= (add + 1ul);
        _outer_index += _inner_index / _inner_size;
        _inner_index %= _inner_size;
        _current = _outer[_outer_index] + _inner_index;
      }
      return *this;
    }

    operator _iterator<true>() {
      _iterator<true> it;
      it._current = _current;
      it._inner_index = _inner_index;
      it._outer_index = _outer_index;
      it._index = _index;
      it._outer = _outer;

      return it;
    }

    reference operator*() const {
      return *_current;
    }

    pointer operator->() const {
      return _current;
    }

    difference_type operator-(const _iterator<true>& it) const {
      return static_cast<difference_type>(static_cast<long>(_index) -
        static_cast<long>(it._index));
    }

    bool operator<(const _iterator<true>& it) const {
      if (_outer_index != it._outer_index) {
        return _outer_index < it._outer_index;
      }
      return _inner_index < it._inner_index;
    }

    bool operator>(const _iterator<true>& it) const {
      if (_outer_index != it._outer_index) {
        return _outer_index > it._outer_index;
      }
      return _inner_index > it._inner_index;
    }

    bool operator==(const _iterator<true>& it) const {
      return !(operator<(it) || operator>(it));
    }

    bool operator!=(const _iterator<true>& it) const {
      return !operator==(it);
    }

    bool operator<=(const _iterator<true>& it) const {
      return operator<(it) || operator==(it);
    }

    bool operator>=(const _iterator<true>& it) const {
      return operator>(it) || operator==(it);
    }

    bool operator<(const _iterator<false>& it) const {
      if (_outer_index != it._outer_index) {
        return _outer_index < it._outer_index;
      }
      return _inner_index < it._inner_index;
    }

    bool operator>(const _iterator<false>& it) const {
      if (_outer_index != it._outer_index) {
        return _outer_index > it._outer_index;
      }
      return _inner_index > it._inner_index;
    }

    bool operator==(const _iterator<false>& it) const {
      return !(operator<(it) || operator>(it));
    }

    bool operator!=(const _iterator<false>& it) const {
      return !operator==(it);
    }

    bool operator<=(const _iterator<false>& it) const {
      return operator<(it) || operator==(it);
    }

    bool operator>=(const _iterator<false>& it) const {
      return operator>(it) || operator==(it);
    }
   private:
    friend class Deque<T>;

    bool _is_end{false};
    T** _outer;
    size_t _inner_index;
    size_t _outer_index;
    size_t _index;
    pointer _current{nullptr};
  };

  using const_iterator = _iterator<true>;
  using iterator = _iterator<false>;

  _iterator<false> insert(const _iterator<true>& it, const T& value);

  _iterator<false> erase(const _iterator<true>& it);

  _iterator<false> begin() {
    _iterator<false> it(0, this);
    return it;
  }
  
  _iterator<false> end() {
    _iterator<false> it(_size, this);
    return it;
  }

  _iterator<true> cbegin() const {
    _iterator<true> it(0, this);
    return it;
  }

  _iterator<true> cend() const {
    _iterator<true> it(_size, this);
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

 private:
  friend class _iterator<false>;
  friend class _iterator<true>;

  void _swap(Deque<T>& deque);
  void _define_size(int size);
  void _clear_mem(size_t i, size_t j);

  static const size_t _inner_size;

  T** _outer;
  size_t _outer_size = 0;
  size_t _size = 0;

  size_t _alloc_count = 0;
  size_t _first_alloc_index = 0;
  size_t _last_alloc_index = 0;

  size_t _inner_first_index = 0;
  size_t _inner_last_index = 0;
};

template <typename T>
const size_t Deque<T>::_inner_size = 32;

template <typename T>
void Deque<T>::_swap(Deque<T>& deque) {
  std::swap(_outer, deque._outer);
  std::swap(_outer_size, deque._outer_size);
  std::swap(_size, deque._size);
  std::swap(_alloc_count, deque._alloc_count);
  std::swap(_first_alloc_index, deque._first_alloc_index);
  std::swap(_last_alloc_index, deque._last_alloc_index);
  std::swap(_inner_first_index, deque._inner_first_index);
  std::swap(_inner_last_index, deque._inner_last_index);
}

template <typename T>
void Deque<T>::_define_size(int size) {
  _alloc_count = static_cast<size_t>(std::ceil(static_cast<double>(size) / _inner_size));
  _outer_size = 3 * _alloc_count;
  _outer = reinterpret_cast<T**>(new char[sizeof(T*) * _outer_size]);
  _size = size;
  
  _first_alloc_index = _alloc_count;
  _last_alloc_index = _alloc_count;
  _inner_first_index = 0;

  _inner_last_index = 0;
}

template <typename T>
void Deque<T>::_clear_mem(size_t i, size_t j) {
  for (; j--;) {
      (_outer[i] + j)->~T();
  }

  delete[] reinterpret_cast<char*>(_outer[i]);

  for (; (i--) > _first_alloc_index;) {
    for (size_t k = 0; k < _inner_size; ++k) {
      (_outer[i] + k)->~T();
    }
    _last_alloc_index = i;
    delete[] reinterpret_cast<char*>(_outer[i]);
  }
  delete[] reinterpret_cast<char**>(_outer);
  _outer_size = 0;
  _size = 0;
  _alloc_count = 0;
  _first_alloc_index = 0;
  _last_alloc_index = 0;
}

template <typename T>
Deque<T>::~Deque() noexcept {
  for (size_t i = _last_alloc_index + 1; (i--) > _first_alloc_index + 1;) {
    for (size_t j = 0; j <= _inner_last_index; ++j) {
      (_outer[i] + j)->~T();
    }
    _inner_last_index = _inner_size - 1;

    delete[] reinterpret_cast<char*>(_outer[i]);
  }

  size_t constraint = _inner_size;
  if (_first_alloc_index == _last_alloc_index) {
    constraint = _inner_last_index;
  }
  for (size_t j = _inner_first_index; (_size > 0) && (j <= constraint); ++j) {
    if (j != _inner_size) {
      (_outer[_first_alloc_index] + j)->~T();
    }
  }

  delete[] reinterpret_cast<char*>(_outer[_first_alloc_index]);
  delete[] reinterpret_cast<char**>(_outer);
}

template <typename T>
Deque<T>::Deque(): _outer(reinterpret_cast<T**>(new char[sizeof(T*) * 5])),
  _outer_size(5), _alloc_count(1), _first_alloc_index(2), _last_alloc_index(2) {
  try {
      _outer[2] = reinterpret_cast<T*>(new char[_inner_size * sizeof(T)]);
  } catch (...) {
    delete[] reinterpret_cast<char**>(_outer);
    _outer_size = 0;
    _alloc_count = 0;
    _first_alloc_index = 0;
    _last_alloc_index = 0;
    throw;
  }
}

template <typename T>
Deque<T>::Deque(int size) {
  _define_size(size);

  size_t i = _alloc_count;
  size_t j = 0;
  for (; (size > 0) && (i < _outer_size); ++i) {
    j = 0;
    _outer[i] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
    _last_alloc_index = i;
    for (; (size > 0) && (j < _inner_size); ++j) {
      _inner_last_index = j % _inner_size;
      try {
        new (_outer[i] + j) T();
      } catch (...) {
        _clear_mem(i, j);
        throw;
      }
      --size;
    }
  }
}

template <typename T>
Deque<T>::Deque(int size, const T& value) {
  _define_size(size);

  size_t i = _alloc_count;
  size_t j = 0;
  try {
    for (; size > 0; ++i) {
      j = 0;
      _outer[i] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
      _last_alloc_index = i;
      for (; (size > 0) && (j < _inner_size); ++j) {
        _inner_last_index = j % _inner_size;
        new (_outer[i] + j) T(value);
        --size;
      }
    }
  } catch (...) {
    _clear_mem(i, j);  
    throw;
  }
}

template <typename T>
Deque<T>::Deque(const Deque<T>& deque): _first_alloc_index(deque._first_alloc_index), _last_alloc_index(deque._last_alloc_index) {
  _outer = reinterpret_cast<T**>(new char[sizeof(T*) * deque._outer_size]);
  size_t i = deque._first_alloc_index;
  size_t j = 0;
  try {
    _outer[i] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
    size_t constraint = _inner_size;
    bool is_one_alloced = deque._first_alloc_index == deque._last_alloc_index;
    if (is_one_alloced) {
      constraint = deque._inner_last_index;
    }
    for (j = deque._inner_first_index; j <= constraint; ++j) {
      if (j != _inner_size) {
        new (_outer[i] + j) T(deque._outer[i][j]);
      }
    }
    ++i;
    for (; i < deque._last_alloc_index; ++i) {
      j = 0;
      _outer[i] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
      for (; j < _inner_size; ++j) {
        new (_outer[i] + j) T(deque._outer[i][j]);
      }
    }

    if (!is_one_alloced) {
      _outer[i] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
      for (j = 0; j <= deque._inner_last_index; ++j) {
        new (_outer[i] + j) T(deque._outer[i][j]);
      }
    }
  } catch (...) {
    _clear_mem(i, j);
    throw;
  }

  _outer_size = deque._outer_size;
  _first_alloc_index = deque._first_alloc_index;
  _size = deque._size;

  _alloc_count = deque._alloc_count;
  _first_alloc_index = deque._first_alloc_index;
  _last_alloc_index = deque._last_alloc_index;

  _inner_first_index = deque._inner_first_index;
  _inner_last_index = deque._inner_last_index;

}

template <typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& deque) {
  if (this == &deque) {
    return *this;
  }
  Deque<T> deque_copy = deque;
  _swap(deque_copy);
  return *this;
}

template <typename T>
size_t Deque<T>::size() const {
  return _size;
}

template <typename T>
void Deque<T>::push_back(const T& value) {
  if (_size == 0) {
    new (_outer[_last_alloc_index]) T(value);
    _inner_first_index = 0;
    _inner_last_index = 0;
  } else if (_inner_last_index + 1 < _inner_size) {
    new (_outer[_last_alloc_index] + _inner_last_index + 1) T(value);
    ++_inner_last_index;
  } else if (_last_alloc_index + 1 < _outer_size) {
    _outer[_last_alloc_index + 1] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
    try {
      new (_outer[_last_alloc_index + 1]) T(value);
    } catch (...) {
      delete[] reinterpret_cast<char*>(_outer[_last_alloc_index + 1]);
      throw;
    }
    ++_last_alloc_index;
    _inner_last_index = 0;
    ++_alloc_count;
  } else {
    T** new_outer = reinterpret_cast<T**>(new char[sizeof(T*) * (_outer_size + _alloc_count)]);
    for (size_t i = _first_alloc_index; i <= _last_alloc_index; ++i) {
      new_outer[i] = _outer[i];
    }

    try {
      new_outer[_last_alloc_index + 1] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
    } catch (...) {
      delete[] reinterpret_cast<char**>(new_outer);
      throw;
    }

    try {
      new (new_outer[_last_alloc_index + 1]) T(value);
    } catch (...) {
      delete[] reinterpret_cast<char*>(new_outer[_last_alloc_index + 1]);
      delete[] reinterpret_cast<char**>(new_outer);
      throw;
    }

    delete[] reinterpret_cast<char**>(_outer);
    _outer = new_outer;
    ++_last_alloc_index;
    _inner_last_index = 0;
    _outer_size += _alloc_count;
    ++_alloc_count;
  }

  ++_size;
}

template <typename T>
void Deque<T>::push_front(const T& value) {
  if (_size == 0) {
    new (_outer[_last_alloc_index]) T(value);
    _inner_first_index = 0;
    _inner_last_index = 0;
  } else if (_inner_first_index > 0) {
    new (_outer[_first_alloc_index] + _inner_first_index - 1) T(value);
    --_inner_first_index;
  } else if (_first_alloc_index > 0) {
    _outer[_first_alloc_index - 1] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
    try {
      new (_outer[_first_alloc_index - 1] + _inner_size - 1) T(value);
    } catch (...) {
      delete[] reinterpret_cast<char*>(_outer[_first_alloc_index - 1]);
      throw;
    }
    --_first_alloc_index;
    _inner_first_index = _inner_size - 1;
    ++_alloc_count;
  } else {
    T** new_outer;
    new_outer = reinterpret_cast<T**>(new char[sizeof(T*) * (_outer_size + _alloc_count)]);
    for (size_t i = _first_alloc_index; i <= _last_alloc_index; ++i) {
      new_outer[i + _alloc_count] = _outer[i];
    }

    try {
      new_outer[_alloc_count - 1] = reinterpret_cast<T*>(new char[sizeof(T) * _inner_size]);
    } catch (...) {
      delete[] reinterpret_cast<char**>(new_outer);
      throw;
    }

    try {
      new (new_outer[_alloc_count - 1] + _inner_size - 1) T(value);
    } catch (...) {
      delete[] reinterpret_cast<char*>(new_outer[_alloc_count - 1]);
      delete[] reinterpret_cast<char**>(new_outer);
      throw;
    }

    delete[] reinterpret_cast<char**>(_outer);
    _outer = new_outer;
    _first_alloc_index = _alloc_count - 1;
    _last_alloc_index += _alloc_count;
    _inner_first_index = _inner_size - 1;
    _outer_size += _alloc_count;
    ++_alloc_count;
  }

  ++_size;
}

template <typename T>
T& Deque<T>::operator[](size_t index) {
  _iterator<false> it(index, this);
  return *it;
}

template <typename T>
const T& Deque<T>::operator[](size_t index) const {
  _iterator<true> it(index, this);
  return *it;
}

template <typename T>
T& Deque<T>::at(size_t index) {
  if (index >= _size) {
    throw std::out_of_range(std::to_string(index) + " >= " + std::to_string(_size));
  }
  return operator[](index);
}

template <typename T>
const T& Deque<T>::at(size_t index) const {
  if (index >= _size) {
    throw std::out_of_range(std::to_string(index) + " >= " + std::to_string(_size));
  }

  return operator[](index);
}

template <typename T>
void Deque<T>::pop_back() {
  (_outer[_last_alloc_index] + _inner_last_index)->~T();
  if (_inner_last_index == 0) {
    if (_last_alloc_index != _first_alloc_index) {
      delete[] reinterpret_cast<char*>(_outer[_last_alloc_index]);
      --_last_alloc_index;
      _inner_last_index = _inner_size - 1;
      --_alloc_count;
    }
  } else {
    --_inner_last_index;
  }
  --_size;
}

template <typename T>
void Deque<T>::pop_front() {
  (_outer[_first_alloc_index] + _inner_first_index)->~T();
  if (_inner_first_index == _inner_size - 1) {
    if (_last_alloc_index != _first_alloc_index) {
      delete[] reinterpret_cast<char*>(_outer[_first_alloc_index]);
      ++_first_alloc_index;
      --_alloc_count;
    }
    _inner_first_index = 0;
  } else {
    ++_inner_first_index;
  }
  --_size;
}

template <typename T>
typename Deque<T>::template _iterator<false> Deque<T>::insert(const _iterator<true>& it,
  const T& value) {

  push_back(value);
  for (_iterator<false> it1 = end() - 1; it1 != it; --it1) {
    new (_outer[it1._outer_index] + it1._inner_index) T(*((it1 - 1)._current));
  }

  if (_size != 1) {
    new (_outer[it._outer_index] + it._inner_index) T(value);
  }

  return _iterator<false>(it._index, this);
}

template <typename T>
typename Deque<T>::template _iterator<false> Deque<T>::erase(const _iterator<true>& it) {
  _iterator<false> it1(it._index, this);
  for (; it1 != end() - 1; ++it1) {
    new (_outer[it1._outer_index] + it1._inner_index) T(*((it1 + 1)._current));
  }
  pop_back();

  return _iterator<false>(it._index, this);
}
