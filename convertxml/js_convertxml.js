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
const convertXml = requireInternal("ConvertXML");
class ConvertXml {
    convertxmlclass;
    constructor() {
        this.convertxmlclass = new convertXml.ConvertXml();
    }
    convert(strXml, options) {
        strXml = DealXml(strXml);
        let converted = this.convertxmlclass.convert(strXml, options);
        let space = 0;
        if (converted.hasOwnProperty("spaces")) {
            space = converted.spaces;
            delete converted.spaces;
        }
        return JSON.stringify(converted, null, space);
    }
}

function DealXml(strXml)
{
    var idx = -1;
    var idxSec = 0;
    var idxThir = 0;
    var idxCData = 0;
    var idxCDataSec = 0;
    while ((idx = strXml.indexOf('>', idxSec)) != -1) {
        idxThir = strXml.indexOf('<', idx);
        var i = idx + 1;
        for (; i < idxThir ; i++) {
            var cXml = strXml.charAt(i);
            if (cXml != '\n' && cXml != '\v' && cXml != '\t' && cXml != ' ')
            {
                break;
            }
        }
        var j = idx + 1;
        for (; j < strXml.indexOf('<', idx) ; j++) {
            var cXml = strXml.charAt(j);
            if (i != idxThir) {
                switch (cXml) {
                case '\n':
                    strXml = strXml.substring(0, j) + '\\n' + strXml.substring(j + 1);
                    break;
                case '\v':
                    strXml = strXml.substring(0, j) + '\\v' + strXml.substring(j + 1);
                    break;
                case '\t':
                    strXml = strXml.substring(0, j) + '\\t' + strXml.substring(j + 1);
                    break;
                }
            } else {
                strXml = strXml.substring(0, j) + strXml.substring(j + 1);
                --j;
            }
        }
        if (strXml.indexOf('<', idx) != -1) {
            idxCData = strXml.indexOf('<![CDATA', idxCDataSec);
            idxSec = strXml.indexOf('<', idx);
            if (idxSec == idxCData) {
                idxSec = strXml.indexOf(']]', idxCData);
                idxCDataSec = idxSec;
            }
        }
        else {
            break;
        }
    }
    return strXml;
}

export default {
    ConvertXml : ConvertXml
}


