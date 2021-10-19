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
#

import os
import platform
import argparse
import subprocess

def run_command(cmd):
    print(" ".join(cmd))
    proc = subprocess.Popen(cmd)
    proc.wait()

if __name__ == '__main__':
    
    build_path = os.path.abspath(os.path.join(os.getcwd(), "../.."))
    os.chdir("%s/base/compileruntime/js_api_module/uri" % build_path)

    parser = argparse.ArgumentParser()
    parser.add_argument('--dst-file',
                        help='the converted target file')
    input_arguments = parser.parse_args()

    node = '../../../../prebuilts/build-tools/common/nodejs/\
node-v12.18.4-linux-x64/bin/node'
    tsc = '../../../../ark/ts2abc/ts2panda/node_modules/typescript/bin/tsc'
    cmd = [node, tsc]
    run_command(cmd)

    cmd = ['cp', "-r", './out/js_uri.js', input_arguments.dst_file]
    run_command(cmd)

    cmd = ['rm', "-rf", './out']
    run_command(cmd)