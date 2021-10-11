#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2021 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import os

if __name__ == '__main__':
    
    build_path = os.path.abspath(os.path.join(os.getcwd(), "../.."))
    os.chdir("%s/base/compileruntime/js_api_module/url" % build_path)
    os.system('../../../../developtools/ace-ets2bundle/compiler/node_modules/typescript/bin/tsc')
    
    if os.access("../../../../out/ohos-arm64-release", os.F_OK):
        os.system('cp -r ./out/js_url.js ../../../../out/ohos-arm64-release/obj/base/compileruntime/js_api_module/url/js_url.js')
        
    if os.access("../../../../out/ohos-arm-release", os.F_OK):
        os.system('cp -r ./out/js_url.js ../../../../out/ohos-arm-release/obj/base/compileruntime/js_api_module/url/js_url.js')
        
    os.system('rm -rf ./out')