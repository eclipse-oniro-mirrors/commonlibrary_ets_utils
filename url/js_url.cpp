/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "js_url.h"
#include "utils/log.h"
#include "securec.h"
#include <sstream>

std::map<std::string, int> g_head = { {"ftp:", 21}, {"file:", -1}, {"gopher:", 70}, {"http:", 80},
    {"https:", 443}, {"ws:", 80}, {"wss:", 443} };

std::vector<std::string> g_doubleSegment = { "..", ".%2e", ".%2E", "%2e.", "%2E.",
    "%2e%2e", "%2E%2E", "%2e%2E", "%2E%2e" };

std::vector<std::string> g_singlesegment = { ".", "%2e", "%2E" };

std::vector<char> g_specialcharacter = { '\0', '\t', '\n', '\r', ' ', '#', '%', '/', ':', '?', '@', '[', '\\', ']' };

#define MAXPORT 65535

void ReplaceSpecialSymbols(std::string& input,std::string& oldstr,std::string& newstr)
{
    size_t oldlen = oldstr.size();
    while(true) {
        size_t pos = 0;    
        if((pos = input.find(oldstr)) != std::string::npos) {
            input.replace(pos,oldlen,newstr);
        } else {
            break;
        }
    }
}

template <typename T>
bool IsASCIITabOrNewline(const T ch)
{
    return (ch == '\t' || ch == '\n' || ch == '\r');
}

template <typename T>
bool IsHexDigit(const T ch)
{
    if (isdigit(ch) || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
        return true;
    }
    return false;
}

template <typename T>
unsigned Hex2bin(const T ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    return static_cast<unsigned>(-1);
}

std::string DecodePercent(const char* input, size_t len)
{
    std::string temp;
    if (len == 0) {
        return temp;
    }
    temp.reserve(len);
    const char* it = input;
    const char* end = input + len;
    while (it < end) {
        const char ch = it[0];
        size_t left = end - it - 1;
        if (ch != '%' || left < 2 || (ch == '%' && (!IsHexDigit(it[1]) || !IsHexDigit(it[2])))) {
            temp += ch;
            it++;
            continue;
        } else {
            unsigned a = Hex2bin(it[1]);
            unsigned b = Hex2bin(it[2]);
            char c = static_cast<char>(a * 16 + b); // 16:Convert hex
            temp += c;
            it += 3;
        }
    }
    return temp;
}

void DeleteC0OrSpace(std::string& str)
{
    if (str.empty()) {
        return;
    }
    size_t i = 0;
    size_t strlen = str.size();
    for (; i < strlen;) {
        if (str[i] >= '\0' && str[i] <= ' ') {
            i++;
        } else {
            break;
        }
    }
    str = str.substr(i);
    strlen = str.size();
    for (i = strlen - 1; i != 0; i--) {
        if (str[i] >= '\0' && str[i] <= ' ') {
            str.pop_back();
        } else {
            break;
        }
    }
}

void DeleteTabOrNewline(std::string& str1)
{
    for (auto item = str1.begin(); item != str1.end(); ) {
        if (IsASCIITabOrNewline(*item)) {
            item = str1.erase(item);
        } else {
            ++item;
        }
    }
}

bool IsSpecial(std::string scheme)
{
    auto temp = g_head.count(scheme);
    if (temp > 0) {
        return true;
    }
    return false;
}

bool AnalysisScheme(std::string& input, std::string& scheme, std::bitset<11>& flags)
{
    if (!isalpha(input[0])) {
        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return false;
    } else {
        size_t strlen = input.size();
        for (size_t i = 0; i < strlen - 1; ++i) {
            if (isalnum(input[i]) || input[i] == '+' || input[i] == '-' || input[i] == '.') {
                if (isupper(input[i])) {
                    input[i] = tolower(input[i]);
                }
            } else {
                flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
                return false;
            }
        }
        scheme = input;
        if (IsSpecial(scheme)) {
            flags.set(1); // 1:Bit 1 Set to true,The protocol is the default protocol
        }
        return true;
    }
}

void AnalysisFragment(std::string& input, std::string& fragment, std::bitset<11>& flags)
{
    fragment = input;
    flags.set(8); // 8:Bit 8 Set to true,The URL has fragment
}

void AnalysisQuery(std::string& input, std::string& query, std::bitset<11>& flags)
{
    query = input;
    flags.set(7); // 7:Bit 7 Set to true,The URL has query
}

void AnalysisUsernameAndPasswd(std::string& input, std::string& username, std::string& password,
    std::bitset<11>& flags)
{
    int pos = input.size() - 1;
    for (; pos >= 0; pos--) {
        if (input[pos] == '@') {
            break;
        }
    }
    std::string userAndPasswd = input.substr(0, pos);
    input = input.substr(pos + 1);
    if (userAndPasswd.empty()) {
        return;
    }
    if (userAndPasswd.find('@') != std::string::npos) {
        while (true) {
            size_t i = 0;
            if ((i = userAndPasswd.find('@')) != std::string::npos) {
                userAndPasswd = userAndPasswd.replace(i, 1, "%40");
            } else {
                break;
            }
        }
    }

    if (userAndPasswd.find(':') != std::string::npos) {
        size_t i = userAndPasswd.find(':');
        std::string user = userAndPasswd.substr(0, i);
        std::string passwd = userAndPasswd.substr(i + 1);
        if (!user.empty()) {
            username = user;
            flags.set(2); // 2:Bit 2 Set to true,The URL has username
        }
        if (!passwd.empty()) {
            password = passwd;
            flags.set(3); // 3:Bit 3 Set to true,The URL has password
        }
    } else {
        username = userAndPasswd;
        flags.set(2); // 2:Bit 2 Set to true,The URL has username
    }
}

void AnalysisPath(std::string& input, std::vector<std::string>& path, std::bitset<11>& flags, bool isSpecial)
{
    std::vector<std::string> temp;
    size_t pos = 0;
    while (((pos = input.find('/')) != std::string::npos) ||
        ((pos = input.find('\\')) != std::string::npos && isSpecial)) {
        temp.push_back(input.substr(0, pos));
        input = input.substr(pos + 1);
    }
    temp.push_back(input);
    for (size_t it = 0; it < temp.size(); ++it) {
        auto result = find(g_doubleSegment.begin(), g_doubleSegment.end(), temp[it]);
        if (result != g_doubleSegment.end()) {
            if (path.empty()) {
                if (it == temp.size() - 1) {
                    path.emplace_back("");
                    flags.set(6); // 6:Bit 6 Set to true,The URL has pathname
                }
                continue;
            }
            path.pop_back();
            if (it == temp.size() - 1) {
                path.emplace_back("");
                flags.set(6); // 6:Bit 6 Set to true,The URL has pathname
            }
            continue;
        }
        result = find(g_singlesegment.begin(), g_singlesegment.end(), temp[it]);
        if (result != g_singlesegment.end() && it == temp.size() - 1) {
            path.emplace_back("");
            flags.set(6); // 6:Bit 6 Set to true,The URL has pathname
            continue;
        }
        if (result == g_singlesegment.end()) {
            path.push_back(temp[it]);
            flags.set(6); // 6:Bit 6 Set to true,The URL has pathname
        }
    }
}

void AnalysisPort(std::string input, url_data& urlinfo, std::bitset<11>& flags)
{
    for (auto i : input) {
        if (!isdigit(i)) {
            flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return;
        }
    }
    int it = stoi(input);
    if (it > MAXPORT) {
        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return;
    }
    flags.set(5);// 5:Bit 5 Set to true,The URL Port is the specially
    for (auto i : g_head) {
        if (i.first == urlinfo.scheme && i.second == it) {
            urlinfo.port = -1;
            flags.set(5, 0); // 5:Bit 5 Set to false,The URL Port is the default
            return;
        }
    }
    urlinfo.port = it;
}

void AnalysisOpaqueHost(std::string input, std::string& host, std::bitset<11>& flags)
{
    size_t strlen = input.size();
    for (size_t i = 0; i < strlen; ++i) {
        char ch = input[i];
        auto result = find(g_specialcharacter.begin(), g_specialcharacter.end(), ch);
        if (ch != '%' && (result != g_specialcharacter.end())) {
            flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return;
        }
    }
    host = input;
    flags.set(4); // 4:Bit 4 Set to true,The URL has hostname
}

