# sph-utils

minimalist filesystem and text utilities.

# dependencies
* c 2017 standard library (for example [musl-libc](https://musl.libc.org/) or [glibc](https://www.gnu.org/software/libc/))
* posix 2008 (for example linux or freebsd)
* for the provided compile script: shell, gcc

# installation
~~~
./exe/compile
~~~
this should create executables under `exe/compiled/`. these executables can be copied to any compatible x86_64 system running the same os or any version with a compatible kernel.

executing as root

~~~
./exe/install /
~~~
installs to /usr/bin.

~~~
./exe/install-symlink /
~~~
installs to /usr/bin as symlinks into `exe/compiled/`.

afterwards, the commands should be immediately available as commands on the command-line.

# main utilities
## drop-long-lines
~~~
arguments: [limit]
~~~

reads lines from standard input and only writes those to standard output that are shorter than limit, which is 300 by default.

## files-filter

search in text files. this can be used for extremely fast command-line filesystem search tools with shell scripts (see below).

usage:
~~~
arguments: [options] keyword ...
description
  search the contents of the given files for matching text.
  input paths are read from standard input, one per line.
  case insensitive unless a keyword contains an uppercase letter.
options
  -a  all keywords must match. this is the default
  -o  any keyword may match
  -n  negate the match result
  -m  match on whole file content instead of individual lines
  -l  print matching lines in addition to file paths
~~~

`search`
~~~
#!/bin/sh
# recursively in ".", output "path" for each file with a line containing all input strings.
# arguments: string ...
find . -type d \( -name node_modules -o -name .git -o -name cache -o -name vendor \) -prune -o -type f -print | files-filter "$@"
~~~

`searchl`
~~~
#!/bin/sh
# recursively in ".", output "path: line" for each line containing all input strings.
# arguments: string ...
find . -type d \( -name node_modules -o -name .git -o -name cache -o -name vendor \) -prune -o -type f -print | files-filter -l "$@"
~~~

examples
~~~
# search for "TODO" or "FIXME" anywhere in the current directory, show filenames + matching lines
searchl -o TODO FIXME

# require both "user" AND "auth" in the same file
searchl user auth

# find files that do NOT contain "deprecated"
search -n deprecated
~~~

## group
~~~
arguments: target file ...
~~~

move files into target directory. the directory is created if it does not exist. duplicate file names are not moved.

## is-empty-directory
~~~
arguments: directory ...
~~~

the exit code will be 0 (success) if all given directories are empty, and 1 if any is not empty.
this program has no options and displays no output.

* it easily handles directories with many files. any errors lead to a non-empty result
* a directory is considered empty when it contains less than three entries - only the standard references "." and ".."

## lines-filter
this is a simpler grep alternative that filters lines using logical and/or/not operations with one or more fixed-string search terms.

~~~
arguments: [options] string ...
  -a  matching lines must contain all strings. the default
  -n  negate
  -o  matching lines must contain at least one of the strings
~~~

it uses smartcase: the search is case-sensitive only if any character in any of the search strings is uppercase (A-Z).

examples:
~~~
find . | lines-filter word1 word2
~~~

~~~
some-command | lines-filter -o jpg png gif
~~~

### fig
lines-filter can be used to build powerful filesystem path search scripts.
for example what is called here `fig`, a name that is a combination of find and grep.

~~~
#!/bin/sh
# recursively in ".", list relative paths where the path itself contains all input strings.
# arguments: [lines-filter-option ...] string ...

find . -type d \( -name node_modules -o -name .git -o -name cache -o -name vendor \) -prune -o -print | lines-filter "$@"
~~~

it ignores a few commonly irrelevant paths.

usage:
~~~
fig src .c
~~~

## rate
move files into 1/2/3/n rating directories. a way to quickly sort and access large numbers of files by quality or relevancy.

the program sorts files by moving them into, or between, numerically named directories. it first searches upwards in the path to check if a numeric directory exists in the ancestor directory hierarchy. if found, only the directory hierarchy under that number is moved into a directory named as the target number. if no numeric directory exists in the path, a directory for the target rating is created in the current working directory and the specified paths are moved under it.

files will not be moved if a file with the same name already exists at the destination.

### usage
~~~
arguments: [-m] [+-]rating path ...
  -m  modify existing rating
~~~

examples
~~~
# rate files down by one (or up, depending on how ratings are interpreted)
rate -m +1 somepath1 somepath2

# rate file up
rate -m -1 somepath

# score zero can be used for grouping files that do not yet have a rating
rate 0 somepath1
~~~

### behavior
~~~
rate 2 /a/0/b/c -> /a/2/b/c
~~~

~~~
rate 2 /a/b/c -> /a/b/2/c
~~~

### thunar right-click-menu actions
rate can be used as a custom action on files using the file context menu.
these commands are configured in the thunar gui under "edit -> add custom actions..." with command patterns like this:
~~~
rate 1 %F
~~~

## rename-lowercase
~~~
arguments: path ...
~~~

replaces ascii uppercase characters in the file basename and renames the file if replacements occurred and the new file name does not already exist.
the full old and new path are written to standard output.

## replace-string
just replace a string in files. no regular expression or escaping nonsense.

* no arguments: show usage
* no path: recursively work on text files in the current directory
* prints changes that would be made by default
* needs "-w" option to make replacements and update files in-place

~~~
replace-string -w aa bb **/*.txt
~~~

~~~
$ replace-string
usage: replacer [-w] <pattern> <replacement> [paths...]
-w: write changes to files (instead of preview)
~~~

~~~
replace-string domains families
./path_1
## domains
## families

./path_2
## domains
## families

replace-string -w domains families
~~~

## splice
~~~
arguments: directory ...
description
  merge the files of the given directories with their parent directories.
  files with duplicate names are renamed by appending a number.
options
  -1  splice if directory contains at most one file
  -h  display this help text
  -v  display the current version number
~~~

## stemname
removes the last dot-separated filename extension from the string.
maximum string length is 65535 characters.

~~~
arguments: filename
~~~

examples:
~~~
$ stemname test.tar.gz
test.tar

$ stemname test.tar
test

$ stemname test
test

$ stemname .hidden
.hidden
~~~

## unique-name
~~~
arguments: path
~~~

displays the given path if it does not exist. otherwise, increments a counter suffix "path.n" until a file name that does not exist is found, then displays the non-existent modified path.

one use case is to rename or move files without overwriting files that have the same name.

# additional utilities
more experimental and not compiled by the default compile script.

## dcat
lists directory entries, and optionally all sub-directory entries, fast.
uses an alternative directory listing function SYS_getdents64.
depends on linux and, for recursive listing, a filesystem with d_type support (for example ext4/3/2).

~~~
arguments: [-r] directory ...
~~~

* accepts multiple directories as arguments
* with the option "-r" directories and sub-directories are listed recursively. the option must be the first argument if provided

### performance
20402 files under /usr after multiple runs of each program like "time find /usr":
* dcat: 0m1.657s
* gnu find: 0m1.727s
* sbase find: 0m1.851s
* fd: 0m2.348s
* eza (exa): 0m4.177s

as can be seen from the results, it is not significantly faster than "find" in this test case.

## line-length
reads from standard input and writes the byte character count of each line to standard output. unicode multibyte characters are counted as multiple characters.
example run time: 5335407 lines, 0m6.532s.
for comparison, the "wc" utility needs to be called per line, which can be extremely slow.

# see also
* [sph-script](https://github.com/sph-mn/sph-script)
* [sbase](https://git.suckless.org/sbase/file/README.html)