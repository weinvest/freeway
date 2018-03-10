#ifndef _DFC_DOUBLECOMPARATOR_H
#define _DFC_DOUBLECOMPARATOR_H
#include <cstdint>
#include <string>
#include <cmath>
#include <limits>
#include "common/IntPow.hpp"
class Double
{
public:
    const static  uint64_t _POSITIVE_INFINITY;
    const static  uint64_t _NEGATIVE_INFINITY;
    const static  uint64_t _NaN;
    const static  uint64_t _MAX_VALUE;
    const static  uint64_t _MIN_VALUE;

public:
    static  double Epsilon();
    static  double PositiveInfinity();

    static  double NegativeInfinity();

    static  double NaN();

    static  double MaxValue();

    static  double MinValue();

    static  bool IsPositiveInfinity(double v);

    static  bool IsNegativeInfinity(double v);

    static  bool IsInfinity(double v);

    static  bool IsNaN(double v);

    static  bool IsMaxValue(double v);

    static  bool IsMinValue(double v);

    static  bool Equal(double v1, double v2);

    static  bool NonEqual(double v1, double v2);

    static  bool LessEqual(double v1, double v2);
    static  bool GreatEqual(double v1, double v2);
    static  bool LessThan(double v1, double v2);

    static  bool GreatThan(double v1, double v2);
    static  bool IsValid(double v);

    static  double Divide(double v1, double v2);

    static  int Sign(double v);

    static  double Abs(double v);
    static  double Max(double v1,double v2);
    static  double Min(double v1,double v2);
    static  double Max(double v1, double v2, double v3);
    static  double Min(double v1, double v2, double v3);
    static  double Floor(double v);
    static  double Ceil(double v);
    static  int32_t Round(double v);
    static  int32_t Round(double v, int nanValue);
    static  double Parse(const std::string& v);
    static  std::string ToString(double v);

    template<int32_t N>
    static inline double Pow(double v)
    {
        return IntPow<double, N>::GetValue(v);
    }

    static double Clip(double v, double lowerBound, double upperBound);

    static double Sqrt(double v);

    static double Log(double v);

    static double NormSDist(double v);
};
#endif // _DFC_DOUBLECOMPARATOR_H
