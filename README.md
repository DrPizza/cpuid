# cpuid

Dump and parse cpuid data, and try to infer the system processor and cache topologies.

Only tested on "modern" processors (Ryzen, Skylake).

## "But there are lots of cpuid printers out there already!"

I wanted a few features in particular:

1. determining the system topology
2. accepting Intel's (awful) syntax used in its documentation for querying cpuid flags.
3. Work on Win32 systems with more than 64 processors.

And since I don't write C, forking something written in C wasn't a good option.

It Works For Me (TM) on Win32 with MSVC++ 2017, and Linux with clang/libc++. I haven't tested any other combinations because I don't care for gcc. It also mostly works under the Windows Subsystem for Linux, but at the time of writnig there appears to be an issue with the thread affinity code.

It needs considerably more testing on older processors. I make no attempt to handle pre-CPUID chips because it's the year 2018, but it would probably be worth implementing the legacy topology calculations that Intel describes.

If you should happen to run the code and find it doesn't work, I'd like to see the output of a raw dump from `cpuid --dump`. File an issue.
