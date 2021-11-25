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

declare function requireInternal(s : string) : any;
const Xml = requireInternal('xml');
class XmlSerializer {
    xmlserializerclass : any;
    constructor(input:any, str:string) {
            if(typeof input !== 'object' || typeof str !== 'string' || str.length === 0) {
                throw new Error("input type err");
            }
            this.xmlserializerclass = new Xml.XmlSerializer(input, str);
            let errStr = this.xmlserializerclass.xmlSerializerError();
            if (errStr.length !== 0) {
                throw new Error(errStr);
            }
    }

    setAttributes(name: string, value: string) {
        if (typeof name !== 'string' || name.length === 0 || typeof value !== 'string' || name.length === 0 ) {
            throw new Error("name or value type err");
        }
        this.xmlserializerclass.setAttributes(name, value);
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
    addEmptyElement(name : string) {
        if (typeof name !== 'string' || name.length === 0) {
            throw new Error("name type err");
        }
        this.xmlserializerclass.addEmptyElement(name);
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }

    }
    setDeclaration() {
        this.xmlserializerclass.setDeclaration();
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
    startElement(name : string) {
        if (typeof name !== 'string' || name.length === 0) {
            throw new Error("name type err");
        }
        this.xmlserializerclass.startElement(name);
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
    endElement() {
        this.xmlserializerclass.endElement();
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
    setNamespace(prefix: string, namespace: string) {
        if (typeof prefix !== 'string' || prefix.length === 0 || typeof namespace !== 'string' || namespace.length === 0 ) {
            throw new Error("prefix or namespace type err");
        }
        this.xmlserializerclass.setNamespace(prefix, namespace);
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
    setCommnet(text : string) {
        if (typeof text !== 'string' || text.length === 0) {
            throw new Error("text type err");
        }
        this.xmlserializerclass.setCommnet(text);
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
    setCData(text : string) {
        if (typeof text !== 'string' || text.length === 0) {
            throw new Error("text type err");
        }
        this.xmlserializerclass.setCData(text);
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
    setText(text : string) {
        if (typeof text !== 'string' || text.length === 0) {
            throw new Error("text type err");
        }
        this.xmlserializerclass.setText(text);
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
    setDocType(text : string) {
        if (typeof text !== 'string' || text.length === 0) {
            throw new Error("text type err");
        }
        this.xmlserializerclass.setDocType(text);
        let errStr = this.xmlserializerclass.xmlSerializerError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    }
}

class XmlPullParser {
    xmlpullparsercalss : any;
    constructor(input:any, str:string) {
        if(typeof input !== 'object' || typeof str !== 'string' || str.length === 0) {
            throw new Error("input type err");
        }
        this.xmlpullparsercalss = new Xml.XmlPullParser(input, str);
        var err = this.xmlpullparsercalss.xmlPullParserError();
        if (err.length !== 0) {
            throw new Error(err);
        }
    }
    parse(options : any) {
        if(typeof options !== 'object') {
            throw new Error("options type err");
        }
        this.xmlpullparsercalss.parse(options);
        let errStr = this.xmlpullparsercalss.xmlPullParserError();
        if (errStr.length !== 0) {
            throw new Error(errStr);
        }
    } 
    }

export default {
    XmlSerializer : XmlSerializer,
    XmlPullParser : XmlPullParser,
}


