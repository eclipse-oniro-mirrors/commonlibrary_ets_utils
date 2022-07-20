# js_util_module子系统/组件

-   [简介](#简介)
-   [目录](#目录)
-   [说明](#说明)
    -   [接口说明](#接口说明)
    -   [使用说明](#使用说明)

-   [相关仓](#相关仓)

## 简介

UTIL接口用于字符编码TextEncoder、解码TextDecoder、帮助函数HelpFunction、基于Base64的字节编码encode和解码decode、有理数RationalNumber。TextEncoder表示一个文本编码器，接受字符串作为输入，以UTF-8格式进行编码，输出UTF-8字节流。TextDecoder接口表示一个文本解码器，解码器将字节流作为输入，输出stirng字符串。HelpFunction主要是对函数做callback化、promise化以及对错误码进行编写输出，及类字符串的格式化输出。encode接口使用Base64编码方案将指定u8数组中的所有字节编码到新分配的u8数组中或者使用Base64编码方案将指定的字节数组编码为String。decode接口使用Base64编码方案将Base64编码的字符串或输入u8数组解码为新分配的u8数组。RationalNumber有理数主要是对有理数进行比较，获取分子分母等方法。LruBuffer该算法在缓存空间不够的时候，将近期最少使用的数据替换为新数据。该算法源自这样一种访问资源的需求：近期访问的数据，可能在不久的将来会再次访问。于是最少访问的数据就是价值最小的，是最应该踢出缓存空间的数据。Scope接口用于描述一个字段的有效范围。 Scope实例的构造函数用于创建具有指定下限和上限的对象，并要求这些对象必须具有可比性。
## 目录

```
base/compileruntime/js_util_module/
├── Class:TextEncoder                   # TextEncoder类
│   ├──  new TextEncoder()              # 创建TextEncoder对象
│   ├──  encode()                       # encode方法
│   ├──  encoding                       # encoding属性
│   └──  encodeInto()                   # encodeInto方法
├── Class:TextDecoder                   # TextDecoder类
│   ├──  new TextDecoder()              # 创建TextDecoder对象
│   ├──  decode()                       # decode方法
│   ├──  encoding                       # encoding属性
│   ├──  fatal                          # fatal属性
│   └──  ignoreBOM                      # ignoreBOM属性
├── printf()                            # printf方法
├── getErrorString()                    # getErrorString方法
├── callbackWrapper()                   # callbackWrapper方法
├── promiseWrapper()                    # promiseWrapper方法
├── Class:Base64                        # Base64类
│   ├──  new Base64()                   # 创建Base64对象
│   ├──  encodeSync()                   # encodeSync方法
│   ├──  encodeToStringSync()           # encodeToStringSync方法
│   ├──  decodeSync()                   # decodeSync方法
│   ├──  encode()                       # encode方法
│   ├──  encodeToString()               # encodeToString方法
│   └──  decode()                       # decode方法
├── Class:RationalNumber                # RationalNumber类
│   ├──  new RationalNumber()           # 创建RationalNumber对象
│   ├──  createRationalFromString()     # createRationalFromString方法
│   ├──  compareTo()                    # compareTo方法
│   ├──  equals()                       # equals方法
│   ├──  valueOf()                      # valueOf方法
│   ├──  getCommonDivisor()             # getCommonDivisor方法
│   ├──  getDenominator()               # getDenominator方法
│   ├──  getNumerator()                 # getNumerator方法
│   ├──  isFinite()                     # isFinite方法
│   ├──  isNaN()                        # isNaN方法
│   ├──  isZero()                       # isZero方法
│   └──  toString()                     # toString方法
├── Class:LruBuffer                     # LruBuffer类
│   ├──  new LruBuffer()                # 创建LruBuffer对象
│   ├──  updateCapacity()               # updateCapacity方法
│   ├──  toString()                     # toString方法
│   ├──  values()                       # values方法
│   ├──  length                         # length属性
│   ├──  getCapacity()                  # getCapacity方法
│   ├──  clear()                        # clear方法
│   ├──  getCreateCount()               # getCreateCount方法
│   ├──  getMissCount()                 # getMissCount方法
│   ├──  getRemovalCount()              # getRemovalCount方法
│   ├──  getMatchCount()                # getMatchCount方法
│   ├──  getPutCount()                  # getPutCount方法
│   ├──  isEmpty()                      # isEmpty方法
│   ├──  get()                          # get方法
│   ├──  put()                          # put方法
│   ├──  keys()                         # keys方法
│   ├──  remove()                       # remove方法
│   ├──  afterRemoval()                 # afterRemoval方法
│   ├──  contains()                     # contains方法
│   ├──  createDefault()                # createDefault方法
│   ├──  entries()                      # entries方法
│   └──  [Symbol.iterator]()            # Symboliterator方法
|—— Class:Scope                         # Scope类
|   ├── constructor()                   # 创建Scope对象
|   ├── toString()                      # toString方法
|   ├── intersect()                     # intersect方法
|   ├── intersect()                     # intersect方法
|   ├── getUpper()                      # getUpper方法
|   ├── getLower()                      # getLower方法
|   ├── expand()                        # expand方法
|   ├── expand()                        # expand方法
|   ├── expand()                        # expand法
|   ├── contains()                      # contains方法
|   ├── contains()                      # contains方法
|   └── clamp()                         # clamp方法
└── Class:Types                         # Types类
    ├── isAnyArrayBuffer()              # isAnyArrayBuffer方法
    ├── isArrayBufferView()             # isArrayBufferView方法
    ├── isArgumentsObject()             # isArgumentsObject方法
    ├── isArrayBuffer()                 # isArrayBuffer方法
    ├── isAsyncFunction()               # isAsyncFunction方法
    ├── isBigInt64Array()               # isBigInt64Array方法
    ├── isBigUint64Array()              # isBigUint64Array方法
    ├── isBooleanObject()               # isBooleanObject方法
    ├── isBoxedPrimitive()              # isBoxedPrimitive方法
    ├── isDataView()                    # isDataView方法
    ├── isDate()                        # isDate方法
    ├── isExternal()                    # isExternal方法
    ├── isFloat32Array()                # isFloat32Array方法
    ├── isFloat64Array()                # isFloat64Array方法
    ├── isGeneratorFunction()           # isGeneratorFunction方法
    ├── isGeneratorObject()             # isGeneratorObject方法
    ├── isInt8Array()                   # isInt8Array方法
    ├── isInt16Array()                  # isInt16Array方法
    ├── isInt32Array()                  # isInt32Array方法
    ├── isMap()                         # isMap方法
    ├── isMapIterator()                 # isMapIterator方法
    ├── isModuleNamespaceObject()       # isModuleNamespaceObject方法
    ├── isNativeError()                 # isNativeError方法
    ├── isNumberObject()                # isNumberObject方法
    ├── isPromise()                     # isPromise方法
    ├── isProxy()                       # isProxy方法
    ├── isRegExp()                      # isRegExp方法
    ├── isSet()                         # isSet方法
    ├── isSetIterator()                 # isSetIterator方法
    ├── isSharedArrayBuffer()           # isSharedArrayBuffer方法
    ├── isStringObject()                # isStringObject方法
    ├── isSymbolObject()                # isSymbolObject方法
    ├── isTypedArray()                  # isTypedArray方法
    ├── isUint8Array()                  # isUint8Array方法
    ├── isUint8ClampedArray()           # isUint8ClampedArray方法
    ├── isUint16Array()                 # isUint16Array方法
    ├── isUint32Array()                 # isUint32Array方法
    ├── isWeakMap()                     # isWeakMap方法
    └── isWeakSet()                     # isWeakSet方法
```
## 说明

### 接口说明


| 接口名 | 说明 |
| -------- | -------- |
| constructor(encoding? : string) | 构造函数，参数encoding表示编码的格式。默认utf-8, 支持gb18030, gbk, gb2312. |
| readonly encoding : string | 在TextEncoder类中，获取编码的格式，只支持UTF-8。 |
| encode(input : string) : Uint8Array | 输入stirng字符串，根据encodeing编码并输出uint8字节流。 |
| encodeInto(input : string, dest : Uint8Array) : {read : number, written : number} | 输入stirng字符串，dest表示编码后存放位置，返回一个对象，read表示已经编码的字符的个数，written表示已编码字符所占字节的大小。 |
| constructor(encoding? : string, options? : {fatal? : boolean, ignoreBOM? : boolean}) | 构造函数，第一个参数encoding表示解码的格式。第二个参数表示一些属性。属性中fatal表示是否抛出异常，ignoreBOM表示是否忽略bom标志。 |
| readonly encoding : string | 在TextDecoder类中，获取设置的解码格式。 |
| readonly fatal : boolean | 获取抛出异常的设置。 |
| readonly ignoreBOM : boolean | 获取是否忽略bom标志的设置。 |
| decode(input : Uint8Array, options?: { stream?: false }) : string | 输入要解码的数据，解出对应的string字符串。第一个参数input表示要解码的数据，第二个参数options表示一个bool标志，表示将跟随附加数据，默认为false。 |
| encodeSync(src: Uint8Array): Uint8Array; | 使用Base64编码方案将指定u8数组中的所有字节编码到新分配的u8数组中。 |
| encodeToStringSync(src: Uint8Array): string; | 使用Base64编码方案将指定的字节数组编码为String。 |
| decodeSync(src: Uint8Array \| string): Uint8Array; | 使用Base64编码方案将Base64编码的字符串或输入u8数组解码为新分配的u8数组。 |
| encode(src: Uint8Array): Promise\<Uint8Array\>; | 使用Base64编码方案将指定u8数组中的所有字节异步编码到新分配的u8数组中。 |
| encodeToString(src: Uint8Array): Promise\<string\>; | 使用Base64编码方案将指定的字节数组异步编码为String。 |
| decode(src: Uint8Array \| string): Promise\<Uint8Array\>; | 使用Base64编码方案将Base64编码的字符串或输入u8数组异步解码为新分配的u8数组。 |
| static createRationalFromString(rationalString: string): RationalNumber | 基于给定的字符串创建一个RationalNumber对象。 |
| compareTo(another: RationalNumber): number | 将当前的RationalNumber对象与给定的对象进行比较。 |
| equals(obj: object): number | 检查给定对象是否与当前 RationalNumber 对象相同。 |
| valueOf(): number | 将当前的RationalNumber对象进行取整数值或者浮点数值。 |
| static getCommonDivisor(number1: number, number2: number,): number | 获得两个指定数的最大公约数。 |
| getDenominator(): number | 获取当前的RationalNumber对象的分母。 |
| getNumerator(): number | 获取当前的RationalNumber对象的分子。 |
| isFinite(): boolean | 检查当前的RationalNumber对象是有限的。 |
| isNaN(): boolean | 检查当前RationalNumber对象是否表示非数字(NaN)值。 |
| isZero(): boolean | 检查当前RationalNumber对象是否表示零值。 |
| toString(): string | 获取当前RationalNumber对象的字符串表示形式。 |
| constructor(capacity?: number) | 创建默认构造函数用于创建一个新的LruBuffer实例，默认容量为64。 |
| updateCapacity(newCapacity: number): void | 将缓冲区容量更新为指定容量，如果 newCapacity 小于或等于 0，则抛出此异常。 |
| toString(): string | 返回对象的字符串表示形式，输出对象的字符串表示  |
| values(): V[] | 获取当前缓冲区中所有值的列表，输出按升序返回当前缓冲区中所有值的列表，从最近访问到最近最少访问。 |
| length: number | 代表当前缓冲区中值的总数，输出返回当前缓冲区中值的总数。 |
| getCapacity(): number | 获取当前缓冲区的容量，输出返回当前缓冲区的容量。 |
| clear(): void | 从当前缓冲区清除键值对，清除键值对后，调用afterRemoval()方法依次对其执行后续操作。 |
| getCreateCount(): number | 获取createDefault()返回值的次数,输出返回createDefault()返回值的次数。 |
| getMissCount(): number | 获取查询值不匹配的次数，输出返回查询值不匹配的次数。 |
| getRemovalCount(): number | 获取从缓冲区中逐出值的次数，输出从缓冲区中驱逐的次数。 |
| getMatchCount​(): number | 获取查询值匹配成功的次数，输出返回查询值匹配成功的次数。 |
| getPutCount(): number | 获取将值添加到缓冲区的次数，输出返回将值添加到缓冲区的次数。 |
| isEmpty(): boolean | 检查当前缓冲区是否为空，输出如果当前缓冲区不包含任何值，则返回 true 。 |
| get(key: K) : V \| undefined | 表示要查询的键，输出如果指定的键存在于缓冲区中，则返回与键关联的值；否则返回undefined。 |
| put(key: K , value: V): V | 将键值对添加到缓冲区，输出与添加的键关联的值；如果要添加的键已经存在，则返回原始值，如果键或值为空，则抛出此异常。 |
| keys(): K[ ] | 获取当前缓冲区中值的键列表，输出返回从最近访问到最近最少访问排序的键列表。 |
| remove​(key: K): V \| undefined |  从当前缓冲区中删除指定的键及其关联的值。 |
| afterRemoval(isEvict: boolean, key: K, value : V, newValue : V):void | 删除值后执行后续操作。 |
| contains(key: K): boolean | 检查当前缓冲区是否包含指定的键，输出如果缓冲区包含指定的键，则返回 true 。 |
| createDefault(key: K): V | 如果未计算特定键的值，则执行后续操作，参数表示丢失的键,输出返回与键关联的值。 |
| entries(): [K,V] | 允许迭代包含在这个对象中的所有键值对。每对的键和值都是对象。 |
| \[Symbol.iterator\](): [K,V] | 返回以键值对得形式得一个二维数组。 |
| constructor(lowerObj: ScopeType, upperObj : ScopeType) | 创建并返回一个Scope对象，用于创建指定下限和上限的作用域实例的构造函数。 |
| toString(): string | 该字符串化方法返回一个包含当前范围的字符串表示形式。 |
| intersect(range: Scope): Scope | 获取给定范围和当前范围的交集。 |
| intersect(lowerObj: ScopeType, upperObj: ScopeType): Scope | 获取当前范围与给定下限和上限范围的交集。 |
| getUpper(): ScopeType | 获取当前范围的上限。 |
| getLower(): ScopeType | 获取当前范围的下限。 |
| expand(lowerObj: ScopeType, upperObj:  ScopeType): Scope | 创建并返回包括当前范围和给定下限和上限的并集。 |
| expand(range: Scope): Scope | 创建并返回包括当前范围和给定范围的并集。 |
| expand(value: ScopeType): Scope | 创建并返回包括当前范围和给定值的并集。 |
| contains(value: ScopeType): boolean | 检查给定value是否包含在当前范围内。 |
| contains(range: Scope): boolean | 检查给定range是否在当前范围内。 |
| clamp(value: ScopeType): ScopeType | 将给定value限定到当前范围内。 |
| function printf(format: string, ...args: Object[]): string | printf()方法使用第一个参数作为格式字符串（其可以包含零个或多个格式说明符）来返回格式化的字符串。 |
| function getErrorString(errno: number): string | getErrorString()方法使用一个系统的错误数字作为参数，用来返回系统的错误信息。 |
| function callbackWrapper(original: Function): (err: Object, value: Object) => void | 参数为一个采用 async 函数（或返回 Promise 的函数）并返回遵循错误优先回调风格的函数，即将 (err, value) => ... 回调作为最后一个参数。 在回调中，第一个参数将是拒绝原因（如果 Promise 已解决，则为 null），第二个参数将是已解决的值。 |
| function promiseWrapper(original: (err: Object, value: Object) => void): Object | 参数为采用遵循常见的错误优先的回调风格的函数（也就是将 (err, value) => ... 回调作为最后一个参数），并返回一个返回 promise 的版本。 |
| isAnyArrayBuffer(value: Object): boolean | 检查输入的value是否是ArrayBuffer或SharedArrayBuffer类型。 |
| isArrayBufferView(value: Object): boolean | 检查输入的value是否是napi_int8_array或napi_uint8_array或napi_uint8_clamped_array或napi_int16_array或napi_uint16_array或napi_int32_array或napi_uint32_array或napi_float32_array或napi_float64_array数组或DataView类型。 |
| isArgumentsObject(value: Object): boolean | 检查输入的value是否是一个arguments对象类型。 |
| isArrayBuffer(value: Object): boolean | 检查输入的value是否是ArrayBuffer类型。 |
| isAsyncFunction(value: Object): boolean | 检查输入的value是否是异步函数类型。 |
| isBigInt64Array(value: Object): boolean | 检查输入的value是否是BigInt64Array数组类型。 |
| isBigUint64Array(value: Object): boolean | 检查输入的value是否是BigUint64Array数组类型。 |
| isBooleanObject(value: Object): boolean | 检查输入的value是否是一个布尔对象类型。 |
| isBoxedPrimitive(value: Object): boolean | 检查输入的value是否是Boolean或Number或String或Symbol对象类型。 |
| isDataView(value: Object): boolean | 检查输入的value是否是DataView类型。 |
| isDate(value: Object): boolean | 检查输入的value是否是Date类型。 |
| isExternal(value: Object): boolean | 检查输入的value是否是一个native External值类型。 |
| isFloat32Array(value: Object): boolean | 检查输入的value是否是Float32Array数组类型。 |
| isFloat64Array(value: Object): boolean | 检查输入的value是否是Float64Array数组类型。 |
| isGeneratorFunction(value: Object): boolean | 检查输入的value是否是一个generator函数类型。 |
| isGeneratorObject(value: Object): boolean | 检查输入的value是否是一个generator对象类型。 |
| isInt8Array(value: Object): boolean | 检查输入的value是否是Int8Array数组类型。 |
| isInt16Array(value: Object): boolean | 检查输入的value是否是Int16Array数组类型。 |
| isInt32Array(value: Object): boolean | 检查输入的value是否是Int32Array数组类型。 |
| isMap(value: Object): boolean | 检查输入的value是否是Map类型。 |
| isMapIterator(value: Object): boolean | 检查输入的value是否是Map的iterator类型。 |
| isModuleNamespaceObject(value: Object): boolean | 检查输入的value是否是Module Namespace Object对象类型。 |
| isNativeError(value: Object): boolean | 检查输入的value是否是Error类型。 |
| isNumberObject(value: Object): boolean | 检查输入的value是否是Number对象类型。 |
| isPromise(value: Object): boolean | 检查输入的value是否是Promise类型。 |
| isProxy(value: Object): boolean | 检查输入的value是否是Proxy类型。 |
| isRegExp(value: Object): boolean | 检查输入的value是否是RegExp类型。 |
| isSet(value: Object): boolean | 检查输入的value是否是Set类型。 |
| isSetIterator(value: Object): boolean | 检查输入的value是否是Set的iterator类型。 |
| isSharedArrayBuffer(value: Object): boolean | 检查输入的value是否是SharedArrayBuffer类型。 |
| isStringObject(value: Object): boolean | 检查输入的value是否是一个String对象类型。 |
| isSymbolObject(value: Object): boolean | 检查输入的value是否是一个Symbol对象类型。 |
| isTypedArray(value: Object): boolean | 检查输入的value是否是TypedArray包含的类型。 |
| isUint8Array(value: Object): boolean | 检查输入的value是否是Uint8Array数组类型。 |
| isUint8ClampedArray(value: Object): boolean | 检查输入的value是否是Uint8ClampedArray数组类型。 |
| isUint16Array(value: Object): boolean | 检查输入的value是否是Uint16Array数组类型。 |
| isUint32Array(value: Object): boolean | 检查输入的value是否是Uint32Array数组类型。 |
| isWeakMap(value: Object): boolean | 检查输入的value是否是WeakMap类型。 |
| isWeakSet(value: Object): boolean | 检查输入的value是否是WeakSet类型。 |

printf中每个说明符都替换为来自相应参数的转换后的值。 支持的说明符有:
| 式样化字符 | 式样要求 |
| -------- | -------- |
|    %s:  | String 将用于转换除 BigInt、Object 和 -0 之外的所有值。|
|    %d:  | Number 将用于转换除 BigInt 和 Symbol 之外的所有值。|
|    %i:  | parseInt(value, 10) 用于除 BigInt 和 Symbol 之外的所有值。|
|    %f:  | parseFloat(value) 用于除 Symbol 之外的所有值。|
|    %j:  | JSON。 如果参数包含循环引用，则替换为字符串 '[Circular]'。|
|    %o:  | Object. 具有通用 JavaScript 对象格式的对象的字符串表示形式。类似于具有选项 { showHidden: true, showProxy: true } 的 util.inspect()。这将显示完整的对象，包括不可枚举的属性和代理。|
|    %O:  | Object. 具有通用 JavaScript 对象格式的对象的字符串表示形式。类似于没有选项的 util.inspect()。 这将显示完整的对象，但不包括不可枚举的属性和代理。|
|    %c:  | 此说明符被忽略，将跳过任何传入的 CSS 。|
|    %%:  | 单个百分号 ('%')。 这不消耗待式样化参数。|

### 使用说明
各接口使用方法如下：

1.readonly encoding()

```
import util from '@ohos.util'
var textEncoder = new util.TextEncoder();
var getEncoding = textEncoder.encoding();
```
2.encode()
```
import util from '@ohos.util'
var textEncoder = new util.TextEncoder();
var result = textEncoder.encode('abc');
```
3.encodeInto()
```
import util from '@ohos.util'
var textEncoder = new util.TextEncoder();
var obj = textEncoder.encodeInto('abc', dest);
```
4.textDecoder()
```
import util from '@ohos.util'
var textDecoder = new util.textDecoder("utf-16be", {fatal : true, ignoreBOM : false});
```
5.readonly encoding()
```
import util from '@ohos.util'
var textDecoder = new util.textDecoder("utf-16be", {fatal : true, ignoreBOM : false});
var getEncoding = textDecoder.encoding();
```
6.readonly fatal()
```
import util from '@ohos.util'
var textDecoder = new util.textDecoder("utf-16be", {fatal : true, ignoreBOM : false});
var fatalStr = textDecoder.fatal();
```
7.readonly ignoreBOM()
```
import util from '@ohos.util'
var textDecoder = new util.textDecoder("utf-16be", {fatal : true, ignoreBOM : false});
var ignoreBom = textDecoder.ignoreBOM();
```
8.decode()
```
import util from '@ohos.util'
var textDecoder = new util.textDecoder("utf-16be", {fatal : true, ignoreBOM : false});
var result = textDecoder.decode(input, {stream : true});
```
9.printf()
```
import util from '@ohos.util'
var format = "%%%o%%%i%s";
var value =  function aa(){};
var value1 = 1.5;
var value2 = "qwer";
var result = util.printf(format,value,value1,value2);
```
10.getErrorString()
```
import util from '@ohos.util'
var errnum = 13;
var result = util.getErrorString(errnum);
```
11.callbackWrapper()
```
import util from '@ohos.util'
async function promiseFn() {
    return Promise.resolve('value');
};
var cb = util.callbackWrapper(promiseFn);
cb((err, ret) => {
    expect(err).strictEqual(null);
    expect(ret).strictEqual('value');
})
```
12.promiseWrapper()
```
import util from '@ohos.util'
function aysnFun(str1, str2, callback) {
    if (typeof str1 === 'string' && typeof str1 === 'string') {
        callback(null, str1 + str2);
    } else {
        callback('type err');
    }
}
let newPromiseObj = util.promiseWrapper(aysnFun)("Hello", 'World');
newPromiseObj.then(res => {
    expect(res).strictEqual('HelloWorld');
})
```
13.encodeSync()
```
import util from '@ohos.util'
var that = new util.Base64();
var array = new Uint8Array([115,49,51]);
var result = that.encodeSync(array);
```
14.encodeToStringSync()
```
import util from '@ohos.util'
var that = new util.Base64();
var array = new Uint8Array([115,49,51]);
var result = that.encodeToStringSync(array);
```
15.decodeSync()
```
import util from '@ohos.util'
var that = new util.Base64()
var buff = 'czEz';
var result = that.decodeSync(buff);

```
16.encode()
```
import util from '@ohos.util'
var that = new util.Base64()
var array = new Uint8Array([115,49,51]);
await that.encode(array).then(val=>{
})
done()
```
17.encodeToString()
```
import util from '@ohos.util'
var that = new util.Base64()
var array = new Uint8Array([115,49,51]);
await that.encodeToString(array).then(val=>{
})
done()
```
18.decode()
```
import util from '@ohos.util'
var that = new util.Base64()
var buff = 'czEz';
await that.decode(buff).then(val=>{
})
done()
```
19.createRationalFromString()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(0, 0);
var res = pro.createRationalFromString("-1:2");
var result1 = res.valueOf();
```
20.compareTo()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(2, 1);
var proc = new util.RationalNumber(3, 4);
var res = pro.compareTo(proc);
```
21.equals()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(2, 1);
var proc = new util.RationalNumber(3, 4);
var res = pro.equals(proc);
```
22.valueOf()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(2, 1);
var res = pro.valueOf();
```
23.getCommonDivisor()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(0, 0);
var res = pro.getCommonDivisor(4, 8);
```
24.getDenominator()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(2, 1);
var res = pro.getDenominator();
```
25.getNumerator()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(-2, 1);
var res = pro.getNumerator();
```
26.isFinite()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(-2, 1);
var res = pro.isFinite();
```
27.isNaN()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(-2, 1);
var res = pro.isNaN();
```
28.isZero()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(-2, 1);
var res = pro.isZero();
```
29.toString()
```
import util from '@ohos.util'
var pro = new util.RationalNumber(-2, 1);
var res = pro.toString();
```
30.updateCapacity()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
var result = pro.updateCapacity(100);
```
31.toString()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
pro.get(2);
pro.remove(20);
var result = pro.toString();
```
32.values()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
pro.put(2,"anhu");
pro.put("afaf","grfb");
var result = pro.values();
```
33.length
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
pro.put(1,8);
var result = pro.length;
```
34.getCapacity()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
var result = pro.getCapacity();
```
35.clear()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
pro.clear();
```
36.getCreateCount()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(1,8);
var result = pro.getCreateCount();
```
37.getMissCount()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
pro.get(2)
var result = pro.getMissCount();
```
38.getRemovalCount()
```

import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
pro.updateCapacity(2);
pro.put(50,22);
var result = pro.getRemovalCount();

```
39.getMatchCount()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
pro.get(2);
var result = pro.getMatchCount();
```
40.getPutCount()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
var result = pro.getPutCount();
```
41.isEmpty()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
var result = pro.isEmpty();
```
42.get()

```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
var result = pro.get(2);
```
43.put()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
var result = pro.put(2,10);
```
44.keys()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
var result = pro.keys();
```
45.remove()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
var result = pro.remove(20);
```
46.contains()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
var result = pro.contains(20);
```
47.createDefault()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
var result = pro.createDefault(50);
```
48.entries()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro.put(2,10);
var result = pro.entries();
```
49.\[Symbol.iterator\]()
```
import util from '@ohos.util'
var pro = new util.LruBuffer();
pro .put(2,10);
var result = pro[symbol.iterator]();
```
50.afterRemoval()
```
import util from '@ohos.util'
var arr = [ ];
class ChildLruBuffer extends util.LruBuffer
{
    constructor()
    {
        super();
    }
    static getInstance()
    {
        if(this.instance ==  null)
        {
            this.instance = new ChildLruBuffer();
        }
        return this.instance;
    }
    afterRemoval(isEvict, key, value, newValue)
    {
        if (isEvict === false)
        {
            arr = [key, value, newValue];
        }
    }
}
ChildLruBuffer.getInstance().afterRemoval(false,10,30,null)
```
Scope接口中构造新类，实现compareTo方法。

```
class Temperature {
    constructor(value) {
        this._temp = value;
    }
    compareTo(value) {
        return this._temp >= value.getTemp();
    }
    getTemp() {
        return this._temp;
    }
    toString() {
        return this._temp.toString();
    }
}
```

51.constructor()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var range = new Scope(tempLower, tempUpper);
```

52.toString()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var range = new Scope(tempLower, tempUpper);
var result = range.toString() // => [30,40]
```

53.intersect()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var range = new Scope(tempLower, tempUpper);
var tempMiDF = new Temperature(35);
var tempMidS = new Temperature(39);
var rangeFir = new Scope(tempMiDF, tempMidS);
var result = range.intersect(rangeFir)  // => [35,39]
```

54.intersect()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var tempMiDF = new Temperature(35);
var tempMidS = new Temperature(39);
var range = new Scope(tempLower, tempUpper);
var result = range.intersect(tempMiDF, tempMidS)  // => [35,39]
```

55.getUpper()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var range = new Scope(tempLower, tempUpper);
var result = range.getUpper() // => 40
```

56.getLower()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var range = new Scope(tempLower, tempUpper);
var result = range.getLower() // => 30
```

57.expand()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var tempMiDF = new Temperature(35);
var tempMidS = new Temperature(39);
var range = new Scope(tempLower, tempUpper);
var result = range.expand(tempMiDF, tempMidS)  // => [30,40]
```

58.expand()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var tempMiDF = new Temperature(35);
var tempMidS = new Temperature(39);
var range = new Scope(tempLower, tempUpper);
var rangeFir = new Scope(tempMiDF, tempMidS);
var result = range.expand(rangeFir) // => [30,40]
```

59.expand()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var tempMiDF = new Temperature(35);
var range = new Scope(tempLower, tempUpper);
var result = range.expand(tempMiDF)  // => [30,40]
```

60.contains()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var tempMiDF = new Temperature(35);
var range = new Scope(tempLower, tempUpper);
var result = range.contains(tempMiDF) // => true
```

61.contains()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var range = new Scope(tempLower, tempUpper);
var tempLess = new Temperature(20);
var tempMore = new Temperature(45);
var rangeSec = new Scope(tempLess, tempMore);
var result = range.contains(rangeSec) // => true
```

62.clamp()

```
var tempLower = new Temperature(30);
var tempUpper = new Temperature(40);
var tempMiDF = new Temperature(35);
var range = new Scope(tempLower, tempUpper);
var result = range.clamp(tempMiDF) // => 35
```
63.isAnyArrayBuffer()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isAnyArrayBuffer(new ArrayBuffer([]))
```
64.isArrayBufferView()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isArrayBufferView(new DataView(new ArrayBuffer(16)));
```
65.isArgumentsObject()
```
import util from '@ohos.util'
function foo() {
        var result = proc.isArgumentsObject(arguments);
    }
var f = foo();
```
66.isArrayBuffer()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isArrayBuffer(new ArrayBuffer([]));
```
67.isAsyncFunction()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isAsyncFunction(async function foo() {});
```
68.isBigInt64Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isBigInt64Array(new Int16Array([]));
```
69.isBigUint64Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isBigUint64Array(new Int16Array([]));
```
70.isBooleanObject()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isBooleanObject(new Boolean(false));
```
71.isBoxedPrimitive()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isBoxedPrimitive(new Boolean(false));
```
72.isDataView()
```
import util from '@ohos.util'
var proc = new util.Types();
const ab = new ArrayBuffer(20);
var result = proc.isDataView(new DataView(ab));
```
73.isDate()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isDate(new Date());
```
74.isExternal()
```
import util from '@ohos.util'
const data = util.createExternalType();
var reult13 = proc.isExternal(data);
```
75.isFloat32Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isFloat32Array(new Float32Array([]));
```
76.isFloat64Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isFloat64Array(new Float64Array([]));
```
77.isGeneratorFunction()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isGeneratorFunction(function* foo() {});
```
78.isGeneratorObject()
```
import util from '@ohos.util'
var proc = new util.Types();
function* foo() {}
const generator = foo();
var result = proc.isGeneratorObject(generator);
```
79.isInt8Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isInt8Array(new Int8Array([]));
```
80.isInt16Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isInt16Array(new Int16Array([]));
```
81.isInt32Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isInt32Array(new Int32Array([]));
```
82.isMap()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isMap(new Map());
```
83.isMapIterator()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isMapIterator(map.keys());
```
84.isModuleNamespaceObject()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isModuleNamespaceObject(util);
```
85.isNativeError()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isNativeError(new TypeError());
```
86.isNumberObject()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isNumberObject(new Number(0));
```
87.isPromise()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isPromise(Promise.resolve(42));
```
88.isProxy()
```
import util from '@ohos.util'
var proc = new util.Types();
const target = {};
const proxy = new Proxy(target, {});
var result = proc.isProxy(proxy);
```
89.isRegExp()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isRegExp(new RegExp('abc'));
```
90.isSet()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isSet(new Set());
```
91.isSetIterator()
```
import util from '@ohos.util'
var proc = new util.Types();
const set = new Set();
var result = proc.isSetIterator(set.keys());
```
92.isSharedArrayBuffer()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isSharedArrayBuffer(new ArrayBuffer([]));
```
93.isStringObject()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isStringObject(new String('foo'));
```
94.isSymbolObject()
```
import util from '@ohos.util'
var proc = new util.Types();
const symbols = Symbol('foo');
var result = proc.isSymbolObject(Object(symbols));
```
95.isTypedArray()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isTypedArray(new Float64Array([]));
```
96.isUint8Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isUint8Array(new Uint8Array([]));
```
97.isUint8ClampedArray()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isUint8ClampedArray(new Uint8ClampedArray([]));
```
98.isUint16Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isUint16Array(new Uint16Array([]));
```
99.isUint32Array()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isUint32Array(new Uint32Array([]));
```
100.isWeakMap()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isWeakMap(new WeakMap());
```
101.isWeakSet()
```
import util from '@ohos.util'
var proc = new util.Types();
var result = proc.isWeakSet(new WeakSet());
```
## 相关仓

[js_util_module子系统](base/compileruntime/js_util_module-readme_zh.md)

## 许可证

Util在[Mozilla许可证](https://www.mozilla.org/en-US/MPL/)下可用，说明文档详见[说明文档](https://gitee.com/openharmony/js_util_module/blob/master/mozilla_docs.txt)。有关完整的许可证文本，有关完整的许可证文本，请参见[许可证](https://gitee.com/openharmony/js_util_module/blob/master/LICENSE)