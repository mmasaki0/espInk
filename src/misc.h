#include <algorithm> 
#include <cctype>
#include <string>

// copied from https://stackoverflow.com/a/217605

namespace misc {
    // Trim from the start (in place)
    inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    // Trim from the end (in place)
    inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // Trim from both ends (in place)
    inline void trim(std::string &s) {
        rtrim(s);
        ltrim(s);
    }

    // Trim from the start (copying)
    inline std::string ltrim_copy(std::string s) {
        ltrim(s);
        return s;
    }

    // Trim from the end (copying)
    inline std::string rtrim_copy(std::string s) {
        rtrim(s);
        return s;
    }

    // Trim from both ends (copying)
    inline std::string trim_copy(std::string s) {
        trim(s);
        return s;
    }
}

