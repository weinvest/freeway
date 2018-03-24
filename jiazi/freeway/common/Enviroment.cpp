#include "common/Enviroment.h"

IMPLEMENT_SINGLETON(Enviroment)
Enviroment::Enviroment(){}
void Enviroment::Set(const std::string& key, const boost::any& value)
{
    ValueMap::accessor itInsert;
    mValues.insert(itInsert, key);
    itInsert->second = value;
}

const boost::any& Enviroment::Get(const std::string& key)
{
    ValueMap::const_accessor itFind;
    if(mValues.find(itFind, key))
    {
        return itFind->second;
    }
    return mEmptyValue;
}