std::string IPv6HostCompress(std::vector<std::string>& tempIPV6, int flag)
{
    std::string input;
    if (flag == 1) {
        size_t strlen = tempIPV6.size();
        for (size_t i = 0; i < strlen; ++i) {
            if (tempIPV6[i][0] == '?' && tempIPV6[i].size() == 1) {
                input += ":";
            }
            else {
                input += tempIPV6[i];
                if (i != tempIPV6.size() - 1) {
                    input += ":";
                }
            }
        }
        return input;
    } else {
        int max = 0;
        int count = 0;
        size_t maxZeroIndex = 0;
        size_t strlen = tempIPV6.size();
        for (size_t i = 0; i < strlen;) {
            if (tempIPV6[i] == "0" && (i + 1 < tempIPV6.size() && tempIPV6[i + 1] == "0")) {
                int index = i;
                while (i < tempIPV6.size() && tempIPV6[i] == "0") {
                    i++;
                    count++;
                }
                max < count ? max = count, maxZeroIndex = index : 0;
            }
            else {
                count = 0;
                i++;
            }
        }
        if (count == 8) {  // 8:If IPv6 is all 0
            return "::";
        } else if (max == 0) {
            strlen = tempIPV6.size();
            for (size_t i = 0; i < strlen; ++i) {
                input += tempIPV6[i];
                if (i != strlen - 1) {
                    input += ":";
                }
            }
            return input;
        } else {
            if (maxZeroIndex == 0) {
                input += "::";
                strlen = tempIPV6.size();
                for (size_t i = max; i < strlen; ++i) {
                    input += tempIPV6[i] + ":";
                }
                input.pop_back();
                return input;
            } else {
                for (size_t i = 0; i < maxZeroIndex; ++i) {
                    input += tempIPV6[i];
                    if (i != maxZeroIndex - 1) {
                        input += ":";
                    }
                }
                input += "::";
                strlen = tempIPV6.size();
                for (size_t i = maxZeroIndex + max; i < strlen; ++i) {
                    input += tempIPV6[i];
                    if (i != strlen - 1) {
                        input += ":";
                    }
                }
                return input;
            }
        }
    }
}

void IPv6Host(std::string& input, std::string& host, std::bitset<11>& flags)
{
    if (input.size() == 0) {
        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return;
    }
    std::string strInput = input;
    std::stringstream ss;
    std::string number_hex;
    unsigned val = 0;
    unsigned number_flag = 0;
    std::vector <std::string> temp;
    std::vector <std::string> temp_end;
    std::vector <std::string> IPV6;
    size_t pos = 0;
    int temp_prot[4] = { 0 };
    int  n = 0;
    int flag = 0;
    if ((pos = strInput.find("::", 0)) != std::string::npos) {
        flag = 1;
        if (strInput.find("::", pos + 2) != std::string::npos) {
            flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return;
        }
    } else {
        flag = 0;
    }
    while (((pos = strInput.find(':')) != std::string::npos)) {
        temp.push_back(strInput.substr(0, pos));
        strInput = strInput.substr(pos + 1);
    }
    temp.push_back(strInput);
    if (temp.size() > 8) { // 8:The incoming value does not meet the criteria
        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return;
    }
    for (size_t i = 0; i < temp.size(); ++i) {
        if (temp[i].empty()) {
            IPV6.push_back("?");
        } else {
            size_t strlen = temp[i].size();
            for (size_t j = 0; j < strlen; ++j) {
                if (((temp[i].find('.')) != std::string::npos)) {
                    number_flag = i;
                    if (temp.size() == i || temp.size() > 7) {
                        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
                        return;
                    }
                    break;
                } else if (IsHexDigit(temp[i][j])) {
                    val = val * 0x10 + Hex2bin(temp[i][j]);
                }
            }
            ss << std::hex << val;
            ss >> number_hex;
            IPV6.push_back(number_hex);
            ss.clear();
            number_hex.clear();
            val = 0;
        }
    }
    if (number_flag != 0) {
        while (((pos = temp[number_flag].find('.')) != std::string::npos)) {
            temp_end.push_back(temp[number_flag].substr(0, pos));
            temp[number_flag] = temp[number_flag].substr(pos + 1);
        }
        temp_end.push_back(temp[number_flag]);
        if (temp_end.size() != 4) {
            flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return;
        } else {
            size_t strlen = temp_end.size();
            for (size_t x = 0; x < strlen; ++x) {
                val = stoi(temp_end[x]);
                if (val > 255) {
                    flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
                    return;
                }
                temp_prot[n] = val;
                n++;
                val = 0;
            }
        }
        ss << std::hex << temp_prot[0] * 0x100 + temp_prot[1];
        ss >> number_hex;
        IPV6.push_back(number_hex);
        ss.clear();
        number_hex.clear();
        ss << std::hex << temp_prot[2] * 0x100 + temp_prot[3];
        ss >> number_hex;
        IPV6.push_back(number_hex);
        ss.clear();
        number_hex.clear();
        IPV6.erase(IPV6.end() - 3);
    }
    strInput = IPv6HostCompress(IPV6, flag);
    host = '[' + strInput + ']';
    flags.set(4); // 4:Bit 4 Set to true,The URL has host
    flags.set(10);// 10:Bit 10 Set to true,The host is IPV6
}

int64_t AnalyseNum(std::string parts)
{
    enum NUMERATION
    {
        OCT = 8, // 8:Octal
        DEC = 10, // 10:Decimal
        HEX = 16 // 16:Hexadecimal
    };
    unsigned num = 10; // 10:Decimal
    std::string partsBeg = parts.substr(0, 2); // 2:Take two digits to determine whether it is hexadecimal 
    size_t partsLen = parts.length();
    if (partsLen >= 2 && (partsBeg == "0X" || partsBeg == "0x")) {
        num = 16;
        parts = parts.substr(2); // 2:delete '0x'
    } else if (num == 10 && partsLen > 1 && parts.substr(0, 1) == "0") { // 0:delete '0'
        num = 8;
        parts = parts.substr(1);
    }
    for (size_t i = 0; i < parts.length(); i++) {
        if (NUMERATION(num) == OCT) {
            if (parts[i] < '0' || parts[i] > '7') { // 0~7:Octal
                return -1;
            }
        } else if (NUMERATION(num) == DEC) {
            if (parts[i] < '0' || parts[i] > '9') { // 0~9:Decimal
                return -1;
            }
        } else if (NUMERATION(num) == HEX) {
            if (!((parts[i] >= '0' && parts[i] <= '9') ||  // 0~9, a~f, A~F:Hexadecimal
                (parts[i] >= 'A' && parts[i] <= 'F') ||
                (parts[i] >= 'a' && parts[i] <= 'f'))) {
                return -1;
            }
        }
    }
    return strtoll(parts.c_str(), nullptr, num);
}

bool OverHex(std::string input)
{
    size_t size = input.size();
    for (size_t i = 0; i < size; i++)
    {
        return !IsHexDigit(input[i]);
    }
    return false;
}

bool NotAllNum(std::string input) {

    size_t size = input.size();
    for (size_t i = 0; i < size; i++)
    {
        if (!isdigit(input[i]))
        {
            return true;
        }
    }
    return false;
}

