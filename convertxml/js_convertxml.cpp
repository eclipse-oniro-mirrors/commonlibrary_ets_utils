#include "js_convertxml.h"
#include "utils/log.h"
#include "securec.h"
#include <stdlib.h>

ConvertXml::ConvertXml(napi_env env) : env_(env)
{
    m_SpaceType = SpaceType::T_INIT;
    m_strSpace = "";
    m_iSpace = 0;
}
std::string ConvertXml::GetNodeType(xmlElementType enumType)
{
    std::string strResult= "";
    switch(enumType) {
        case xmlElementType::XML_ELEMENT_NODE :
            strResult = "element";
            break;
        case xmlElementType::XML_ATTRIBUTE_NODE :
            strResult = "attribute";
            break;
        case xmlElementType::XML_TEXT_NODE :
            strResult = "text";
            break;
        case xmlElementType::XML_CDATA_SECTION_NODE :
            strResult = "cdata";
            break;
        case xmlElementType::XML_ENTITY_REF_NODE :
            strResult = "entity_ref";
            break;
        case xmlElementType::XML_ENTITY_NODE :
            strResult = "entity";
            break;
        case xmlElementType::XML_PI_NODE :
            strResult = "instruction";
            break;
        case xmlElementType::XML_COMMENT_NODE :
            strResult = "comment";
            break;
        case xmlElementType::XML_DOCUMENT_NODE :
            strResult = "document";
            break;
        case xmlElementType::XML_DOCUMENT_TYPE_NODE :
            strResult = "document_type";
            break;
        case xmlElementType::XML_DOCUMENT_FRAG_NODE :
            strResult = "document_frag";
            break;
        case xmlElementType::XML_DTD_NODE :
            strResult = "doctype";
            break;
#ifdef LIBXML_DOCB_ENABLED
        case xmlElementType::XML_DOCB_DOCUMENT_NODE:
            strResult =  "docb_document";
            break;
#endif
        default:
            break;
    }
    return strResult;
}

void ConvertXml::SetKeyValue(napi_value& object, std::string strKey, std::string strValue)
{
    napi_value attrValue = nullptr;
    napi_create_string_utf8(env_, strValue.c_str(), NAPI_AUTO_LENGTH, &attrValue); 
    napi_set_named_property(env_, object, strKey.c_str(), attrValue);
}
std::string ConvertXml::Trim(std::string strXmltrim) 
{
    if (strXmltrim.empty()) {
        return "";
    }
    size_t i = 0;
    size_t strlen = strXmltrim.size();
    for (; i < strlen; ) {
        if (strXmltrim[i] == ' ') {
            i++;
        } else {
            break;
        }
    }
    strXmltrim = strXmltrim.substr(i);
    strlen = strXmltrim.size();
    for (i = strlen - 1; i != 0; i--) {
        if (strXmltrim[i] == ' ') {
            strXmltrim.pop_back();
        } else {
            break;
        }
    }
    return strXmltrim;
}

void ConvertXml::GetPrevNodeList (xmlNodePtr curNode)
{
    while (curNode->prev != nullptr) {
        curNode = curNode->prev;
        napi_value elementsObject = nullptr;
        napi_create_object(env_, &elementsObject);
        SetKeyValue(elementsObject, m_Options.type, GetNodeType(curNode->type));
        if (curNode->type == xmlElementType::XML_PI_NODE) {
            SetKeyValue(elementsObject, m_Options.name, (char*)curNode->name);
            SetKeyValue(elementsObject, m_Options.instruction, (const char*)xmlNodeGetContent(curNode));
        }
        if (curNode->type == xmlElementType::XML_COMMENT_NODE) {
            SetKeyValue(elementsObject, m_Options.comment, (const char*)xmlNodeGetContent(curNode));
        }
        if (curNode->type == xmlElementType::XML_DTD_NODE) {
           SetKeyValue(elementsObject, m_Options.doctype, (char*)curNode->name);
        }
        m_prevObj.push_back(elementsObject);
    }
}

