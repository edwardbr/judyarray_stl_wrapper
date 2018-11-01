# JudyArray STL wrapper
A set of stl style map and set wrappers for Judy Arrays

## Introduction
STL style wrappers for Judy Arrays
In most part using std::map or std::set classes are perfectly satisfactory solution for sorted dictionary style lookups and collections. 

However these traditional red black trees are often inefficient in memory, and if you are caching lots of data then they can become a problem as they use up more memory than the data that they are referencing to.  Secondly they wipe out the cache lines faster than some other implementations hurting performance.  

There are alternatives to this such as B-Trees and Judy Arrays, logically they are more complex and therefore can be sometimes slower than red-black trees, however they protect their cachelines better and in large structures this improves performance.

Also as the Judy array is essentially one blob (for POD values smaller or equal in size to a void*) they are very fast if they need to be copied, as the entire blob can be duplicated with a single malloc and memcpy.  

Providing STL style collection classes is an easy way of of providing a drop in alternative implementation to std::map and set.  #include library and you should be good to go.

## Background

This project has been lurking on my hard drives for almost half a decade, and having some spare time I used it as an opportunity to get up to speed on some more advanced parts of C++11. As this project needed some serious rewrite it made a perfect opportunity.  So there are liberal uses of R-value references, std::move and perfect forwarding and SFINAE.  


## Usage differences
These collection classes work almost identically to the STL libraries, except that iterators are invalidated as soon as they are updated.  This is because the nodes in the stl implementation remains intact, even if their location may change.  Judy arrays however have a radically different internal arrangement, this problem occur with other collection classes such as google's btree stl alternatives.

## Build instructions.

This is a header only library that requires the JudyArray library http://judy.sourceforge.net/.

template<typename S, typename T> class judy_int_map
template<typename T> class judy_string_map 
template<typename T> class judy_int_set
  
These classes use a integer or a string as a key which are used internally by the JudyArray library, other types such as dates may also work if converted into something like a julian integer.

More info when I get around to it...
