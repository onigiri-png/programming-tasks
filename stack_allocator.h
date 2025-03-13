
#include <memory>

template <typename T, size_t N>
class StackAllocator;

template <size_t N>
class StackStorage {
 public:
  char storage[N];
  size_t shift = 0;
};

template <typename T, size_t N>
class StackAllocator {
 public:
  using value_type = T;
  using propagate_on_container_copy_assignment = std::true_type;

  StackAllocator(): _storage(nullptr) {}

  explicit StackAllocator(StackStorage<N>& storage): _storage(&storage) {}

  template <typename U>
  explicit StackAllocator(const StackAllocator<U, N>& alloc): _storage(alloc._storage) {}

  T* allocate(size_t count);

  void deallocate(T* ptr, size_t count);

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

 private:
  StackStorage<N>* _storage;

  template <typename U, size_t M>
  friend class StackAllocator;

  template <typename L, size_t K, typename U, size_t M>
  friend bool operator==(const StackAllocator<L, K>& alloc1, const StackAllocator<U, M>& alloc2);
};

template <typename T, size_t N>
T* StackAllocator<T, N>::allocate(size_t count) {
  void* ptr = _storage->storage + _storage->shift;
  size_t free_space = N - _storage->shift;
  if(std::align(alignof(T), count * sizeof(T), ptr, free_space) == nullptr) {
    throw std::bad_alloc();
  }
  _storage->shift = (N - free_space) + count * sizeof(T);
  return reinterpret_cast<T*>(ptr);
}

template <typename T, size_t N>
void StackAllocator<T, N>::deallocate(T* ptr, size_t count) {
  std::ignore = ptr;
  std::ignore = count;
}

template <typename T, size_t N, typename U, size_t M>
bool operator==(const StackAllocator<T, N>& alloc1, const StackAllocator<U, M>& alloc2) {
  return alloc1._storage == alloc2._storage;
}

template <typename T, size_t N, typename U, size_t M>
bool operator!=(const StackAllocator<T, N>& alloc1, const StackAllocator<U, M>& alloc2) {
  return !(alloc1 == alloc2);
}