void ConvertXml::SetAttributes(xmlNodePtr curNode, napi_value& elementsObject)
{     
    xmlAttr * attr = curNode->properties;
    if (attr && !m_Options.ignoreAttributes) {
        napi_value attrTitleObj = nullptr;
        napi_create_object(env_, &attrTitleObj);
        while (attr) {
            SetKeyValue(attrTitleObj, (const char*)attr->name, (const char*)attr->children->content);
            attr = attr->next;
        }
        napi_set_named_property(env_, elementsObject, m_Options.attributes.c_str(), attrTitleObj);
    }
}

void ConvertXml::SetXmlElementType(xmlNodePtr curNode, napi_value& elementsObject)
{
    if (curNode->type == xmlElementType::XML_PI_NODE && !m_Options.ignoreInstruction) {
        SetKeyValue(elementsObject, m_Options.instruction.c_str(), (const char*)xmlNodeGetContent(curNode));
    } else if (curNode->type == xmlElementType::XML_COMMENT_NODE && !m_Options.ignoreComment) {
        SetKeyValue(elementsObject, m_Options.comment.c_str(), (const char*)xmlNodeGetContent(curNode));
    } else if (curNode->type == xmlElementType::XML_CDATA_SECTION_NODE && !m_Options.ignoreCdata) {
        SetKeyValue(elementsObject, m_Options.cdata, (const char*)xmlNodeGetContent(curNode));
    }
}
void ConvertXml::SetNodeInfo(xmlNodePtr curNode, napi_value& elementsObject)
{
    if (curNode->type == xmlElementType::XML_PI_NODE) {
        SetKeyValue(elementsObject, m_Options.type, m_Options.instruction);
    } else {
        SetKeyValue(elementsObject, m_Options.type, GetNodeType(curNode->type));
    }
    SetKeyValue(elementsObject, m_Options.type, GetNodeType(curNode->type));
    if((curNode->type != xmlElementType::XML_COMMENT_NODE) && (curNode->type != xmlElementType::XML_CDATA_SECTION_NODE)) {
        SetKeyValue(elementsObject, m_Options.name, (char*)curNode->name);
    }
}

void ConvertXml::SetEndInfo(xmlNodePtr curNode, napi_value& elementsObject, bool& bFlag, bool& TextFlag, int32_t index)
{
    SetKeyValue(elementsObject, m_Options.type, GetNodeType(curNode->type)); 
    if (curNode->type == xmlElementType::XML_ELEMENT_NODE) {
        SetKeyValue(elementsObject, m_Options.name.c_str(), (const char*)curNode->name);
        bFlag = true;
    } else if (curNode->type == xmlElementType::XML_TEXT_NODE) {
        if (m_Options.trim) {
            SetKeyValue(elementsObject, m_Options.text, Trim((const char*)xmlNodeGetContent(curNode)));
        } else {
            SetKeyValue(elementsObject, m_Options.text, (const char*)xmlNodeGetContent(curNode));
        }
        if (!m_Options.ignoreText) {
            bFlag = true;
        }
        if (index != 0)
        {
            TextFlag = false;
        }
    }
}

void ConvertXml::SetPrevInfo(napi_value& recvElement, int flag, int32_t& index1)
{
    if (!m_prevObj.empty() && !flag) {
        for(int i = (m_prevObj.size() - 1); i >= 0; --i) {
            napi_set_element(env_, recvElement, index1++, m_prevObj[i]);
        }
    }
}

