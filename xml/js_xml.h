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
#ifndef FOUNDATION_ACE_CCRUNTIME_XML_CLASS_H
#define FOUNDATION_ACE_CCRUNTIME_XML_CLASS_H

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "utils/log.h"

namespace OHOS::xml {
    class XmlSerializer {
    public:
        XmlSerializer(char *pStart, size_t bufferLength, std::string encoding = "utf-8") :pStart_(pStart),
            iLength_(bufferLength), encoding_(encoding){};
        ~XmlSerializer(){}
        void SetAttributes(std::string name, std::string value);
        void AddEmptyElement(std::string name);
        void SetDeclaration();
        void StartElement(std::string name);
        void EndElement();
        void SetNamespace(std::string prefix, std::string nsTemp);
        void SetCommnet(std::string comment);
        void SetCData(std::string data);
        void SetText(std::string text);
        void SetDocType(std::string text);
        void WriteEscaped(std::string s);
        void SplicNsp();
        void NextItem();
        std::string XmlSerializerError();
        static napi_status DealNapiStrValue(napi_env env, const napi_value napiStr, std::string &result);
    private:
        char *pStart_;
        size_t iPos_ = 0;
        size_t iLength_;
        std::string xmlSerializerError_;
        std::string encoding_;
        size_t depth_ = 0;
        std::string type;
        std::vector<std::string> elementStack = { "", "", ""};
        std::map<int, std::map<int, std::string>> multNsp;
        int CurNspNum = 0;
        std::string out_;
        bool isHasDecl = false;
    };

    enum class TagEnum {
        XML_DECLARATION = -1,
        START_DOCUMENT,
        END_DOCUMENT,
        START_TAG,
        END_TAG,
        TEXT,
        CDSECT,
        COMMENT,
        DOCDECL,
        INSTRUCTION,
        ENTITY_REFERENCE,
        WHITESPACE,
        ELEMENTDECL,
        ENTITYDECL,
        ATTLISTDECL,
        NOTATIONDECL,
        PARAMETER_ENTITY_REF,
        OK,
        ERROR1
    };

