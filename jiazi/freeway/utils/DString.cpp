//
// Created by shgli on 17-8-15.
//

#include "utils/DString.h"
void DString::Replace(std::string& origin, const std::string& from, const std::string& to)
{
    auto fromPos = origin.rfind(from);
    while(fromPos != origin.npos)
    {
        origin.replace(fromPos, from.size(), to);
        if(fromPos > 0)
        {
            fromPos = origin.rfind(from, fromPos - 1);
        }
        else
        {
            break;
        }
    }
}

std::regex DString::Wildcard2Regex(std::string wildcardPattern)
{
#ifdef _WIN32
    boost::replace_all(wildcardPattern, "/", "\\");
#endif
    Replace(wildcardPattern, "\\", "\\\\");
    Replace(wildcardPattern, "^", "\\^");
    Replace(wildcardPattern, ".", "\\.");
    Replace(wildcardPattern, "$", "\\$");
    Replace(wildcardPattern, "|", "\\|");
    Replace(wildcardPattern, "(", "\\(");
    Replace(wildcardPattern, ")", "\\)");
    Replace(wildcardPattern, "[", "\\[");
    Replace(wildcardPattern, "]", "\\]");
    Replace(wildcardPattern, "*", "\\*");
    Replace(wildcardPattern, "+", "\\+");
    Replace(wildcardPattern, "?", "\\?");
    Replace(wildcardPattern, "/", "\\/");

    Replace(wildcardPattern, "\\?", ".");
    Replace(wildcardPattern, "\\*", ".*");
    wildcardPattern += '$';
    return std::regex(wildcardPattern);
}

bool DString::IsMatch(const std::string& text, const std::string& wildcard)
{
    auto wildcardCopy = wildcard;
    auto wildcardPattern = std::move(Wildcard2Regex(wildcardCopy));
    return std::regex_match(text, wildcardPattern);
}