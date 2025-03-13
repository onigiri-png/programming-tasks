#include <memory>
#include <type_traits>

class BaseControlBlock;

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

template <typename T>
class EnableSharedFromThis {
public:
  SharedPtr<T> shared_from_this() {
    if (weak_this == nullptr) {
      throw std::bad_weak_ptr{};
    }
    return SharedPtr<T>(weak_this);
  }

  SharedPtr<T const> shared_from_this() const {
    if (weak_this == nullptr) {
      throw std::bad_weak_ptr{};
    }
    return SharedPtr<T>(weak_this);
  }

private:
  template <typename>
  friend class SharedPtr;

  BaseControlBlock* weak_this{nullptr};
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  using ControlBlockAlloc =
    typename std::allocator_traits<Alloc>::
    template rebind_alloc<typename SharedPtr<T>::template ControlBlockMakeShared<Alloc>>;
  using CBAllocTraits = std::allocator_traits<ControlBlockAlloc>;

  ControlBlockAlloc cb_alloc(alloc);
  auto cb = CBAllocTraits::allocate(cb_alloc, 1);
  CBAllocTraits::construct(cb_alloc, cb, alloc, std::forward<Args>(args)...);
  return SharedPtr<T>(cb);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

class BaseControlBlock {
  virtual void use_deleter() = 0;
  virtual ~BaseControlBlock() = default;
  virtual void* get_object() = 0;
  virtual void destroy() = 0;

  size_t shared_count;
  size_t weak_count;

  template <typename U>
  friend class SharedPtr;

  template <typename U>
  friend class WeakPtr;
};

template <typename T>
class SharedPtr {
public:
  SharedPtr() noexcept: cb(nullptr) {}

  template <typename Y, typename = std::enable_if_t<std::is_same_v<T, Y> ||
    std::is_base_of_v<T, Y>> >
  explicit SharedPtr(Y* ptr);

  SharedPtr(const SharedPtr& other) noexcept;
  
  template <typename Y, typename = std::enable_if_t<std::is_same_v<T, Y> ||
    std::is_base_of_v<T, Y>> >
  SharedPtr(const SharedPtr<Y>& other) noexcept;

  SharedPtr(SharedPtr&& other) noexcept;

  template <typename Y, typename = std::enable_if_t<std::is_same_v<T, Y> ||
    std::is_base_of_v<T, Y>> >
  SharedPtr(SharedPtr<Y>&& other) noexcept;

  template <typename Y, typename Deleter, typename = std::enable_if_t<std::is_same_v<T, Y> ||
    std::is_base_of_v<T, Y>> >
  SharedPtr(Y* ptr, Deleter deleter);

  template <typename Y, typename Deleter, typename Alloc,
    typename = std::enable_if_t<std::is_same_v<T, Y> || std::is_base_of_v<T, Y>> >
  SharedPtr(Y* ptr, Deleter deleter, Alloc alloc);

  template <typename Y, std::enable_if_t<std::is_same_v<T, Y> || std::is_base_of_v<T, Y>, bool> = true>
  explicit SharedPtr(const WeakPtr<Y>& weak): SharedPtr(static_cast<Y*>(weak.cb->get_object())) {}

  ~SharedPtr();

  SharedPtr& operator=(const SharedPtr& other);

  template <typename Y, typename = std::enable_if_t<std::is_same_v<T, Y> ||
    std::is_base_of_v<T, Y>> >
  SharedPtr& operator=(const SharedPtr<Y>& other);

  SharedPtr& operator=(SharedPtr&& other) noexcept;

  template <typename Y, typename = std::enable_if_t<std::is_same_v<T, Y> ||
    std::is_base_of_v<T, Y>> >
  SharedPtr& operator=(SharedPtr<Y>&& other) noexcept;

  size_t use_count() const { return cb->shared_count; }

  void swap(SharedPtr& other) noexcept;

  template <typename Y>
  void reset(Y* ptr) { SharedPtr<T>(ptr).swap(*this); }

  void reset() noexcept { SharedPtr().swap(*this); }

  T* get() const noexcept { return (cb ? static_cast<T*>(cb->get_object()) : nullptr); }

  T* operator->() const noexcept { return get(); }

  T& operator*() const noexcept { return *get(); }

private:
  template
    <typename Y = T, typename Deleter = std::default_delete<Y>, typename Alloc = std::allocator<Y>>
  class ControlBlockCommon : BaseControlBlock {
   public:
    explicit ControlBlockCommon(Y* ptr):
      _object(ptr) {
      BaseControlBlock::shared_count = 1;
      BaseControlBlock::weak_count = 0;
    }

    ControlBlockCommon(Y* ptr, const Deleter& deleter, const Alloc& alloc = Alloc()):
      _object(ptr), _deleter(deleter), _alloc(alloc) {

      BaseControlBlock::shared_count = 1;
      BaseControlBlock::weak_count = 0;
    }