    enum class TextEnum {
        ATTRI,
        TEXT,
        ENTITY_DECL
    };
    class XmlPullParser {
    public:
        class ParseInfo {
        public:
            static napi_value GetDepth(napi_env env, napi_callback_info info);
            static napi_value GetColumnNumber(napi_env env, napi_callback_info info);
            static napi_value GetLineNumber(napi_env env, napi_callback_info info);
            static napi_value GetAttributeCount(napi_env env, napi_callback_info info);
            static napi_value GetName(napi_env env, napi_callback_info info);
            static napi_value GetNamespace(napi_env env, napi_callback_info info);
            static napi_value GetPrefix(napi_env env, napi_callback_info info);
            static napi_value GetText(napi_env env, napi_callback_info info);
            static napi_value IsEmptyElementTag(napi_env env, napi_callback_info info);
            static napi_value IsWhitespace(napi_env env, napi_callback_info info);
        };
        struct TagText {
            const std::string START_CDATA = "<![CDATA[";
            const std::string END_CDATA = "]]>";
            const std::string START_COMMENT = "<!--";
            const std::string END_COMMENT = "-->";
            const std::string COMMENT_DOUBLE_DASH = "--";
            const std::string END_PROCESSING_INSTRUCTION = "?>";
            const std::string START_DOCTYPE = "<!DOCTYPE";
            const std::string SYSTEM = "SYSTEM";
            const std::string PUBLIC = "PUBLIC";
            const std::string DOUBLE_QUOTE = "\"";
            const std::string SINGLE_QUOTE = "\\";
            const std::string START_ELEMENT = "<!ELEMENT";
            const std::string EMPTY = "EMPTY";
            const std::string ANY = "ANY";
            const std::string START_ATTLIST = "<!ATTLIST";
            const std::string NOTATION = "NOTATION";
            const std::string REQUIRED = "REQUIRED";
            const std::string IMPLIED = "IMPLIED";
            const std::string FIXED = "FIXED";
            const std::string START_ENTITY = "<!ENTITY";
            const std::string NDATA = "NDATA";
            const std::string START_NOTATION = "<!NOTATION";
            const std::string ILLEGAL_TYPE = "Wrong event type";
            const std::string START_PROCESSING_INSTRUCTION = "<?";
            const std::string XML = "xml ";
        };
        struct SrcLinkList {
            SrcLinkList* next;
            std::string strBuffer;
            int position;
            int max;
            SrcLinkList()
            {
                this->next = nullptr;
                this->strBuffer = "";
                this->position = -1;
                this->max = -1;
            };
            SrcLinkList(SrcLinkList* pNext, std::string strTemp, int iPos, int iMax) :next(pNext),
                strBuffer(strTemp), position(iPos), max(iMax){}
        };
        XmlPullParser(napi_env env, std::string strXml, std::string encoding) : env_(env),
            strXml_(strXml), encoding_(encoding) {};
        ~XmlPullParser()
        {
            while (srcLinkList_) {
                PopSrcLinkList();
            }
        };
        int GetDepth() const;
        int GetColumnNumber() const;
        int GetLineNumber() const;
        int GetAttributeCount() const;
        std::string GetName() const;
        std::string GetNamespace() const;
        std::string GetPrefix() const;
        std::string GetText() const;
        bool IsEmptyElementTag() const;
        bool IsWhitespace() const;
        void PushSrcLinkList(std::string strBuffer);
        void PopSrcLinkList();
        bool DealLength(size_t minimun);
        void Replace(std::string &strTemp, std::string strSrc, std::string strDes) const;
        size_t GetNSCount(size_t iTemp);
        void Parse(napi_value thisVar);
        std::string GetNamespace(std::string prefix);
        napi_value DealOptionInfo(napi_value napiObj, napi_callback_info info);
        TagEnum ParseTagType(bool inDeclaration);
        void SkipText(std::string chars);
        int PriorDealChar();
        void SkipChar(char expected);
        std::string ParseNameInner(int start);
        std::string ParseName();
        void SkipInvalidChar();
        void ParseEntity(std::string& out, bool isEntityToken, bool throwOnResolveFailure, TextEnum textEnum);
        std::string ParseTagValue(char delimiter, bool resolveEntities, bool throwOnResolveFailure, TextEnum textEnum);
        bool ParseNsp();
        void ParseStartTag(bool xmldecl, bool throwOnResolveFailure);
        void ParseDeclaration();
        void ParseEndTag();
        std::string ParseDelimiterInfo(std::string delimiter, bool returnText);
        std::string ParseDelimiter(bool returnText);
        bool ParserDoctInnerInfo(bool requireSystemName, bool assignFields);
        void ParseComment(bool returnText);
        void ParseSpecText();
        void ParseInnerEleDec();
        void ParseInnerAttriDecl();
        void ParseEntityDecl();
        void ParseInneNotaDecl();
        void ReadInternalSubset();
        void ParseDoctype(bool saveDtdText);
        TagEnum ParseOneTag();
        void ParserPriorDeal();
        void ParseInstruction();
        void ParseText();
        void ParseCdect();
        std::string XmlPullParserError() const;
        bool ParseAttri(napi_value thisVar) const;
        bool ParseToken(napi_value thisVar) const;
        void ParseNspFunction();
        void ParseNspFunc(size_t &i, std::string &attrName, bool &any);
        void ParseInnerAttriDeclFunc(int &c);
        TagEnum DealExclamationGroup();
        void ParseEntityFunc(int start, std::string &out, bool isEntityToken, TextEnum textEnum);
        bool ParseStartTagFuncDeal(bool throwOnResolveFailure);
        bool ParseStartTagFunc(bool xmldecl, bool throwOnResolveFailure);
        TagEnum ParseOneTagFunc();
        int ParseTagValueInner(size_t &start, std::string &result, char delimiter, TextEnum textEnum, bool bFlag);
        bool ParseTagValueFunc(char c, bool bFlag, TextEnum textEnum, size_t &start, std::string &result);
        void MakeStrUpper(std::string &src) const;
        TagEnum DealLtGroup();
        void DealWhiteSpace(char c);
    private:
        napi_env env_;
        bool bDoctype_ = false;
        bool bIgnoreNS_ = false;
        bool bStartDoc_ = true;
        napi_value tagFunc_ = nullptr;
        napi_value attrFunc_ = nullptr;
        napi_value tokenFunc_ = nullptr;
        TagText tagText_;
        std::string strXml_ = "";
        std::string version_ = "";
        std::string encoding_ = "";
        std::string prefix_ = "";
        std::string namespace_ = "";
        std::string name_ = "";
        std::string text_ = "";
        std::string sysInfo_ = "";
        std::string pubInfo_ = "";
        std::string keyInfo_ = "";
        std::string xmlPullParserError_ = "";
        std::vector<size_t> nspCounts_;
        std::vector<std::string> nspStack_;
        std::vector<std::string> elementStack_;
        std::vector<std::string> attributes;
        std::map<std::string, std::string> documentEntities;
        std::map<std::string, std::map<std::string, std::string>> defaultAttributes;
        std::map<std::string, std::string> DEFAULT_ENTITIES = {
            {"lt;", "<"}, {"gt;", ">"}, {"amp;", "&"}, {"apos;", "'"}, {"quot;", "\""}
        };
        size_t position_ = 0;
        size_t depth = 0;
        size_t max_ = 0;
        size_t bufferStartLine_ = 0;
        size_t bufferStartColumn_ = 0;
        size_t attriCount_ = 0;
        TagEnum type = TagEnum::START_DOCUMENT;
        bool bWhitespace_ = false;
        SrcLinkList* srcLinkList_ = new SrcLinkList;
        bool bEndFlag_ = false;
        bool bAlone_ = false;
        bool bUnresolved_ = false;
        bool relaxed = false;
        bool bKeepNsAttri = false;
        bool bDocDecl = false;
    };
} // namespace
#endif /* FOUNDATION_ACE_CCRUNTIME_XML_CLASS_H */