#ifndef STDAFX__H
#define STDAFX__H

#if defined(_WIN32)

#include <SDKDDKVer.h>

#if !defined(_STL_EXTRA_DISABLED_WARNINGS)
#define _STL_EXTRA_DISABLED_WARNINGS 4061 4324 4365 4514 4571 4582 4583 4623 4625 4626 4710 4774 4800 4820 4987 5026 5027 5039
#endif

#if !defined(_SCL_SECURE_NO_WARNINGS)
#define _SCL_SECURE_NO_WARNINGS 1
#endif

#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#if !defined(_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING 1
#endif

#if !defined(_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING 1
#endif

#if !defined(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
#endif

#define STRICT
#define NOMINMAX

#pragma warning(disable: 4571) // warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
#pragma warning(disable: 4668) // warning C4668: '%s' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(disable: 4710) // warning C4710: '%s': function not inlined
#pragma warning(disable: 4711) // warning C4711: function '%s' selected for automatic inline expansion
#pragma warning(disable: 4820) // warning C4820: '%s': '%d' bytes padding added after data member '%s'
#pragma warning(disable: 5045) // warning C5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#include <Windows.h>

#pragma warning(push)
#pragma warning(disable: 4005) // warning C4005: '%s': macro redefinition
#include <Winternl.h>
#include <ntstatus.h>
#pragma warning(pop)

#else // defined(_WIN32)

#include <cpuid.h>

#endif

#if defined(_MSC_VER)

// currently broken (triggers on iterators and other objects that are usually unnamed)
#pragma warning(disable: 26444) // warning C26444: Avoid unnamed objects with custom construction and destruction (es.84: http://go.microsoft.com/fwlink/?linkid=862923).

// disable for everything
#pragma warning(disable: 4061) // warning C4061: enumerator '%s' in switch of enum '%s' is not explicitly handled by a case label
#pragma warning(disable: 4324) // warning C4234: structure was padded due to alignment specifier
#pragma warning(disable: 4514) // warning C4514: '%s': unreferenced inline function has been removed
#pragma warning(disable: 4623) // warning C4623: '%s': default constructor was implicitly defined as deleted
#pragma warning(disable: 4625) // warning C4625: '%s': copy constructor was implicitly defined as deleted
#pragma warning(disable: 4626) // warning C4626: '%s': assignment operator was implicitly defined as deleted
#pragma warning(disable: 4710) // warning C4710: '%s': function not inlined
#pragma warning(disable: 4820) // warning C4820: '%s': '%d' bytes padding added after data member '%s'
#pragma warning(disable: 5026) // warning C5026: '%s': move constructor was implicitly defined as deleted
#pragma warning(disable: 5027) // warning C5027: '%s': move assignment operator was implicitly defined as deleted

#pragma warning(disable: 26412) // warning C26412: Do not dereference an invalid pointer (lifetimes rule 1). 'return of %s' was invalidated at line %d by 'no initialization'.
#pragma warning(disable: 26426) // warning C26426: Global initializer calls a non-constexpr function '%s' (i.22: http://go.microsoft.com/fwlink/?linkid=853919).
#pragma warning(disable: 26481) // warning C26481: Don't use pointer arithmetic. Use span instead. (bounds.1: http://go.microsoft.com/fwlink/p/?LinkID=620413)
#pragma warning(disable: 26482) // warning C26482: Only index into arrays using constant expressions (bounds.2: http://go.microsoft.com/fwlink/p/?LinkID=620414).
#pragma warning(disable: 26485) // warning C26485: Expression '%s::`vbtable'': No array to pointer decay. (bounds.3: http://go.microsoft.com/fwlink/p/?LinkID=620415)
#pragma warning(disable: 26490) // warning C26490: Don't use reinterpret_cast. (type.1: http://go.microsoft.com/fwlink/p/?LinkID=620417)
#pragma warning(disable: 26499) // warning C26499: Could not find any lifetime tracking information for '%s'

