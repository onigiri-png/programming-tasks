#include <cstddef>
#include <type_traits>
#include <utility>

struct tuple_cat_tag {};

template <typename T, typename Head, typename... Tail>
consteval size_t count_type() {
  size_t count = 0;
  if (std::is_same_v<T, Head>) {
    ++count;
  }

  if constexpr (sizeof...(Tail) != 0) {
    count += count_type<T, Tail...>();
  }
  return count;
}

template <size_t N, typename Head, typename... Tail>
struct Get {
  using type = typename Get<N - 1, Tail...>::type;
};

template <typename Head, typename... Tail>
struct Get<0, Head, Tail...> {
  using type = Head;
};

template <size_t N, typename Tup>
constexpr decltype(auto) get(Tup&& tuple) {
  return std::forward<decltype(tuple)>(tuple).template get<N>(); 
}

template <typename T, typename Tup>
constexpr decltype(auto) get(Tup&& tuple) {
  return std::forward<decltype(tuple)>(tuple).template get<T>();
}

template <typename... Ts>
class Tuple;

// tuple size

template <typename T>
struct TupleSize : std::integral_constant<size_t, 0> {};

template <typename... Ts>
struct TupleSize<Tuple<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

template <typename T>
constexpr size_t tuple_size_v = TupleSize<T>::value;

//


template <>
class Tuple<> {};

