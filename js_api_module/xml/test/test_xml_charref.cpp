/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "test.h"

#include "js_api_module/xml/js_xml.h"

namespace OHOS::xml {
// Wraps the parser internals the numeric character reference tests need;
// befriended by XmlPullParser like XmlTest, so the tests can observe the
// parser error and the unresolved flag directly.
class XmlCharRefTest {
public:
    XmlCharRefTest() = default;
    ~XmlCharRefTest() = default;
    // Feeds out to ParseEntityFunc the way the entity branch of a TEXT token
    // does, and reports the parser error and the unresolved flag.
    static std::string ParseEntityFuncIllegalRef(napi_env env, std::string &out, bool &unresolved)
    {
        std::string strXml = "<todo>Work</todo>";
        OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
        xmlPullParser.ParseEntityFunc(0, out, false, TextEnum::TEXT);
        unresolved = xmlPullParser.bUnresolved_;
        return xmlPullParser.XmlPullParserError();
    }

    // Parses a whole document containing an illegal numeric character
    // reference, and reports the parser error.
    static std::string ParseXmlIllegalCharRef(napi_env env)
    {
        std::string strXml = "<?xml version=\"1.0\"?><r>&#xyz;</r>";
        OHOS::xml::XmlPullParser xmlPullParser(env, strXml, "utf-8");
        while (xmlPullParser.type != TagEnum::END_DOCUMENT) {
            if (xmlPullParser.ParseOneTag() == TagEnum::ERROR) {
                break;
            }
        }
        return xmlPullParser.XmlPullParserError();
    }
};
} // namespace OHOS::xml

using namespace OHOS::xml;

/* @tc.name: ParseEntityFuncIllegalCharRef001
 * @tc.desc: Test ParseEntityFunc with illegal numeric character references
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ParseEntityFuncIllegalCharRef001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    // "&#xyz;": hex reference with non-hex digits, std::stoi used to throw std::invalid_argument
    std::string out = "&#xyz;";
    bool unresolved = false;
    std::string err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    // "&#;": decimal reference without digits
    out = "&#;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    // "&#x;": hex reference without digits
    out = "&#x;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    // "&#99999999999;": value out of int range, std::stoi used to throw std::out_of_range
    out = "&#99999999999;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    // "&#x80000000;": hex value just above INT_MAX, rejected by the overflow
    // guard in base 16
    out = "&#x80000000;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    // "&#a5;" and "&#A5;": hex letters are not decimal digits, the digit
    // value of a letter exceeds the decimal base, so the reference is illegal
    out = "&#a5;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    out = "&#A5;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    // "&#-2147483649;" and "&#x-80000001;": signed values just below INT_MIN,
    // std::stoi used to throw std::out_of_range on them
    out = "&#-2147483649;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    out = "&#x-80000001;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    // "&#" and "&#x": references truncated before any digit, the digit part
    // is empty; std::stoi used to throw std::invalid_argument on the empty
    // string. "&#x" also covers the size >= 2 boundary of the hex branch.
    out = "&#";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);

    out = "&#x";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(unresolved);
}

/* @tc.name: ParseEntityFuncIllegalCharRef002
 * @tc.desc: Test ParseEntityFunc with references whose digit run is followed by junk;
 *           they keep resolving to the digit prefix exactly like std::stoi
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ParseEntityFuncIllegalCharRef002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    // "&#x0x41;": std::stoi accepts the "0x" prefix in base 16 and resolves
    // this to 'A'; the conversion keeps doing so
    std::string out = "&#x0x41;";
    bool unresolved = true;
    std::string err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_STREQ(out.c_str(), "A");

    // "&#x0x;": the "0x" prefix is not followed by a hex digit, so only the
    // leading '0' is consumed and the reference resolves to the NUL character
    out = "&#x0x;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(out[0], '\0');

    // "&#0x41;": decimal reference, 'x' is not a decimal digit; only the
    // leading '0' is consumed and the reference resolves to the NUL character
    out = "&#0x41;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(out[0], '\0');

    // "&#6a5;" and "&#65-5;": junk after a valid digit prefix keeps resolving
    // to the prefix value
    out = "&#6a5;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 0x06); // 0x06: value of &#6a5;

    out = "&#65-5;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_STREQ(out.c_str(), "A");

    // "&#6a5": truncated before the ';', still resolves to the digit prefix
    out = "&#6a5";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 0x06); // 0x06: value of &#6a5
}

/* @tc.name: ParseEntityFuncIllegalCharRef003
 * @tc.desc: Test ParseEntityFunc with signed references and int boundaries;
 *           they keep resolving exactly like std::stoi
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ParseEntityFuncIllegalCharRef003, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    // "&#-65;" and "&#x-41;": signed references keep resolving through the
    // std::stoi sign semantics; static_cast<char> of -65 is 0xBF
    std::string out = "&#-65;";
    bool unresolved = true;
    std::string err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 0xBF); // 0xBF: value of &#-65;

    out = "&#x-41;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 0xBF); // 0xBF: value of &#x-41;

    // "&#+65;" and "&# 65;": the '+' sign and leading whitespace are accepted
    // like std::stoi; ParseEntity stops before them, so these forms are only
    // reachable through ParseEntityFunc
    out = "&#+65;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_STREQ(out.c_str(), "A");

    out = "&# 65;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_STREQ(out.c_str(), "A");

    // "&#-2147483648;" and "&#x-80000000;": INT_MIN exactly, the lowest value
    // std::stoi accepts; static_cast<char> of INT_MIN is 0x00
    out = "&#-2147483648;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(out[0], '\0');

    out = "&#x-80000000;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(out[0], '\0');
}

/* @tc.name: ParseEntityFuncLegalCharRef001
 * @tc.desc: Test ParseEntityFunc with legal numeric character references
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ParseEntityFuncLegalCharRef001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    // legal references are still resolved as before
    std::string out = "&#65;";
    bool unresolved = true;
    std::string err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_STREQ(out.c_str(), "A");

    out = "&#x41;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_STREQ(out.c_str(), "A");

    // "&#xa0;": hex reference with lowercase hex letters, keeps resolving as before
    out = "&#xa0;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 0xA0); // 0xA0: value of &#xa0;

    // "&#xFF;": hex reference with uppercase hex letters, keeps resolving as before
    out = "&#xFF;";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_EQ(out.size(), 1); // 1: length of the decoded character
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 0xFF); // 0xFF: value of &#xFF;

    // "&#65": reference truncated before the ';', keeps resolving to the
    // digits read so far, as before
    out = "&#65";
    err = XmlCharRefTest::ParseEntityFuncIllegalRef(env, out, unresolved);
    EXPECT_TRUE(err.empty());
    EXPECT_FALSE(unresolved);
    EXPECT_STREQ(out.c_str(), "A");
}

/* @tc.name: ParseXmlIllegalCharRef001
 * @tc.desc: Test parsing a document with an illegal numeric character reference
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ParseXmlIllegalCharRef001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    // Parsing "&#xyz;" must not abort the process; the error is reported through XmlPullParserError.
    std::string err = XmlCharRefTest::ParseXmlIllegalCharRef(env);
    EXPECT_FALSE(err.empty());
}
