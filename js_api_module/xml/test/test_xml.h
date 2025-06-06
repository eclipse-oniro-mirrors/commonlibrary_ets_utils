/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef TEST_XML_H
#define TEST_XML_H

#include "js_api_module/xml/js_xml.h"

namespace OHOS::xml {
class XmlTest {
public:
    XmlTest() = default;
    ~XmlTest() = default;
    static XmlSerializer construct(napi_env env);
    static void SetDeclaration(napi_env env);
    static void SetNamespace(napi_env env);
    static void StartElement(napi_env env);
    static void WriteEscaped(napi_env env);
    static void XmlSerializerError(napi_env env);
    static void PushSrcLinkList(napi_env env);
    static size_t GetNSCount(napi_env env, size_t iTemp);
    static std::string XmlPullParserError(napi_env env);
    static TagEnum DealExclamationGroup(napi_env env, std::string xml);
    static TagEnum DealLtGroup(napi_env env);
    static TagEnum ParseTagType(napi_env env, std::string str, int apiVersion);
    static std::string SkipText(napi_env env, std::string strXml, std::string str);
    static std::string ParseNameInner(napi_env env, size_t start);
    static std::string ParseName(napi_env env);
    static bool ParseEntityFunc(napi_env env, std::string out, std::string sysInfo, bool flag, TextEnum textEnum);
    static std::string ParseEntity(napi_env env, bool relaxed);
    static size_t ParseTagValueInner(napi_env env, size_t &start,
                                     std::string &result, size_t position, std::string xmlStr);
    static std::string ParseTagValue(napi_env env, char delimiter, bool resolveEntities, TextEnum textEnum, size_t max);
    static std::string GetNamespace(napi_env env, const std::string prefix, size_t depth);
    static std::string ParseNspFunc(napi_env env);
    static std::string ParseNspFunction(napi_env env, std::string pushStr);
    static bool ParseNsp(napi_env env);
    static bool ParseStartTagFuncDeal(napi_env env, std::string xml, bool relax);
    static bool ParseDeclaration(napi_env env, std::string str);
    static bool ReadInternalSubset();
    static bool ParseStartTag(napi_env env, std::string str);
    static bool ParseEndTagFunction(napi_env env, std::string str);
    static bool ParseTagValueFunc(napi_env env, std::string str, char &c, size_t &start, std::string &result);
    static std::string DealNapiStrValueFunction(napi_env env, std::string pushStr);
    static int SplicNspFunction(napi_env env, std::string pushStr);
    static std::string SetNamespaceFunction(napi_env env, std::string prefix, const std::string &nsTemp);
    static std::string XmlSerializerErrorFunction(napi_env env);
    static std::string DealLengthFuc(napi_env env, std::string str, size_t minimum, std::string pushStr);
    int SkipCharFunction(napi_env env, std::string str, char expected);
    int TestGetColumnNumber(napi_env env);
    int TestGetLineNumber(napi_env env);
    std::string TestGetText(napi_env env);
    bool TestParseNsp(napi_env env);
    void TestParseDeclaration(napi_env env);
    std::string TestParseDelimiterInfo(napi_env env);
    bool TestParseEndTag(napi_env env);
    bool TestParseComment(napi_env env);
    TagEnum TestParseOneTagFunc(napi_env env);
    static TagEnum ParseStartTagFuncTest(napi_env env, std::string str, bool xmldecl, bool throwOnResolveFailure);
    void TestParseEntityDecl(napi_env env);
};

XmlSerializer XmlTest::construct(napi_env env)
{
    napi_value arrayBuffer = nullptr;
    void* pBuffer = nullptr;
    size_t size = 1024;  // 1024: buffer size
    napi_create_arraybuffer(env, size, &pBuffer, &arrayBuffer);
    OHOS::xml::XmlSerializer xmlSerializer(reinterpret_cast<char*>(pBuffer), size, "utf-8");
    return xmlSerializer;
}

void XmlTest::SetDeclaration(napi_env env)
{
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.isHasDecl = true;
    xmlSerializer.SetDeclaration();
}

void XmlTest::SetNamespace(napi_env env)
{
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.type = "isStart";
    xmlSerializer.iLength_ = 0;
    xmlSerializer.depth_ = 1;
    xmlSerializer.elementStack.push_back("");
    xmlSerializer.elementStack.push_back("");
    xmlSerializer.SetNamespace("xml", "convert");
}

void XmlTest::StartElement(napi_env env)
{
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.depth_ = 1;
    xmlSerializer.elementStack[0] = "x";
    xmlSerializer.elementStack.push_back("");
    xmlSerializer.elementStack.push_back("");
    xmlSerializer.elementStack.push_back("");
    xmlSerializer.StartElement("val");
}

void XmlTest::WriteEscaped(napi_env env)
{
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.WriteEscaped("'\"&><q");
}

void XmlTest::XmlSerializerError(napi_env env)
{
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.XmlSerializerError();
}

void XmlTest::PushSrcLinkList(napi_env env)
{
    std::string strXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>    <title>Happy</title>    <todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.PushSrcLinkList("str");
}

size_t XmlTest::GetNSCount(napi_env env, size_t iTemp)
{
    std::string strXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.nspCounts_.push_back(0);
    xmlPullParser.nspCounts_.push_back(1);
    return xmlPullParser.GetNSCount(iTemp);
}

std::string XmlTest::XmlPullParserError(napi_env env)
{
    std::string strXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, "strXml", "utf-8");
    xmlPullParser.xmlPullParserError_ = "IndexOutOfBoundsException";
    return xmlPullParser.XmlPullParserError();
}

