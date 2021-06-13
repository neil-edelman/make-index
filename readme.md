# Recursor\.c #

## make\-index ##

 * [Description](#user-content-preamble)
 * [Struct, Union, and Enum Definitions](#user-content-tag): [Recursor](#user-content-tag-6d9a56d2)
 * [Function Summary](#user-content-summary)
 * [Function Definitions](#user-content-fn)
 * [License](#user-content-license)

## <a id = "user-content-preamble" name = "user-content-preamble">Description</a> ##

`Recursor` is the main part of `make-index`, a content management system that generates static content, \(mostly `index.html`,\) on all the directories rooted at the directory specified by `--directory` or `-d`\. It is based on a template file, `.index.html` and `.newsfeed.rss`, which are changeable\. Also included are files to summarize the directory structure for a `xml` site map, compatible with Google, and any `.news` for an `rss` feed\.

There should be an `example` directory that has a bunch of files in it\. Run `bin/make-index -d example/`; it should make a webpage out of the directory structure and `.index.html`, open `example/index.html` after running to see\.

If the `.index.html` file exists in the `<directory>`, prints `index.html` recursively; overwrites any `index.html` on all the directories rooted at `<directory>`; if the `.sitemap.xml` file exists in `<directory>`, prints \(and overwrites\) an index called `sitemap.xml`; if the `.newsfeed.rss` file exists in `<directory>`, prints \(and overwrites\) to `newsfeed.rss` all the `.news` files \(if there are any\.\)

Treats `.d` as a description of the file without the `.d`; if this is an empty text\-file or a zero\-byte file, it skips over this file\. treats `index.d` as a description of the directory; treats `content.d` as an in\-depth description of the directory, replacing `index.d` when in the directory, when it exists; treats `.d.jpg` as a image that will go with the description; treats `.news` as a newsworthy item; the format of this file is ISO 8601 date \(YYYY\-MM\-DD,\) next line title; treats `.link` as a link with the href in the file\.

`.index.html`, `.sitemap.xml`, `.newsfeed.rss`, see `Parser` for recognised symbols\. Assumes `..` is the parent directory, `.` is the current directory, and `/` is the directory separator; works for UNIX, MacOS, and Windows\. If this is not the case, the constants are in `Files.c`\.



 * Author:  
   Neil
 * Standard:  
   POSIX\.1
 * Caveat:  
   Don't have <directory> be an argument; just do it in the current\. Borrow `Cdoc` parser for the `.d` files\. \(`lex` is pretty good\.\) Encoding is an issue; especially the newsfeed, which requires 7\-bit\. It's not robust; eg @\(files\)\{@\(files\)\{Don't do this\.\}\}\. Simplify the command line arguments\.


## <a id = "user-content-tag" name = "user-content-tag">Struct, Union, and Enum Definitions</a> ##

### <a id = "user-content-tag-6d9a56d2" name = "user-content-tag-6d9a56d2">Recursor</a> ###

<code>struct <strong>Recursor</strong>;</code>

Exported to the rest of the project _via_ a pointer\.



## <a id = "user-content-summary" name = "user-content-summary">Function Summary</a> ##

<table>

<tr><th>Modifiers</th><th>Function Name</th><th>Argument List</th></tr>

<tr><td align = right>struct Recursor *</td><td><a href = "#user-content-fn-6d9a56d2">Recursor</a></td><td>idx, map, news</td></tr>

<tr><td align = right>void</td><td><a href = "#user-content-fn-16f63ff7">Recursor_</a></td><td></td></tr>

<tr><td align = right>int</td><td><a href = "#user-content-fn-9dbb04a0">RecursorGo</a></td><td></td></tr>

</table>



## <a id = "user-content-fn" name = "user-content-fn">Function Definitions</a> ##

### <a id = "user-content-fn-6d9a56d2" name = "user-content-fn-6d9a56d2">Recursor</a> ###

<code>struct Recursor *<strong>Recursor</strong>(const char *<em>idx</em>, const char *<em>map</em>, const char *<em>news</em>)</code>

Constructor\.

 * Parameter: _idx_  
   File name of the prototype index file, eg, "\.index\.html"\.
 * Parameter: _map_  
   File name of the prototype map file, eg, "\.sitmap\.xml"\.
 * Parameter: _news_  
   File name of the prototype news file, eg, "\.newsfeed\.rss"\.




### <a id = "user-content-fn-16f63ff7" name = "user-content-fn-16f63ff7">Recursor_</a> ###

<code>void <strong>Recursor_</strong>(void)</code>

Destructor\.



### <a id = "user-content-fn-9dbb04a0" name = "user-content-fn-9dbb04a0">RecursorGo</a> ###

<code>int <strong>RecursorGo</strong>(void)</code>

Actually does the recursion\.





## <a id = "user-content-license" name = "user-content-license">License</a> ##

2000, 2012 Neil Edelman, distributed under the terms of the [GNU General Public License 3](https://opensource.org/licenses/GPL-3.0)\.



