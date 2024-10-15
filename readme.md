# sph-utils

basic file, shell and text manipulation utilities.
compared to the [sph-script](https://github.com/sph-mn/sph-script) collection of scripts, the programs here run more efficiently and are written in c. especially when the commands are used frequently, these utilities are much faster compared to shell scripts.

# dependencies
* c 2011 standard library (for example [musl-libc](https://musl.libc.org/) or [glibc](https://www.gnu.org/software/libc/))
* posix 2008 features (for example linux or freebsd)
* for the provided compile script: shell, gcc

# installation
~~~
sh ./exe/compile
~~~
this should create executables under `exe/compiled/`. these executables can copied anywhere as is because they do not depend on shared libraries. for example, the programs can be copied or symlinked into `/usr/bin` (as root) after which they should be available as commands on the command-line as long as the executable bit is set.

# main utilities
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

example:
~~~
find . | lines-filter word1 word2
~~~

## rate
move files into 1/2/3/n rating directories. the most efficient way to sort and access large numbers of files by quality or relevancy.

this program sorts files by moving them into or between numerically named directories. it first searches upwards in the directory structure to check if a numeric directory exists in the path. if found, only the file hierarchy under that number is moved into a directory with the same number. if no numeric directory exists in the path, a numeric directory is created in the current working directory, and the specified paths are moved under it.

### usage
~~~
arguments: [-m] [+-]rating path ...
~~~

### behavior
~~~
cwd: /
rate 2 /a/0/b/c -> /a/2/b/c
~~~

~~~
cwd: /
rate 2 /a/b/c -> /2/a/b/c
~~~

~~~
cwd: /a/b
rate 2 /a/b/c -> /a/b/2/c
~~~

### custom thunar right-click-menu actions
add via the thunar gui, under "edit -> add custom actions..." using command patterns like this:
~~~
rate 1 %F
~~~

## rename-lowercase
~~~
arguments: path ...
~~~

replaces ascii uppercase characters in the file basename and renames the file if replacements occurred.
the full old and new path are written to standard output.

## splice
~~~
arguments: directory ...
description
  merge the files of the given directories with their parent directories.
  files with duplicate names are kept by renaming.
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

displays the given path if it does not exist. otherwise, increments a counter suffix path.n until a file name that does not exist is found, then displays the modified path.

one use case is to rename or move files without overwriting files that have the same name.

# additional utilities
## dcat
depends on linux (SYS_getdents64) and, for recursive listing, a filesystem with d_type support (for example ext4/3/2).
lists directory entries, and optionally all sub-directory entries, fast.

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