bool AnalyseIPv4(const char* instr, size_t len, std::string& host, std::bitset<11>& flags)
{
    int count = 0;
    for (const char* ptr = instr; *ptr != '\0'; ptr++) {
        if (*ptr == '.') {
            if (++count > 3) { //3: The IPV4 address has only four segments
                return false;
            }
        }
    }
    if (count != 3) {  // 3:The IPV4 address has only four segments
        return false;
    }

    size_t pos = 0;
    std::vector<std::string> strVec;
    std::string input = static_cast<std::string>(instr);
    while (((pos = input.find('.')) != std::string::npos)) {
        strVec.push_back(input.substr(0, pos));
        input = input.substr(pos + 1);
    }
    strVec.push_back(input);
    size_t size = strVec.size();
    for (size_t i = 0; i < size; i++) {
        if (strVec[i].empty())
        {
            return false;
        }
        std::string begStr = strVec[i].substr(0, 2); // Intercept the first two characters
        if ((begStr == "0x" || begStr == "0X") && OverHex(strVec[i].substr(2)))
        {
            return false;
        } else if ((begStr == "0x" || begStr == "0X") && !(OverHex(strVec[i].substr(2))))
        {
            continue;
        }
        if (NotAllNum(strVec[i]))
        {
            return false;
        }
    }
    for (size_t i = 0; i < size; i++) {
        int64_t value = AnalyseNum(strVec[i].c_str());
        if ((value < 0) || (value > 255)) { // 255:Only handle numbers between 0 and 255
            flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return false;
        } else {
            host += std::to_string(value);
            if (i != size - 1)
            {
                host += ".";
            }
        }
    }
    flags.set(4); // 4:Bit 4 Set to true,The URL has hostname
    return true;
}
void AnalysisHost(std::string& input, std::string& host, std::bitset<11>& flags, bool is_Specoal)
{
    if (input.empty()) {
        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return;
    }
    if (input[0] == '[') {
        if ((input[input.length() - 1]) == ']') {
            size_t  b = input.length();
            input = input.substr(1, b - 2);
            IPv6Host(input, host, flags);
            return;
        } else {
            flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return;
        }
    }
    if (!is_Specoal) {
        AnalysisOpaqueHost(input, host, flags);
        return;
    }
    std::string decodeInput = DecodePercent(input.c_str(), input.length());
    size_t strlen = decodeInput.size();
    for (size_t pos = 0; pos < strlen; ++pos) {
        char ch = decodeInput[pos];
        auto result = find(g_specialcharacter.begin(), g_specialcharacter.end(), ch);
        if (result != g_specialcharacter.end()) {
            flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return;
        }
    }
    bool ipv4 = AnalyseIPv4(decodeInput.c_str(), decodeInput.length(), host, flags);
    if (ipv4) {
        return;
    }
    host = decodeInput;
    flags.set(4); // 4:Bit 4 Set to true,The URL has hostname
}
bool ISFileNohost(const std::string& input)
{
    if ((isalpha(input[0]) && (input[1] == ':' || input[1] == '|'))) {
        return true;
    }
    return false;
}
void AnalysisFilePath(std::string& input, url_data& urlinfo, std::bitset<11>& flags)
{
    std::vector<std::string> temp;
    size_t pos = 0;
    while (((pos = input.find('/')) != std::string::npos) || ((pos = input.find('\\')) != std::string::npos)) {
        temp.push_back(input.substr(0, pos));
        input = input.substr(pos + 1);
    }
    temp.push_back(input);

    for (size_t i = 0; i < temp.size(); ++i) {
        auto a = find(g_doubleSegment.begin(), g_doubleSegment.end(), temp[i]);
        if (a != g_doubleSegment.end()) {
            if ((urlinfo.path.size() == 1) && (isalpha(urlinfo.path[0][0])
                && ((urlinfo.path[0][1] == ':') || (urlinfo.path[0][1] == '|')))) {
                if (urlinfo.path[0].size() == 2) {
                    urlinfo.path[0][1] = ':';
                }
            } else if (!urlinfo.path.empty()) {
                urlinfo.path.pop_back();
            }
            if (i == temp.size() - 1) {
                urlinfo.path.push_back("");
            }
            continue;
        }
        a = find(g_singlesegment.begin(), g_singlesegment.end(), temp[i]);
        if (a != g_singlesegment.end()) {
            if (i == temp.size() - 1) {
                urlinfo.path.push_back("");
            }
            continue;
        }
        urlinfo.path.push_back(temp[i]);
        flags.set(6); // 6:Bit 6 Set to true,The URL has pathname
    }
    std::string it = urlinfo.path[0];
    if (isalpha(it[0]) && (it[1] == ':' || it[1] == '|')) {
        if (it.size() == 2) {
            it[1] = ':';
            flags.set(4, 0); // 4:Bit 4 Set to false,The URL not have pathname
            urlinfo.host.clear();
        }
    }
}

void AnalysisFile(std::string& input, url_data& urlinfo, std::bitset<11>& flags)
{
    bool is_Specoal = true;
    if ((input[0] == '/' || input[0] == '\\') && (input[1] == '/' || input[1] == '\\')) {
        std::string temp = input.substr(2);
        size_t pos = 0;
        if (((pos = temp.find('/')) != std::string::npos) || ((pos = temp.find('\\')) != std::string::npos)) {
            if (pos == 0) {
                temp = temp.substr(1);
                AnalysisFilePath(temp, urlinfo, flags);
            } else {
                std::string strHost = temp.substr(0, pos);
                std::string strPath = temp.substr(pos + 1);

                if (!ISFileNohost(strHost)) {
                    AnalysisHost(strHost, urlinfo.host, flags, is_Specoal);
                    if (flags.test(0)) { // 0:Bit 0 is true,The URL Parsing failed
                        return;
                    }
                    AnalysisFilePath(strPath, urlinfo, flags);
                } else {
                    AnalysisFilePath(temp, urlinfo, flags);
                }
            }
        } else {
            if (!temp.empty()) {
                AnalysisHost(temp, urlinfo.host, flags, is_Specoal);
                if (flags.test(0)) { // 0:Bit 0 is true,The URL Parsing failed
                    return;
                }
            }
        }
    } else {
        if (input[0] == '/' || input[0] == '\\') {
            input = input.substr(1);
        }
        AnalysisFilePath(input, urlinfo, flags);
    }
}

void AnalysisFilescheme(std::string& input, url_data& urlinfo, std::bitset<11>& flags)
{
    std::string strPath = urlinfo.scheme + input;
    urlinfo.scheme = "file:";
    flags.set(1); // 1:Bit 1 Set to true,The protocol is the default protocol
    AnalysisFilePath(strPath, urlinfo, flags);
}