void ConvertXml::GetXMLInfo(xmlNodePtr curNode, napi_value& object, int flag)
{
    napi_value elements = nullptr;
    napi_create_array(env_, &elements);
    napi_value recvElement;
    napi_create_array(env_, &recvElement);
    xmlNodePtr pNode = curNode;
    int32_t index = 0;
    int32_t index1 = 0;
    bool bFlag = false;
    bool TextFlag = true;
    while (pNode != nullptr) {
        bFlag = false;
        TextFlag = true;
        napi_value elementsObject = nullptr;
        napi_create_object(env_, &elementsObject);
        if (flag == 0 || (index %2 != 0)) {
            SetNodeInfo(pNode, elementsObject);
        }
        SetAttributes(pNode, elementsObject);
        napi_value tempElement = nullptr;
        napi_create_array(env_, &tempElement);
        napi_value elementObj = nullptr;
        napi_create_object(env_, &elementObj);
        if (xmlNodeGetContent(pNode) != nullptr) {
            if (pNode->children != nullptr) {
                curNode = pNode->children;
                GetXMLInfo(curNode, elementsObject, 1);
                bFlag = true;
            } else if (index % 2 != 0) {
                SetXmlElementType(pNode, elementsObject);
                bFlag = true;
            } else if (pNode->next == nullptr) {
                SetEndInfo(pNode, elementsObject, bFlag, TextFlag, index);
            }
        }
        SetPrevInfo(recvElement, flag, index1);
        if (elementsObject != nullptr && bFlag && TextFlag) {
                napi_set_element(env_, recvElement, index1++, elementsObject);
                elementsObject = nullptr;
        }
        index++;
        pNode = pNode->next;
    }
    if (bFlag) {
        napi_set_named_property(env_, object, m_Options.elements.c_str(), recvElement);
    }
}

napi_value ConvertXml::convert(std::string strXml)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr curNode = NULL;
    napi_status status = napi_ok;
    size_t len = strXml.size();
    doc = xmlParseMemory(strXml.c_str(), len);
    if (!doc) {
        xmlFreeDoc(doc);
    }
    napi_value object = nullptr;
    status = napi_create_object(env_, &object);
    if (status != napi_ok) {
        return NULL;
    }
    napi_value subObject = nullptr;
    napi_value subSubObject = nullptr;
    napi_value napiKey = nullptr;
    napi_create_object(env_, &subSubObject);
    napi_create_object(env_, &subObject);
    napi_create_string_utf8(env_, (const char*)doc->version, NAPI_AUTO_LENGTH, &napiKey);
    napi_set_named_property(env_, subSubObject,"version", napiKey);
    napi_create_string_utf8(env_, (const char*)doc->encoding, NAPI_AUTO_LENGTH, &napiKey);
    napi_set_named_property(env_, subSubObject, "encoding", napiKey);
    if (!m_Options.ignoreDeclaration) {
        napi_set_named_property(env_, subObject, m_Options.attributes.c_str(), subSubObject);
        napi_set_named_property(env_, object, m_Options.declaration.c_str(), subObject);
    }
    curNode = xmlDocGetRootElement(doc);
    GetPrevNodeList(curNode);
    GetXMLInfo(curNode, object, 0);
    napi_value iTemp = nullptr;
    switch (m_SpaceType) {
        case (SpaceType::T_INT32):
            napi_create_int32(env_, m_iSpace, &iTemp);
            napi_set_named_property(env_, object, "spaces", iTemp);
            break;
        case (SpaceType::T_STRING):
            SetKeyValue(object, "spaces", m_strSpace);
            break;
        case (SpaceType::T_INIT):
            SetKeyValue(object, "spaces", m_strSpace);
            break;
    }
    return object;
}

napi_status ConvertXml::DealNapiStrValue(napi_value napi_StrValue, std::string &result)
{
    char* buffer = nullptr;
    size_t bufferSize = 0;
    napi_status status = napi_ok;
    status = napi_get_value_string_utf8(env_, napi_StrValue, buffer, -1, &bufferSize);
    if (status != napi_ok) {
        return status;
    }
    buffer = new char[bufferSize + 1];
    napi_get_value_string_utf8(env_, napi_StrValue, buffer, bufferSize + 1, &bufferSize);

    if (buffer != nullptr) {
        result = buffer;
        delete []buffer;
        buffer = nullptr;
    }
    return status;
}

