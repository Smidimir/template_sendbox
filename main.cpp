#include <iostream>

#include <functional>
#include <variant>
#include <iomanip>



/**
 * Step function sequence
 */


using TCallback = std::function<void()>;

using StepCallback = std::function<void(TCallback)>;

template <typename F>
auto makeSequence(F&& f)
{
  if constexpr (std::is_constructible_v<StepCallback, std::decay_t<F>>)
  {
    return std::bind(std::forward<F>(f), [](){});
  }
  else if constexpr (std::is_constructible_v<TCallback, std::decay_t<F>>)
  {
    return std::forward<F>(f);
  }
}

template <typename F, typename ... Fs>
auto makeSequence(F&& f, Fs&& ... fs)
{
  if constexpr (std::is_constructible_v<StepCallback, std::decay_t<F>>)
  {
    return std::bind(f, TCallback{ makeSequence(std::forward<Fs>(fs)...)});
  }
  else if constexpr (std::is_constructible_v<TCallback, std::decay_t<F>>)
  {
    return std::bind([f](TCallback onEnd) { f(); onEnd(); }, TCallback{ makeSequence(std::forward<Fs>(fs)...)});
  }
}


template <typename ... Fs>
void runSequence(Fs&& ... fs)
{
  makeSequence(std::forward<Fs>(fs)...)();
}

namespace usage
{

void stepFunctionSequence()
{
  runSequence(
      [](TCallback onEnd)
      {
        std::cout << 1 << std::endl;
        onEnd();
      },
      []()
      {
        std::cout << 2 << std::endl;
      },
      [](TCallback onEnd)
      {
        std::cout << 3 << std::endl;
        onEnd();
      },
      []()
      {
        std::cout << 4 << std::endl;
      },
      []()
      {
        std::cout << 5 << std::endl;
      },
      []()
      {
        std::cout << 6 << std::endl;
      });
}

}


/**
 * MeberFunctionToFunction
 */


template <auto MemberFunction, typename T = decltype(MemberFunction)>
struct MeberFunctionToFunction;

template <auto MemberFunction, typename ReturnType, typename BaseClass, typename ... Args>
struct MeberFunctionToFunction<MemberFunction, ReturnType(BaseClass::*)(Args...)>
{
  static ReturnType value(BaseClass* pBaseClass, Args ... args)
  {
    return (pBaseClass->*MemberFunction)(std::forward<Args>(args) ...);
  }
};

template <auto MemberFunction>
auto MeberFunctionToFunction_v = MeberFunctionToFunction<MemberFunction>::value;

namespace usage
{

void meberFunctionToFunction()
{
  class A
  {
  public:
    void x()
    {
      std::cout << "Hello world" << std::endl;
    }
  };

  auto x = MeberFunctionToFunction_v<&A::x>;

  A a;

  x(&a);
}

}


/**
 * constexpr test
 */


#include <array>


std::array<int, 3> constexpr constexpr_text()
{
  std::array<int, 3> constexpr arr = []() -> std::array<int, 3>
  {
    std::array<int, 3> out = {3, 1, 2};

    for(size_t i = 0; i < out.size() - 1; ++i)
    {
      for(size_t j = i + 1; j < out.size(); ++j)
      {
        if(out[i] < out[j])
        {
          int tmp = out[i];
          out[i] = out[j];
          out[j] = tmp;
        }
      }
    }

    return out;
  }();

  return arr;
}


/**
 * ConstructInformer
 */


class ConstructInformer
{
public:
  ConstructInformer()
  {
    std::cout << "ConstructInformer: default" << std::endl;
  }

  ConstructInformer(ConstructInformer const&)
  {
    std::cout << "ConstructInformer: copy" << std::endl;
  }

  ConstructInformer(ConstructInformer&&)
  {
    std::cout << "ConstructInformer: move" << std::endl;
  }

  ConstructInformer& operator = (ConstructInformer const&)
  {
    std::cout << "ConstructInformer: assign" << std::endl;
    return *this;
  }

  ConstructInformer& operator = (ConstructInformer&&)
  {
    std::cout << "ConstructInformer: move assign" << std::endl;
    return *this;
  }
};


/**
 * value to type mapping v2
 */

#include <variant>
#include <type_traits>
#include <map>
#include <utility>
#include <tuple>


template <typename T, typename ... Ts>
struct type_list
{
  using first = T;
  using rest = type_list<Ts ...>;

  static std::size_t constexpr size = sizeof ... (Ts) + 1u;
};

template <typename T1, typename T2>
struct type_list<T1, T2>
{
  using first = T1;
  using rest = T2;

  static std::size_t constexpr size = 2u;
};

template <typename T>
struct type_list<T>
{
  using first = T;

  static std::size_t constexpr size = 1u;
};

template <typename T>
using type_holder = type_list<T>;

template <typename ... Ts>
using type_list_ft = typename type_list<Ts ...>::first;

template <typename ... Ts>
using type_list_rt = typename type_list<Ts ...>::rest;

template <typename ... Ts>
auto constexpr type_list_sv = type_list<Ts ...>::size;


// >>>>
// >>>>>
// >>>>


#include <cassert>

template <typename T>
class variant_object_creator;

