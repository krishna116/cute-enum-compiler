#include "CecCodeGenerator.h"
#include "CecEnumLexer.h"
#include "cec.h"
#include "config.h"

#include <vector>
#include <sstream>
#include <ctime>
#include <cassert>

using namespace cec;

// Replace all.
std::string rep(const std::string &source,
                const std::string &from,
                const std::string &to)
{
    if(source.empty()) return {};
    if(from.empty()) return source;

    std::ostringstream oss;
    std::size_t pos = 0;
    std::size_t prev;
    while (true)
    {
        prev = pos;
        pos = source.find(from, pos);
        if (pos == std::string::npos)
        {
            break;
        }
        oss << source.substr(prev, pos - prev);
        oss << to;
        pos += from.size();
    }
    oss << source.substr(prev);
    return oss.str();
}

std::string GetCurrentTimeStr()
{
    auto tm = std::time(nullptr);
    auto timeStr = std::string(std::asctime(std::localtime(&tm)));

    // No implicit CR/LF, as it may case generated code abnormal.
    while (!timeStr.empty() && (timeStr.back() == '\n' || timeStr.back() == '\r'))
    {
        timeStr.pop_back();
    }

    return timeStr;
}

std::string GenCecEnumName(const EnumClass &ec)
{
    return ec.name;
}

std::string GenCecEnumFullName(const EnumClass &ec)
{
    return ec.fullName;
}

std::string GenCecEnumType(const EnumClass &ec)
{
    return ec.type;
}

std::string GenCecEnumMin(const EnumClass &ec)
{
    return std::to_string(ec.number);
}

std::string GenCecEnumMax(const EnumClass &ec)
{
    return std::to_string(ec.number + ec.keys.size() - 1);
}

std::string GenCecEnumSize(const EnumClass &ec)
{
    return std::to_string(ec.keys.size());
}

std::string GenCecEnumFirstKey(const EnumClass &ec)
{
    return ec.keys.front();
}

std::string GenCecEnumLastKey(const EnumClass &ec)
{
    return ec.keys.back();
}

std::string GenCecEnumKeyList(const EnumClass &ec, const std::string& indention="")
{
    std::string keyTable;

    auto quota = [](const std::string &str)
    {
        return std::string("\"" + str + "\"");
    };

    for(auto it = ec.keys.begin(); it!= ec.keys.end(); ++it)
    {
        keyTable += indention + quota(*it);
        if((it+1) != ec.keys.end())
        {
            keyTable += ",\n";
        }
    }

    return keyTable;
}

std::string GenCecEnumKeyValueList(const EnumClass &ec, 
                                    bool keepComment = false,
                                    const std::string& indention=""
                                    )
{
    std::string keyValueTable;
    const std::string equal{" = "};
    auto const sizeOfKeys = ec.keys.size();
    auto number = ec.number;

    // Check if need keep comments.
    if(keepComment && (ec.comment.keyComments.empty() 
        || ec.comment.keyComments.size() != sizeOfKeys))
    {
        keepComment = false;
        print_info("CecCodeGenerator, code comment won't generated.\n");
    }

    auto trimBackSpace = [](std::string in)
    {
        while(!in.empty())
        {
            if(in.back() == ' ' || in.back() == '\t' || in.back() == '\r'
                ||in.back()=='\n' || in.back() == '\v')
            {
                in.pop_back(); 
            }
            else
            {
                break;
            }
        }
        return in;
    };

    // Generata all the key value pairs.
    std::size_t i = 0;
    for (auto const &key : ec.keys)
    {
        // Add key value.
        keyValueTable += indention + config::EnumKeyValueDataType
                      + " " + ec.type + " " + key + equal 
                      + std::to_string(number++) + ";";

        // Add comment.
        if(keepComment)
        {
            if(i+1 != sizeOfKeys)
            {
                // Align patch for the last comment.
                keyValueTable += " ";
            }
            keyValueTable += trimBackSpace(ec.comment.keyComments[i]);
        }
        if(i+1 < sizeOfKeys)
        {
            keyValueTable += "\n";
        }
        ++i;
    }
    return keyValueTable;
}

std::string GenCecSignature()
{
    std::string s{"Generated by <"};
    s += config::VersionStr;
    s += "> -- " + GetCurrentTimeStr();
    return s;
}

std::string GenerateKeyAndValueList(const EnumClass &ec, const std::string& in)
{
    bool isKeepComment = false;
    std::string fullCecKeepCommentTag;
    std::string code = in;

    // Check if keep comment.
    while(CecEnumLexer::getFullCecKeepCommentTag(code,fullCecKeepCommentTag))
    {
        isKeepComment = true;
        code = std::move(rep(code, fullCecKeepCommentTag, ec.comment.headComment));
    }

    // Generate key list.
    std::string fullCecEnumKeyListTag;
    std::size_t indention = 0;
    while(CecEnumLexer::getFullCecEnumKeyListTag(code,
                                fullCecEnumKeyListTag,
                                indention))
    {
        code = std::move(rep(code, fullCecEnumKeyListTag, 
                    GenCecEnumKeyList(ec,std::string(indention,' '))));
    }

    // Generate key=value list.
    std::string fullCecEnumKeyValueListTag;
    indention = 0;
    while(CecEnumLexer::getFullCecEnumKeyValueListTag(code,
                                fullCecEnumKeyValueListTag,
                                indention))
    {
        code = std::move(rep(code, fullCecEnumKeyValueListTag, 
                    GenCecEnumKeyValueList(ec,isKeepComment,std::string(indention,' '))));
    }

    return code;
}

std::string CecCodeGenerator::genCode(const EnumClass &ec,
                                      const std::string &sample)
{
    std::string code =  "//" + GenCecSignature() + "\n";
    code += sample.empty() ? config::CodeSample : sample;
    
    code = std::move(GenerateKeyAndValueList(ec,code));
    code = std::move(rep(code, "{cec:enum:name}", GenCecEnumName(ec)));
    code = std::move(rep(code, "{cec:enum:fullName}", GenCecEnumFullName(ec)));
    code = std::move(rep(code, "{cec:enum:type}", GenCecEnumType(ec)));
    code = std::move(rep(code, "{cec:enum:min}", GenCecEnumMin(ec)));
    code = std::move(rep(code, "{cec:enum:max}", GenCecEnumMax(ec)));
    code = std::move(rep(code, "{cec:enum:size}", GenCecEnumSize(ec)));
    code = std::move(rep(code, "{cec:enum:firstKey}", GenCecEnumFirstKey(ec)));
    code = std::move(rep(code, "{cec:enum:lastKey}", GenCecEnumLastKey(ec)));

    if(code.empty())
    {
        print_error("CecCodeGenerator::genCode failed.\n");
    }
    
    return code;
}