void AnalysisHostAndPath(std::string& input, url_data& urlinfo, std::bitset<11>& flags)
{
    if (flags.test(1)) { // 1:Bit 1 is true,The protocol is the default protocol
        size_t pos = 0;
        bool is_Special = true;
        for (; pos < input.size();) {
            if (input[pos] == '/' || input[pos] == '\\') {
                pos++;
            } else {
                break;
            }
        }

        input = input.substr(pos);
        if (input.size() == 0) {
            flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return;
        } else {
            if (input.find('/') != std::string::npos || input.find('\\') != std::string::npos) {
                for (pos = 0; pos < input.size(); pos++) {
                    if (input[pos] == '/' || input[pos] == '\\') {
                        break;
                    }
                }
                std::string strHost = input.substr(0, pos);
                std::string strPath = input.substr(pos + 1);
                if (strHost.find('@') != std::string::npos) {
                    AnalysisUsernameAndPasswd(strHost, urlinfo.username, urlinfo.password, flags);
                }
                if (strHost.empty()) {
                    flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
                    return;
                }
                if (strHost[strHost.size() - 1] != ']') {
                    if ((pos = strHost.find_last_of(':')) != std::string::npos) {
                        std::string port = strHost.substr(pos + 1);
                        strHost = strHost.substr(0, pos);
                        AnalysisPort(port, urlinfo, flags);
                        if (flags.test(0)) { // 0:Bit 0 is true,The URL Parsing failed
                            return;
                        }
                    }
                }
                AnalysisHost(strHost, urlinfo.host, flags, is_Special);
                AnalysisPath(strPath, urlinfo.path, flags, is_Special);
            } else {
                std::string strHost = input;
                if (strHost.find('@') != std::string::npos) {
                    AnalysisUsernameAndPasswd(strHost, urlinfo.username, urlinfo.password, flags);
                }
                if (strHost.empty()) {
                    flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
                    return;
                }
                if (strHost[strHost.size() - 1] != ']') {
                    if ((pos = strHost.find_last_of(':')) != std::string::npos) {
                        std::string port = strHost.substr(pos + 1);
                        strHost = strHost.substr(0, pos);
                        AnalysisPort(port, urlinfo, flags);
                        if (flags.test(0)) { // 0:Bit 0 is true,The URL Parsing failed
                            return;
                        }
                    }
                }
                AnalysisHost(strHost, urlinfo.host, flags, is_Special);
            }
        }
    } else {
        if (urlinfo.scheme.size() == 2) {
            AnalysisFilescheme(input, urlinfo, flags);
            return;
        }
        if (input[0] == '/') {
            if (input[1] == '/') {
                std::string hostandpath = input.substr(2);
                if (hostandpath.empty()) {
                    return;
                }
                size_t i = 0;
                bool is_Special = false;
                if (hostandpath.find('/') != std::string::npos) {
                    i = hostandpath.find('/');
                    std::string strHost = hostandpath.substr(0, i);
                    std::string strPath = hostandpath.substr(i + 1);
                    if (strHost.find('@') != std::string::npos) {
                        AnalysisUsernameAndPasswd(strHost, urlinfo.username, urlinfo.password, flags);
                    }
                    if (strHost.empty()) {
                        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
                        return;
                    }
                    size_t pos = 0;
                    if (strHost[strHost.size() - 1] != ']') {
                        if ((pos = strHost.find_last_of(':')) != std::string::npos) {
                            std::string port = strHost.substr(pos + 1);
                            strHost = strHost.substr(0, pos);
                            AnalysisPort(port, urlinfo, flags);
                            if (flags.test(0)) { // 0:Bit 0 is true,The URL Parsing failed
                                return;
                            }
                        }
                    }
                    AnalysisHost(strHost, urlinfo.host, flags, is_Special);
                    AnalysisPath(strPath, urlinfo.path, flags, is_Special);
                } else {
                    std::string strHost = hostandpath;
                    if (strHost.find('@') != std::string::npos) {
                        AnalysisUsernameAndPasswd(strHost, urlinfo.username, urlinfo.password, flags);
                    }
                    if (strHost.empty()) {
                        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
                        return;
                    }
                    if (strHost[strHost.size() - 1] != ']') {
                        if (size_t pos = (strHost.find_last_of(':')) != std::string::npos) {
                            std::string port = strHost.substr(pos + 1);
                            strHost = strHost.substr(0, pos);
                            AnalysisPort(port, urlinfo, flags);
                            if (flags.test(0)) { // 0:Bit 0 is true,The URL Parsing failed
                                return;
                            }
                        }
                    }
                    AnalysisHost(strHost, urlinfo.host, flags, is_Special);
                }
            } else {
                std::string strPath = input.substr(1);
                bool is_Special = false;
                AnalysisPath(strPath, urlinfo.path, flags, is_Special);
            }
        } else {
            flags.set(9); // 6:Bit 6 Set to true,The URL Can not be base
            if (urlinfo.path.empty()) {
                urlinfo.path.emplace_back("");
            }
            urlinfo.path[0] = input;
            flags.set(6); // 6:Bit 6 Set to true,The URL has pathname
        }
    }
}

void AnalysisInput(std::string& input, url_data& urlData, std::bitset<11>& flags, std::bitset<11>& baseflags)
{
    if (input.find('#') != std::string::npos) {
        size_t i = input.find('#');
        std::string fragment = input.substr(i);
        AnalysisFragment(fragment, urlData.fragment, flags);
        input = input.substr(0, i);
    }
    if (input.find('?') != std::string::npos) {
        size_t i = input.find('?');
        std::string query = input.substr(i);
        AnalysisQuery(query, urlData.query, flags);
        input = input.substr(0, i);
    }
    bool is_Special = (baseflags.test(1) ? true : false); // 1:Bit 1 is true,The URL Default for protocol
    AnalysisPath(input, urlData.path, flags, is_Special);
}

void BaseInfoToUrl(const url_data& baseInfo, const std::bitset<11>& baseflags, url_data& urlData,
    std::bitset<11>& flags, bool inputIsEmpty)
{
    urlData.scheme = baseInfo.scheme;
    flags.set(1, baseflags.test(1)); // 1:Base flag to the current URL
    urlData.host = baseInfo.host;
    flags.set(4); // 4:Bit 4 Set to true,The URL has hostname  
    urlData.username = baseInfo.username;
    flags.set(2, baseflags.test(2)); // 2:Base flag to the current URL
    urlData.password = baseInfo.password;
    flags.set(3, baseflags.test(3)); // 3:Base flag to the current URL
    urlData.port = baseInfo.port;
    flags.set(5, baseflags.test(5)); // 5:Base flag to the current URL
    if (inputIsEmpty) {
        urlData.path = baseInfo.path;
        flags.set(6, baseflags.test(6)); // 6:Base flag to the current URL
        urlData.query = baseInfo.query;
        flags.set(7, baseflags.test(7)); // 7:Base flag to the current URL
        urlData.fragment = baseInfo.fragment;
        flags.set(8, baseflags.test(8)); // 8:Base flag to the current URL
    }
    flags.set(9,baseflags.test(9)); // 9:Base flag to the current URL
    flags.set(10,baseflags.test(10)); // 10:Base flag to the current U
}

URL::URL(napi_env env, const std::string& input)
{
    env_ = env;
    std::string str = input;
    DeleteC0OrSpace(str);
    DeleteTabOrNewline(str);
    InitOnlyInput(str, urlData_, flags_);
}

URL::URL(napi_env env, const std::string& input, const std::string& base)
{
    env_ = env;
    url_data baseInfo;
    std::bitset<11> baseflags;
    std::string strBase = base;
    std::string strInput = input;
    if (strBase.empty()) {
        baseflags.set(0); // 0:Bit 0 Set to true,The baseURL analysis failed
    }
    DeleteC0OrSpace(strBase);
    DeleteTabOrNewline(strBase);
    DeleteC0OrSpace(strInput);
    DeleteTabOrNewline(strInput);
    InitOnlyInput(strBase, baseInfo, baseflags);
    if (baseflags.test(0)) {
        flags_.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return;
    } else if (!baseflags.test(0)) { // 0:Bit 0 not is true,The baseURL analysis success
        InitOnlyInput(strInput, urlData_, flags_);
        if (!flags_.test(0)) { // 0:Bit 0 not is true,The URL analysis success
            return;
        }
        if ((input[0] == '/' || input[0] == '\\') && (input[1] == '/' || input[1] == '\\')) {
            std::string new_input = baseInfo.scheme + input;
            flags_.set(0, 0); // 0:Bit 0 Set to false,The URL analysis success
            InitOnlyInput(new_input, urlData_, flags_);
            return;
        }
        if (baseflags.test(4)) { //4:Bit 4 is true,The URL has hostname
            flags_.set(0, 0);
            BaseInfoToUrl(baseInfo, baseflags, urlData_, flags_, input.empty());
            if (!input.empty()) {
                if ((input[0] == '/') || (input[0] == '\\' && flags_.test(1))) {
                    strInput = input.substr(1);
                }
                AnalysisInput(strInput, urlData_, flags_, baseflags);
            }
        } else if (!baseflags.test(4)) { //4:Bit 4 is true,The URL has hostname
            flags_.set(0); // 0:Bit 0 Set to true,The URL analysis failed
            return;
        }
    }
}

URL::URL(napi_env env, const std::string& input, const URL& base)
{
    env_ = env;
    std::string strInput = input;
    url_data baseInfo = base.urlData_;
    std::bitset<11> baseflags = base.flags_;
    DeleteC0OrSpace(strInput);
    DeleteTabOrNewline(strInput);
    InitOnlyInput(strInput, urlData_, flags_);
    if (!flags_.test(0)) { // 0:Bit 0 not is true,The URL analysis failed
        return;
    }
    if ((input[0] == '/' || input[0] == '\\') && (input[1] == '/' || input[1] == '\\')) {
        std::string new_input = baseInfo.scheme + input;
        flags_.set(0, 0); // 0:Bit 0 Set to false
        InitOnlyInput(new_input, urlData_, flags_);
        return;
    }
    if (baseflags.test(4)) { // 4:Bit 4 is true,The baseURL has host
        flags_.set(0, 0); // 0:Bit 0 set to true
        BaseInfoToUrl(baseInfo, baseflags, urlData_, flags_, input.empty());
        if (!input.empty()) {
            if ((input[0] == '/') ||
                (input[0] == '\\' && flags_.test(1))) { // 1:Bit 1 is true,The URL Default for protocol
                strInput = input.substr(1);               
            }
            AnalysisInput(strInput, urlData_, flags_, baseflags);
        }
    } else if (!baseflags.test(4)) { // 4:Bit 4 is false,The URL analysis failed
        flags_.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return;
    }
}

