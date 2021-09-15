# js_api_module Subsystem/Component

-   [Introduction](#Introduction)
-   [Contents](#Contents)
-   [Illustrate](#Illustrate)
    -   [Interface Description](#Interface Description)
    -   [Instructions for use](#Instructions for use)

-   [Related warehouse](#Related warehouse)

## Introduction

The interface of URL is used to parse, construct, normalize, and encode URLs. The URL constructor creates a new URL object. In order to make changes to the resolved components of the URL or to the URL. The URLSearchParams interface defines some practical methods to process URL query strings.

## Contents

```
base/compileruntime/js_api_module/
├── Class:URL                              # URL class
│   ├── new URL(input[, base])             # Create URL object
│   ├── hash                               # hash attribute
│   ├── host                               # host attribute
│   ├── hostname                           # hostname attribute
│   ├── href                               # href attribute
│   ├── origin                             # origin attribute
│   ├── password                           # password attribute
│   ├── pathname                           # pathname attribute
│   ├── port                               # port attribute
│   ├── protocol                           # protocol attribute
│   ├── search                             # search attribute
│   ├── searchParams                       # searchParams attribute
│   ├── username                           # username attribute
│   ├── toString()                         # toString method
│   └── toJSON()                           # toJSON method
└─── Class: URLSearchParams                # URLSearchParams class
    ├── new URLSearchParams()              # Create URLSearchParams object
    ├── new URLSearchParams(string)        # Create URLSearchParams object
    ├── new URLSearchParams(obj)           # Create URLSearchParams object
    ├── new URLSearchParams(iterable)      # Create URLSearchParams object
    ├── append(name, value)                # append method
    ├── delete(name)                       # delete method
    ├── entries()                          # entries method
    ├── forEach(fn[, thisArg])             # forEach method
    ├── get(name)                          # get method
    ├── getAll(name)                       # getAll method
    ├── has(name)                          # has method
    ├── keys()                             # keys method
    ├── set(name, value)                   # set method
    ├── sort()                             # sort method
    ├── toString()                         # toString method
    ├── values()                           # values method
    └── urlSearchParams[Symbol.iterator]() # Create URLSearchParams object
```

## Illustrate

### Interface Description


| Interface name | Illustrate                                                         |
| -------- | -------- |
| new URL(url: string,base?:string I URL) | Create and return a URL object that references the URL specified by the absolute URL string, the relative URL string, and the basic URL string. |
| tostring():string | The stringification method returns a USVString containing the complete URL. It is equivalent to the read-only URL.href. |
| toJSON():string | This method returns a USVString, which contains a serialized URL version. |
| new URLSearchParams() | The URLSearchParams() constructor has no parameters. This method creates and returns a new URLSearchParams object. The beginning'?' character will be ignored. |
| new URLSearchParams(string) | The input parameter of URLSearchParams(string) constructor is string data type. This method creates and returns a new URLSearchParams object. The beginning'?' character will be ignored. |
| new URLSearchParams(obj) | URLSearchParams(obj) The input parameter of the constructor is the obj data type. This method creates and returns a new URLSearchParams object. The beginning'?' character will be ignored. |
| new URLSearchParams(iterable) | URLSearchParams(iterable) The input parameter of the constructor is the iterable data type. This method creates and returns a new URLSearchParams object. The beginning'?' character will be ignored. |
| has(name: string): boolean | Retrieve whether the searchParams object contains name. If yes, it returns true, otherwise it returns false. |
| set(name: string, value string): void |  Retrieve whether the searchParams object contains a key-value pair whose key is name. If not, add the key-value pair, if any, modify the value corresponding to the first key in the object, and delete the remaining key-value pairs whose key is name. |
| sort(): void | According to the Unicode code point of the key, sort all key/value pairs contained in this object and return undefined. |
| toString(): string | According to the searchParams object, the query string applicable in the URL is returned. |
| keys(): iterableIterator<string> | Return an iterator, which allows iterating through all the key values contained in the object. |
| values(): iterableIterator<string> | Returns an iterator, which allows iterating over all the value values contained in the object. |
| append(name: string, value: string): void | Insert the name, value key-value pair in the searchParams object. |
| delete(name: string): void | Traverse the searchParams object, find all the names, and delete the corresponding key-value pairs. |
| get(name: string): string | Retrieve the first name in the searchParams object and return the value corresponding to the name key. |
| getAll(name: string): string[] | Retrieve all names in the searchParams object and return all the values corresponding to the name key. |
| entries(): iterableIterator<[string, string]> | Returns an iterator that allows iterating through all key/value pairs contained in the searchParams object. |
| forEach(): void | Through the callback function to traverse the key-value pairs on the URLSearchParams instance object. |
| urlSearchParams[Symbol.iterator] () | Returns an ES6 iterator for each name-value pair in the query string. Each item of the iterator is a JavaScript array. |

### Instructions for use

The usage of each interface is as follows:

1、new URL(url: string,base?:string|URL)
```
let b = new URL('https://developer.mozilla.org'); // => 'https://developer.mozilla.org/'

let a = new URL( 'sca/./path/path/../scasa/text', 'http://www.example.com');
// => 'http://www.example.com/sca/path/scasa/text'
```
2、tostring():string
```
const url = new URL('http://10.0xFF.O400.235:8080/directory/file?query#fragment');
url.toString() // => 'http://10.0xff.o400.235:8080/directory/file?query#fragment'
   
const url = new URL("http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html");
url.toString() // => 'http://[fedc:ba98:7654:3210:fedc:ba98:7654:3210]/index.html'

const url = new URL("http://username:password@host:8080/directory/file?query#fragment");
url.toString() // => 'http://username:password@host:8080/directory/file?query#fragment'
```
3、toJSON():string
```
const url = new URL("https://developer.mozilla.org/en-US/docs/Web/API/URL/toString");
url.toJSON(); // =>  'https://developer.mozilla.org/en-US/docs/Web/API/URL/toString'
```
4、new URLSearchParams()
```
let params = new URLSearchParams('foo=1&bar=2');
```
5、new URLSearchParams(string)
```
params = new URLSearchParams('user=abc&query=xyz');
console.log(params.get('user'));
// Prints 'abc'
```
6、new URLSearchParams(obj)
```
const params = new URLSearchParams({
    user: 'abc',
    query: ['first', 'second']
});
console.log(params.getAll('query'));
// Prints [ 'first,second' ]
```
7、new URLSearchParams(iterable)
```
let params;

// Using an array
params = new URLSearchParams([
    ['user', 'abc'],
    ['query', 'first'],
    ['query', 'second'],
]);
console.log(params.toString());
// Prints 'user = abc & query = first&query = second'
```
8、has(name: string): boolean
```
console.log(params.has('bar')); // =>ture
```
9、set(name: string, value string): void
```
params.set('baz', 3);
```
10、sort(): void
```
params .sort();
```
11、toString(): string
```
console.log(params .toString()); // =>bar=2&baz=3&foo=1'
```
12、keys(): iterableIterator<string>
```
for(var key of params.keys()) {
  console.log(key);
} // =>bar  baz  foo
```
13、values(): iterableIterator<string>
```
for(var value of params.values()) {
  console.log(value);
} // =>2  3  1
```
14、append(name: string, value: string): void
```
params.append('foo', 3); // =>bar=2&baz=3&foo=1&foo=3
```
15、delete(name: string): void
```
params.delete('baz'); // => bar=2&foo=1&foo=3
```
16、get(name: string): string
```
params.get('foo'); // => 1
```
17、getAll(name: string): string[]
```
params.getAll('foo'); // =>[ '1', '3' ]
```
18、entries(): iterableIterator<[string, string]>
```
for(var pair of searchParams.entries()) {
   console.log(pair[0]+ ', '+ pair[1]);
} // => bar, 2   foo, 1  foo, 3
```
19、forEach(): void
```
url.searchParams.forEach((value, name, searchParams) => {
  console.log(name, value, url.searchParams === searchParams);
});
// => foo 1 true
// => bar 2 true
```
20、urlSearchParams[Symbol.iterator] ()
```
const params = new URLSearchParams('foo=bar&xyz=baz');
for (const [name, value] of params) {
    console.log(name, value);
}
// Prints:
// foo bar
// xyz ba
```
## Related warehouse
[js_api_module Subsystem](https://gitee.com/OHOS_STD/js_api_module)

[base/compileruntime/js_api_module/](base/compileruntime/js_api_module-readme.md)
