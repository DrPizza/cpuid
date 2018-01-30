# cpuid

Dump and parse cpuid data, and try to infer the system processor and cache topologies.

Only tested on "modern" processors (Ryzen, Skylake).

##"But there are lots of cpuid printers out there already!"

I wanted a few features in particular:

1. determining the system topology
2. accepting Intel's (awful) syntax used in its documentation for querying cpuid flags.
3. Work on Win32 systems with more than 64 processors.

And since I don't write C, forking something written in C wasn't a good option.

It's currently Win32/VC++-specific, because of the thread affinity code and use of intrinsics. It should be straightforward to make it work on Linux too. I just haven't got around to it because I don't really want to write a makefile, and I refuse to use CMake.

It needs considerably more testing on older processors. I make no attempt to handle pre-CPUID chips because it's the year 2018, but it would probably be worth implementing the legacy topology calculations that Intel describes.
