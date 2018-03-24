#ifndef ENVIROMENT_H
#define ENVIROMENT_H
#include <boost/any.hpp>
#include <tbb/concurrent_hash_map.h>
#include "Singleton.h"
#include "common/Types.h"

class Enviroment
{
    DECLARE_AS_SINGLETON(Enviroment)
    typedef tbb::concurrent_hash_map<std::string, boost::any> ValueMap;
    ValueMap mValues;
    boost::any mEmptyValue;

public:
    void Set(const std::string& key, const boost::any& value);
    const boost::any& Get(const std::string& key);

    template<typename T>
    T& Get(const std::string &key)
    {
        return boost::any_cast<T&>(Get(key));
    }
};

#endif // ENVIROMENT_H
