# multi-file lines process - an algorithm for fast parallel-processing of lines read from many files
* input: standard input and files via file paths read from standard input
* output: standard output

## read file paths
* use indexes instead of pointers because the data array may move when growing. indexes can also be smaller than pointers
* if using an array of pointers to data instead, the array would have to be build after all data has been read

* two buffers: data and indexes
* resize buffers as needed with a growth factor
* indexes will contain offsets to the start of paths in data
* initialize first element of indexes to 0
* loop until all data has been read from standard input
  * read block into data
  * search for newline characters in newly read block
  * store found offsets in indexes
  * replace newline character with 0 so that the paths are c strings usable with open() and similar
* return the two buffers
* paths start addresses are at (data + indexes[n - 1]) and lengths are at (indexes[n] - indexes[n - 1])

## parallel file processing
* first search does not need thread
* keep array of thread states
* iterate over thread states to limit concurrent threads
* print on finished state with matches, provision after printing, when no matches, or no thread
* allocate buffers in thread state array. limit number of buffers
* iterate over paths
* open file (translates path to inode)
* get file size (lseek for performance)
* each thread will get a range of paths
* if file fits into buffer, increment range
* if file does not fit into buffer
* designate file index as range end, pass range to thread
* pass offsets to other threads until distributed

## threads
* use growing thread-pool to avoid frequent forking
* read the first n page-aligned bytes from file to check if binary file (use pread to use offset)
* check for binary
* if binary skip file
* otherwise read all page-aligned bytes till offset
* process buffer for matches

## matching
* find next line-end
* if line too short for longest and-keyword or shortest or-keyword, skip
* memmem for first keyword on line
* if match found, repeat for remaining patterns

## printing
* print in single thread. could be designated thread

# rationale
reading file paths from standard input has the following benefits:
* the number of paths is not limited as they would be for the number of exec arguments
* an arbitrary number of exec arguments for other purposes can be provided. for example, a list of keywords to search for
* the paths can be arbitrarily pre-filtered by dedicated programs
* the complexity of path filtering (for example, exclusions) does not have to be re-implemented

# considerations
## notes
* performance when reading from standard input should not be less than when reading from files
* files can be opened and read in parallel
* standard input can only be read in one thread at a time

## how to provision delimiter-aligned buffers
* option
  * read end of large files, search for newline
  * copy end to beginning of next buffer
  * continue until all chunks distributed
  * con
    * code complexity
* option
  * read into larger buffer, pass ranges to threads
  * con
    * large memory requirement

## how to print matches
option
  collect matches in match buffer
  preferable if there are tendentially few matches
  main thread prints
  con
    match buffer has to be provided
    buffer expansion and realloc
    printing large match buffer blocks printing thread
option
  dedicated printer thread
  print the limited-size match buffers of other threads
  con
    overhead of creating the thread
option
  finish thread if match buffer full
  re-use state to continue after matches printed
  con
    may be more overhead than a mutex
option
  limited-size match buffer
  print every n matches
  con
    expensive mutex for standard output sharing
option
  print matches as they are found. expensive mutex

## motivation
i have been using three custom command-line tools intensively:
* fig (show paths containing all given strings. find | lines-filter)
* searchl (show paths and matching lines of files containing all given strings. find | lines-filter -f).
* search (show paths of files containing all given strings. find | lines-filter -fs)

i use the above commands for finding code references and discovering files on the command-line.
because i use it so frequently, i want the results from these commands as fast as possible.

even though tools like ripgrep are fast, lines-filter might be able to be slightly faster through specialization.

## performance-relevant choices
* far reduced feature set (no utf-16, no reporting contextual lines, no line numbers, et cetera)
* read files with large buffers. do not use mmap to map files because it reads in pages and has overhead requesting new pages, and so on
* use mmap memory allocation directly instead of calling malloc (which uses mmap) to allocate memory. the generalizations done by malloc are not needed (optimizations for a large range of variable size allocations, frequent de- and reallocations, etc)
* lseek instead of stat to get file sizes
* compile-time configured options like thread-count
* no preprocessing of the search pattern arguments needed before calling the command
* no pipe chain and subprocesses necessary
* much fewer features
* much fewer customization options to parse
* far lower code size and complexity (one 500L c file versus 50000L in 211 files for other tools)
* use a simple thread pool with wait and notify that grows only on demand to avoid startup costs
* memmem inspired searcher searching short patterns as integers

