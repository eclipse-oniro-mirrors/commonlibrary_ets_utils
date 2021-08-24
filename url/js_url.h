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
#include <string>
#include <vector>
#include <iostream>
#include <bitset>
#include <regex>
#include <ctype.h>
#include <algorithm>
#include <map>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <sstream>
#include "napi/native_api.h"
#include "napi/native_node_api.h"

struct url_data {
    int port = -1;
    std::string scheme;
    std::string username;
    std::string password;
    std::string host;
    std::string query;
    std::string fragment;
    std::vector<std::string> path;
};

class URL
{
public:
    URL(napi_env env, const std::string& input);
    URL(napi_env env, const std::string& input, const std::string& base);
    URL(napi_env env, const std::string& input, const URL& base);

    napi_value GetHostname() const;
    void SetHostname(const std::string& input);
    void SetUsername(const std::string& input);
    void SetPassword(const std::string& input);
    void SetScheme(const std::string& input);
    void SetFragment(const std::string& input);
    void SetSearch(const std::string& input);
    void SetHost(const std::string& input);
    void SetPort(const std::string& input);
    void SetHref(const std::string& input);
    void SetPath(const std::string& input);

    napi_value GetSearch() const;
    napi_value GetUsername() const;
    napi_value GetPassword() const;
    napi_value GetFragment() const;
    napi_value GetScheme() const;
    napi_value GetPath() const;
    napi_value GetPort() const;
    napi_value GetOnOrOff() const;
    napi_value GetIsIpv6() const;
    napi_value GetHost() const;

    static void InitOnlyInput(std::string& input, url_data& urlData, std::bitset<11>& flags);
    virtual ~URL() {}
private:
    url_data urlData_;
    std::bitset<11> flags_;
    // bitset<11>:Similar to bool array, each bit status represents the real-time status of current URL parsing
    napi_env env_ = nullptr;
};

class URLSearchParams {
public:
    URLSearchParams(napi_env env);
    virtual ~URLSearchParams() {}
    napi_value IsHas(napi_value  name);
    napi_value Get(napi_value buffer);
    napi_value GetAll(napi_value buffer);
    void Append(napi_value buffer, napi_value temp);
    void Delete(napi_value buffer);
    void ForEach(napi_value function, napi_value thisVar);
    napi_value Entries();
    void Set(napi_value name, napi_value value);
    void Sort();
    napi_value ToString();
    napi_value IterByKeys();
    napi_value IterByValues();
    void SetArray(std::vector<std::string> input);
    napi_value GetArray();
    std::vector<std::string> StringParmas (std::string Stringpar);
private:
    std::string ToUSVString(std::string inputStr);
    void HandleIllegalChar(std::wstring& inputStr, std::wstring::const_iterator it);
    std::vector<std::string> searchParams;
    napi_env env;
};