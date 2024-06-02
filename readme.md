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

# dcat
depends on linux (SYS_getdents64) and, for recursive listing, a filesystem with d_type support (for example ext4/3/2).

list directory entries, and optionally all sub-directory entries, fast.

~~~
arguments: [-r] directory ...
~~~

* accepts multiple directories as arguments
* with the option "-r" directories and sub-directories are listed recursively. the option must be the first argument if provided

## performance
20402 files under /usr after multiple runs of each program like "time find /usr":
* dcat: 0m1.657s
* gnu find: 0m1.727s
* sbase find: 0m1.851s
* fd: 0m2.348s
* eza (exa): 0m4.177s

# rename-lowercase
~~~
arguments: path ...
~~~

replaces ascii uppercase characters in the file basename and renames the file if replacements occurred.
the full old and new path are written to standard output.

# rate
  "this program sorts files by moving them into and between numerically named directories.
   first it searches upwards to see if a numeric directory name exists in path,
   if yes, then only the file hierarchy under that number is moved into a directory with the given number.
   if no numeric directory exists in path, a numeric directory is created in the current working directory and given paths are moved under there.
   examples:
   cwd: /
   rate 2 /a/0/b/c -> /a/2/b/c
   cwd: /
   rate 2 /a/b/c -> /2/a/b/c
   cwd: /a/b
   rate 2 /a/b/c -> /a/b/2/c"

# see also
* [sph-script](https://github.com/sph-mn/sph-script)
* [sbase](https://git.suckless.org/sbase/file/README.html)