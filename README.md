#### js_api_module
####一、URL简介
URL接口用于解析，构造，规范化和编码 URLs。 URL的构造函数创建新的URL对象。 以便对URL的已解析组成部分或对URL进行更改。

接口介绍

1.new URL(url: string,base?:string|URL)

创建并返回一个URL对象，该URL对象引用使用绝对URL字符串，相对URL字符串和基本URL字符串指定的URL。

2.tostring():string;

该字符串化方法返回一个包含完整 URL 的 USVString。它的作用等同于只读的 URL.href。

3.toJSON():string;

该方法返回一个USVString，其中包含一个序列化的URL版本。

各接口使用方法如下：

let b = new URL('https://developer.mozilla.org');                    // => 'https://developer.mozilla.org/'

let a = new URL( 'sca/./path/path/../scasa/jjjjj', 'http://www.example.com');     
// =>   'http://www.example.com/sca/path/scasa/jjjjj'

const url = new URL('http://10.0xFF.O400.235:8080/directory/file?query#fragment');
url.toString()    // => 'http://10.0xff.o400.235:8080/directory/file?query#fragment'   
   
const url = new URL("http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html");
url.toString()    // => 'http://[fedc:ba98:7654:3210:fedc:ba98:7654:3210]/index.html'

const url = new URL("http://username:password@host:8080/directory/file?query#fragment");
url.toString()    // => 'http://username:password@host:8080/directory/file?query#fragment'

const url = new URL("https://developer.mozilla.org/en-US/docs/Web/API/URL/toString");
url.toJSON();   // =>  'https://developer.mozilla.org/en-US/docs/Web/API/URL/toString'


####二、URLSreachParams简介

URLSearchParams 接口定义了一些实用的方法来处理 URL 的查询字符串。

接口介绍

1.new URLSearchParams()

URLSearchParams() 构造器无入参，该方法创建并返回一个新的URLSearchParams 对象。 开头的'?' 字符会被忽略。

2.new URLSearchParams(string)

URLSearchParams(string) 构造器的入参为string数据类型，该方法创建并返回一个新的URLSearchParams 对象。 开头的'?' 字符会被忽略。

3.new URLSearchParams(obj)

URLSearchParams(obj) 构造器的入参为obj数据类型，该方法创建并返回一个新的URLSearchParams 对象。 开头的'?' 字符会被忽略。

4.new URLSearchParams(iterable)

URLSearchParams(iterable) 构造器的入参为iterable数据类型，该方法创建并返回一个新的URLSearchParams 对象。 开头的'?' 字符会被忽略。

5.isHas(name: string): boolean

检索searchParams对象中是否含有name。有则返回ture，否则返回false。

6.set(name: string, value string): void

检索searchParams对象中是否含有key为name的键值对。
没有的话则添加该键值对，有的话则修改对象中第一个key所对应的value，并删除键为name的其余键值对。

7.sort(): void

根据键的Unicode代码点，对包含在此对象中的所有键/值对进行排序，并返回undefined。

8.toString(): string

根据searchParams对象,返回适用在URL中的查询字符串。

9.keys(): iterableIterator<string>

返回一个iterator，遍历器允许遍历对象中包含的所有key值。

10.values(): iterableIterator<string>

返回一个iterator，遍历器允许遍历对象中包含的所有value值。

11.append(name: string, value: string): void

在searchParams对象中插入name, value键值对。

12.delete(name: string): void

遍历searchParams对象，查找所有的name,删除对应的键值对。

13.get(name: string): string

检索searchParams对象中第一个name,返回name键对应的值。

14.getAll(name: string): string[]

检索searchParams对象中所有name,返回name键对应的所有值。

15.entries(): iterableIterator<[string, string]>

返回一个iterator，允许遍历searchParams对象中包含的所有键/值对。

16.forEach(): void
通过回调函数来遍历URLSearchParams实例对象上的键值对.

各接口使用方法如下：

let params = new URLSearchParams('foo=1&bar=2');

console.log(params.has('bar'));        // =>ture

params.set('baz', 3); 
   
params .sort();   
console.log(params .toString());      // =>bar=2&baz=3&foo=1'  

for(var key of params.keys()) {
  console.log(key);
}     
                                                      // =>bar  baz  foo
for(var value of params.values()) {
  console.log(value);
}                                                     // =>2  3  1

params.append('foo', 3);                // =>bar=2&baz=3&foo=1&foo=3

params.delete('baz');                      // => bar=2&foo=1&foo=3

params.get('foo');                          // => 1

params.getAll('foo');                      // =>[ '1', '3' ]

for(var pair of searchParams.entries()) {                   
   console.log(pair[0]+ ', '+ pair[1]);                    
}                                                   // => bar, 2   foo, 1  foo, 3

url.searchParams.forEach((value, name, searchParams) => {
  console.log(name, value, url.searchParams === searchParams);
});

// => foo 1 true
// => bar 2 true
