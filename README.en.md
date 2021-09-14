# js_api_module
# Introduction to URL
The URL interface is used to parse, construct, normalize, and encode URLs. The URL constructor creates a new URL object. In order to make changes to the resolved components of the URL or to the URL.

Interface introduction

1.new URL(url: string,base?:string|URL)

Create and return a URL object that references the URL specified by the absolute URL string, the relative URL string, and the basic URL string.

2.tostring():string;

The stringification method returns a USVString containing the complete URL. It is equivalent to the read-only URL.href.

3.toJSON():string;

This method returns a USVString, which contains a serialized URL version.

The usage of each interface is as follows:

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


####2nd. Introduction to URLSreachParams

The URLSearchParams interface defines some practical methods to process URL query strings.

Interface introduction

1.new URLSearchParams()

The URLSearchParams() constructor has no parameters. This method creates and returns a new URLSearchParams object. The beginning'?' character will be ignored.

2.new URLSearchParams(string)

The input parameter of URLSearchParams(string) constructor is string data type.

3.new URLSearchParams(obj)

The input parameter of URLSearchParams(obj) constructor is obj data type.

4.new URLSearchParams(iterable)

The input parameter of URLSearchParams(iterable) constructor is iterable data type.

5.isHas(name: string): boolean

Retrieve whether the searchParams object contains name. If yes, it returns true, otherwise it returns false.

6.set(name: string, value string): void

Retrieve whether the searchParams object contains a key-value pair whose key is name.
If not, add the key-value pair, if any, modify the value corresponding to the first key in the object, and delete the remaining key-value pairs whose key is name.

7.sort(): void

According to the Unicode code point of the key, sort all key/value pairs contained in this object and return undefined.

8.toString(): string

According to the searchParams object, the query string applicable in the URL is returned.

9.keys(): iterableIterator<string>

Return an iterator, which allows iterating through all the key values contained in the object.

10.values(): iterableIterator<string>

Returns an iterator, which allows iterating over all the value values contained in the object.

11.append(name: string, value: string): void

Insert the name, value key-value pair in the searchParams object.

12.delete(name: string): void

Traverse the searchParams object, find all the names, and delete the corresponding key-value pairs.

13.get(name: string): string

Retrieve the first name in the searchParams object and return the value corresponding to the name key.

14.getAll(name: string): string[]

Retrieve all names in the searchParams object and return all the values corresponding to the name key.

15.entries(): iterableIterator<[string, string]>

Returns an iterator that allows iterating through all key/value pairs contained in the searchParams object.

16.forEach(): void
The callback function is used to traverse the key-value pairs on the URLSearchParams instance object.

The usage of each interface is as follows:

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
