#include <iostream>

#include <functional>



/**
 * Step function sequence
 */


using TCallback = std::function<void()>;

using StepCallback = std::function<void(TCallback)>;

template <typename F>
constexpr auto makeSequence(F&& f)
{
  return std::bind(f, [](){});
}

template <typename F, typename ... Fs>
constexpr auto makeSequence(F&& f, Fs&& ... fs)
{
  return std::bind(f, TCallback{ makeSequence(std::forward<Fs>(fs)...)});
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
      [](TCallback onEnd)
      {
        std::cout << 2 << std::endl;
        onEnd();
      },
      [](TCallback onEnd)
      {
        std::cout << 3 << std::endl;
        onEnd();
      },
      [](TCallback onEnd)
      {
        std::cout << 5 << std::endl;
        onEnd();
      },
      [](TCallback onEnd)
      {
        std::cout << 4 << std::endl;
        onEnd();
      },
      [](TCallback onEnd)
      {
        std::cout << 6 << std::endl;
        onEnd();
      });




  std::cout << "Hello, World!" << std::endl;
  return 0;
}