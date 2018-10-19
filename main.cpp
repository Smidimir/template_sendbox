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


int main()
{
  auto constexpr arr = constexpr_text();

  std::array<int, arr[0]> arr1;
  std::array<int, arr[1]> arr2;
  std::array<int, arr[2]> arr3;

  std::cout << arr1.size() << " " << arr2.size() << " " << arr3.size() << std::endl;

  return 0;
}