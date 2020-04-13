/*
 * $Id: utils.cpp 3071 2014-07-26 18:01:08Z darren_janeczek $
 */

#include "utils.h"
#include <cctype>
#include <cstdlib>
#include <ctime>

/**
 * Trims whitespace from a std::string
 * @param val The string you are trimming
 * @param chars_to_trim A list of characters that will be trimmed
 */
string& trim(string &val, const string &chars_to_trim) {
    using namespace std;
    string::iterator i;
    if (val.size()) {
        string::size_type pos;
        for (i = val.begin(); (i != val.end()) && (pos = chars_to_trim.find(*i)) != string::npos; )
            i = val.erase(i);    
        for (i = val.end()-1; (i != val.begin()) && (pos = chars_to_trim.find(*i)) != string::npos; )
            i = val.erase(i)-1;
    }
    return val;
}

/**
 * Converts the string to lowercase
 */ 
string& lowercase(string &val) {
    using namespace std;
    string::iterator i;
    for (i = val.begin(); i != val.end(); i++)
        *i = tolower(*i);
    return val;
}

/**
 * Converts the string to uppercase
 */ 
string& uppercase(string &val) {
    using namespace std;
    string::iterator i;
    for (i = val.begin(); i != val.end(); i++)
        *i = toupper(*i);
    return val;
}

/**
 * Converts an integer value to a string
 */ 
string zu4_to_string(int val) {
    char buffer[16];    
    sprintf(buffer, "%d", val);
    return buffer;
}

/**
 * Splits a string into substrings, divided by the charactars in
 * separators.  Multiple adjacent seperators are treated as one.
 */
std::vector<string> split(const string &s, const string &separators) {
    std::vector<string> result;
    string current;

    for (unsigned i = 0; i < s.length(); i++) {
        if (separators.find(s[i]) != string::npos) {
            if (current.length() > 0)
                result.push_back(current);
            current.erase();
        } else
            current += s[i];
    }

    if (current.length() > 0)
        result.push_back(current);

    return result;
}
