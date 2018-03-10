#ifndef _DFC_INT_POWER_HPP
#define _DFC_INT_POWER_HPP

template<typename T, int N>
struct IntPow
{
    inline static const T GetValue(T v)
    {
        return v * IntPow<T, N - 1>::GetValue(v);
    }
};

#define MAKE_SPECIAL_INT_POW(N,s)\
template<typename T>\
struct IntPow<T,N>\
{\
    inline static const T GetValue(T v)\
    {\
        return s;\
    }\
};

MAKE_SPECIAL_INT_POW(0, 1)
MAKE_SPECIAL_INT_POW(1, v)
MAKE_SPECIAL_INT_POW(2, v*v)
MAKE_SPECIAL_INT_POW(3, v*v*v)
MAKE_SPECIAL_INT_POW(4, (IntPow<T,2>::GetValue(IntPow<T,2>::GetValue(v))))
//MAKE_SPECIAL_INT_PER(5, IntPow<T,4>::GetValue(v)*v)
MAKE_SPECIAL_INT_POW(6, (IntPow<T,2>::GetValue(IntPow<T,3>::GetValue(v))))
//MAKE_SPECIAL_INT_PER(7, 1)
MAKE_SPECIAL_INT_POW(8, (IntPow<T,2>::GetValue(IntPow<T,2>::GetValue(IntPow<T,2>::GetValue(v)))))
//MAKE_SPECIAL_INT_POW(9, 1)

#endif
