/*
 * assert.h — Circle's assert.h, with one macro taken back out again.
 *
 * THE COLLISION. Circle's <assert.h> includes <circle/macros.h>, which
 * defines WEAK as __attribute__((weak)). fheroes2 has an enumerator called
 * WEAK — the weakest setting for how strongly a random map's monsters guard
 * things — and the two meet in any translation unit that reaches for
 * <cassert>, which in a C++ program of this size is most of them. The
 * enumerator is replaced by an attribute and the enumeration stops parsing.
 *
 * Neither side is wrong. Circle's macro has been WEAK since 2014 and is part
 * of its published interface; fheroes2's enumerator is an ordinary name in
 * its own namespace, which a macro does not respect. It is a plain name
 * collision, and the only place it can be resolved is between them.
 *
 * THE FIX. This header sits ahead of Circle's on the include path, includes
 * Circle's through #include_next so that assert() and everything else it
 * carries arrive exactly as they always do, and then undefines WEAK alone.
 * Nothing else is touched: NORETURN has already been consumed by the
 * declaration that uses it, and `likely` must survive because the assert
 * macro expands to it at every call site.
 *
 * WHY IT IS IN A DIRECTORY OF ITS OWN. Only fheroes2's own sources are
 * compiled with this directory on their include path — see FH2_CPPFLAGS in
 * the host Makefile. This port's own code keeps Circle's WEAK, because
 * Circle's headers declare functions with it and a translation unit that had
 * lost the macro would fail on the next one it met.
 */
#ifndef _fh2compat_assert_h
#define _fh2compat_assert_h

#include_next <assert.h>

#undef WEAK

#endif
