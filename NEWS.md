scrm Version History
========================

scrm 1.7.4
------------------------
Released: 2020-03-07

### Bug Fixes
+ Fixes the initialization with an inital tree in Newick format (`-init`).
  Big thanks to @nspope for reporting and locating this bug along with
  suggesting a fix! 



scrm 1.7.3
------------------------
Released: 2018-11-18

### Improvements
+ The labeling of internal nodes in the oriented forest output
  no longer changes within the same tree topology. That makes
  it easier to identify different topologies.

### Bug Fixes
+ Now requires that subpopulation are defined via the `-I` argument
  before any demographic options for all population (`-M`, `-G` or `-N`)
  are given (#108).
  Thanks to Jonathan Terhorst (@terhorst) for reporting this bug.
+ When the multiple of the merge/admixure arguemtns (`-ep`, `-eps` and `-ej`)
  are used at the time, they are now executed in order in which
  they are provided on the command line (#121). This behavior now
  follows ms' implementation. 
  Thanks to Scott T Small (@stsmall) for reporting this.



scrm 1.7.2
------------------------
Released: 2016-04-10

### Bug Fixes
+ Fixes a different bug that could occur when using the "-eI" option.
  The contemporaries buffer was always cleared correctly between 
  repetitions, and could cause scrm to throw an error (#104).
  Thanks again to @jackkamm for reporting this bug.



scrm 1.7.1
------------------------
Released: 2016-03-23

### Bug Fixes
+ Fixes that bug that could lead to false results when the "-eI" option
  was used to include non-contemporary samples. In that case, scrm
  could skip the time interval between the MRCA of the contemporary samples
  and the time of the "-eI" event (if any), and neglect all events 
  during that time. We recommend to repeat all simulations which
  used "-eI" with the updated version (#102).
  Thanks to @jackkamm for reporting this bug.



scrm 1.7.0
------------------------
Released: 2016-02-07

### New Features
+ New command line option "--transpose-segsites" that prints the segregating
  sites matrix with rows repesenting mutations instead of individuals. This
  also adds the time at which the mutation occurred as additional information
  (#85).

### Improvements
+ scrm now throws an error when mutation or recombination rate changes
  have invalid sequence positions (#82).
+ The length of segments between recombinations is now always reported
  in non-scientific notation (#81).
+ Improved the error message when lines could not coalescence because
  of missing migration or negative population growth (#87).

### Bug Fixes
+ `Forest.coalescence_finished` has not set to `true` after a pairwise
  coalescence event (#89). This had no effect on the command line version
  of scrm.
+ Fixes scrm's tests suite on 32 bit systems (#98).


 
scrm 1.6.1
------------------------
Released: 2015-07-09

### Bug Fixes
+ scrm had extensive memory consumption when simulating trees in 
  large models (#79). This is now fixed, it is however no longer
  possible to use "-O" and "-T" at the same time.


scrm 1.6.0
------------------------
Released: 2015-06-04

### New Features
+ It is now possible to specify the approximation exact window in number 
  of recombination events (#73).
+ scrm now uses a conservative approximation by default (#75).

### Bug Fixes
+ Critical:  Position based rates changes (`-sr` and `-st`) were only
  applied to the first locus. All other loci are simulated with the rates from
  the end of the first one (#74).
+ In large models, the Newick trees could get larger than the character limit
  of a C++ string. In this cases, only incomplete trees were printed (#76).



scrm 1.5.1
------------------------
Released: 2015-05-18

+ Bug fix: Fixed a cache invalidation error that could lead to runs being
  aborted after 100k recombinations (#70). Thanks to Jerome Kelleher for 
  report this.
+ Minor: Small updates to the documentation and citation information.



scrm 1.5.0
------------------------
Released: 2015-04-07

+ New feature: Added flag "--print-model" which prints a textual representation
  of the demographic model for verification and debugging (#60). 
+ Bug fix: When multiple population splits and/or merges occurred at the same
  time, only one of them affected each line while the others were ignored (#61). 
+ New feature: Added support for partial population admixtures (`-eps`, #62).
+ In addition to using "-seed" scrm now also supports "-seeds", as ms also 
  supports both.



scrm 1.4.1
------------------------
Released: 2015-04-04

+ Bug fix: Wrong population size where calculated when migration rates
  changes or population splits and merges occurred in an growth period (#56)
+ Bug fix: When -es was used at time 0, _scrm_ ran into an endless loop (#53)
+ Improvement: Added an error message when a population size is set to 0 (#52)



scrm 1.4.0
------------------------
Released: 2015-03-29

+ Improved memory management (#36)
+ Added option '-p' to set number of significant digits in output (#47) 
+ Switched to std::mt19937_64 as default random generator (#49)
+ Support arguments in scientific notation (#50, #51)



scrm 1.3.2
------------------------
Released: 2014-12-23

+ Bug fix: Fix reproducibility problem when the large sample optimization was active (#42).



scrm 1.3.1
------------------------
Released: 2014-10-23

+ Bug fix: Implement missing '-m' and '-em' arguments (#32).




scrm 1.3.0
------------------------
Released: 2014-10-21

### Improvements
+ Improved the autotools configuration to support more compilers and operating
  systems (#10, #27).
+ Changed the Oriented Forest summary statistic (#25).
+ Various minor cleanups in the code base to simplify creation of an R package
  containing scrm (#29).




scrm 1.2.0
------------------------
Released: 2014-09-10

### New Features
+ New `oriented forest` summary statistic as suggested by 

    J. Kelleher, A.M. Etheridge, N.H. Barton (2014) Coalescent simulation in 
    continuous space: Algorithms for large neighbourhood size, 
    Theoretical Population Biology, Volume 95, August2014, Pages 13-23, 
    ISSN 0040-5809, http://dx.doi.org/10.1016/j.tpb.2014.05.001. 

### Improvements
+ Optimized the generation of newick trees (#22) and use a buffer (#23)


 

scrm 1.1.0
------------------------
Released: 2014-08-04

### Improvements
+ Improved the handling & storage of contemporary nodes. This gives a huge
  performance boost if scrm is used with large sample sizes (>1000) (#20). 
+ Optimized scrm for use with many populations (#20). 
+ Added more automatic tests of the produced distribution of trees (#20).




scrm 1.0.0 
------------------------
Released: 2014-07-09

### Bug fixes
+ Fixed an access to unmapped memory




scrm 1.0-beta2
------------------------
Released: 2014-06-18

### Improvements
+ Fix file permissions
+ Remove clang warning suppression
+ Added a man page




scrm 1.0-beta1
------------------------
Released: 2014-06-02

### Bug fixes:
+ Option '-es' is now ms-compatible (#16).
+ It is now possible to use 3 seeds (as in ms).

### Improvements:
+ Added help and version information.
+ Small performance tweaks.

### New Features
+ Added variable recombination & mutation rates




scrm 0.9-1
------------------------

### Bug fixes
+ Very first node in the tree was assigned to wrong population
+ The input time of "-eI" option was not scaled
+ Fixed the scaling of growth rates




scrm 0.9-0
------------------------

Algorithm passes all tests now, starting explicit versioning.

