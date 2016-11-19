# judyarray_stl_wrapper
A set of stl style map and set wrappers for Judy Arrays

Copyright (c) 2016 Edward Boggis-Rolfe
All rights reserved.

The BSD License

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Third party library dependency: JudyArray, which is registered (the last time I checked) with an LGPL licence.


#STL style wrappers for Judy Arrays
Collection classes are an essential part of any application, and in most part using std::map or std::set are perfectly satisfactory.  However these traditional red black trees are memory hogs and if you are caching lots of data then they can become a problem.  Secondly the wipe out the cache lines faster hurting performance.  

There are alternatives to this such as B-Trees and Judy Arrays, logically they are more complex and therefore slower than red-black trees, however they protect their cachelines better and in large structures this improves performance.  If all elements are deleted in a judy array it will delete itself until a record is added to it.

Also as the Judy array is essentially one blob (for POD values smaller or equal in size to a void*) they are very fast if they need to be copied as the entire blob can be duplicated with a single malloc and memcpy.  

##Background

This project has been lurking on my hard drives for almost half a decade, and having some spare time to get up to speed on some more advanced parts of C++11 and this project needing some serious rewrite made a perfect opportunity.  So there are liberal uses of R-value references, std::move and perfect forwarding, std::enable_if and std::conditional.  


##Usage differences
These collection classes work almost identically to the STL libraries, except that iterators are invalidated as soon as they are updated.  This is because the nodes in the stl implementation remains intact, even if their location may change.  Judy arrays however have a radically different internal arrangement, this problem occur with other collection classes such as google's btree stl alternatives.

##Implementation
...

Build instructions.  This is a header only library that includes the JudyArray library.


More info when I get around to it...