template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {
public:
  template <typename = void>
  requires std::is_default_constructible_v<Head> &&
           std::conjunction_v<std::is_default_constructible<Tail>...>
  explicit(!requires {
    std::declval<void(*)(Head)>()({});
    (std::declval<void(*)(Tail)>()({}), ...);
  })
  Tuple(): head{}, tail{} {}

  template <typename = void>
  requires std::is_copy_constructible_v<Head> &&
           std::conjunction_v<std::is_copy_constructible<Tail>...>
  explicit(!std::conjunction_v<std::is_convertible<const Head&, Head>,
                               std::is_convertible<const Tail&, Tail>...>)
  Tuple(const Head& head, const Tail&... args)
    : head(head),
      tail(args...)
  {}

  template <typename HeadArg, typename... TailArg>
  requires (sizeof...(Tail) == sizeof...(TailArg)) &&
           std::is_constructible_v<Head, HeadArg> &&
           std::conjunction_v<std::is_constructible<Tail, TailArg>...>
  explicit(!std::conjunction_v<std::is_convertible<HeadArg, Head>,
                               std::is_convertible<TailArg, Tail>...>)
  Tuple(HeadArg&& arg, TailArg&&... args)
    : head(std::forward<HeadArg>(arg)),
      tail(std::forward<TailArg>(args)...)
  {}

  template <typename UHead, typename... UTail>
  explicit(!std::conjunction_v<std::is_convertible<const UHead&, Head>,
            std::is_convertible<const UTail&, Tail>...>)
  Tuple(const Tuple<UHead, UTail...>& other)
    requires (sizeof...(Tail) == sizeof...(UTail) &&
              std::is_constructible_v<Head, const UHead&> &&
              std::conjunction_v<std::is_constructible<Tail, const UTail&>...> &&
              (sizeof...(Tail) != 0 ||
               !(std::is_convertible_v<decltype(other), Head> ||
                 std::is_constructible_v<Head, decltype(other)> ||
                 std::is_same_v<Head, UHead>)))
    : head(::get<0>(std::forward<decltype(other)>(other))),
      tail(other.tail)
  {}

  template <typename UHead, typename... UTail>
  requires (sizeof...(Tail) == sizeof...(UTail) &&
            std::is_constructible_v<Head, UHead&&> &&
            std::conjunction_v<std::is_constructible<Tail, UTail&&>...> &&
            (sizeof...(Tail) != 0 ||
             !(std::is_convertible_v<UHead&&, Head> ||
               std::is_constructible_v<Head, UHead&&> ||
               std::is_same_v<Head, UHead>)))
  explicit(!std::conjunction_v<std::is_convertible<UHead&&, Head>,
            std::is_convertible<UTail&&, Tail>...>)
  Tuple(Tuple<UHead, UTail...>&& other)
    : head(::get<0>(std::forward<decltype(other)>(other))),
      tail(std::move(other.tail))
  {}

  template <typename U1, typename U2>
  requires (sizeof...(Tail) == 1 &&
            std::is_constructible_v<Head, const U1&> &&
            (std::is_constructible_v<Tail, const U2&> && ...)) 
  explicit(!(std::is_convertible_v<const U1&, Head> &&
            (std::is_convertible_v<const U2&, Tail> && ...)))
  Tuple(const std::pair<U1, U2>& pair)
    : head(std::get<0>(std::forward<decltype(pair)>(pair))),
      tail(std::get<1>(std::forward<decltype(pair)>(pair)))
  {}

  template <typename U1, typename U2>
  requires (sizeof...(Tail) == 1 &&
            std::is_constructible_v<Head, U1&&> &&
            (std::is_constructible_v<Tail, U2&&> && ...))
  explicit(!(std::is_convertible_v<U1&&, Head> &&
            (std::is_convertible_v<U2&&, Tail> && ...)))
  Tuple(std::pair<U1, U2>&& pair)
    : head(std::get<0>(std::forward<decltype(pair)>(pair))),
      tail(std::get<1>(std::forward<decltype(pair)>(pair)))
  {}

  Tuple(const Tuple& other)
    requires std::conjunction_v<std::is_copy_constructible<Head>,
                                std::is_copy_constructible<Tail>...>
    : head(::get<0>(std::forward<decltype(other)>(other))),
      tail(other.tail)
  {}

  Tuple(Tuple&& other)
    requires std::conjunction_v<std::is_move_constructible<Head>,
                                std::is_move_constructible<Tail>...>
    : head(::get<0>(std::forward<decltype(other)>(other))),
      tail(std::move(other.tail))
  {}

  Tuple& operator=(const Tuple& other)
    requires std::conjunction_v<std::is_copy_assignable<Head>,
                                std::is_copy_assignable<Tail>...> {
    head = other.head;
    tail = other.tail;
    return *this;
  }

  Tuple& operator=(Tuple&& other) 
    requires std::conjunction_v<std::is_move_assignable<Head>,
                                std::is_move_assignable<Tail>...> {
    ::get<0>(*this) = std::forward<Head>(::get<0>(other));
    tail = std::move(other.tail);
    return *this;
  }

  template <typename UHead, typename... UTail>
  requires (sizeof...(Tail) == sizeof...(UTail) &&
            std::conjunction_v<std::is_assignable<Head&, const UHead&>,
                               std::is_assignable<Tail&, const UTail&>...>)
  Tuple& operator=(const Tuple<UHead, UTail...>& other) {
    ::get<0>(*this) = ::get<0>(other);
    tail = other.tail;
    return *this;
  }

  template <typename UHead, typename... UTail>
  requires (sizeof...(Tail) == sizeof...(UTail) &&
            std::conjunction_v<std::is_assignable<Head&, UHead>,
                               std::is_assignable<Tail&, UTail>...>)
  Tuple& operator=(Tuple<UHead, UTail...>&& other) {
    ::get<0>(*this) = std::forward<UHead>(::get<0>(other));
    tail = std::move(other.tail);
    return *this;
  }

private:
  template <typename HeadTup, typename... TailTuples>
  requires (tuple_size_v<std::decay_t<HeadTup>> > 1)
  Tuple(tuple_cat_tag, HeadTup&& head_tuple, TailTuples&&... tuples)
    : head(::get<0>(std::forward<HeadTup>(head_tuple))),
      tail(tuple_cat_tag{}, std::forward_like<decltype(head_tuple)>(head_tuple.tail), std::forward<TailTuples>(tuples)...)
  {}

  template <typename HeadTup, typename SecondTup, typename... TailTuples>
  requires (tuple_size_v<std::decay_t<HeadTup>> == 1)
  Tuple(tuple_cat_tag, HeadTup&& head_tuple, SecondTup&& second_tuple, TailTuples&&... tuples)
    : head(::get<0>(std::forward<HeadTup>(head_tuple))),
      tail(tuple_cat_tag{}, std::forward<SecondTup>(second_tuple), std::forward<TailTuples>(tuples)...)
  {}

  template <typename Tup>
  requires (tuple_size_v<std::decay_t<Tup>> == 1)
  Tuple(tuple_cat_tag, Tup&& tuple)
    : head(::get<0>(std::forward<Tup>(tuple)))
  {}
      
  template <typename T, bool FromNumber = false>
  requires (FromNumber || count_type<T, Head, Tail...>() == 1)
  constexpr decltype(auto) get(this auto&& self) {
    using Self = decltype(self);
    if constexpr (std::is_same_v<T, Head>) {
      using Type = std::conditional_t<std::is_lvalue_reference_v<decltype(self.head)>, decltype(self.head), Self>;
      if constexpr (std::is_const_v<std::remove_reference_t<Self>> && std::is_rvalue_reference_v<decltype(self.head)> &&
                    !std::is_const_v<std::remove_reference_t<decltype(self.head)>>) {
        if constexpr (std::is_lvalue_reference_v<Self>) {
          return std::forward_like<std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<Type>>>>(self.head);
        } else {
          return std::forward_like<std::add_rvalue_reference_t<std::remove_const_t<std::remove_reference_t<Type>>>>(self.head);
        }
      } else {
        return std::forward_like<Type>(self.head);
      }
    } else {
      return std::forward_like<Self>(self.tail).template get<T>();
    }
  }

  template <size_t N>
  constexpr decltype(auto) get(this auto&& self) {
    return std::forward<decltype(self)>(self).template get<typename Get<N, Head, Tail...>::type, true>();
  }

  template <size_t N, typename Tup>
  friend constexpr decltype(auto) get(Tup&&);

  template <typename T, typename Tup>
  friend constexpr decltype(auto) get(Tup&&);

  template <typename... Tuples>
  friend auto tupleCat(Tuples&&...);

  template <typename... Ts>
  friend class Tuple;

  Head head;
  Tuple<Tail...> tail;
};

