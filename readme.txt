Copyright (C) 2008, 2012, 2013 Neil Edelman, see copying.txt.
neil dot edelman each mail dot mcgill dot ca

Version 0.8.

Usage: MakeIndex <directory>

MakeIndex is a static compiler that generates content
(index.html, mostly) on all the directories rooted at the
directory specified by the argument, <directory>, based on a
template file in <directory>. Also included are files to
summarise the directory structure for a xml site map and any
news for an rss feed.

There should be an <example> directory that has a bunch of
files in it. Run <./MakeIndex example/> (from the command
line;) it should make a webpage out of the directory
structure and <.index.html>, open example/index.html after
running to see.

* If the <.index.html> file exists in the <directory>,
prints <index.html> recursively; overwrites any index.html
on all the directories rooted at <directory>.
* If the <.sitemap.xml> file exists in <directory>,
prints (and overwrites) an index called <sitemap.xml>.
* If the <.newsfeed.rss> file exists in <directory>,
prints (and overwrites) to <newsfeed.rss> all the <.news>
files (if there are any.)

* Treats <.d> as a description of the file without the <.d>.
If this is an empty text-file or a zero-byte file, it skips over this file.
* Treats <index.d> as a description of the directory.
* Treats <content.d> as an in-depth description of the directory,
replacing <index.d> when in the directory.
* Treats <.d.jpg> as a image that will go with the description.
* Treats <.news> as a newsworthy item; ISO 8601 date (YYYY-MM-DD,) next line title.
* Treats <.link> as a link with the href in the file.

<.index.html>, <.sitemap.xml>, <.newsfeed.rss> recognised symbols,

* @(now) - prints the date and the time in UTC;
* @(date) - prints the date on the news file;
* @(title) - prints the second line in the .news;
* @(content) - prints content.d, or if it is missing, index.d;
* @(files){} - (only in index) repeats for all the files in the directory,
	* @(filealt) - prints Dir or File;
	* @(filehref) - prints the filename or the .link;
	* @(fileicon) - prints .d.jpeg, root dir.jpeg, or file.jpeg;
	* @(filename) - prints the file name;
	* @(filesize) - prints the file size, if it exits;
* @(news) - prints the contents of the file (as a text file;)
* @(newsname) - prints the filename of the news story;
* @(pwd){} - (only in index) prints the directory, the argument is the separator; eg, @(pwd){/};
* @(root){} - (only in index) prints ..<argument> all the way
up to the root; eg, @root{/}.
* ~ on a line by itself is recognised in .newsfile.rss and .sitemap.xml
as a <head>~<body>~<tail>, there should be two of them.

I use this for my webpage, but I have a Christmas wish list that will
probably never get done, eg,

* I would really like to have a subset of LaTeX converted into html
for the .d files;
* encoding is an issue, especially the newsfeed (which I believe needs
to be 7bit;) I need to build a unicode-to-html converter;
* it's not robust; eg @(files){@(files){Don't do this.}}.

If you downloaded the source and have GCC, type <make>; otherwise
you're on your own. It should be entirely POSIX compatible.

Assumes .. is the parent directory, . is the current directory, and
/ is the directory separator; works for UNIX, MacOS, Windows.
If this is not the case, the constants are in Files.c.

Changes

0.7 -- 0.8
Okay, strcasecmp instead of strcmp to sort (case-insensitive sort.)

0.6 -- 0.7
Fixed bug: <something>.d<something>.d used to show up in the page
because the first d<something> said "I do want to show in the list"
without checking the second.
