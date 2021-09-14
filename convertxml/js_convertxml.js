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

'use strict';
console.log("LHC...test      01");
const convertXml = requireInternal("ConvertXML");
console.log("LHC...test      02");
class ConvertXml {
    convertxmlclass;
    constructor() {
        this.convertxmlclass = new convertXml.ConvertXml();
        console.log("LHC...test      03");
    }
    convert(strXml, options) {
        let converted = this.convertxmlclass.convert(strXml, options);
        
        var ses = JSON.stringify(converted);
        console.log("LHC...test      04 ses =" + ses);
        let space = 0;
        if (converted.hasOwnProperty("spaces")) {
            space = converted.spaces;
            delete converted.spaces;
        }
        console.log("LHC...test      05");
        return JSON.stringify(converted, null, space);
    }
}

export default {
    ConvertXml : ConvertXml
}