TagEnum XmlTest::DealExclamationGroup(napi_env env, std::string xml)
{
    std::string strXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, "strXml", "utf-8");
    xmlPullParser.strXml_ = xml;
    return xmlPullParser.DealExclamationGroup();
}

TagEnum XmlTest::DealLtGroup(napi_env env)
{
    std::string strXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.strXml_ = "?/";
    return xmlPullParser.DealLtGroup();
}

TagEnum XmlTest::ParseTagType(napi_env env, std::string str, int apiVersion)
{
    std::string strXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.bStartDoc_ = false;
    xmlPullParser.max_ = 0;
    xmlPullParser.strXml_ = str;
    xmlPullParser.apiVersion_ = apiVersion;
    return xmlPullParser.ParseTagType(false);
}

std::string XmlTest::SkipText(napi_env env, std::string strXml, std::string str)
{
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.SkipText(str);
    return xmlPullParser.XmlPullParserError();
}

int XmlTest::TestGetColumnNumber(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "1\n1", "utf8");
    xml.position_ = 3; // 3: index is three
    int res = xml.GetColumnNumber();
    return res;
}

int XmlTest::TestGetLineNumber(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "\n", "utf8");
    xml.position_ = 1;
    int res = xml.GetLineNumber();
    return res;
}

std::string XmlTest::TestGetText(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "1\n1", "utf8");
    xml.type = TagEnum::WHITESPACE;
    return xml.GetText();
}

bool XmlTest::TestParseNsp(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "", "utf8");
    xml.attriCount_ = 1;
    xml.attributes.push_back("");
    xml.attributes.push_back("");
    xml.attributes.push_back("xmlns");
    xml.attributes.push_back("");
    xml.nspCounts_.push_back(0);
    xml.name_ = ":";
    return xml.ParseNsp();
}

void XmlTest::TestParseDeclaration(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "", "utf8");
    xml.bufferStartLine_ = 1;
    xml.attributes.push_back("");
    xml.attributes.push_back("");
    xml.attributes.push_back("");
    xml.attributes.push_back("1.0");
    xml.ParseDeclaration();
}

std::string XmlTest::TestParseDelimiterInfo(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "12", "utf8");
    xml.text_ = "123";
    std::string res = xml.ParseDelimiterInfo("456", true);
    xml.strXml_ = "456";
    xml.position_ = 0;
    res = xml.ParseDelimiterInfo("456", false);
    return res;
}

bool XmlTest::TestParseEndTag(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "123456789", "utf8");
    xml.relaxed = false;
    xml.depth = 1;
    xml.elementStack_.resize(20); // 20 :vector size
    xml.elementStack_[3] = "!"; // 3: index of three
    xml.ParseEndTag();
    xml.depth = 0;
    xml.ParseEndTag();
    return false;
}

bool XmlTest::TestParseComment(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "1", "utf8");
    xml.relaxed = true;
    xml.ParseComment(true);
    return false;
}

TagEnum XmlTest::TestParseOneTagFunc(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "1", "utf8");
    xml.type = TagEnum::ERROR;
    TagEnum res = xml.ParseOneTagFunc();
    return res;
}

void XmlTest::TestParseEntityDecl(napi_env env)
{
    OHOS::xml::XmlPullParser xml(env, "%1234", "utf8");
    xml.ParseEntityDecl();
}

std::string XmlTest::ParseNameInner(napi_env env, size_t start)
{
    std::string strXml = "<todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.position_ = 1;
    xmlPullParser.max_ = 1;
    xmlPullParser.strXml_ = "version";
    return xmlPullParser.ParseNameInner(start);
}

std::string XmlTest::ParseName(napi_env env)
{
    std::string strXml = "><todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.strXml_ = "encoding";
    size_t len = xmlPullParser.strXml_.length();
    xmlPullParser.position_ = len;
    xmlPullParser.max_ = len;
    return xmlPullParser.ParseName();
}

