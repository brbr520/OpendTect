#ifndef angles_h
#define angles_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2009
 Contents:	Angle functions
 RCS:		$Id$
________________________________________________________________________

 Converting degrees, radians and user degrees to one another.
 Users (or rather: geologists) have North=0, and then clockwise to E=90,
 S=180s and W=270.

*/

#include "gendefs.h"
#include <math.h>

#ifndef M_PI
# define M_PI           3.14159265358979323846
#endif

#ifndef M_PIl
# define M_PIl          3.1415926535897932384626433832795029L
#endif

namespace Angle
{

enum Type { Rad, Deg, UsrDeg };
// Generic conversion function see bottom: Angle::convert

template <class T>
T cPI() { return (T)M_PIl; }

template <class T>
inline void getFullCircle( Type typ, T& t )
{
    t = typ == Rad ? 2 * cPI<T>() : 360;
}


inline double deg2rad( int deg )
{
    static const double deg2radconst = cPI<double>() / 180;
    return deg * deg2radconst;
}


inline float deg2rad( float deg )
{
    static const float deg2radconst = cPI<float>() / 180;
    return deg * deg2radconst;
}


inline double deg2rad( double deg )
{
    static const double deg2radconst = cPI<double>() / 180;
    return deg * deg2radconst;
}


inline long double deg2rad( long double deg )
{
    static const long double deg2radconst = cPI<long double>() / 180;
    return deg * deg2radconst;
}


inline float rad2deg( float rad )
{
    static const float rad2degconst = 180 / cPI<float>();
    return rad * rad2degconst;
}


inline double rad2deg( double rad )
{
    static const double rad2degconst = 180 / cPI<double>();
    return rad * rad2degconst;
}


inline long double rad2deg( long double rad )
{
    static const long double rad2degconst = 180 / cPI<long double>();
    return rad * rad2degconst;
}


//! User degrees are from North, clockwise
template <class T>
inline T deg2usrdeg( T deg )
{
    T usrdeg = 90 - deg;
    while ( usrdeg >= 360 ) usrdeg -= 360;
    while ( usrdeg < 0 ) usrdeg += 360;
    return usrdeg;
}


//! User degrees are from North, clockwise
template <class T>
inline T usrdeg2deg( T udeg )
{
    T deg = 90 - udeg;
    if ( deg < 0 ) deg += 360;
    return deg;
}


template <class T>
inline T rad2usrdeg( T rad )
{
    return deg2usrdeg( rad2deg(rad) );
}


template <class T>
inline T usrdeg2rad( T udeg )
{
    return deg2rad( usrdeg2deg(udeg) );
}


template <class T>
inline T convert( Type inptyp, T val, Type outtyp )
{
    if ( inptyp == outtyp )
	return val;

    switch ( inptyp )
    {
        case Rad:
            val = outtyp == Deg ? rad2deg(val) : rad2usrdeg(val);
            break;
        case Deg:
            val = outtyp == Rad ? deg2rad(val) : deg2usrdeg(val);
            break;
        case UsrDeg:
            val = outtyp == Deg ? usrdeg2deg(val) : usrdeg2rad(val);
            break;
    }

    return val;
}




} // namespace

#endif