template <typename ... Types>
class variant_object_creator<std::variant<Types ...>>
{
public:
  using variant_type = std::variant<type_holder<Types> ...>;
  using variant_object_type = std::variant<Types ...>;

  constexpr variant_object_creator(variant_object_creator const&) = default;
  constexpr variant_object_creator(variant_object_creator&&) noexcept = default;

  template <typename T>
  explicit constexpr variant_object_creator(T&& t)
    : m_cur_type {std::forward<T>(t)}
  {;}


  variant_object_creator& operator = (variant_object_creator const&) = default;
  variant_object_creator& operator = (variant_object_creator&&) noexcept = default;

  template <typename T>
  variant_object_creator& operator = (type_holder<T> const& t)
  {
    m_cur_type = t;
  }

  template <typename T>
  variant_object_creator& operator = (type_holder<T>&& t)
  {
    m_cur_type = std::move(t);
  }

  variant_object_creator& operator = (variant_type const& t)
  {
    m_cur_type = t;
  }

  variant_object_creator& operator = (variant_type&& t)
  {
    m_cur_type = std::move(t);
  }

public:

  template <typename ... Args>
  variant_object_type operator() (Args&& ... args)
  {
    return std::visit([this, t_args = std::forward_as_tuple(args ...)](auto&& t) -> variant_object_type
    {
      return construct_from_tuple(t, t_args, std::make_index_sequence<std::tuple_size<decltype(t_args)>{}>{});
    }, m_cur_type);
  }

private:
  template <typename T, typename Tuple, size_t ... Indeces>
  variant_object_type construct_from_tuple(type_holder<T> t, Tuple&& tuple, std::index_sequence<Indeces ...>)
  {
    return construct(t, std::forward<std::tuple_element_t<Indeces, std::decay_t<Tuple>>>(std::get<Indeces>(tuple))...);
  }

  template <typename T, typename ... Args>
  variant_object_type construct(type_holder<T>, Args&& ... args)
  {
    if constexpr (std::is_constructible_v<T, decltype(std::forward<Args>(args)) ...>)
    {
      return T{std::forward<Args>(args) ...};
    }
    else
    {
      assert(false);
      return T{};
    }
  }

private:
  variant_type m_cur_type;
};

// >>>>
// >>>>>
// >>>>


template <typename KeyType, typename VariantType>
class vt_mapper;

template <typename KeyType, typename ... Types>
class vt_mapper<KeyType, std::variant<Types ...>>
{
public:
  using variant_type = std::variant<type_holder<Types> ...>;
  using variant_object_type = std::variant<Types ...>;

  template <typename ... Ts>
  explicit vt_mapper(Ts&& ... keys)
  {
    (m_map.insert_or_assign(std::forward<Ts>(keys), variant_type{type_holder<Types>{}}), ...);
  }

  variant_object_creator<variant_object_type> operator[] (KeyType&& key)
  {
    return variant_object_creator<variant_object_type>{m_map.at(std::forward<KeyType>(key))};
  }

private:
  std::map<KeyType, variant_type> m_map;
};


int main()
{
  using V = std::variant<char, int, long, ConstructInformer>;

  vt_mapper<int, V> mapper{1, 2, 3, 4};

  variant_object_creator<V> voc{type_holder<ConstructInformer>{}};

  V v1 = mapper[4](ConstructInformer{});
  V v2 = mapper[4](std::move(ConstructInformer{}));


//  std::visit([](auto&& v) { std::cout << typeid(v).name() << std::endl; }, v1);


//  auto constexpr arr = constexpr_text();
//
//  std::array<int, arr[0]> arr1;
//  std::array<int, arr[1]> arr2;
//  std::array<int, arr[2]> arr3;
//
//  std::cout << arr1.size() << " " << arr2.size() << " " << arr3.size() << std::endl;

//  using V = std::variant<int, char, double, ConstructInformer>;

//  ValToTypeMap<int, V> mapper{1, 2, 3, 4};

//  std::cout << mapper.m_map.size() << std::endl;
//
//  std::visit([](auto const& v){ std::cout << typeid(v).name() << std::endl; }, mapper.m_map[1]);
//  std::visit([](auto const& v){ std::cout << typeid(v).name() << std::endl; }, mapper.m_map[2]);
//  std::visit([](auto const& v){ std::cout << typeid(v).name() << std::endl; }, mapper.m_map[3]);
//  std::visit([](auto const& v){ std::cout << typeid(v).name() << std::endl; }, mapper.m_map[4]);

//  auto a = mapper.create(1);
//  auto b = mapper.create(1);
//  auto c = mapper.create(2);
//  auto d = mapper.create(3, 'a');
//  auto e = mapper.create(4, ConstructInformer{});

//  std::visit([](auto const& v){ std::cout << typeid(v).name() << " : " << v << std::endl; }, a);
//  std::visit([](auto const& v){ std::cout << typeid(v).name() << " : " << v << std::endl; }, b);
//  std::visit([](auto const& v){ std::cout << typeid(v).name() << " : " << v << std::endl; }, c);
//  std::visit([](auto const& v){ std::cout << typeid(v).name() << " : " << v << std::endl; }, d);

  return 0;
}