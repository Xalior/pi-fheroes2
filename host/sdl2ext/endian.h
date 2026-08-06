/*
 * endian.h — the byte-order header POSIX names, spelled for newlib.
 *
 * fheroes2 reads and writes the original game's files, which are
 * little-endian on disk, so it converts every field it touches. It reaches
 * for the two things POSIX 1003.1-2024 puts in <endian.h>: the BYTE_ORDER,
 * BIG_ENDIAN and LITTLE_ENDIAN constants, and the htobe16/le32toh family of
 * conversions.
 *
 * newlib has all of that, under BSD's older spelling, in two headers:
 * <machine/endian.h> for the constants and <sys/endian.h> for the
 * conversions. It has no <endian.h> of its own, so upstream's include finds
 * nothing and the build stops at a missing file.
 *
 * This file is that missing name. It adds nothing and changes nothing — it
 * includes newlib's two headers, which is what a glibc <endian.h> amounts to
 * on any other machine. It is here rather than in the game because the game
 * is right: <endian.h> is the standard name for this.
 *
 * __BSD_VISIBLE is what makes <machine/endian.h> publish the unprefixed
 * BYTE_ORDER, BIG_ENDIAN and LITTLE_ENDIAN, and it is exactly what upstream
 * compares against. The build defines it before the C library's headers are
 * first reached, so it is asserted here rather than set: a translation unit
 * that got here without it would compile with BYTE_ORDER undefined, and an
 * undefined macro in an #if is silently zero — the file order would come out
 * as big-endian and every value read from disk would be byte-swapped.
 */
#ifndef _rapi_endian_h
#define _rapi_endian_h

#include <sys/cdefs.h>

#if !defined(__BSD_VISIBLE) || !__BSD_VISIBLE
#error "endian.h needs __BSD_VISIBLE for BYTE_ORDER — see the host Makefile"
#endif

#include <machine/endian.h>
#include <sys/endian.h>

#endif