napi_value URL::GetHostname() const
{
    napi_value result;
    const char* temp = nullptr;
    if (flags_.test(4)) { // 4:Bit 4 is true,The URL has hostname
        temp = urlData_.host.c_str();
    } else {
        temp = "";
    }
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetSearch() const
{
    napi_value result;
    const char* temp = nullptr;
    if (flags_.test(7)) { // 7:Bit 7 is true,The URL has Search
        if (urlData_.query.size() == 1) {
            temp = "";
        } else {
            temp = urlData_.query.c_str();
        }
    } else {
        temp = "";
    }
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetUsername() const
{
    napi_value result;
    const char* temp = nullptr;
    if (flags_.test(2)) { // 2:Bit 2 is true,The URL has username
        temp = urlData_.username.c_str();
    } else
        temp = "";
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetPassword() const
{
    napi_value result;
    const char* temp = nullptr;
    if (flags_.test(3)) { // 3:Bit 3 is true,The URL has Password
        temp = urlData_.password.c_str();
    } else {
        temp = "";
    }
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetFragment() const
{
    napi_value result;
    const char* temp = nullptr;
    if (flags_.test(8)) { // 8:Bit 8 is true,The URL has Fragment
        if (urlData_.fragment.size() == 1) {
            temp = "";
        } else {
            temp = urlData_.fragment.c_str();
        }
    } else {
        temp = "";
    }
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetScheme() const
{
    napi_value result;
    const char* temp = nullptr;
    if (!urlData_.scheme.empty()) {
        temp = urlData_.scheme.c_str();
    } else {
        temp = "";
    }
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetPath() const
{
    napi_value result;
    std::string temp1 = "/";
    const char* temp = nullptr;
    if (flags_.test(6)) { // 6:Bit 6 is true,The URL has pathname
        for (size_t i = 0; i < urlData_.path.size(); i++) {
            if (i < urlData_.path.size() - 1) {
                temp1 += urlData_.path[i] + "/";
            } else {
                temp1 += urlData_.path[i];
            }
            temp = temp1.c_str();
        }
    } else {
        bool Special = IsSpecial(urlData_.scheme);
        if(Special) {
            temp = "/";
        } else {
            temp = "";
        }
    }
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetPort() const
{
    napi_value result;
    const char* temp = nullptr;
    if (flags_.test(5)) { // 5:Bit 5 is true,The URL has Port
        temp = std::to_string(urlData_.port).c_str();
    } else {
        temp = "";
    }
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetHost() const
{
    napi_value result;
    const char* temp = nullptr;
    std::string temp1 = urlData_.host;
    if (flags_.test(5)) { // 5:Bit 5 is true,The URL has Port
        temp1 += ":";
        temp1 += std::to_string(urlData_.port);
    }
    temp = temp1.c_str();
    size_t templen = strlen(temp);
    NAPI_CALL(env_, napi_create_string_utf8(env_, temp, templen, &result));
    return result;
}

napi_value URL::GetOnOrOff() const
{
    napi_value result;
    if (flags_.test(0)) { // 1:Bit 1 is true,The URL Parsing succeeded
        bool flag = false;
        NAPI_CALL(env_, napi_get_boolean(env_, flag, &result));
    } else {
        bool flag = true;
        NAPI_CALL(env_, napi_get_boolean(env_, flag, &result));
    }
    return result;
}

napi_value URL::GetIsIpv6() const
{
    napi_value result;
    if (flags_.test(10)) { // 10:Bit 10 is true,The URL is ipv6
        bool flag = true;
        NAPI_CALL(env_, napi_get_boolean(env_, flag, &result));
    } else {
        bool flag = false;
        NAPI_CALL(env_, napi_get_boolean(env_, flag, &result));
    }
    return result;
}

void URL::SetHostname(const std::string& input)
{
    if (flags_.test(9)) { // 9:Bit 9 is true,The URL Can not be base
        return;
    }
    std::string strHost = input;
    for (size_t pos = 0; pos < strHost.size(); pos++) {
        if ((strHost[pos] == ':') || (strHost[pos] == '?') || (strHost[pos] == '#') ||
            (strHost[pos] == '/') || (strHost[pos] == '\\')) {
            strHost = strHost.substr(0, pos);
            break;
        }
    }
    if (strHost.size() == 0) {
        return;
    }
    bool is_Special = IsSpecial(urlData_.scheme);
    std::bitset<11> thisflags_;
    std::string thisHostname = "";
    AnalysisHost(strHost, thisHostname, thisflags_, is_Special);
    if (thisflags_.test(4)) { //4:Bit 4 is true,The URL has hostname
        if ((urlData_.scheme == "file:") && (thisHostname == "localhost")) {
            thisHostname = "";
        }
        urlData_.host = thisHostname;
        flags_.set(4);
    }
}

void URL::SetHref(const std::string& input)
{
    std::string str = input;
    DeleteC0OrSpace(str);
    DeleteTabOrNewline(str);
    url_data thisNewUrl;
    std::bitset<11> thisNewFlags;
    InitOnlyInput(str, thisNewUrl, thisNewFlags);
    if (!thisNewFlags.test(0)) {
        urlData_ = thisNewUrl;
        flags_ = thisNewFlags;
    }
}

void URL::SetPath(const std::string& input)
{
    std::string strPath = input;
    if (flags_.test(9)) {
        return;
    }
    if (strPath.empty()) {
        return;
    }
    std::string oldstr = "%3A";
    std::string newstr = ":";
    ReplaceSpecialSymbols(strPath,oldstr,newstr);
    bool is_Special = IsSpecial(urlData_.scheme);
    if (urlData_.scheme == "file:") {
        url_data thisFileDate;
        std::bitset<11> thisFileFlag;
        if ((strPath[0] == '/') || (strPath[0] == '\\' && flags_.test(1))) {
            strPath = strPath.substr(1);
        }
        AnalysisFilePath(strPath, thisFileDate, thisFileFlag);
        if (thisFileFlag.test(6)) {
            urlData_.path = thisFileDate.path;
            flags_.set(6);
        }
    } else {
        std::vector<std::string> thisPath_;
        std::bitset<11> thisFlags_;
        if ((strPath[0] == '/') || (strPath[0] == '\\' && flags_.test(1))) {
            strPath = strPath.substr(1);
        }
        AnalysisPath(strPath, thisPath_, thisFlags_, is_Special);
        if (thisFlags_.test(6)) {
            urlData_.path = thisPath_;
            flags_.set(6);
        }
    }
}

void URL::SetHost(const std::string& input)
{
    if (flags_.test(9)) {
        return;
    }
    if(input.empty()) {
        return;
    }
    std::string strHost = input;
    std::string port = "";
    size_t strlen = input.size();
    for (size_t pos = 0; pos < strlen; pos++) {
        if ((input[pos] == ':') || (input[pos] == '?') || (input[pos] == '#') ||
            (input[pos] == '/') || (input[pos] == '\\')) {
            strHost = input.substr(0, pos);
            if (input[pos] == ':') {
                pos++;
                port = input.substr(pos);
            }
            break;
        }
    }
    if (strHost.size() == 0) {
        return;
    }
    bool is_Special = IsSpecial(urlData_.scheme);
    std::bitset<11> hostnameflags;
    std::string thisHostname = "";
    AnalysisHost(strHost, thisHostname, hostnameflags, is_Special);
    if (hostnameflags.test(4)) { //4:Bit 4 is true,The URL has hostname
        if ((urlData_.scheme == "file:") && (thisHostname == "localhost")) {
            thisHostname = "";
        }
        urlData_.host = thisHostname;
        flags_.set(4); // 4:Bit 4 Set to true,The URL has hostname
    } else {
        return;
    }
    if (port.size() > 0) {
        strlen = port.size();
        for (size_t pos = 0; pos < strlen; pos++) {
            if ((port[pos] == '?') || (port[pos] == '#') || (port[pos] == '/') || (port[pos] == '\\')) {
                port = port.substr(0, pos);
                break;
            }
        }
        if (port.size() > 0) {
            std::bitset<11> thisflags_;
            url_data thisport;
            AnalysisPort(port, thisport, thisflags_);
            if (thisflags_.test(5)) { // 5:Bit 5 is true,The URL has port
                flags_.set(5); // 5:Bit 5 get to true,The URL has port
                urlData_.port = thisport.port;
            }
        }
    }
}

void URL::SetPort(const std::string& input)
{
    std::string port = input;
    size_t portlen = port.size();
    for (size_t pos = 0; pos < portlen; pos++) {
        if ((port[pos] == '?') || (port[pos] == '#') || (port[pos] == '/') || (port[pos] == '\\')) {
            port = port.substr(0, pos);
            break;
        }
    }
    if (port.size() > 0) {
        std::bitset<11> thisflags_;
        url_data thisport;
        AnalysisPort(port, thisport, thisflags_);
        if (thisflags_.test(5)) { // 5:Bit 5 is true,The URL has port
            flags_.set(5); // 5:Bit 5 get to true,The URL has port
            urlData_.port = thisport.port;
        }
    }
}

void URL::SetSearch(const std::string& input)
{
    std::string temp;
    if (input.size() == 0) {
        urlData_.query = "";
        flags_.set(7, 0); // 7:Bit 7 set to false,The URL not have Search
    } else {
        if (input[0] != '?') {
            temp = "?";
            temp += input;
        } else {
            temp = input;
        }
        std::string oldstr = "#";
        std::string newstr = "%23";
        ReplaceSpecialSymbols(temp,oldstr,newstr);
        AnalysisQuery(temp, urlData_.query, flags_);
    }
}

void URL::SetFragment(const std::string& input)
{
    std::string temp;
    if (input.size() == 0) {
        urlData_.fragment = "";
        flags_.set(8, 0); // 8:Bit 8 set to false,The URL not have Fragment
    } else {
        if (input[0] != '#') {
            temp = "#";
            temp += input;
        } else {
            temp = input;
        }
        AnalysisFragment(temp, urlData_.fragment, flags_);
    }
}

void URL::SetScheme(const std::string& input)
{
    std::string strInput = input;
    bool is_Special = IsSpecial(urlData_.scheme);
    bool inputIsSpecial = IsSpecial(input);
    if ((is_Special != inputIsSpecial) || ((input == "file") && (flags_.test(2) ||
        flags_.test(3) || flags_.test(5)))) { //2 3 5:The URL has  username password host
        return;
    }
    std::string thisScheme = "";
    std::bitset<11> thisFlags;
    if (AnalysisScheme(strInput, thisScheme, thisFlags)) {
        if (thisFlags.test(1)) { // 1:Bit 1 is true,The inputURL Default for protocol
            flags_.set(1); // 1:Bit 1 set to true,The URL Default for protocol
        }
        urlData_.scheme = thisScheme;
    }
}

void URL::SetUsername(const std::string& input)
{
    if (input.size() == 0) {
        urlData_.username = "";
        flags_.set(2, 0); // 2:Bit 2 set to false,The URL not have username
    } else {
        if (!input.empty()) {
            std::string usname = input; 
            std::string oldstr = "@";
            std::string newstr = "%40";
            ReplaceSpecialSymbols(usname,oldstr,newstr);
            oldstr = "/";
            newstr = "%2F";
            ReplaceSpecialSymbols(usname,oldstr,newstr);
            urlData_.username = usname;
            flags_.set(2); // 2:Bit 2 set to true,The URL  have username
        }
    }
}

void URL::SetPassword(const std::string& input)
{
    if (input.size() == 0) {
        urlData_.password = "";
        flags_.set(3, 0); // 3:Bit 3 set to false,The URL not have Password
    } else {
        if (!input.empty()) {
            std::string passwd = input;
            std::string oldstr = "@";
            std::string newstr = "%40";
            ReplaceSpecialSymbols(passwd,oldstr,newstr);
            oldstr = "/";
            newstr = "%2F";
            ReplaceSpecialSymbols(passwd,oldstr,newstr);
            urlData_.password = passwd;
            flags_.set(3); // 3:Bit 3 set to true,The URL  have passwd
        }
    }
}

void URL::InitOnlyInput(std::string& input, url_data& urlData, std::bitset<11>& flags)
{
    if (input.empty()) {
        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return;
    }
    if (input.find(':') != std::string::npos) {
        size_t pos = input.find(':');
        pos++;
        std::string scheme_ = input.substr(0, pos);
        if (!AnalysisScheme(scheme_, urlData.scheme, flags)) {
            return;
        }
    
        if (input.find('#') != std::string::npos) {
            size_t i = input.find('#');
            std::string fragment = input.substr(i);
            AnalysisFragment(fragment, urlData.fragment, flags);
            input = input.substr(0, i);

        }
        if (input.find('?') != std::string::npos) {
            size_t i = input.find('?');
            std::string query = input.substr(i);
            AnalysisQuery(query, urlData.query, flags);

            input = input.substr(0, i);
        }

        std::string str = input.substr(pos);
        if (urlData.scheme == "file:") {
            AnalysisFile(str, urlData, flags);
        } else {
            AnalysisHostAndPath(str, urlData, flags);
        }
    } else {
        flags.set(0); // 0:Bit 0 Set to true,The URL analysis failed
        return;
    }
}

URLSearchParams::URLSearchParams(napi_env env) : env(env)
{
}
std::wstring strToWstr(const std::string& str)
{
    setlocale(LC_ALL, "");
    const char* addressofS = str.c_str();
    size_t len = str.size() + 1;
    wchar_t* wch = new wchar_t[len];
    wmemset(wch, 0, len);
    mbstowcs(wch, addressofS, len);
    std::wstring wstr = wch;
    delete[]wch;
    setlocale(LC_ALL, "");
    return wstr;
}
std::string ReviseStr(std::string str, std::string* reviseChar)
{
    const size_t len_Str = str.length();
    if (len_Str == 0) {
        return "";
    }
    std::string output = "";
    int numOfAscii = 128; // 128:Number of ASCII characters
    size_t i = 0;
    for (; i < len_Str; i++)
    {
        int charaEncode = static_cast<int>(str[i]);
        if (charaEncode < 0 || charaEncode >= numOfAscii) {
            int bytOfSpeChar = 3; // 3:Bytes of special characters in Linux
            std::string subStr = str.substr(i, bytOfSpeChar);
            i += 2;
            const char* firstAddr = subStr.c_str();
            std::wstring wstr = strToWstr(firstAddr);
            wchar_t wch = wstr[0];
            charaEncode = static_cast<int>(wch);
        }  
        if (0 <= charaEncode && charaEncode < numOfAscii) {
            //2:Defines the escape range of ASCII characters
            if ((0 < charaEncode && charaEncode < '*') ||
                ('*' < charaEncode && charaEncode < '-') ||
                (charaEncode == '/') ||
                ('9' < charaEncode && charaEncode < 'A') ||
                ('Z' < charaEncode && charaEncode < '_') ||
                (charaEncode == '`') ||
                (charaEncode > 'z')) {
                output += reviseChar[charaEncode];
            } else {
                output += str.substr(i, 1);
            }
        } else if (charaEncode <= 0x000007FF) { // Convert the Unicode code into two bytes
            std::string output1 = reviseChar[0x000000C0 |
                (charaEncode / 64)]; // Acquisition method of the first byte
            std::string output2 = reviseChar[numOfAscii |
                (charaEncode & 0x0000003F)]; // Acquisition method of the second byte
            output += output1 + output2;
        } else if ((charaEncode < 0x0000D800) ||
                (charaEncode >= 0x0000E000)) { // Convert the Unicode code into three bytes
            std::string output1 = reviseChar[0x000000E0 |
                (charaEncode / 4096)]; // Acquisition method of the first byte
            std::string output2 = reviseChar[numOfAscii |
                ((charaEncode / 64) & 0x0000003F)]; // method of the second byte
            std::string output3 = reviseChar[numOfAscii |
                (charaEncode & 0x0000003F)]; // Acquisition method of the third  byte
            output += output1 + output2 + output3;
        } else {
            // 1023:Convert codes greater than 0x000e000 into 4 bytes
            const size_t charaEncode1 = static_cast<size_t>(str[++i]) & 1023; 
            charaEncode = 65536 + (((charaEncode & 1023) << 10) |
            charaEncode1); // Specific transcoding method
            std::string output1 = reviseChar[0x000000F0 |
                (charaEncode / 262144)]; // Acquisition method of the first byte
            std::string output2 = reviseChar[numOfAscii |
                ((charaEncode / 4096) & 0x0000003F)]; // Acquisition method of the second byte
            std::string output3 = reviseChar[numOfAscii |
                ((charaEncode / 64) & 0x0000003F)]; // Acquisition method of the third  byte
            std::string output4 = reviseChar[numOfAscii |
                (charaEncode & 0x0000003F)]; // Acquisition method of the fourth   byte
            output += output1 + output2 + output3 + output4;
        }
    }
    return output;
}

napi_value URLSearchParams::ToString()
{
    std::string output = "";
    std::string reviseChar[256];
    for (size_t i = 0; i < 256; ++i) {
        std::stringstream ioss;
        std::string str1;
        size_t j = i;
        ioss << std::hex << j;
        ioss >> str1;
        transform(str1.begin(), str1.end(), str1.begin(), ::toupper);
        if (i < 16) { // 16:Total number of 0-F
            reviseChar[i] = '%' + ("0" + str1);
        } else {
            reviseChar[i] = '%' + str1;
        }
    }
    reviseChar[0x20] = "+"; // 0x20:ASCII value of spaces
    const size_t len_Str = searchParams.size();
    if (len_Str == 0) {
        napi_value result = nullptr;
        napi_create_string_utf8(env, output.c_str(), output.size(), &result);
        return result;
    }
    std::string first_StrKey = ReviseStr(searchParams[0], reviseChar);
    std::string first_StrValue = ReviseStr(searchParams[1], reviseChar);
    output = first_StrKey + "=" + first_StrValue;
    if (len_Str % 2 == 0) {
        size_t couple = 2;
        size_t i = 2;
        for (; i < len_Str; i += couple) {
            std::string strKey = ReviseStr(searchParams[i], reviseChar);
            std::string strValue = ReviseStr(searchParams[i + 1], reviseChar);
            output += +"&" + strKey + "=" + strValue;
        }
    }
    napi_value result = nullptr;
    napi_create_string_utf8(env, output.c_str(), output.size(), &result);
    return result;
}
void URLSearchParams::HandleIllegalChar(std::wstring& inputStr, std::wstring::const_iterator it)
{
    std::wstring::iterator iter = inputStr.begin();
    advance(iter, std::distance<std::wstring::const_iterator>(iter, it));
    while (iter != inputStr.end()) {
        char16_t ch = *iter;
        if (!((ch & 0xF800) == 0xD800)) {
            ++iter;
            continue;
        } else if ((ch & 0x400) != 0 || iter == inputStr.end() - 1) {
            *iter = 0xFFFD;
        } else {
            char16_t dh = *(iter + 1);
            if ((dh & 0xFC00) == 0xDC00) {
                ++iter;
            } else {
                *iter = 0xFFFD;
            }
        }
        ++iter;
    }
}
std::string URLSearchParams::ToUSVString(std::string inputStr)
{
    size_t strLen = strlen(inputStr.c_str());
    wchar_t* strPtr = nullptr;
    int strSize = mbstowcs(strPtr, inputStr.c_str(), 0) + 1;
    strPtr = new wchar_t[strSize];
    wmemset(strPtr, 0, strSize);
    mbstowcs(strPtr, inputStr.c_str(), strLen);

    const char* expr = "(?:[^\\uD800-\\uDBFF]|^)[\\uDC00-\\uDFFF]|[\\uD800-\\uDBFF](?![\\uDC00-\\uDFFF])";
    size_t exprLen = strlen(expr);
    wchar_t* exprPtr = nullptr;
    int exprSize = mbstowcs(exprPtr, expr, 0) + 1;
    exprPtr = new wchar_t[exprSize];
    wmemset(exprPtr, 0, exprSize);
    mbstowcs(exprPtr, expr, exprLen);

    std::wregex wexpr(exprPtr);
    delete[] exprPtr;
    std::wsmatch result;
    std::wstring winput = strPtr;
    delete[] strPtr;
    std::wstring::const_iterator iterStart = winput.begin();
    std::wstring::const_iterator iterEnd = winput.end();
    if (!regex_search(iterStart, iterEnd, result, wexpr)) {
        return inputStr;
    }
    HandleIllegalChar(winput, result[0].first);
    size_t inputLen = wcslen(winput.c_str());
    char* rePtr = nullptr;
    int reSize = wcstombs(rePtr, winput.c_str(), 0) + 1;
    rePtr = new char[reSize];
    memset(rePtr, 0, reSize);
    wcstombs(rePtr, winput.c_str(), inputLen);
    std::string reStr = rePtr;
    delete[] rePtr;
    return reStr;
}
napi_value URLSearchParams::Get(napi_value buffer)
{
    char* name = nullptr;
    size_t nameSize = 0;
    napi_get_value_string_utf8(env, buffer, nullptr, 0, &nameSize);
    name = new char[nameSize + 1];
    napi_get_value_string_utf8(env, buffer, name, nameSize + 1, &nameSize);

    std::string sname = ToUSVString(name);
    delete[] name;
    napi_value result = nullptr;
    if(searchParams.size() == 0){
        return result;  
    }
    size_t size = searchParams.size() - 1;
    for (size_t i = 0; i < size; i += 2) {
        if (searchParams[i] == sname) {
            std::string str = searchParams[i + 1];
            napi_create_string_utf8(env, searchParams[i + 1].c_str(), searchParams[i + 1].length(), &result);
            return result;
        }
    }
    return result;
}
napi_value URLSearchParams::GetAll(napi_value buffer)
{
    char* name = nullptr;
    size_t nameSize = 0;
    napi_get_value_string_utf8(env, buffer, nullptr, 0, &nameSize);
    name = new char[nameSize + 1];
    napi_get_value_string_utf8(env, buffer, name, nameSize + 1, &nameSize);

    std::string sname = ToUSVString(name);
    delete[] name;
    napi_value result = nullptr;
    napi_value napiStr = nullptr;
    NAPI_CALL(env, napi_create_array(env, &result));
    size_t flag = 0;
    if(searchParams.size() == 0){
        return result;  
    }
    size_t size = searchParams.size() -1 ;
    for (size_t i = 0; i < size; i += 2) {
        if (searchParams[i] == sname) {
            napi_create_string_utf8(env, searchParams[i + 1].c_str(), searchParams[i + 1].length(), &napiStr);
            napi_status status = napi_set_element(env, result, flag, napiStr);
            if (status != napi_ok) {
                HILOG_INFO("set element error");
            }
            flag++;
        }
    }
    return result;
}
void URLSearchParams::Append(napi_value buffer, napi_value temp)
{
    char* name = nullptr;
    size_t nameSize = 0;
    napi_get_value_string_utf8(env, buffer, nullptr, 0, &nameSize);
    name = new char[nameSize + 1];
    napi_get_value_string_utf8(env, buffer, name, nameSize + 1, &nameSize);

    char* value = nullptr;
    size_t valueSize = 0;
    napi_get_value_string_utf8(env, temp, nullptr, 0, &valueSize);
    value = new char[valueSize + 1];
    napi_get_value_string_utf8(env, temp, value, valueSize + 1, &valueSize);
    searchParams.push_back(name);
    searchParams.push_back(value);
    delete[] name;
    delete[] value;
}
void URLSearchParams::Delete(napi_value buffer)
{
    char* name = nullptr;
    size_t nameSize = 0;
    napi_get_value_string_utf8(env, buffer, nullptr, 0, &nameSize);
    name = new char[nameSize + 1];
    napi_get_value_string_utf8(env, buffer, name, nameSize + 1, &nameSize);
    std::string sname = ToUSVString(name);
    delete[] name;
    for (std::vector<std::string>::iterator iter = searchParams.begin(); iter != searchParams.end();) {
        if (*iter == sname) {
            iter = searchParams.erase(iter, iter + 2);
        } else {
            iter += 2;
        }
    }
}
napi_value URLSearchParams::Entries()
{
    napi_value resend = nullptr;
    napi_value firNapiStr = nullptr;
    napi_value secNapiStr = nullptr;
    napi_create_array(env, &resend);
    if(searchParams.size() == 0) {
        return resend;  
    }
    size_t size = searchParams.size() - 1;
    for (size_t i = 0; i < size; i += 2) {
        napi_value result = nullptr;
        napi_create_array(env, &result);

        napi_create_string_utf8(env, searchParams[i].c_str(), searchParams[i].length(), &firNapiStr);
        napi_create_string_utf8(env, searchParams[i + 1].c_str(), searchParams[i + 1].length(), &secNapiStr);
        napi_set_element(env, result, 0, firNapiStr);
        napi_set_element(env, result, 1, secNapiStr);
        napi_set_element(env, resend, i / 2, result);
    }
    return resend;
}
void URLSearchParams::ForEach(napi_value function, napi_value thisVar)
{
    if(searchParams.size() == 0){
        return ;  
    }
    size_t size = searchParams.size() - 1;
    for (size_t i = 0; i < size; i += 2) {
        napi_value return_val = nullptr;
        size_t argc = 3;
        napi_value global = nullptr;
        napi_get_global(env, &global);

        napi_value key = nullptr;
        napi_create_string_utf8(env, searchParams[i].c_str(), strlen(searchParams[i].c_str()), &key);
        napi_value value = nullptr;
        napi_create_string_utf8(env, searchParams[i + 1].c_str(), strlen(searchParams[i + 1].c_str()), &value);

        napi_value argv[3] = {key, value, thisVar};
        napi_call_function(env, global, function, argc, argv, &return_val);
    }
}
napi_value URLSearchParams::IsHas(napi_value name)
{
    char* buffer = nullptr;
    size_t bufferSize = 0;
    napi_get_value_string_utf8(env, name, nullptr, 0, &bufferSize);
    buffer = new char[bufferSize + 1];
    napi_get_value_string_utf8(env, name, buffer, bufferSize + 1, &bufferSize);
    bool flag = false;
    napi_value result;
    int lenStr = searchParams.size();
    int couple = 2;
    for (size_t i = 0; i != lenStr; i += couple) {
        std::string b = buffer;
        if (searchParams[i] == b) {
            flag = true;
            napi_get_boolean(env, flag, &result);
            return result;
        }
    }
    delete []buffer;
    napi_get_boolean(env, flag, &result);
    return result;
}
void URLSearchParams::Set(napi_value name, napi_value value)
{
    char* buffer0 = nullptr;
    size_t bufferSize0 = 0;
    napi_get_value_string_utf8(env, name, nullptr, 0, &bufferSize0);
    buffer0 = new char[bufferSize0 + 1];
    napi_get_value_string_utf8(env, name, buffer0, bufferSize0 + 1, &bufferSize0);
    char* buffer1 = nullptr;
    size_t bufferSize1 = 0;
    napi_get_value_string_utf8(env, value, nullptr, 0, &bufferSize1);
    buffer1 = new char[bufferSize1 + 1];
    napi_get_value_string_utf8(env, value, buffer1, bufferSize1 + 1, &bufferSize1);
    bool flag = false;
    std::string cpp_name = buffer0;
    std::string cpp_value = buffer1;
    delete[] buffer0;
    delete[] buffer1;
    for (std::vector<std::string>::iterator it = searchParams.begin(); it < searchParams.end() - 1;) {
        if (*it == cpp_name) {
            if (!flag) {
                *(it + 1) = cpp_value;
                flag = true;
                it += 2;
            } else {
                it = searchParams.erase(it, it + 2);
            }
        } else {
            it += 2;
        }
    }
    if (!flag) {
        searchParams.push_back(cpp_name);
        searchParams.push_back(cpp_value);
    }
}
void SortMerge(std::vector<std::string> out, unsigned int start, int mid, int end, 
                std::vector<std::string> lBuffer, std::vector<std::string> rBuffer)
{
    const unsigned int flag1 = mid - start;
    const unsigned int flag2 = end - mid;
    unsigned int flag3 = 0;
    unsigned int flag4 = 0;
    unsigned int flag5 = 0;
    for (flag3 = 0; flag3 < flag1; flag3++) {
        lBuffer[flag3] = out[start + flag3];
    }   
    for (flag4 = 0; flag4 < flag2; flag4++) {
        rBuffer[flag4] = out[mid + flag4];
    } 
    flag3 = 0;
    flag4 = 0;
    flag5 = start;
    while (flag3 < flag1 && flag4 < flag2) {
        if (lBuffer[flag3] <= rBuffer[flag3]) {
            out[flag5++] = lBuffer[flag3++];
            out[flag5++] = lBuffer[flag3++];
        } else {
            out[flag5++] = rBuffer[flag4++];
            out[flag5++] = rBuffer[flag4++];
        }
    }
    while (flag3 < flag1) {
        out[flag5++] = lBuffer[flag3++];
    }   
    while (flag4 < flag2) {
        out[flag5++] = rBuffer[flag4++];
    }
}

void URLSearchParams::Sort()
{
    unsigned int len = searchParams.size();
    if (2 < len && len < 100 && (len % 2 == 0)) {
        unsigned int i = 0;
        for (; i < len - 2; i += 2) {
            unsigned int  j = i + 2;
            for (; j < len; j += 2) {
                if (searchParams[i] > searchParams[j]) {
                    const std::string curKey = searchParams[i];
                    const std::string curVal = searchParams[i + 1];
                    searchParams[i] = searchParams[j];
                    searchParams[i + 1] = searchParams[j + 1];
                    searchParams[j] = curKey;
                    searchParams[j + 1] = curVal;
                }
            }
        }
    } if (len >= 100) {
        const std::vector<std::string> Buffer1;
        const std::vector<std::string> Buffer2;
        for (unsigned int i = 2; i < searchParams.size(); i *= 2) {
            for (unsigned int j = 0; j < searchParams.size() - 2; j += 2 * i) {
                const unsigned int TempMid = j + i;
                unsigned int TempEnd = TempMid + i;
                TempEnd = TempEnd < len ? TempEnd : len;
                if (TempMid > TempEnd) {
                    continue;
                } 
                SortMerge(searchParams, j, TempMid, TempEnd, Buffer1, Buffer2);
            }
        }
    }
}
napi_value URLSearchParams::IterByKeys()
{
    std::vector<std::string> toKeys;
    napi_value result = nullptr;
    napi_value napiStr;
    napi_create_array(env, &result);
    int lenStr = searchParams.size();
    if (lenStr % 2 == 0) {
        int couple = 2;
        for (std::vector<std::string>::iterator it = searchParams.begin(); it != searchParams.end(); it += couple) {
            toKeys.push_back(*it);
        }
        int lenToKeys = toKeys.size();
        for (size_t i = 0; i < lenToKeys; i++) {
            napi_create_string_utf8(env, toKeys[i].c_str(), toKeys[i].length(), &napiStr);
            napi_set_element(env, result, i, napiStr);
        }
    }
    return result;
}
napi_value URLSearchParams::IterByValues()
{
    std::vector<std::string> toKeys;
    napi_value result = nullptr;
    napi_value napiStr;
    napi_create_array(env, &result);
    int lenStr = searchParams.size();
    if (lenStr % 2 == 0) {
        int couple = 2;
        for (std::vector<std::string>::iterator it = searchParams.begin(); it != searchParams.end(); it += couple) {
            toKeys.push_back(*(it + 1));
        }
        int lenToKeys = toKeys.size();
        for (size_t i = 0; i < lenToKeys; i++) {
            napi_create_string_utf8(env, toKeys[i].c_str(), toKeys[i].length(), &napiStr);
            napi_set_element(env, result, i, napiStr);
        }
    }
    return result;
}
void URLSearchParams::SetArray(const std::vector<std::string> vec)
{
    searchParams = vec;
}
napi_value URLSearchParams::GetArray()
{
    napi_value arr = nullptr;
    napi_create_array(env, &arr);
    size_t length = searchParams.size();
    for (size_t i = 0; i < length; i++) {
        napi_value result = nullptr;
        napi_create_string_utf8(env, searchParams[i].c_str(), searchParams[i].size(), &result);
        napi_set_element(env, arr, i, result);
    }
    return arr;
}