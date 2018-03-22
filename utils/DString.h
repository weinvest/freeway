//
// Created by shgli on 17-8-15.
//

#ifndef ARAGOPROJECT_DSTRING_H
#define ARAGOPROJECT_DSTRING_H

#include <regex>
class DString
{
public:
    static std::regex Wildcard2Regex(std::string wildcardPattern);
    static void Replace(std::string& origin, const std::string& from, const std::string& to);
    static bool IsMatch(const std::string& text,const std::string& wildcard);
};


#endif //ARAGOPROJECT_DSTRING_H