bool XmlTest::ParseEntityFunc(napi_env env, std::string out, std::string sysInfo, bool flag, TextEnum textEnum)
{
    std::string strXml = "<todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    std::string key = "Work";
    xmlPullParser.documentEntities[key] = "value";
    xmlPullParser.bDocDecl = flag;
    xmlPullParser.sysInfo_ = sysInfo;
    xmlPullParser.ParseEntityFunc(0, out, true, textEnum);
    return xmlPullParser.bUnresolved_;
}

std::string XmlTest::ParseEntity(napi_env env, bool relaxed)
{
    std::string strXml = "Wor";
    std::string out = "W#13434";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.position_ = 0;
    xmlPullParser.max_ = 1;
    xmlPullParser.relaxed = relaxed;
    xmlPullParser.ParseEntity(out, true, true, TextEnum::ENTITY_DECL);
    return xmlPullParser.XmlPullParserError();
}

size_t XmlTest::ParseTagValueInner(napi_env env, size_t &start,
                                   std::string &result, size_t position, std::string xmlStr)
{
    std::string strXml = "<todo>Work</todo>";
    OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
    xmlPullParser.position_ = position;
    xmlPullParser.max_ = 1;
    xmlPullParser.strXml_ = xmlStr;
    return xmlPullParser.ParseTagValueInner(start, result, 'o', TextEnum::ENTITY_DECL, false);
}

std::string XmlTest::ParseTagValue(napi_env env, char delimiter, bool resolveEntities, TextEnum textEnum, size_t max)
{
    std::string xml = "W";
    OHOS::xml::XmlPullParser xmlPullParser(env, xml, "utf-8");
    xmlPullParser.text_ = "xml";
    xmlPullParser.position_ = 1;
    xmlPullParser.max_ = max;
    return xmlPullParser.ParseTagValue(delimiter, resolveEntities, false, textEnum);
}

std::string XmlTest::GetNamespace(napi_env env, const std::string prefix, size_t depth)
{
    std::string xml = "Work";
    const std::string preStr = prefix;
    OHOS::xml::XmlPullParser xmlPullParser(env, xml, "utf-8");
    xmlPullParser.depth = depth;
    xmlPullParser.nspCounts_.push_back(0);
    xmlPullParser.nspCounts_.push_back(1);
    xmlPullParser.nspCounts_.push_back(2); // values greater than pos_
    xmlPullParser.nspStack_.push_back("Q");
    xmlPullParser.nspStack_.push_back("E");
    xmlPullParser.nspStack_.push_back("");
    xmlPullParser.nspStack_.push_back("W");
    return xmlPullParser.GetNamespace(preStr);
}

std::string XmlTest::ParseNspFunc(napi_env env)
{
    std::string xml = "Work";
    size_t count = 0;
    std::string attrName = "sub";
    bool any = true;
    OHOS::xml::XmlPullParser xmlPullParser(env, xml, "utf-8");
    xmlPullParser.attributes.push_back("q");
    xmlPullParser.attributes.push_back("e");
    xmlPullParser.attributes.push_back("r");
    xmlPullParser.attributes.push_back("");
    xmlPullParser.nspCounts_.push_back(0);
    xmlPullParser.nspStack_.push_back("t");
    xmlPullParser.nspStack_.push_back("c");
    xmlPullParser.nspStack_.push_back("y");
    xmlPullParser.nspStack_.push_back("p");
    xmlPullParser.bKeepNsAttri = true;
    xmlPullParser.ParseNspFunc(count, attrName, any);
    return xmlPullParser.XmlPullParserError();
}

std::string XmlTest::ParseNspFunction(napi_env env, std::string pushStr)
{
    std::string xml = "Work";
    OHOS::xml::XmlPullParser xmlPullParser(env, xml, "utf-8");
    xmlPullParser.attriCount_ = 1;
    xmlPullParser.depth = 1;
    xmlPullParser.nspCounts_.push_back(0);
    xmlPullParser.nspCounts_.push_back(1);
    xmlPullParser.nspCounts_.push_back(2); // values greater than pos_
    xmlPullParser.nspStack_.push_back("Q");
    xmlPullParser.nspStack_.push_back("E");
    xmlPullParser.nspStack_.push_back("");
    xmlPullParser.nspStack_.push_back("W");
    xmlPullParser.attributes.push_back("r");
    xmlPullParser.attributes.push_back("t");
    xmlPullParser.attributes.push_back(pushStr);
    xmlPullParser.ParseNspFunction();
    return xmlPullParser.XmlPullParserError();
}

bool XmlTest::ParseNsp(napi_env env)
{
    std::string xml = "Work";
    OHOS::xml::XmlPullParser xmlPullParser(env, xml, "utf-8");
    xmlPullParser.attributes.push_back("");
    xmlPullParser.attributes.push_back("");
    xmlPullParser.attributes.push_back("xmlns");
    xmlPullParser.nspCounts_.push_back(0);
    xmlPullParser.name_ = ":xml";
    return xmlPullParser.ParseNsp();
}