## reading
* [the design of ripgrep](https://blog.burntsushi.net/ripgrep/), [code and search algorithms of ripgrep](https://github.com/BurntSushi/aho-corasick/tree/master)
* [aho-corasick algorithm](https://en.wikipedia.org/wiki/Aho%E2%80%93Corasick_algorithm)
* [aho-corasick implementation](https://github.com/mischasan/aho-corasick)

## how to align read buffers to newlines
* option
  * single-threaded read
  * copy overlap to next buffer
  * can not be done from multiple threads

## should stdin processing always be single-threaded
* reading from stdin must be single threaded
* threads would be waiting for input
* needs to be tested
* file-read knows file size earlier

## substring search algorithm
* memmem - compares in word-size and seems easily optimized. for and-matching, we re-search portions with the remaining patterns after the first match has been found. for or-matching, one match is enough. the user may provide the likely most unique keywords first
  * memmem can be implemented with horspool or similar algorithms
* boyer-moore-horspool-galil - easy to implement but still character by character. in the case of lines-filter, the texts to be searched are highly non-random. it is unclear if the skipping it does is then actually superior to full word-wise comparisons
* aho-corasick with an interleaved state transition matrix is interesting because it matches multiple patterns at once and the needed memory can be stack allocated
  * not ignored should be the preparation costs, considering that lines-filter should have extremely low startup times
  * for each character we still go through considerable processing to make the trie accesses and comparisons. might be a modulo, if-branches, things like this per character
* [string-searching algorithm](https://en.wikipedia.org/wiki/String-searching_algorithm)

## matching
* use optimized integer value comparing memmem versions for patterns up to 8 characters length
* use memmem two-way search for patterns longer than 8 characters
* besides the call-overhead memmem two-way search has significant initialization overhead
* match result buffer: (line-start line-end) ...
* tasks: find first matches, find other matches, find line boundaries
* option
  * find and record all first matches in buffer
  * record the matches in the line-start fields of the match buffer
* option
  * search in buffer
  * on match, find line boundaries
  * find remaining matches in line
  * continue from line-end
* option
  * find next line-end
  * if line too short for longest and-keyword or shortest or-keyword, skip
  * memmem for first keyword on line
  * if match found, repeat for remaining patterns
  * con
    * many calls to memmem

## alternatives
* ripgrep is perhaps the fastest and is also fast used with pipes. this is currently used in [sph-script](https://github.com/sph-mn/sph-script)
  * "rg --color=never --no-heading -NFS '$1' ."
* ugrep is fast and supports and-matching with "-% '"a" "b"'"
  * it needs pre-processing of the pattern argument and several options for my use case
  * given the numerous features of ugrep, there could perhaps be a more specific faster program, and i need maximum performance
  * for a in "$@"; do pattern="${pattern} \"$a\""; done
  * "ugrep --color=never --format="%f%~" --ignore-files --exclude=.git -jUIR -m1 -% "$pattern" | uniq"
* grep process chain
  * grep [| grep] ...
* [ucg](https://github.com/gvansickle/ucg)
* a program that reads files from paths given on standard input (fcat). it copies the file content to standard output (thereby converting files into one data stream). lines-filter must then only support reading from standard input
  * find | fcat | lines-filter
  * source file paths not available to lines-filter
  * extra read and write for copying the file content
* find | xargs cat | lines-filter. surprisingly slow
time find . -type d \( -name node_modules -o -name .git -o -name cache -o -name vendor \) -prune -o -type f -print | g -F .php | xargs cat

## stopping after the first match
* if a file is read by multiple threads, not all threads can be immediately stopped. however, a thread encountering a match can stop its processing

## fixed strings versus regular expressions
* the targeted use case is fixed string only because it is for quickly user-typed strings and simple fast filtering
* example patterns that need regular expressions: dates, ip addresses, hex numbers.
* if regular expressions are needed, the logical conditions are probably not needed or not as important

## things lines-filter does not do
* tracking where exactly in a line text matched
* file path filtering
* regular expressions
* output colorization
* depend on libraries other than libc
* unicode multibyte character counting
* compressed files
* reading git index
