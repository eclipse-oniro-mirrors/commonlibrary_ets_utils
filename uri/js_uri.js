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
const uriUtil = requireInternal("uri");

class URI {
    constructor(input) {
        if (typeof input !== 'string' || input.length === 0) {
            throw new Error("input type err");
        }
        this.uricalss = new uriUtil.uri(input);
    }
    toString() {
        return this.uricalss.toString();
    }

    equals(other) {
        return this.uricalss.equals(other.uricalss);
    }

    isAbsolute() {
        return this.uricalss.isAbsolute();
    }

    normalize() {
        return this.uricalss.normalize();
    }

    get scheme() {
        return this.uricalss.scheme;
    }

    get authority() {
        return this.uricalss.authority;
    }

    get ssp() {
        return this.uricalss.ssp;
    }

    get userinfo() {
        return this.uricalss.userinfo;
    }

    get host() {
        return this.uricalss.host;
    }

    get port() {
        return this.uricalss.port;
    }

    get path() {
        return this.uricalss.path;
    }

    get query() {
        return this.uricalss.query;
    }

    get fragment() {
        return this.uricalss.fragment;
    }
}

export default {
    URI: URI,
}