bool XmlTest::ParseStartTagFuncDeal(napi_env env, std::string xml, bool relax)
{
    OHOS::xml::XmlPullParser xmlPullParser(env, xml, "utf-8");
    xmlPullParser.position_ = 0;
    xmlPullParser.max_ = 1;
    xmlPullParser.relaxed = relax;
    return xmlPullParser.ParseStartTagFuncDeal(true);
}

bool XmlTest::ParseDeclaration(napi_env env, std::string str)
{
    OHOS::xml::XmlPullParser xmlPullParser(env, str, "utf-8");
    xmlPullParser.attriCount_ = 3; // values greater than pos_
    xmlPullParser.ParseDeclaration();
    return true;
}

std::string XmlTest::DealNapiStrValueFunction(napi_env env, std::string pushStr)
{
    napi_value arg = nullptr;
    std::string output = "";
    napi_create_string_utf8(env, pushStr.c_str(), pushStr.size(), &arg);
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.DealNapiStrValue(env, arg, output);
    return output;
}

int XmlTest::SplicNspFunction(napi_env env, std::string pushStr)
{
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.type = pushStr;
    return xmlSerializer.curNspNum;
}

std::string XmlTest::SetNamespaceFunction(napi_env env, std::string prefix, const std::string &nsTemp)
{
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.type = "isStart";
    xmlSerializer.SetDeclaration();
    xmlSerializer.SetNamespace(prefix, nsTemp);
    xmlSerializer.StartElement("note");
    xmlSerializer.EndElement();
    return xmlSerializer.out_;
}

std::string XmlTest::XmlSerializerErrorFunction(napi_env env)
{
    XmlSerializer xmlSerializer = construct(env);
    xmlSerializer.isHasDecl = true;
    xmlSerializer.SetDeclaration();
    return xmlSerializer.xmlSerializerError_;
}

bool XmlTest::ParseStartTag(napi_env env, std::string str)
{
    OHOS::xml::XmlPullParser xmlPullParser(env, str, "utf-8");
    xmlPullParser.attriCount_ = 3; // values greater than pos_
    xmlPullParser.defaultAttributes["lt;"]["<"] = "lt;";
    xmlPullParser.defaultAttributes["lt;"]["<"] = "<";
    xmlPullParser.defaultAttributes["gt;"]["<"] = "gt;";
    xmlPullParser.defaultAttributes["gt;"]["<"] = ">";
    xmlPullParser.ParseStartTag(false, false);
    xmlPullParser.ParseDeclaration();
    return true;
}
 
bool XmlTest::ParseEndTagFunction(napi_env env, std::string str)
{
    OHOS::xml::XmlPullParser xml(env, str, "utf8");
    xml.relaxed = false;
    xml.depth = 1;
    xml.elementStack_.resize(20); // 20 :vector size
    xml.elementStack_[3] = "!"; // 3: index of three
    xml.ParseEndTag();
    xml.depth = 0;
    xml.ParseEndTag();
    return true;
}

bool XmlTest::ParseTagValueFunc(napi_env env, std::string str, char &c, size_t &start, std::string &result)
{
    OHOS::xml::XmlPullParser xml(env, str, "utf8");
    xml.max_ = 100; // 100: max_ size
    return xml.ParseTagValueFunc(c, true, TextEnum::ATTRI, start, result);
}

std::string XmlTest::DealLengthFuc(napi_env env, std::string str, size_t minimum, std::string pushStr)
{
    OHOS::xml::XmlPullParser xmlPullParser(env, str, "utf-8");
    xmlPullParser.keyInfo_ = pushStr;
    xmlPullParser.position_ = 10; // 10: position_ size
    xmlPullParser.DealLength(minimum);
    return xmlPullParser.keyInfo_;
}

TagEnum XmlTest::ParseStartTagFuncTest(napi_env env, std::string str, bool xmldecl, bool throwOnResolveFailure)
{
    OHOS::xml::XmlPullParser xmlPullParser(env, str, "utf-8");
    size_t minimum = 10; // 10: minimum size
    xmlPullParser.position_ = 100; // 100: position_ size
    xmlPullParser.DealLength(minimum);
    TagEnum res = xmlPullParser.ParseStartTagFunc(xmldecl, throwOnResolveFailure);
    return res;
}

int XmlTest::SkipCharFunction(napi_env env, std::string str, char expected)
{
    OHOS::xml::XmlPullParser xmlPullParser(env, str, "utf-8");
    xmlPullParser.DealLength(666); // 666: minimum size
    xmlPullParser.position_ = 666; // 666: position_ size
    xmlPullParser.SkipChar(expected);
    return xmlPullParser.PriorDealChar();
}
}
#endif // TEST_XML_H
