#include <boost/lexical_cast.hpp>
#include <boost/math/distributions/normal.hpp>
#include "common/Double.h"
const uint64_t Double::_POSITIVE_INFINITY = 0x7ff0000000000000L;
const uint64_t Double::_NEGATIVE_INFINITY = 0xfff0000000000000L;
const uint64_t Double::_NaN = 0x7ff8000000000000L;
const uint64_t Double::_MAX_VALUE = 0x7fefffffffffffffL;
const uint64_t Double::_MIN_VALUE = 0xffefffffffffffffL;
double Double::Epsilon()
{
    return 1e-14;
}

double Double::PositiveInfinity()
{
    return std::numeric_limits<double>::infinity();
}

double Double::NegativeInfinity()
{
    return -std::numeric_limits<double>::infinity();
}

double Double::NaN()
{
    return std::numeric_limits<double>::quiet_NaN();
}

double Double::MaxValue()
{
#if defined(max)
#undef max
#endif
    return std::numeric_limits<double>::max();
}

double Double::MinValue()
{
#if defined(min)
#undef min
#endif
    return -std::numeric_limits<double>::max();
}

bool Double::IsPositiveInfinity(double v)
{
    return 1 == std::isinf(v);
}

bool Double::IsNegativeInfinity(double v)
{
    return std::isinf(v);
}

bool Double::IsInfinity(double v)
{
    return IsPositiveInfinity(v) || IsNegativeInfinity(v);
}

bool Double::IsNaN(double v)
{
    return std::isnan(v);
}

bool Double::IsMaxValue(double v)
{
    return 0 == memcmp(&v, &_MAX_VALUE, sizeof(double));
}

bool Double::IsMinValue(double v)
{
    return 0 == memcmp(&v, &_MIN_VALUE, sizeof(double));
}

bool Double::Equal(double v1, double v2)
{
    return std::abs(v1 - v2) < 1e-14;
}

bool Double::NonEqual(double v1, double v2)
{
    return !Equal(v1, v2);
}

bool Double::LessEqual(double v1, double v2)
{
    return Equal(v1, v2) || v1 < v2;
}

bool Double::GreatEqual(double v1, double v2)
{
    return Equal(v1, v2) || v1 > v2;
}

bool Double::LessThan(double v1, double v2)
{
    return NonEqual(v1, v2) && v1 <  v2;
}

bool Double::GreatThan(double v1, double v2)
{
    return NonEqual(v1, v2) && v1 > v2;
}

bool Double::IsValid(double v)
{
    return !IsInfinity(v) && !IsNaN(v) && !IsMaxValue(v) && !IsMinValue(v);
}

double Double::Divide(double v1, double v2)
{
    if(Double::Equal(v2, 0.0))
    {
	return Double::NaN();
    }
    return v1 / v2;
}

int Double::Sign(double v)
{
    if (GreatThan(v, 0.0))
    {
        return 1;
    }
    else if (LessThan(v, 0.0))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

double Double::Abs(double v)
{
    if (Double::GreatEqual(v, 0.0))
    {
        return v;
    }
    return -v;
}

double Double::Max(double v1, double v2)
{
    if (v1 > v2)
    {
        return v1;
    }
    return v2;
}

double Double::Min(double v1, double v2)
{
    if (v1 < v2)
    {
        return v1;
    }
    return v2;
}

double Double::Max(double v1, double v2, double v3)
{
    return Max(Max(v1, v2), v3);
}

double Double::Min(double v1, double v2, double v3)
{
    return Min(Min(v1, v2), v3);
}

double Double::Floor(double v)
{
    if (Double::Equal(v, std::round(v)))
    {
        return std::round(v);
    }
    return std::floor(v);
}

double Double::Ceil(double v)
{
    if (Double::Equal(v, std::round(v)))
    {
        return std::round(v);
    }
    return std::ceil(v);
}

int Double::Round(double v)
{
    v += 6755399441055744.0;
    return reinterpret_cast<int&>(v);

}

int32_t Double::Round(double v, int nanValue)
{
    if(Double::IsNaN(v))
    {
	    return nanValue;
    }

    return Round(v);
}

double Double::Parse(const std::string& v)
{
    try
    {
	return boost::lexical_cast<double>(v);
    }
    catch(...)
    {
	if(v.size() > 0)
	{
	    if(v[0] == '-')
	    {
		return Double::MinValue();
	    }
	    return Double::MaxValue();
	}
	return Double::NaN();
    }
}

std::string Double::ToString(double v)
{
    return boost::lexical_cast<std::string>(v);
}

double Double::Clip(double v, double lowerBound, double upperBound)
{
    if(Double::LessThan(v, lowerBound))
    {
        return lowerBound;
    }
    else if(Double::GreatThan(v, upperBound))
    {
        return upperBound;
    }
    else
    {
        return v;
    }
}

double Double::Sqrt(double v)
{
    return std::sqrt(v);
}

double Double::Log(double v)
{
    return std::log(v);
}

double Double::NormSDist(double x)
{
    //boost::math::normal_distribution<> normal(0.0, 1.0);
    //return boost::math::cdf(normal, v);
    static double a1 =  0.254829592;
    static double a2 = -0.284496736;
    static double a3 =  1.421413741;
    static double a4 = -1.453152027;
    static double a5 =  1.061405429;
    static double p  =  0.3275911;
    static double s = sqrt(2.0);

    // Save the sign of x
    int sign = 1;
    if (x < 0)
    {
        sign = -1;
        x /= -s;
    }
    else
    {
        x /= s;
    }

    // A&S formula 7.1.26
    double t = 1.0/(1.0 + p*x);
    double y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

    return 0.5*(1.0 + sign*y);
}
