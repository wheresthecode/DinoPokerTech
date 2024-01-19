#include "pch.h"
#include "StringUtility.h"
#include <sstream>
#include <fstream>
using namespace std;

std::vector<std::string> StringSplit(const std::string &s, char delim)
{
    vector<string> result;
    stringstream ss(s);
    string item;

    while (getline(ss, item, delim))
    {
        result.push_back(item);
    }

    return result;
}

vector<string> StringSplit(string s, string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

std::string StripWhitespace(const std::string &str)
{
    std::string output;
    output.reserve(str.size()); // optional, avoids buffer reallocations in the loop
    for (size_t i = 0; i < str.size(); ++i)
        if (str[i] != ' ')
            output += str[i];
    return output;
}

std::string StringsJoin(const std::string &m, const std::vector<std::string> &list)
{
    std::string s = "";
    for (int i = 0; i < list.size(); i++)
    {
        if (i != 0)
        {
            s += m;
        }
        s += list[i];
    }
    return s;
}

bool ReadFullTextFile(const std::string &path, std::string &outString)
{
    std::string output;
    std::ifstream file(path);

    if (!file.good())
        return false;

    file.seekg(0, std::ios::end);

    outString.resize((size_t)file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(&outString[0], outString.size());
    file.close();
    return true;
}