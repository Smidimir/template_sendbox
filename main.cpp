#include <iostream>

#include <functional>



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





int main()
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




  std::cout << "Hello, World!" << std::endl;
  return 0;
}