   private:
    void use_deleter() override {
      _deleter(_object);
    }

    void* get_object() override {
      return _object;
    }

    void destroy() override {
      using ControlBlockAlloc =
        typename std::allocator_traits<Alloc>::
        template rebind_alloc<ControlBlockCommon<Y, Deleter, Alloc>>;
      using CBAllocTraits = std::allocator_traits<ControlBlockAlloc>;

      ControlBlockAlloc cb_alloc(_alloc);
      this->~ControlBlockCommon();
      CBAllocTraits::deallocate(cb_alloc, this, 1);
    }

    Y* _object;
    Deleter _deleter;
    [[no_unique_address]] Alloc _alloc;

    template <typename U>
    friend class SharedPtr;

    template <typename U>
    friend class WeakPtr;
  };

  template <typename Alloc = std::allocator<T>>
  class ControlBlockMakeShared : BaseControlBlock {
   public:
    ControlBlockMakeShared() = default;

    template <typename... Args>
    ControlBlockMakeShared(const Alloc& alloc, Args&&... args): _alloc(alloc) {
      BaseControlBlock::shared_count = 1;
      BaseControlBlock::weak_count = 0;
      new (_object) T(std::forward<Args>(args)...);
    }

   private:
    void use_deleter() override {
      reinterpret_cast<T*>(_object)->~T();
    }

    void* get_object() override {
      return _object;
    }

    void destroy() override {
      using ControlBlockAlloc =
        typename std::allocator_traits<Alloc>::
        template rebind_alloc<ControlBlockMakeShared<Alloc>>;
      using CBAllocTraits = std::allocator_traits<ControlBlockAlloc>;

      ControlBlockAlloc cb_alloc(_alloc);
      CBAllocTraits::destroy(cb_alloc, this);
      CBAllocTraits::deallocate(cb_alloc, this, 1);
    }

    char _object[sizeof(T)];
    [[no_unique_address]] Alloc _alloc;
    using AllocTraits = std::allocator_traits<Alloc>;

    template <typename U>
    friend class SharedPtr;

    template <typename U>
    friend class WeakPtr;

    template <typename U, typename Alloc_, typename... Args>
    friend SharedPtr<U> allocateShared(const Alloc_& alloc, Args&&... args);
  };

  template <typename Alloc>
  explicit SharedPtr(ControlBlockMakeShared<Alloc>* control_block);

  explicit SharedPtr(BaseControlBlock* block);

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc& alloc, Args&&... args);

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

  friend EnableSharedFromThis<T>;

  BaseControlBlock* cb;
};

template <typename T>
template <typename Alloc>
SharedPtr<T>::SharedPtr(ControlBlockMakeShared<Alloc>* control_block): cb(control_block) {
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    static_cast<T*>(cb->get_object())->weak_this = cb;
  }
}

template <typename T>
SharedPtr<T>::SharedPtr(BaseControlBlock* block): cb(block) {
  if (block == nullptr) {
    cb = nullptr;
  } else {
    ++cb->shared_count;
  }
}

template <typename T>
void SharedPtr<T>::swap(SharedPtr& other) noexcept {
  BaseControlBlock* temp = cb;
  cb = other.cb;
  other.cb = temp;
}

template <typename T>
template <typename Y, typename>
SharedPtr<T>::SharedPtr(Y* ptr): cb(new ControlBlockCommon<Y>(ptr)) {
  if constexpr (std::is_base_of_v<EnableSharedFromThis<Y>, Y>) {
    ptr->weak_this = cb;
  }
}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) noexcept: cb(other.cb) {
  if (cb == nullptr) {
    return;
  }
  ++cb->shared_count;
}

template <typename T>
template <typename Y, typename>
SharedPtr<T>::SharedPtr(const SharedPtr<Y>& other) noexcept: cb(other.cb) {
  if (cb == nullptr) {
    return;
  }
  ++cb->shared_count;
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) noexcept: cb(other.cb) {
  other.cb = nullptr;
}

template <typename T>
template <typename Y, typename>
SharedPtr<T>::SharedPtr(SharedPtr<Y>&& other) noexcept: cb(other.cb) {
  other.cb = nullptr;
}

template <typename T>
template <typename Y, typename Deleter, typename>
SharedPtr<T>::SharedPtr(Y* ptr, Deleter deleter):
  cb(new ControlBlockCommon<Y, Deleter>(ptr, deleter)) {
  if constexpr (std::is_base_of_v<EnableSharedFromThis<Y>, Y>) {
    ptr->weak_this = cb;
  }
}

