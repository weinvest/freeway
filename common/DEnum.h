#ifndef _DFC_DENUM_H
#define _DFC_DENUM_H
#include <cstdint>
#include <mutex>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/algorithm/string/split.hpp>
#include "common/Types.h"
#include <thread>
#include <mutex>
#define DENUM_EXPAND(z,d,i,e) BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2,0,e) = BOOST_PP_TUPLE_ELEM(2,1,e)
#define DENUM_VALUE(seq) BOOST_PP_SEQ_FOR_EACH_I(DENUM_EXPAND,-,seq)
#define DENUM_PARSE(z,d,i,e) gString2Enum->insert(std::make_pair(BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2,0,e)),BOOST_PP_TUPLE_ELEM(2,0,e)));
#define DENUM_TOSTR(z,d,i,e) case BOOST_PP_TUPLE_ELEM(2,0,e): {static const char* v=BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2,0,e)); return v;}

#define DENUM_IMPL_TO_STRING(EnumName,seq) \
    const char* EnumName::ToString(EnumName::type v)\
    {\
        static const char* UNKNOWN_VALUE("Unknow value for "#EnumName);\
        switch(v)\
        {\
            BOOST_PP_SEQ_FOR_EACH_I(DENUM_TOSTR,~,seq)\
            default:\
                return UNKNOWN_VALUE;\
        }\
    }

#define DENUM_IMPL_PARSE(EnumName,seq)\
    bool EnumName::Parse(const std::string& value,EnumName::type& v)\
    {\
        static dunordered_map<std::string,EnumName::type>* gString2Enum = nullptr;\
        static std::once_flag initMap;\
        std::call_once(initMap\
        ,[]()\
		{\
            gString2Enum = new dunordered_map<std::string,EnumName::type>();\
            BOOST_PP_SEQ_FOR_EACH_I(DENUM_PARSE,~,seq)\
		});\
        auto itResult = gString2Enum->find(value);\
        if(gString2Enum->end() != itResult)\
        {\
            v = itResult->second;\
            return true;\
        }\
        return false;\
   }

#define DENUM_IMPL_PARSE_EX(EnumName, seq)\
    bool EnumName::ParseEx(const std::string& value,int32_t& v)\
    {\
        dvector<std::string> actions;\
        boost::algorithm::split(actions,value,[](char c) {return c == '|'; },boost::algorithm::token_compress_on);\
\
        v=0;\
        for(auto& action : actions)\
        {\
            EnumName::type tmpAction;\
            if(Parse(action,tmpAction))\
            {\
                v |= tmpAction;\
            }\
            else\
            {\
                return false;\
            }\
        }\
\
        return true;\
    }


#endif
