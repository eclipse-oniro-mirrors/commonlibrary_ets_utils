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
#ifndef FOUNDATION_ACE_CCRUNTIME_CONVERT_XML_CLASS_H
#define FOUNDATION_ACE_CCRUNTIME_CONVERT_XML_CLASS_H

#include <string>
#include <vector>
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS::Xml {
    enum class SpaceType {
        T_INT32,
        T_STRING,
        T_INIT = -1
    };

struct Options {
    std::string declaration = "_declaration";
    std::string instruction = "_instruction";
    std::string attributes = "_attributes";
    std::string text = "_text";
    std::string cdata = "_cdata";
    std::string doctype = "_doctype";
    std::string comment = "_comment";
    std::string parent = "_parent";
    std::string type = "_type";
    std::string name = "_name";
    std::string elements = "_elements";
    bool compact = false;
    bool trim = false;
    bool nativetype = false;
    bool nativetypeattributes = false;
    bool addparent = false;
    bool alwaysArray = false;
    bool alwaysChildren = false;
    bool instructionHasAttributes = false;
    bool ignoreDeclaration = false;
    bool ignoreInstruction = false;
    bool ignoreAttributes = false;
    bool ignoreComment = false;
    bool ignoreCdata = false;
    bool ignoreDoctype = false;
    bool ignoreText = false;
    bool spaces = false;
};

struct XmlInfo {
    bool bXml = false;
    bool bVersion = false;
    std::string strVersion = "";
    bool bEncoding = false;
    std::string strEncoding = "";
};

    class ConvertXml {
    public:
            explicit ConvertXml(napi_env env);
            virtual ~ConvertXml() {}
            void SetAttributes(xmlNodePtr curNode, const napi_value &elementsObject) const;
            void SetXmlElementType(xmlNodePtr curNode, const napi_value &elementsObject, bool &bFlag) const;
            void SetNodeInfo(xmlNodePtr curNode, const napi_value &elementsObject) const;
            void SetEndInfo(xmlNodePtr curNode, const napi_value &elementsObject, bool &bFlag) const;
            void GetXMLInfo(xmlNodePtr curNode, const napi_value &object, int flag = 0);
            napi_value Convert(std::string strXml);
            std::string GetNodeType(const xmlElementType enumType) const;
            napi_status DealNapiStrValue(const napi_value napi_StrValue, std::string &result) const;
            void SetKeyValue(const napi_value &object, const std::string strKey, const std::string strValue) const;
            void DealOptions(const napi_value napiObj);
            std::string Trim(std::string strXmltrim) const;
            void GetPrevNodeList(xmlNodePtr curNode);
            void DealSpaces(const napi_value napiObj);
            void DealIgnore(const napi_value napiObj);
            void SetPrevInfo(const napi_value &recvElement, int flag, int32_t &index1) const;
            void SetDefaultKey(size_t i, const std::string strRecv);
            void SetSpacesInfo(const napi_value &object) const;
            void DealSingleLine(std::string &strXml, const napi_value &object);
            void DealComplex(std::string &strXml, const napi_value &object) const;
            void Replace(std::string &str, const std::string src, const std::string dst) const;
            void DealCDataInfo(bool bCData, xmlNodePtr &curNode) const;
    private:
            napi_env env_;
            SpaceType spaceType_;
            int32_t iSpace_;
            std::string strSpace_;
            Options options_;
            std::vector<napi_value> prevObj_;
            XmlInfo xmlInfo_;
    };
} // namespace
#endif