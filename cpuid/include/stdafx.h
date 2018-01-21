#ifndef STDAFX__H
#define STDAFX__H

#include <SDKDDKVer.h>

#if !defined(_STL_EXTRA_DISABLED_WARNINGS)
#define _STL_EXTRA_DISABLED_WARNINGS 4061 4324 4365 4514 4571 4582 4583 4623 4625 4626 4710 4774 4820 4987 5026 5027 5039
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

#define STRICT
#define NOMINMAX

#pragma warning(disable: 4668) // warning C4668: '%s' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(disable: 4710) // warning C4710: '%s': function not inlined
#pragma warning(disable: 4711) // warning C4711: function '%s' selected for automatic inline expansion
#pragma warning(disable: 4820) // warning C4820: '%s': '%d' bytes padding added after data member '%s'

#pragma warning(push)
#pragma warning(disable: 5039) // warning C5039: '%s': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
#include <Windows.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4005) // warning C4005: '%s': macro redefinition
#include <Winternl.h>
#include <ntstatus.h>
#pragma warning(pop)

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
#pragma warning(disable: 5039) // warning C5039: '%s': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.

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

#include <cstddef>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <type_traits>
#include <utility>
#include <memory>
#include <tuple>
#include <thread>
#include <cstdlib>

// disable additionally for third-party libraries
#pragma warning(push)
#pragma warning(disable: 4365) // warning C4365: '%s': conversion from '%s' to '%s', signed/unsigned mismatch
#pragma warning(disable: 4371) // warning C4371: '%s': layout of class may have changed from a previous version of the compiler due to better packing of member '%s'
#pragma warning(disable: 4619) // warning C4619: #pragma warning: there is no warning number '%d'
#pragma warning(disable: 5031) // warning C5031: #pragma warning(pop): likely mismatch, popping warning state pushed in different file
#pragma warning(disable: 26429) // warning C26429: Symbol '%s' is never tested for nullness, it can be marked as not_null (f.23: http://go.microsoft.com/fwlink/?linkid=853921).
#pragma warning(disable: 26432) // warning C26432: If you define or delete any default operation in the type '%s', define or delete them all (c.21: http://go.microsoft.com/fwlink/?linkid=853922).
#pragma warning(disable: 26434) // warning C26434: Function '%s' hides a non-virtual function '%s' (c.128: http://go.microsoft.com/fwlink/?linkid=853923).
#pragma warning(disable: 26439) // warning C26439: This kind of function may not throw. Declare it 'noexcept' (f.6: http://go.microsoft.com/fwlink/?linkid=853927).
#pragma warning(disable: 26440) // warning C26440: Function '%s' can be declared 'noexcept' (f.6: http://go.microsoft.com/fwlink/?linkid=853927).
#pragma warning(disable: 26472) // warning C26472: Don't use a static_cast for arithmetic conversions. Use brace initialization, gsl::narrow_cast or gsl::narow (type.1: http://go.microsoft.com/fwlink/p/?LinkID=620417).

#ifndef BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#define BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#endif

#include <gsl/gsl>

#include <fmt/format.h>

#pragma warning(pop)

#pragma warning(pop)

#endif
