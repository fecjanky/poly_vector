#pragma once
#include <catch.hpp>

namespace detail {
template<template<typename> class Func, typename... Ts>
struct for_each_type;
}

#define INTERNAL_TYPE_P_TEST_CASE(TESTSTRUCT, NAME, DESCR, TPARAM, ...)        \
  template<typename TPARAM>                                                    \
  struct TESTSTRUCT                                                            \
  {                                                                            \
    static void execute();                                                     \
  };                                                                           \
  TEST_CASE(NAME, DESCR)                                                       \
  {                                                                            \
    ::detail::for_each_type<TESTSTRUCT, __VA_ARGS__>::execute();               \
  }                                                                            \
  template<typename TPARAM>                                                    \
  void TESTSTRUCT<TPARAM>::execute()

#define TYPE_P_TEST_CASE(NAME, DESCR, TPARAM, ...)                             \
  INTERNAL_TYPE_P_TEST_CASE(                                                   \
    INTERNAL_CATCH_UNIQUE_NAME(                                                \
      ____C_A_T_C_H____T_E_S_T___E_X_T_E_N_S_I_O_N____),                       \
    NAME,                                                                      \
    DESCR,                                                                     \
    TPARAM,                                                                    \
    __VA_ARGS__)

namespace detail {

template<template<typename> class Func>
struct for_each_type<Func>
{
  static void execute() {}
};

template<template<typename> class Func, typename T, typename... Ts>
struct for_each_type<Func, T, Ts...>
{
  static void execute()
  {
    Func<T>::execute();
    for_each_type<Func, Ts...>::execute();
  }
};
}
