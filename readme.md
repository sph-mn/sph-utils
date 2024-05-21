# sph-utils

basic file, shell and text manipulation utilities.
compared to the [sph-script](https://github.com/sph-mn/sph-script) collection of scripts, the programs here run more efficiently and are written in c. especially when the commands are needed frequently or for many files, these utilities are much faster compared to shell scripts.

# dependencies
* c 2011 standard library (for example [musl-libc](https://musl.libc.org/) or [glibc](https://www.gnu.org/software/libc/))
* posix 2008 features (for example linux or freebsd)
* for the provided compile script: shell, gcc

# installation
~~~
sh ./exe/compile
~~~
this should create executables under `exe/compiled/`. these executables can copied anywhere as is because they do not depend on shared libraries. for example, the programs can be copied or symlinked into `/usr/bin` (as root) after which they should be available as commands on the command-line.

# splice
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

# is-empty-directory
~~~
arguments: directory ...
~~~

the exit code will be 0 (success) if all given directories are empty, and 1 if any is not empty.
this program has no options and displays no output.

* it easily handles directories with many files. any errors lead to a non-empty result
* a directory is considered empty when it contains less than three entries - only the standard references "." and ".."

# unique-name
~~~
arguments: path
~~~

displays the given path if it does not exist. otherwise, increments a counter suffix path.n until a file name that does not exist is found, then displays the modified path.

one use case is to rename or move files without overwriting files that have the same name.

# stemname
removes the last dot-separated filename extension from the string.

~~~
arguments: filename
~~~

examples:
~~~
$ remove-extension test.tar.gz
test.tar

$ remove-extension test.tar
test

$ remove-extension test
test

$ remove-extension .hidden
.hidden
~~~

maximum string length is 65535 characters.

# line-length
reads from standard input and writes the byte character count of each line to standard output. unicode multibyte characters are counted as multiple characters.
this is a performance optimized version. example run time: 5335407 lines, 0m6.532s.

for comparison, the "wc" utility needs to be called per line, which is much slower.

# rename-lowercase
# file-size-sum

# see also
* [sph-script](https://github.com/sph-mn/sph-script)
* [sbase](https://git.suckless.org/sbase/file/README.html)