// deduction guides

template <typename U1, typename U2>
Tuple(std::pair<U1, U2>) -> Tuple<U1, U2>;

template <typename... Ts>
Tuple(Ts...) -> Tuple<Ts...>;

//

template <typename... Ts>
Tuple<std::unwrap_ref_decay_t<Ts>...> makeTuple(Ts&&... args) {
  return Tuple<std::unwrap_ref_decay_t<Ts>...>(std::forward<Ts>(args)...);
}

template <typename... Ts>
Tuple<Ts&...> tie(Ts&... args) {
  return {args...};
}

template <typename... Ts>
Tuple<Ts&&...> forwardAsTuple(Ts&&... args) {
  return {std::forward<Ts>(args)...};
}

// tupleCat

template <typename Tup, typename... Tups>
struct GetTupleFromTuples {
  using type = typename GetTupleFromTuples<Tup, typename GetTupleFromTuples<Tups...>::type>::type;
};

template <typename... Ts1, typename... Ts2>
struct GetTupleFromTuples<Tuple<Ts1...>, Tuple<Ts2...>> {
  using type = Tuple<Ts1..., Ts2...>;
};

template <typename... Tuples>
auto tupleCat(Tuples&&... tuple) {
  using Tup = typename GetTupleFromTuples<std::decay_t<Tuples>...>::type;
  return Tup(tuple_cat_tag{}, std::forward<Tuples>(tuple)...);
}

// comparison operators

template <typename... Ts, typename... Us>
bool operator<(const Tuple<Ts...>& t1, const Tuple<Ts...>& t2) {
  size_t i = 0;
  while (i < sizeof...(Ts) && get<i>(t1) == get<i>(t2)) {
    ++i;
  }

  if (i == sizeof...(Ts)) {
    return false;
  }

  return get<i>(t1) < get<i>(t2);
}

template <typename... Ts, typename... Us>
bool operator>(const Tuple<Ts...>& t1, const Tuple<Us...>& t2) {
  return t2 < t1;
}

template <typename... Ts, typename... Us>
bool operator==(const Tuple<Ts...>& t1, const Tuple<Us...>& t2) {
  for (size_t i = 0; i < sizeof...(Ts); ++i) {
    if (get<i>(t1) != get<i>(t2)) {
      return false;
    }
  }

  return true;
}

template <typename... Ts, typename... Us>
bool operator!=(const Tuple<Ts...>& t1, const Tuple<Us...>& t2) {
  return !(t1 == t2);
}