template <typename T>
template <typename Y, typename Deleter, typename Alloc, typename>
SharedPtr<T>::SharedPtr(Y* ptr, Deleter deleter, Alloc alloc) {
  using ControlBlockAlloc = typename std::allocator_traits<Alloc>::
    template rebind_alloc<ControlBlockCommon<Y, Deleter, Alloc>>;
  using CBAllocTraits = std::allocator_traits<ControlBlockAlloc>;
  ControlBlockAlloc cb_alloc(alloc);
  cb = CBAllocTraits::allocate(cb_alloc, 1);
  new (cb) ControlBlockCommon<Y, Deleter, Alloc>(ptr, deleter, alloc);

  if constexpr (std::is_base_of_v<EnableSharedFromThis<Y>, Y>) {
    ptr->weak_this = cb;
  }
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other) {
  if (other.cb == cb) {
    return *this;
  }

  SharedPtr<T>(other).swap(*this);
  return *this;
}

template <typename T>
template <typename Y, typename>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<Y>& other) {
  if (other.cb == cb) {
    return *this;
  }

  SharedPtr<T>(other).swap(*this);
  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) noexcept {
  SharedPtr<T>(std::move(other)).swap(*this);
  return *this;
}

template <typename T>
template <typename Y, typename>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<Y>&& other) noexcept {
  SharedPtr<T>(std::move(other)).swap(*this);
  return *this;
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
  if (cb == nullptr) {
    return;
  }

  if (cb->shared_count != 0) {
    --cb->shared_count;
  }

  if (cb->shared_count == 0) {
    cb->use_deleter();
    if (cb->weak_count == 0) {
      cb->destroy();
      cb = nullptr;
    }
  }
}


template <typename T>
class WeakPtr {
public:
  ~WeakPtr();

  WeakPtr() noexcept: cb(nullptr) {}

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
    std::is_base_of_v<T, U>> >
  WeakPtr(const SharedPtr<U>& shared) noexcept;

  WeakPtr(const WeakPtr& other) noexcept;

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
    std::is_base_of_v<T, U>> >
  WeakPtr(const WeakPtr<U>& other) noexcept;

  WeakPtr(WeakPtr&& other) noexcept;

  template <typename U, typename = std::enable_if_t<std::is_same_v<T, U> ||
    std::is_base_of_v<T, U>> >
  WeakPtr(WeakPtr<U>&& other) noexcept;

  WeakPtr<T>& operator=(const WeakPtr& other) noexcept;

  template <typename Y>
  WeakPtr<T>& operator=(const SharedPtr<Y>& other) noexcept;

  WeakPtr<T>& operator=(WeakPtr&& other) noexcept;

  size_t use_count() const noexcept { return (cb ? cb->shared_count : 0); }

  bool expired() const noexcept { return use_count() == 0; }

  SharedPtr<T> lock() const noexcept { return expired() ? SharedPtr<T>() : SharedPtr<T>(cb); }

  void swap(WeakPtr& other) noexcept;

private:
  BaseControlBlock* cb;

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;
};

template <typename T>
WeakPtr<T>::~WeakPtr() {
  if (cb == nullptr) {
    return;
  }

  if (cb->weak_count != 0) {
    --cb->weak_count;
  }

  if ((cb->weak_count == 0) && expired()) {
    cb->destroy();
    cb = nullptr;
  }
}

template <typename T>
template <typename U, typename>
WeakPtr<T>::WeakPtr(const SharedPtr<U>& shared) noexcept: cb(shared.cb) {
  ++cb->weak_count;
}

template <typename T>
WeakPtr<T>::WeakPtr(const WeakPtr& other) noexcept: cb(other.cb) {
  ++cb->weak_count;
}

template <typename T>
template <typename U, typename>
WeakPtr<T>::WeakPtr(const WeakPtr<U>& other) noexcept: cb(other.cb) {
  ++cb->weak_count;
}

template <typename T>
WeakPtr<T>::WeakPtr(WeakPtr&& other) noexcept: cb(other.cb) {
  other.cb = nullptr; 
}

template <typename T>
template <typename U, typename>
WeakPtr<T>::WeakPtr(WeakPtr<U>&& other) noexcept: cb(other.cb) {
  other.cb = nullptr; 
}

template <typename T>
void WeakPtr<T>::swap(WeakPtr& other) noexcept {
  BaseControlBlock* temp = cb;
  cb = other.cb;
  other.cb = temp;
}

template <typename T>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr& other) noexcept {
  WeakPtr<T>(other).swap(*this);
  return *this;
}

template <typename T>
template <typename Y>
WeakPtr<T>& WeakPtr<T>::operator=(const SharedPtr<Y>& other) noexcept {
  WeakPtr<T>(other).swap(*this);
  return *this;
}

template <typename T>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr&& other) noexcept {
  WeakPtr<T>(std::move(other)).swap(*this);
  return *this;
}
