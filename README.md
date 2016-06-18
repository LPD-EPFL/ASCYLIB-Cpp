ASCYLIB(Cpp) + OPTIK
===============

ASCYLIB (with OPTIK) is a concurrent data-structure library. The original C version (found at https://github.com/LPD-EPFL/ASCYLIB) contains over 40 implementations of linked lists, hash tables, skip lists, binary search trees (BSTs), queues, priority queues, and stacks, containing sequential, lock-based, and lock-free implementations for each data structure.

This C++ port, in its current state, contains 27 of those concurrent data structures.

ASCYLIB works on x86, SPARC, and Tilera architectures and contains tests to evaluate the throughput, latency, latency distribution, and energy efficiency of the included data structures.

OPTIK is a new design pattern for easily implementing fast and scalable concurrent data structures. We have merged several concurrent data structures developed with OPTIK in ASCYLIB. More details can be found here: http://lpd.epfl.ch/site/optik.

* Website             : http://lpd.epfl.ch/site/ascylib - http://lpd.epfl.ch/site/optik
* Authors             : Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
                        Tudor David <tudor.david@epfl.ch> 
* Related Publications:
  * [*Optimistic Concurrency with OPTIK*](https://dl.acm.org/citation.cfm?id=2851146),  
    Rachid Guerraoui, Vasileios Trigonakis (alphabetical order),  
  PPoPP 2016
  * [*Asynchronized Concurrency: The Secret to Scaling Concurrent Search Data Structures*](https://dl.acm.org/citation.cfm?id=2694359),  
  Tudor David, Rachid Guerraoui, Vasileios Trigonakis (alphabetical order),  
  ASPLOS 2015


Algorithms
----------

The following table contains the algorithms (and various implementations of some algorithms) included in ASCYLIB (respecting the original ASCYLIB numbering):

| # |    Name   |  Progress  |  Year | Referece |
|:-:|-----------|:-----:|:-----:|:-----:|
|| **Array Maps** ||||
|3| [OPTIK global-lock array map](./src/algorithms/array_map_optik.h) |	lock-based | 2016 | [[GT+16]](#GT+16) |
|| **Linked lists** ||||
|4| [Sequential linked list](./src/algorithms/linkedlist_seq.h) |	sequential | | |
|5| [Hand-over-hand locking linked list](./src/algorithms/linkedlist_coupling.h) |	lock-based | | [[HS+12]](#HS+12) |
|6| [Pugh's linked list](./src/algorithms/linkedlist_pugh.h) |	lock-based | 1990 | [[P+90]](#P+90) |
|7| [Harris linked list](./src/algorithms/linkedlist_harris.h/) |	lock-free | 2001 | [[H+01]](#H+01) |
|9| [Lazy linked list](./src/algorithms/linkedlist_lazy.h/) |	lock-based | 2006 | [[HHL+06]](#HHL+06) |
|10| [Harris linked list with ASCY](./src/algorithms/linkedlist_harris_opt.h) |	lock-free | 2015 | [[DGT+15]](#DGT+15) |
|12| [OPTIK global-lock linked list](./src/algorithms/linkedlist_optik_gl.h) |	lock-based | 2016 | [[GT+16]](#GT+16) |
|13| [OPTIK fine-grained linked list](./src/algorithms/linkedlist_optik.h) |	lock-based | 2016 | [[GT+16]](#GT+16) |
|| **Hash Tables** ||||
|18| [Hash table using Pugh's list](./src/algorithms/hashtable_pugh.h) |	lock-based | 1990 | [[P+90]](#P+90) |
|19| [Hash table using Harris' list](./src/algorithms/hashtable_harris.h) |	lock-free | 2001 | [[H+01]](#H+01) |
|20| [Java's ConcurrentHashMap](./src/algorithms/hashtable_java.h) |	lock-based | 2003 | [[L+03]](#L+03) |
|21| [Hash table using Java's CopyOnWrite array map](./src/algorithms/hashtable_copy.h) |	lock-based | 2004 | [[ORACLE+04]](#ORACLE+04) |
|26| [Hash table using fine-grained OPTIK list](./src/algorithms/hashtable_optik.h) |	lock-based | 2016 | [[GT+16]](#GT+16) |
|27| [Hash table using global-lock OPTIK list](./src/algorithms/hashtable_optik_gl.h) |	lock-based | 2016 | [[GT+16]](#GT+16) |
|28| [Hash table using OPTIK array map](./src/algorithms/hashtable_optik_arraymap.h) |	lock-based | 2016 | [[GT+16]](#GT+16) |
|| **Skip Lists** ||||
|29| [Sequential skip list](./src/algorithms/skiplist_seq.h) |	sequential | | |
|31| [Fraser skip list](./src/algorithms/skiplist_fraser.h) |	lock-free | 2003 | [[F+03]](#F+03) |
|32| [Herlihy et al. skip list](./src/algorithms/skiplist_herlihy_lb.h) |	lock-based | 2007 | [[HLL+07]](#HLL+07) |
|33| [Fraser skip list with Herlihy's optimization](./src/algorithms/skiplist_herlihy_lf.h) |	lock-free | 2011 | [[HLS+11]](#HLS+11) |
|35| [OPTIK skip list using trylocks (*default OPTIK skip list*)](./src/algorithms/skiplist_optik.h) |	lock-based | 2016 | [[GT+16]](#GT+16) |
|| **Queues** ||||
|45| [Michael and Scott (MS) lock-based queue](./src/algorithms/queue_ms_lb.h) |	lock-based | 1996 | [[MS+96]](#MS+96) |
|46| [Michael and Scott (MS) lock-free queue](./src/algorithms/queue_ms_lf.h) |	lock-free | 1996 | [[MS+96]](#MS+96) |
|47| [Michael and Scott (MS) hybrid queue](./src/algorithms/queue_ms_hybrid.h) |	lock-based | 1996 | [[MS+96]](#MS+96) |
|| **Stacks** ||||
|56| [Global-lock stack](./src/algorithms/stack_lock.h) |	lock-based | | |
|57| [Treiber stack](./src/algorithms/stack_treiber.h) |	lock-free | 1986 | [[T+86]](#T+86) |

References
----------

* <a name="AKL+15">**[AKL+15]**</a>
D. Alistarh, J. Kopinsky, J. Li, N. Shavit. The SprayList:
*A Scalable Relaxed Priority Queue*. 
PPoPP '15.
* <a name="BCH+10">**[BCH+10]**</a>
N. G. Bronson, J. Casper, H. Chafi, and K. Olukotun.
*A Practical Concurrent Binary Search Tree*.
PPoPP '10.
* <a name="DGT+15">**[DGT+15]**</a>
T. David, R. Guerraoui, and V. Trigonakis.
*Asynchronized Concurrency: The Secret to Scaling Concurrent Search Data Structures*.
ASPLOS '15.
* <a name="DMS+12">**[DMS+12]**</a>
M. Desnoyers, P. E. McKenney, A. S. Stern, M. R. Dagenais, and J. Walpole.
*User-level implementations of read-copy update*.
PDS '12.
* <a name="DVY+14">**[DVY+14]**</a>
D. Drachsler, M. Vechev, and E. Yahav.
*Practical Concurrent Binary Search Trees via Logical Ordering*.
PPoPP '14.
* <a name="EFR+10">**[EFR+10]**</a>
F. Ellen, P. Fatourou, E. Ruppert, and F. van Breugel.
*Non-blocking Binary Search Trees*.
PODC '10.
* <a name="F+03">**[F+03]**</a>
K. Fraser.
*Practical Lock-Freedom*.
PhD thesis, University of Cambridge, 2004.
* <a name="GT+16">**[GT+16]**</a>
R. Guerraoui, and V. Trigonakis.
*Optimistic Concurrency with OPTIK*.
PPoPP '16.
* <a name="H+01">**[H+01]**</a>
T. Harris.
*A Pragmatic Implementation of Non-blocking Linked Lists*.
DISC '01.
* <a name="HHL+06">**[HHL+06]**</a>
S. Heller, M. Herlihy, V. Luchangco, M. Moir, W. N. Scherer, and N. Shavit.
*A Lazy Concurrent List-Based Set Algorithm*.
OPODIS '05.
* <a name="HS+12">**[HS+12]**</a>
M. Herlihy and N. Shavit.
*The Art of Multiprocessor Programming, Revised First Edition*.
2012.
* <a name="HLL+07">**[HLL+07]**</a>
M. Herlihy, Y. Lev, V. Luchangco, and N. Shavit.
*A Simple Optimistic Skiplist Algorithm*.
SIROCCO '07.
* <a name="HLS+11">**[HLS+11]**</a>
M. Herlihy, Y. Lev, and N. Shavit.
*Concurrent lock-free skiplist with wait-free contains operator*, May 3 2011.
US Patent 7,937,378.
* <a name="HJ+12">**[HJ+12]**</a>
S. V. Howley and J. Jones. 
*A non-blocking internal binary search tree*. 
SPAA '12.
* <a name="INTEL+06">**[INTEL+06]**</a>
Intel.
*Intel Thread Building Blocks*.
https://www.threadingbuildingblocks.org.
* <a name="L+03">**[L+03]**</a>
D. Lea.
*Overview of Package util.concurrent Release 1.3.4*.
http://gee.cs.oswego.edu/dl/classes/EDU/oswego/cs/dl/util/concurrent/intro.html,
2003.
* <a name="LS+00">**[LS+00]**</a>
I. Lotan and N. Shavit. 
*Skiplist-based concurrent priority queues*.
IPDPS '00.
* <a name="M+02">**[M+02]**</a>
M. M. Michael.
*High Performance Dynamic Lock-free Hash tables and List-based Sets*.
SPAA '02.
* <a name="MS+96">**[MS+96]**</a>
M. M. Michael and M. L. Scott.
*Simple, Fast, and Practical Non-blocking and Blocking Concurrent Queue Algorithms*.
PODC '96.
* <a name="NM+14">**[NM+14]**</a>
A. Natarajan and N. Mittal.
*Fast Concurrent Lock-free Binary Search Trees*.
PPoPP '14.
* <a name="ORACLE+04">**[ORACLE+04]**</a>
Oracle.
*Java CopyOnWriteArrayList*.
http://docs.oracle.com/javase/7/docs/api/java/util/concurrent/CopyOnWriteArrayList.html.
* <a name="P+90">**[P+90]**</a>
W. Pugh.
*Concurrent Maintenance of Skip Lists*.
Technical report, 1990.
* <a name="T+86">**[T+86]**</a>
R. Treiber.
*Systems Programming: Coping with Parallelism*.
Technical report, 1986.

New Algorithms
--------------

BST-TK is a new lock-based BST, introduced in ASCYLIB. 
Additionally, CLHT is a new hash hash table, introduced in ASCYLIB. We provide lock-free and lock-based variants of CLHT as a separate repository (https://github.com/LPD-EPFL/CLHT).
Details of the algorithms and proofs/sketches of correctness can be found in the following technical report: https://infoscience.epfl.ch/record/203822

We have developed the following algorithms using OPTIK:
  1. A simple array map (in `src/hashtable-map_optik`).  
  We use this map in a hash table (in `src/hashtable-optik0`);
  2. An optimistic global-lock-based linked list (in `src/linkedlist-optik_gl`).  
  We use this list in a hash table (in `src/hashtable-optik1`);
  3. A fine-grained linked list (in `src/linkedlist-optik`).  
  We use this list in a hash table (in `src/hashtable-optik0`);
  4. A skip list algorithm (in `src/skiplist-optik1`).   
  We also provide a variant of the same algorithm (in `src/skiplist-optik`).

Additionally, we have optimized existing algorithms using OPTIK:
  1. Java's ConcurrentHashMap algorithm (in`src/hashtable-java_optik`);
  2. Herlihy's optimistic skip list (in `src/skiplist-optik2`);
  3. The classic Michael-Scott queues:
    * lock-based `push`, `pop` optimized with `optik_lock_version_backoff` (in `src/queue-optik0`)
    * lock-based `push`, `pop` optimized with `optik_trylock_version` (in `src/queue-optik1`)
    * lock-free `push`, `pop` optimized with `optik_trylock_version` (in `src/queue-optik2`)

Finally, we have introduced two optimization techniques inspired by OPTIK:
  1. Node caching for optimizing lists (in `src/linkedlist-optik_cache`);
  2. Victim queue for optimizing `push` in queues (in `src/queue-optik3`).

Compilation
-----------

ASCYLIB requires the ssmem memory allocator (https://github.com/LPD-EPFL/ssmem).
We have already compiled and included ssmem in external/lib for x86_64, SPARC, and the Tilera architectures.
Still, if you would like to create your own build of ssmem, take the following steps.
Clone ssmem, do `make libssmem.a` and then copy `libssmem.a` in `ASCYLIB/external/lib` and `smmem.h` in `ASCYLIB-Cpp/external/include`.

Additionally, the sspfd profiler library is required (https://github.com/trigonak/sspfd).
We have already compiled and included sspfd in external/lib for x86_64, SPARC, and the Tilera architectures.
Still, if you would like to create your own build of sspfd, take the following steps.
Clone sspfd, do `make` and then copy `libsspfd.a` in `ASCYLIB-Cpp/external/lib` and `sspfd.h` in `ASCYLIB-Cpp/external/include`.

Finally, to measure power on new Intel processors (e.g., Intel Ivy Bridge), the raplread library is required (https://github.com/LPD-EPFL/raplread).
We have already compiled and included raplread in external/lib.
Still, if you would like to create your own build of raplread, take the following steps.
Clone raplread, do `make` and then copy `libraplread.a` in `ASCYLIB-Cpp/external/lib` and `sspfd.h` in `ASCYLIB-Cpp/external/include`.

To build all data structures, you can execute `make`.
This target builds all lock-free, lock-based, and sequential data structures.

ASCYLIB includes a default configuration that uses `g++` and tries to infer the number of cores and the frequency of the target/build platform. If this configuration is incorrect, you can always create a manual configurations in `common/Makefile.common` and `include/utils.h` (look in these files for examples). If you do not care about pinning threads to cores, these settings do not matter. You can compile with `make SET_CPU=0 ...` to disable thread pinning.

ASCYLIB accepts various compilation parameters. Please refer to the `COMPILE` file.

Adding an algorithm
-------------------
To add an algorithm, preferably implement (inherit from) one of the interfaces/abstract classes relevant to the data structure you are implementing (`src/search.h` or `src/stack_queue.h`).  We also recommend you add it to:
1. The relevant test file (`src/test_search.cc` or `src/test_stackqueue.cc). This means adding the algorithm to:
	* The head of the file, including the relevant source header (_algorithm_.h)
	* The `algorithms` enum near the head of the file.
	* The `parse_algorithm` function starting around line 123, which will parse the algorithm quick name from the command line option (`-a`).
	* The list of algorithms in the help description, around line 550.
	* The actual initialization of the appropriate data structure, starting around line 655.
2. The Makefile at the root of the project, either under `SEARCH_ALGORITHMS` or `STACKQUEUE_ALGORITHMS`.  Add the name of the `.h` file without the extension, as it will be added later.  This ensures that a modification of your file causes recompilation of the test file.
3. Don't forget to include any aux files (extra classes, or other types of data nodes) you may create to this file as well, to also ensure that the test files are recompiled if they change.

Tests
-----

This version of ASCYLIB, unlike the C implementation, has 2 single test files.

One is for search data structures, and one is for stack/queue data structures. You can find & execute either one in the `bin` directory.
Run `./bin/test_search -h` or `./bin/test_stackqueue -h` for help on the parameters (selecting the algorithm, workload, etc.)

Scripts
-------

ASCYLIB includes tons of usefull scripts (in the `scripts` folders). Some particularly useful ones are:
* `scalability.sh` and `scalability_rep.h`: run the given list of executable on the given (list of) number of threads, with the given parameters, and report throughput and scalability over single-threaded execution.
* scripts in `apslos/` directory: they were used to create the plots for the ASPLOS '15 paper. In particular, `apslos/run_scy.sh` accepts configuration files (see `asplos/config`) so it can be configured to execute almost any per-data-structure scenario.
* scripts in `ppopp/` directory: they were used to create the plots for the PPoPP '16 paper. In particular, `ppopp/run_and_plot.sh` can run and plot graphs for all the tests in the paper.

Thanks
------

Some of the initial implementations used in ASCYLIB were taken from Synchrobench (https://github.com/gramoli/synchrobench -  V. Gramoli. More than You Ever Wanted to Know about Synchronization. PPoPP 2015.).
