# MakeIndex\.c #

## make\-index ##

 * [Description](#user-content-preamble)
 * [License](#user-content-license)

## <a id = "user-content-preamble" name = "user-content-preamble">Description</a> ##

`MakeIndex` is the main part of `make-index`, a content management system that generates static content on all the directories based on templates rooted at the working directory\.

There should be an `example` directory that has a bunch of files in it\. Run `../bin/make-index` in the example directory; it should make a webpage out of the directory structure and the templates\.



 * Author:  
   Neil
 * Standard:  
   POSIX\.1
 * Caveat:  
   Borrow `Cdoc` parser for the `.d` files\. \(`lex` is pretty good\.\) Encoding is an issue; especially the newsfeed, which requires 7\-bit\. It's not robust; eg @\(files\)\{@\(files\)\{Don't do this\.\}\}\.


## <a id = "user-content-license" name = "user-content-license">License</a> ##

2000, 2012 Neil Edelman, distributed under the terms of the [GNU General Public License 3](https://opensource.org/licenses/GPL-3.0)\.



