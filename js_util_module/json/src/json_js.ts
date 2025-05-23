/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

const typeErrorCode = 401;
class BusinessError extends Error {
  code: number;
  constructor(msg: string) {
    super(msg);
    this.name = 'BusinessError';
    this.code = typeErrorCode;
  }
}

type TransformsFunc = (this: Object, key: string, value: Object) => Object | undefined | null;
type ReplacerType = (number | string)[] | null | TransformsFunc;

const enum BigIntMode {
  DEFAULT = 0,
  PARSE_AS_BIGINT = 1,
  ALWAYS_PARSE_AS_BIGINT = 2,
}

interface ParseOptions {
  bigIntMode?: BigIntMode;
}

export interface JSON {
  parseBigInt(text: string, reviver?: TransformsFunc, options?: ParseOptions): Object | null;
  stringifyBigInt(value: Object, replacer?: TransformsFunc, space?: string | number): string;
  stringifyBigInt(value: Object, replacer?: (number | string)[] | null, space?: string | number): string;
}

function parse(text: string, reviver?: TransformsFunc, options?: ParseOptions): Object | null {
  if (typeof text !== 'string') {
    let error = new BusinessError(`Parameter error. The type of ${text} must be string`);
    throw error;
  }
  if (reviver) {
    if (typeof reviver !== 'function') {
      let error = new BusinessError(`Parameter error. The type of ${reviver} must be a method`);
      throw error;
    }
  }

  try {
    return (JSON as unknown as JSON).parseBigInt(text, reviver, options);
  } catch (e) {
    let error = new BusinessError(`e.message: ` + (e as Error).message + `, e.name: ` + (e as Error).name);
    throw error;
  }
}

function stringfyFun(value: Object, replacer?: (this: Object, key: string, value: Object) => Object,
    space?: string | number): string {
  if (arguments.length === 1) {
    return JSON.stringify(value);
  } else {
    return JSON.stringify(value, replacer, space);
  }
}

function stringfyArr(value: Object, replacer?: (number | string)[] | null, space?: string | number): string {
  if (arguments.length === 1) {
    return (JSON as unknown as JSON).stringifyBigInt(value);
  } else {
    return (JSON as unknown as JSON).stringifyBigInt(value, replacer, space);
  }
}

function isParameterType(self: unknown): boolean {
  return (Array.isArray(self) && self.every((item) => typeof item === 'number' || typeof item === 'string') ||
  self === null || typeof self === 'function');
}

function isSpaceType(self: unknown): boolean {
  return (typeof self === 'string' || typeof self === 'number');
}

function isCirculateReference(value: Object, seenObjects: Set<Object> = new Set()): boolean {
  if (seenObjects.has(value)) {
    return true;
  }
  seenObjects.add(value);
  if (value === null || typeof value !== 'object') {
    return false;
  }
  for (const key in value) {
    if (Object.prototype.hasOwnProperty.call(value, key)) {
      const temp = value[key];
      if (temp !== null && typeof temp === 'object' &&
        (seenObjects.has(temp) || isCirculateReference(temp, seenObjects))) {      
        return true;
      }
    }
  }
  seenObjects.delete(value);
  return false;
}

function stringify(value: Object, replacer?: ReplacerType, space?: string | number): string {
  if (isCirculateReference(value)) {
    let error = new BusinessError(`Parameter error. The object circular Reference`);
    throw error;
  }
  if (replacer) {
    if (!isParameterType(replacer)) {
      let error = new BusinessError(`Parameter error. The type of ${replacer} must be a method or array`);
      throw error;
    }
    if (space) {
      if (!isSpaceType(space)) {
        let error = new BusinessError(`Parameter error. The type of ${space} must be a string or number`);
        throw error;
      }
    } else if (space === null) {
      let error = new BusinessError(`Parameter error. The type of ${space} must be a string or number`);
      throw error;
    }
  }

  try {
    if (arguments.length === 1) {
      return stringfyArr(value);
    } else {
      if (typeof replacer === 'function') {
        return stringfyFun(value, replacer, space);
      } else {
        return stringfyArr(value, replacer, space);
      }
    }
  } catch (e) {
    let error = new BusinessError((e as Error).message + `, e.name: ` + (e as Error).name);
    throw error;
  }
}

function has(value: object, key: string): boolean {
  if (typeof value !== 'object' || typeof value === 'undefined' || value === null) {
    let error = new BusinessError(`Parameter error. The type of ${value} must be object`);
    throw error;
  }
  if (value instanceof Array) {
    let error = new BusinessError(`Parameter error. The type of ${value} must be json object`);
    throw error;
  }
  if (!(typeof key === 'string' && key.length)) {
    let error = new BusinessError(`Parameter error. The type of ${key} must be string and not empty`);
    throw error;
  }
  return Object.prototype.hasOwnProperty.call(value, key);
}

function remove(value: object, key: string): void {
  if (typeof value !== 'object' || typeof value === 'undefined' || value === null) {
    let error = new BusinessError(`Parameter error. The type of ${value} must be object`);
    throw error;
  }
  if (value instanceof Array) {
    let error = new BusinessError(`Parameter error. The type of ${value} must be json object`);
    throw error;
  }
  if (!(typeof key === 'string' && key.length)) {
    let error = new BusinessError(`Parameter error. The type of ${key} must be string and not empty`);
    throw error;
  }
  if (Object.prototype.hasOwnProperty.call(value, key)) {
    delete value[key];
  }
}

export default {
  parse: parse,
  stringify: stringify,
  has: has,
  remove: remove,
};
