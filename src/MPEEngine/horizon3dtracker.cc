/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2002
 RCS:           $Id: horizon3dtracker.cc,v 1.8 2008-02-20 20:19:33 cvskris Exp $
________________________________________________________________________

-*/

#include "horizon3dtracker.h"

#include "cubicbeziercurve.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "horizonadjuster.h"
#include "horizon3dextender.h"
#include "horizon3dseedpicker.h"
#include "mpeengine.h"
#include "sectionselectorimpl.h"
#include "sectiontracker.h"
#include "survinfo.h"


#include <math.h>

namespace MPE
{

Horizon3DTracker::Horizon3DTracker( EM::Horizon3D* hor )
    : EMTracker(hor)
    , seedpicker( 0 )
{}


Horizon3DTracker::~Horizon3DTracker()
{
    delete seedpicker;
}


EMTracker* Horizon3DTracker::create( EM::EMObject* emobj )
{
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon3DTracker( hor );
}


void Horizon3DTracker::initClass()
{ TrackerFactory().addCreator( create, EM::Horizon3D::typeStr() ); }


#define mErrRet(msg) { errmsg = msg; return false; }

SectionTracker* Horizon3DTracker::createSectionTracker( EM::SectionID sid )
{
    if ( !getHorizon() ) return 0;

    return new SectionTracker( *emObject(), sid,
	    new BinIDSurfaceSourceSelector(*getHorizon(),sid),
	    new Horizon3DExtender(*getHorizon(),sid),
	    new HorizonAdjuster(*getHorizon(),sid) );
}


bool Horizon3DTracker::trackIntersections( const TrackPlane& plane )
{
    return true;
}


EMSeedPicker* Horizon3DTracker::getSeedPicker(bool createifnotpresent)
{
    if ( seedpicker )
	return seedpicker;

    if ( !createifnotpresent )
	return 0;

    seedpicker = new Horizon3DSeedPicker(*this);
    return seedpicker;
}


EM::Horizon3D* Horizon3DTracker::getHorizon()
{
    mDynamicCastGet(EM::Horizon3D*,hor,emObject());
    return hor;
}


const EM::Horizon3D* Horizon3DTracker::getHorizon() const 
{ return const_cast<Horizon3DTracker*>(this)->getHorizon(); }


}; // namespace MPE
