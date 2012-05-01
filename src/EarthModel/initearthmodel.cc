/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki	
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initearthmodel.cc,v 1.13 2012-05-01 13:59:38 cvskris Exp $";


#include "moddepmgr.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmarchingcubessurface.h"
#include "emhorizonztransform.h"
#include "emobject.h"
#include "emrandomposbody.h"
#include "empolygonbody.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emsurfaceposprov.h"
#include "lmkemfaulttransl.h"
#include "embodytr.h"
#include "emsticksettransl.h"


mDefModInitFn(EarthModel)
{
    mIfNotFirstTime( return );
    
    EMFault3DTranslatorGroup::initClass();
    EMFaultStickSetTranslatorGroup::initClass();
    EMBodyTranslatorGroup::initClass();
    EMHorizon3DTranslatorGroup::initClass();
    EMHorizon2DTranslatorGroup::initClass();
    EMAnyHorizonTranslatorGroup::initClass();
    
    dgbEMHorizon3DTranslator::initClass();
    dgbEMHorizon2DTranslator::initClass();
    dgbEMFault3DTranslator::initClass();
    lmkEMFault3DTranslator::initClass();
    dgbEMFaultStickSetTranslator::initClass();
    
    mcEMBodyTranslator::initClass();
    polygonEMBodyTranslator::initClass();
    randposEMBodyTranslator::initClass();
    dGBEMBodyTranslator::initClass();
    
    EM::FaultStickSet::initClass();
    EM::Fault3D::initClass();
    EM::Horizon2D::initClass();
    EM::Horizon3D::initClass();
    EM::HorizonZTransform::initClass();
    EM::MarchingCubesSurface::initClass();
    EM::RandomPosBody::initClass();
    EM::PolygonBody::initClass();
    Pos::EMSurfaceProvider3D::initClass();
    Pos::EMSurfaceProvider2D::initClass();
    Pos::EMImplicitBodyProvider::initClass();
}