// disable for standard headers
#pragma warning(push)
#pragma warning(disable: 26400) // warning C26400: Do not assign the result of an allocation or a function call with an owner<T> return value to a raw pointer, use owner<T> instead. (i.11 http://go.microsoft.com/fwlink/?linkid=845474)
#pragma warning(disable: 26401) // warning C26401: Do not delete a raw pointer that is not an owner<T>. (i.11: http://go.microsoft.com/fwlink/?linkid=845474)
#pragma warning(disable: 26408) // warning C26408: Avoid malloc() and free(), prefer the nothrow version of new with delete. (r.10 http://go.microsoft.com/fwlink/?linkid=845483)
#pragma warning(disable: 26409) // warning C26409: Avoid calling new and delete explicitly, use std::make_unique<T> instead. (r.11 http://go.microsoft.com/fwlink/?linkid=845485)
#pragma warning(disable: 26411) // warning C26411: The parameter '%s' is a reference to unique pointer and it is never reassigned or reset, use T* or T& instead. (r.33 http://go.microsoft.com/fwlink/?linkid=845479)
#pragma warning(disable: 26412) // warning C26412: Do not dereference an invalid pointer (lifetimes rule 1). 'return of %s' was invalidated at line %d by 'end of function scope (local lifetimes end)'.
#pragma warning(disable: 26413) // warning C26413: Do not dereference nullptr (lifetimes rule 2). 'nullptr' was pointed to nullptr at line %d.
#pragma warning(disable: 26423) // warning C26423: The allocation was not directly assigned to an owner.
#pragma warning(disable: 26424) // warning C26424: Failing to delete or assign ownership of allocation at line %d.
#pragma warning(disable: 26425) // warning C26425: Assigning '%s' to a static variable.
#pragma warning(disable: 26444) // warning C26444: Avoid unnamed objects with custom construction and destruction (es.84: http://go.microsoft.com/fwlink/?linkid=862923).
#pragma warning(disable: 26461) // warning C26461: The reference argument '%s' for function %s can be marked as const. (con.3: https://go.microsoft.com/fwlink/p/?LinkID=786684)
#pragma warning(disable: 26471) // warning C26471: Don't use reinterpret_cast. A cast from void* can use static_cast. (type.1: http://go.microsoft.com/fwlink/p/?LinkID=620417).
#pragma warning(disable: 26481) // warning C26481: Don't use pointer arithmetic. Use span instead. (bounds.1: http://go.microsoft.com/fwlink/p/?LinkID=620413)
#pragma warning(disable: 26482) // warning C26482: Only index into arrays using constant expressions. (bounds.2: http://go.microsoft.com/fwlink/p/?LinkID=620414)
#pragma warning(disable: 26490) // warning C26490: Don't use reinterpret_cast. (type.1: http://go.microsoft.com/fwlink/p/?LinkID=620417)
#pragma warning(disable: 26493) // warning C26493: Don't use C-style casts that would perform a static_cast downcast, const_cast, or reinterpret_cast. (type.4: http://go.microsoft.com/fwlink/p/?LinkID=620420)
#pragma warning(disable: 26494) // warning C26494: Variable '%s' is uninitialized. Always initialize an object. (type.5: http://go.microsoft.com/fwlink/p/?LinkID=620421)
#pragma warning(disable: 26495) // warning C26495: Variable '%s' is uninitialized. Always initialize a member variable. (type.6: http://go.microsoft.com/fwlink/p/?LinkID=620422)
#pragma warning(disable: 26496) // warning C26496: Variable '%s' is assigned only once, mark it as const. (con.4: https://go.microsoft.com/fwlink/p/?LinkID=784969)
#pragma warning(disable: 26497) // warning C26497: This function %s could be marked constexpr if compile-time evaluation is desired. (f.4: https://go.microsoft.com/fwlink/p/?LinkID=784970)

#else

// linux warnings go here

#endif

#include <cstddef>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <type_traits>
#include <utility>
#include <memory>
#include <tuple>
#include <thread>
#include <cstdlib>
#include <codecvt>

#if defined(_MSC_VER)

// disable additionally for third-party libraries
#pragma warning(push)
#pragma warning(disable: 4456) // warning C4456: declaration of '%s' hides previous local declaration
#pragma warning(disable: 4458) // warning C4458: declaration of '%s' hides class member
#pragma warning(disable: 4459) // warning C4459: declaration of '%s' hides global declaration
#pragma warning(disable: 4702) // warning C4702: unreacha3eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeble code

#include <CppCoreCheck\Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#endif

#ifndef BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#define BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#endif

#include <boost/algorithm/string.hpp>
#include <boost/xpressive/xpressive.hpp>

#include <gsl/gsl>

#include <fmt/format.h>

#if defined(_MSC_VER)

#pragma warning(pop)
#pragma warning(pop)

#else

// linux warning restoration goes here

#endif

#endif

