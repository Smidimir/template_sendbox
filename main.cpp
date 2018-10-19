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
 * value to type mapping
 */

#include <map>

template <typename T>
struct type_wrapper {};

template <typename Variant>
struct VariantToTypeWrapper;

template <typename ... Ts>
struct VariantToTypeWrapper<std::variant<Ts ...>>
{
  using type = std::variant<type_wrapper<Ts> ...>;
};

template <typename ... Ts>
struct TypeList {};

template <typename Variant>
struct VariantToTypeList;

template <typename ... Ts>
struct VariantToTypeList<std::variant<Ts ...>>
{
  using type = TypeList<Ts...>;
};

template <typename ValType, typename Variant>
class ValToTypeMap
{
public:
  using TWVariant = typename VariantToTypeWrapper<Variant>::type;
  using TList = typename VariantToTypeList<Variant>::type;

  template <typename ... Ts>
  ValToTypeMap(Ts&& ... ts)
  {
    addType(TList{}, std::forward<Ts>(ts) ...);
  }

  Variant create(ValType const& val)
  {
    return std::visit([this](auto const& v) { return create(v);}, m_map[val]);
  }

  template <typename ... Args>
  Variant create(ValType const& val, Args&& ... args)
  {
    return std::visit([this, args = std::make_tuple(std::forward<Args>(args) ...)](auto const& v) { return create(v, std::move(args));}, m_map[val]);
  }

//private:
  template <typename T>
  Variant create(type_wrapper<T>)
  {
    return T{};
  }

  template <typename T, typename Tuple>
  Variant create(type_wrapper<T>, Tuple&& t)
  {
    return create(type_wrapper<T>{}, std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size<Tuple>::value>() );
  }

  template <typename T, typename Tuple, std::size_t... Inds>
  Variant create(type_wrapper<T>, Tuple&& tuple, std::index_sequence<Inds...>)
  {
    return T(std::get<Inds>(std::forward<Tuple>(tuple))...);
  }

  template <typename T, typename Arg, typename ... Ts, typename ... Args>
  void addType(TypeList<T, Ts ...>, Arg&& arg, Args&& ... args)
  {
    m_map[arg] = type_wrapper<T>{};
    addType(TypeList<Ts ...>{}, std::forward<Args>(args) ...);
  }

  template <typename T, typename Arg>
  void addType(TypeList<T> typeList, Arg&& arg)
  {
    m_map[arg] = type_wrapper<T>{};
  }

//private:

  std::map<ValType, TWVariant> m_map;
};


int main()
{
//  auto constexpr arr = constexpr_text();
//
//  std::array<int, arr[0]> arr1;
//  std::array<int, arr[1]> arr2;
//  std::array<int, arr[2]> arr3;
//
//  std::cout << arr1.size() << " " << arr2.size() << " " << arr3.size() << std::endl;

  using V = std::variant<int, char, double>;

  ValToTypeMap<int, V> mapper{1, 2, 3};

  std::cout << mapper.m_map.size() << std::endl;

  std::visit([](auto const& v){ std::cout << typeid(v).name() << std::endl; }, mapper.m_map[1]);
  std::visit([](auto const& v){ std::cout << typeid(v).name() << std::endl; }, mapper.m_map[2]);
  std::visit([](auto const& v){ std::cout << typeid(v).name() << std::endl; }, mapper.m_map[3]);

  auto a = mapper.create(1);
  auto b = mapper.create(1, 'a');
  auto c = mapper.create(2, 'a');
  auto d = mapper.create(3, 'a');

  std::visit([](auto const& v){ std::cout << typeid(v).name() << " : " << v << std::endl; }, a);
  std::visit([](auto const& v){ std::cout << typeid(v).name() << " : " << v << std::endl; }, b);
  std::visit([](auto const& v){ std::cout << typeid(v).name() << " : " << v << std::endl; }, c);
  std::visit([](auto const& v){ std::cout << typeid(v).name() << " : " << v << std::endl; }, d);

  return 0;
}