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
import platform
import argparse
import subprocess

def run_command(in_cmd):
    print(" ".join(in_cmd))
    proc = subprocess.Popen(in_cmd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,
                          universal_newlines=True,
                          shell=False)
    stdout, stderr = proc.communicate()
    if stdout != "":
        print(stdout)
        exit(1)

if __name__ == '__main__':

    BUILD_PATH = os.path.abspath(os.path.join(os.getcwd(), "../.."))
    os.chdir("%s/base/compileruntime/js_api_module/convertxml" % BUILD_PATH)

    PARSER_INST = argparse.ArgumentParser()
    PARSER_INST.add_argument('--dst-file',
                        help='the converted target file')
    INPUT_ARGUMENTS = PARSER_INST.parse_args()


    NODE_PATH = '../../../../prebuilts/build-tools/common/nodejs/\
node-v12.18.4-linux-x64/bin/node'
    TSC_PATH = '../../../../ark/ts2abc/ts2panda/node_modules/\
typescript/bin/tsc'
    CMD_INST = [NODE_PATH, TSC_PATH]
    run_command(CMD_INST)

    CMD_INST = ['cp', "-r", './out/js_convertxml.js', INPUT_ARGUMENTS.dst_file]
    run_command(CMD_INST)

    CMD_INST = ['rm', "-rf", './out']
    run_command(CMD_INST)
    exit(0)