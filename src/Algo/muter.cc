/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: muter.cc,v 1.1 2007-03-15 20:02:40 cvskris Exp $";

#include "muter.h"

#include "errh.h"
#include <math.h>


void Muter::mute( float* arr, int sz, float pos ) const
{
    if ( tail_ )
	tailMute( arr, sz, pos );
    else
	topMute( arr, sz, pos );
}


void Muter::topMute( float* arr, int sz, float pos ) const
{
    int endidx = pos < 0 ? (int)pos - 1 : (int)pos;
    if ( endidx > sz-1 ) endidx = sz - 1;
    for ( int idx=0; idx<=endidx; idx++ )
	arr[idx] = 0;

    float endpos = pos + taperlen_;
    if ( endpos <= 0 ) return;

    const int startidx = endidx + 1;
    endidx = (int)endpos; if ( endidx > sz-1 ) endidx = sz - 1;
    for ( int idx=startidx; idx<=endidx; idx++ )
    {
	float relpos = (idx-pos) / taperlen_;
	arr[idx] *= 0.5 * ( 1 - cos(M_PI * relpos) );
    }
}


void Muter::tailMute( float* arr, int sz, float pos ) const
{
    static bool msgdone = false;
    if ( !msgdone )
    {
	pErrMsg( "TODO: impl tail mute" );
	msgdone = true;
    }
}
