# lines-filter
* filters lines containing all, some, or none of the given strings. fast also for large amounts of input text. faster alternative to "grep -F".
* reads text or file paths from standard input
* matching is case-insensitive for ascii-only character search strings unless any search string contains an uppercase character (smart case).
* matches may be displayed in any order because of parallelization
* uses the aho-corasick substring search algorithm
* implementation of a fast and parallelized file processing algorithm

## usage
~~~
arguments: [options] pattern ...
description
  read from standard input and filter lines by given strings.
options
  -h  show this help text
  -f  read filesystem paths from standard input and filter file content, prefixing the file name to matching lines
  -n  negate the matching condition
  -o  matching lines need to contain only one of the patterns
  -v  display the current version number
~~~

* lines-filter -o is the same as a regular expression a|b
* lines-reject is "lines-filter -no"