void ConvertXml::DealSpaces(napi_value napi_obj)
{
    napi_value recvTemp = nullptr;
    napi_get_named_property(env_, napi_obj, "spaces" ,&recvTemp);
    napi_valuetype valuetype = napi_undefined;
    napi_typeof(env_, recvTemp, &valuetype);
    if (valuetype == napi_string) {
        DealNapiStrValue(recvTemp, m_strSpace);
        m_SpaceType = SpaceType::T_STRING;
    } else if (valuetype == napi_number) {
        int32_t iTemp;
        if (napi_get_value_int32(env_, recvTemp, &iTemp) == napi_ok) {
            m_iSpace = iTemp;
            m_SpaceType = SpaceType::T_INT32;
        }
    }
}

void ConvertXml::DealIgnore(napi_value napi_obj)
{
    std::vector<std::string>vctIgnore = { "compact", "trim", "ignoreDeclaration", "ignoreInstruction",
        "ignoreAttributes", "ignoreComment", "ignoreCdata", "ignoreDoctype", "ignoreText" };
    for (size_t i = 0; i < vctIgnore.size(); ++i) {
        napi_value recvTemp = nullptr;
        bool bRecv = false;
        napi_get_named_property(env_, napi_obj, vctIgnore[i].c_str(), &recvTemp);
        if ((napi_get_value_bool(env_, recvTemp, &bRecv)) == napi_ok) {
            switch (i) {
                case 0:
                    m_Options.compact = bRecv;
                    break;
                case 1:
                    m_Options.trim = bRecv;
                    break;
                case 2:
                    m_Options.ignoreDeclaration = bRecv;
                    break;
                case 3:
                    m_Options.ignoreInstruction = bRecv;
                    break;
                case 4:
                    m_Options.ignoreAttributes = bRecv;
                    break;
                case 5:
                    m_Options.ignoreComment = bRecv;
                    break;
                case 6:
                    m_Options.ignoreCdata = bRecv;
                    break;
                case 7:
                    m_Options.ignoreDoctype = bRecv;
                    break;
                case 8:
                    m_Options.ignoreText = bRecv;
                    break;
                default:
                    break;
            }
        }
    }
}

void ConvertXml::DealOptions(napi_value napi_obj)
{
    std::vector<std::string>vctOptions = { "declarationKey", "instructionKey", "attributesKey", "textKey",
    "cdataKey", "doctypeKey", "commentKey", "parentKey", "typeKey", "nameKey", "elementsKey" };
    for (size_t i = 0; i < vctOptions.size(); ++i) {
        napi_value recvTemp = nullptr;
        std::string strRecv = "";
        napi_get_named_property(env_, napi_obj, vctOptions[i].c_str(), &recvTemp);
        if ((DealNapiStrValue(recvTemp, strRecv)) == napi_ok) {
            switch (i) {
                case 0:
                    m_Options.declaration = strRecv;
                    break;
                case 1:
                    m_Options.instruction = strRecv;
                    break;
                case 2:
                    m_Options.attributes = strRecv;
                    break;
                case 3:
                    m_Options.text = strRecv;
                    break;
                case 4:
                    m_Options.cdata = strRecv;
                    break;
                case 5:
                    m_Options.doctype = strRecv;
                    break;
                case 6:
                    m_Options.comment = strRecv;
                    break;
                case 7:
                    m_Options.parent = strRecv;
                    break;
                case 8:
                    m_Options.type = strRecv;
                    break;
                case 9:
                    m_Options.name = strRecv;
                    break;
                case 10:
                    m_Options.elements = strRecv;
                    break;
            }
        }
    }
    DealIgnore(napi_obj);
    DealSpaces(napi_obj);
}
