/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurface.cc,v 1.32 2003-12-08 21:27:59 kristofer Exp $";

#include "emsurface.h"
#include "emsurfaceiodata.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "emhistoryimpl.h"
#include "emmanager.h"
#include "executor.h"
#include "geommeshsurface.h"
#include "grid.h"
#include "ioman.h"
#include "ioobj.h"
#include "linsolv.h"
#include "pca.h"
#include "toplist.h"
#include "ptrman.h"
#include "survinfo.h"


void EM::SurfaceIOData::clear()
{
    dbinfo = "";
    deepErase(valnames);
    deepErase(patches);
}

void EM::SurfaceIOData::use( const EM::Surface& surf )
{
    clear();

    StepInterval<int> hrg;
    surf.getRange( hrg, true );
    rg.start.inl = hrg.start; rg.stop.inl = hrg.stop;
    rg.step.inl = hrg.step;
    surf.getRange( hrg, false );
    rg.start.crl = hrg.start; rg.stop.crl = hrg.stop;
    rg.step.crl = hrg.step;

    for ( int idx=0; idx<surf.nrPatches(); idx++ )
	patches += new BufferString( surf.patchName( surf.patchID(idx) ) );

    for ( int idx=0; idx<surf.nrAuxData(); idx++ )
	valnames += new BufferString( surf.auxDataName(idx) );
}


void EM::SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    selvalues.erase(); selpatches.erase();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
    for ( int idx=0; idx<sd.patches.size(); idx++ )
	selpatches += idx;
}


EM::Surface::Surface( EMManager& man, const EM::ObjectID& id_ )
    : EMObject( man, id_ )
    , step_( SI().getStep(true,false), SI().getStep(false,false) )
    , loadedstep( SI().getStep(true,false), SI().getStep(false,false) )
    , rowinterval(0)
    , colinterval(0)
    , shift(0)
{
    auxdatanames.allowNull(true);
    auxdatainfo.allowNull(true);
    auxdata.allowNull(true);
}


EM::Surface::~Surface()
{
    cleanUp();
}


void EM::Surface::cleanUp()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepErase( *auxdata[idx] );
    }

    deepErase( auxdata );

    deepErase( surfaces );
    deepErase( patchnames );
    patchids.erase();
    origos.erase();

    delete rowinterval;
    delete colinterval;
    rowinterval = 0;
    colinterval = 0;
}


bool EM::Surface::findClosestNodes(TopList<float,EM::PosID>& toplist,
				const Coord3& pos_,
				const MathFunction<float>* time2depthfunc) const
{
    //TODO Make faster impl
    Coord3 origpos = pos_;
    if ( time2depthfunc ) origpos.z = time2depthfunc->getValue( pos_.z );
    const int nrpatches = nrPatches();

    for ( int patch=0; patch<nrpatches; patch++ )
    {
	const EM::PatchID patchid = patchID(patch);
	StepInterval<int> rowrange; StepInterval<int> colrange;
	getRange( rowrange, true ); getRange( colrange, false );

	RowCol rc;
	for ( rc.row=rowrange.start;rc.row<=rowrange.stop;rc.row+=rowrange.step)
	{
	    for ( rc.col=colrange.start; rc.col<=colrange.stop;
		    					rc.col+=colrange.step )
	    {
		if ( isDefined(patchid,rc) )
		{
		    Coord3 pos = getPos( patchid, rc );
		    if ( time2depthfunc )
			pos.z = time2depthfunc->getValue( pos.z );

		    double dist = pos.distance( origpos );
		    toplist.addValue( dist,
				      EM::PosID(id_,patchid,rowCol2SubID(rc)));
		
		}
	    }
	}
    }

    return toplist.size();
}


#define mGetNeigborCoord( coordname, rowdiff, coldiff ) \
for ( int idy=0; idy<nrnodealiases; idy++ ) \
{ \
    const EM::PosID& nodealias = nodealiases[idy]; \
    const EM::PatchID patchid = nodealias.patchID(); \
    const RowCol noderc = subID2RowCol(nodealias.subID()); \
    const RowCol neighborrc( noderc.row rowdiff, noderc.col coldiff ); \
    if ( isDefined(patchid, neighborrc) ) \
    { \
	coordname = getPos(patchid, neighborrc); \
	if ( time2depthfunc ) \
	    coordname.z = time2depthfunc->getValue(coordname.z); \
	break; \
    } \
} \


bool EM::Surface::findClosestMesh(EM::PosID& res, const Coord3& timepos,
			  const MathFunction<float>* time2depthfunc) const
{
    TopList<float, EM::PosID> closestnodes( 20, mUndefValue, false );
    if ( !findClosestNodes(closestnodes,timepos,time2depthfunc) )
	return false;

    const Coord3 pos = time2depthfunc
	? Coord3( timepos, time2depthfunc->getValue( timepos.z ) )
	: timepos;

    float mindist;
    const int nrnodes = closestnodes.size();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	EM::PosID pid = closestnodes.getAssociatedValue(idx);
	Coord3 c00, c10, c01, c11;
	getMeshCoords( pid, c00, c10, c01, c11, time2depthfunc );

	int nrvalidcoords = 0;
	float totaldist = 0;
	if ( c00.isDefined() ) { nrvalidcoords++; totaldist+=c00.distance(pos);}
	if ( c10.isDefined() ) { nrvalidcoords++; totaldist+=c10.distance(pos);}
	if ( c01.isDefined() ) { nrvalidcoords++; totaldist+=c01.distance(pos);}
	if ( c11.isDefined() ) { nrvalidcoords++; totaldist+=c11.distance(pos);}

	if ( nrvalidcoords<3 ) continue;

	totaldist /=nrvalidcoords;
	if ( !idx || totaldist<mindist )
	{
	    res = pid;
	    mindist = totaldist;
	}
    }

    return true;
}


bool EM::Surface::computeMeshNormal( Coord3& res, const EM::PosID& pid,
			     const MathFunction<float>* time2depthfunc ) const
{
    Coord3 c00, c10, c01, c11;
    getMeshCoords( pid, c00, c10, c01, c11, time2depthfunc );
    const bool c00def = c00.isDefined(); const bool c10def = c10.isDefined();
    const bool c01def = c01.isDefined(); const bool c11def = c11.isDefined();

    TypeSet<Coord3> normals;
    if ( c00def && c10def && c01def )
    {
	const Coord3 rowvec = c10-c00;
	const double rowveclen = rowvec.abs();
	if ( !mIS_ZERO(rowveclen) )
	{
	    const Coord3 colvec = c01-c00;
	    const double colveclen = colvec.abs();
	    if ( !mIS_ZERO(colveclen) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c10def && c00def && c11def )
    {
	const Coord3 rowvec = c10-c00;
	const double rowveclen = rowvec.abs();
	if ( !mIS_ZERO(rowveclen) )
	{
	    const Coord3 colvec = c11-c10;
	    const double colveclen = colvec.abs();
	    if ( !mIS_ZERO(colveclen) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c01def && c00def && c11def )
    {
	const Coord3 rowvec = c11-c01;
	const double rowveclen = rowvec.abs();
	if ( !mIS_ZERO(rowveclen) )
	{
	    const Coord3 colvec = c01-c00;
	    const double colveclen = colvec.abs();
	    if ( !mIS_ZERO(colveclen) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c11def && c10def && c01def )
    {
	const Coord3 rowvec = c11-c01;
	const double rowveclen = rowvec.abs();
	if ( !mIS_ZERO(rowveclen) )
	{
	    const Coord3 colvec = c11-c10;
	    const double colveclen = colvec.abs();
	    if ( !mIS_ZERO(colveclen) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    const int nrnormals = normals.size();
    if ( !nrnormals ) return false;
    if ( nrnormals==1 )
    {
	res = normals[0];
	return true;
    }

    Coord3 average(0,0,0);
    for ( int idx=0; idx<nrnormals; idx++ )
	average += normals[idx];

    if ( average.abs() )
    {
	res = average.normalize();
	return true;
    }

    //Note that the sign can be wrong with this method
    PCA pca(3);
    for ( int idx=0; idx<nrnormals; idx++ )
	pca.addSample( normals[idx] );

    if ( !pca.calculate() )
	return false;

    pca.getEigenVector(0, res );
    res = res.normalize();
    return true;
}


bool EM::Surface::computeNormal( Coord3& res, const CubeSampling* cs,
			    const MathFunction<float>* time2depthfunc ) const 
{
    TypeSet<EM::PosID> nodes;
    if ( cs ) findPos(*cs, &nodes );
    else
    {
	for ( int idy=0; idy<nrPatches(); idy++ )
	{
	    const EM::PatchID patchid = patchID(idy);
	    const int nrpatches = nrPatches();

	    StepInterval<int> rowrange; getRange( patchid, rowrange, true );
	    StepInterval<int> colrange; getRange( patchid, colrange, false );

	    RowCol idx( rowrange.start, colrange.start );
	    for ( ; rowrange.includes( idx.row ); idx.row+=rowrange.step )
	    {
		for ( ; colrange.includes( idx.col ); idx.col+=colrange.step )
		{
		    if ( isDefined(patchid,idx) )
		    {
			nodes += EM::PosID(id(),patchid,rowCol2SubID(idx));
		    }
		}
	    }
	}
    }

    return computeNormal( res, nodes, time2depthfunc );
}


bool EM::Surface::computeNormal( Coord3& res, const TypeSet<EM::PosID>& nodes,
			     const MathFunction<float>* time2depthfunc ) const
{
    PCA pca(3);
    const int nrnodes = nodes.size();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	const EM::PosID& node = nodes[idx];

	TypeSet<EM::PosID> nodealiases;
	getLinkedPos( node, nodealiases );
	nodealiases += node;

	const int nrnodealiases = nodealiases.size();

	Coord3 nodecoord = getPos(node);
	if ( time2depthfunc )
	    nodecoord.z = time2depthfunc->getValue(nodecoord.z);

	Coord3 prevrowcoord = nodecoord;
	mGetNeigborCoord( prevrowcoord, -step_.row, +0 );

	Coord3 nextrowcoord = nodecoord;
	mGetNeigborCoord( nextrowcoord, +step_.row, +0 );

	Coord3 prevcolcoord = nodecoord;
	mGetNeigborCoord( prevcolcoord, +0, -step_.col );

	Coord3 nextcolcoord = nodecoord;
	mGetNeigborCoord( nextcolcoord, +0, +step_.col );

	const Coord3 rowvector = nextrowcoord-prevrowcoord;
	const Coord3 colvector = nextcolcoord-prevcolcoord;
	const double rowvectorlen = rowvector.abs();
	const double colvectorlen = colvector.abs();
	const bool rowvectorvalid = !mIS_ZERO(rowvectorlen);
	const bool colvectorvalid = !mIS_ZERO(colvectorlen);

	if ( rowvectorvalid && colvectorvalid )
	{
	    pca.addSample( rowvector.cross(colvector).normalize() );
	    continue;
	}

	if ( rowvectorvalid && nextrowcoord!=nodecoord &&
		prevrowcoord!=nodecoord )
	{
	    const Coord3 prevvector = (prevrowcoord-nodecoord).normalize();
	    const Coord3 nextvector = (nextrowcoord-nodecoord).normalize();
	    const Coord3 average = prevvector+nextvector;
	    const double len = average.abs();
	    if ( mIS_ZERO(len) )
		continue;

	    pca.addSample(average.normalize());
	    continue;
	}

	if ( colvectorvalid && nextcolcoord!=nodecoord &&
		prevcolcoord!=nodecoord )
	{
	    const Coord3 prevvector = (prevcolcoord-nodecoord).normalize();
	    const Coord3 nextvector = (nextcolcoord-nodecoord).normalize();
	    const Coord3 average = prevvector+nextvector;
	    const double len = average.abs();
	    if ( mIS_ZERO(len) )
		continue;

	    pca.addSample(average.normalize());
	    continue;
	}
    }

    if ( !pca.calculate() )
	return false;

    pca.getEigenVector(0, res );
    res = res.normalize();
    return true;
}


char EM::Surface::whichSide( const Coord3& timepos,
			     const MathFunction<float>* time2depthfunc,
			     float fuzzy ) const
{
    EM::PosID closestmesh(0,0,0);
    if ( !findClosestMesh(closestmesh,timepos,time2depthfunc) )
	return -2;

    Coord3 meshnormal;
    if ( !computeMeshNormal(meshnormal,closestmesh,time2depthfunc) )
	return -2;

    Coord3 c00, c10, c01, c11;
    getMeshCoords( closestmesh, c00, c10, c01, c11, time2depthfunc );

    Coord3 average(0,0,0);
    int nrvals = 0;
    if ( c00.isDefined() ) { average += c00; nrvals++; }
    if ( c10.isDefined() ) { average += c10; nrvals++; }
    if ( c01.isDefined() ) { average += c01; nrvals++; }
    if ( c11.isDefined() ) { average += c11; nrvals++; }

    average /= nrvals;

    const Coord3 pos = time2depthfunc
	? Coord3( timepos, time2depthfunc->getValue( timepos.z ) )
	: timepos;

    const Plane3 plane( meshnormal, average, false );
    const Line3 line( pos, meshnormal );
    Coord3 intersection;
    plane.intersectWith( line, intersection );
    const Coord3 vector = pos-intersection;
    double factor = meshnormal.dot( vector );
    if ( factor>fuzzy ) return 1;
    if ( factor<-fuzzy ) return -1;
    return 0;
}


void EM::Surface::getMeshCoords( const EM::PosID& pid,
	Coord3& c00, Coord3& c10, Coord3& c01, Coord3& c11,
	const MathFunction<float>* time2depthfunc ) const
{
    static const Coord3 undefnode = Coord3(mUndefValue,mUndefValue,mUndefValue);

    TypeSet<EM::PosID> nodealiases;
    getLinkedPos( pid, nodealiases );
    nodealiases += pid;
    const int nrnodealiases = nodealiases.size();

    c00 = getPos(pid);
    if ( time2depthfunc ) c00.z = time2depthfunc->getValue(c00.z);

    c10 = undefnode; mGetNeigborCoord( c10, +step_.row, +0 );
    c01 = undefnode; mGetNeigborCoord( c01, +0, +step_.col );
    c11 = undefnode; mGetNeigborCoord( c11, +step_.row, +step_.col );
}


int EM::Surface::nrPatches() const
{
    return patchids.size();
}


EM::PatchID EM::Surface::patchID( int idx ) const
{
    return patchids[idx];
}


EM::PatchID EM::Surface::patchID( const char* nm ) const
{
    for ( int idx=0; idx<patchnames.size(); idx++ )
	if ( *patchnames[idx] == nm ) return patchids[idx];
    return -1;
}


const char* EM::Surface::patchName( const EM::PatchID& patchid ) const
{
    int idx = patchids.indexOf(patchid);
    const char* res = idx!=-1 ? patchnames[idx]->buf() : 0;
    return  res && *res ? res : 0;
}


EM::PatchID EM::Surface::addPatch( const char* nm, bool addtohistory )
{
    PatchID res = 0;
    while ( patchids.indexOf(res)!=-1 ) res++;

    addPatch( nm, res, addtohistory );
    return res;
}


bool EM::Surface::addPatch( const char* nm, PatchID patchid, bool addtohistory )
{
    if ( patchids.indexOf(patchid) != -1 ) return false;

    BufferString name;
    patchids += patchid;
    if ( nm && *nm )
	name = nm;
    else
	{ name = "["; name += patchid + 1; name += "]"; }

    patchnames += new BufferString(name);

    Geometry::MeshSurface* newsurf = createPatchSurface( patchid );
    surfaces += newsurf;

    for ( int idx=0; idx<nrAuxData(); idx++ )
    {
	if ( !auxdata[idx] )
	    continue;

	(*auxdata[idx]) += 0;
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfacePatchEvent( true, id(),
							patchid, name );
	manager.history().addEvent( history, 0, 0 );
    }

    return true;
}


void EM::Surface::removePatch( EM::PatchID patchid, bool addtohistory )
{
    int idx=patchids.indexOf(patchid);
    if ( idx==-1 ) return;

    BufferString name = *patchnames[idx];

    delete surfaces[idx];
    surfaces.remove( idx );
    patchids.remove( idx );
    delete patchnames[idx];
    patchnames.remove( idx );

    for ( int idy=0; idy<nrAuxData(); idy++ )
    {
	if ( !auxdata[idy] )
	    continue;

	delete (*auxdata[idy])[idx];
	auxdata[idy]->replace( 0, idx );
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfacePatchEvent( false, id(),
							patchid, name );
	manager.history().addEvent( history, 0, 0 );
    }
}


bool EM::Surface::setPos( const PatchID& patch, const RowCol& surfrc,
				   const Coord3& pos, bool autoconnect,
				   bool addtohistory)
{
    RowCol geomrowcol;
    if ( !getMeshRowCol( surfrc, geomrowcol, patch ) )
	return false;

    int patchindex=patchids.indexOf(patch);
    if ( patchindex==-1 ) return false;

    const Geometry::PosID posid = Geometry::MeshSurface::getPosID(geomrowcol);
    Geometry::MeshSurface* surface = surfaces[patchindex];
    const Coord3 oldpos = surface->getMeshPos( geomrowcol );

    if ( addtohistory )
    {
	EM::PosID pid( id(), patch, rowCol2SubID(surfrc) );
	HistoryEvent* history = new SetPosHistoryEvent( pos, oldpos, pid );
	manager.history().addEvent( history, 0, 0 );
    }

    if ( oldpos==pos ) return true;

    TypeSet<EM::PosID> nodeonotherpatches;
    if ( autoconnect )
	findPos( geomrowcol, nodeonotherpatches );

    const int auxdataindex = surface->indexOf( geomrowcol );

    surface->setMeshPos( geomrowcol, pos );
    surface->setFillType( geomrowcol, Geometry::MeshSurface::Filled );

    if ( auxdataindex==-1 )
    {
	const int newauxdataindex = surface->indexOf( geomrowcol );
	for ( int idx=0; idx<nrAuxData(); idx++ )
	{
	    if ( !auxdata[idx] ) continue;

	    TypeSet<float>* dataptr = (*auxdata[idx])[patchindex];
	    if ( !dataptr ) continue;

	    dataptr->insert( newauxdataindex, mUndefValue );
	}
    }

    if ( autoconnect )
    {
	for ( int idx=0; idx<nodeonotherpatches.size(); idx++ )
	{
	    const int patchsurfidx =
		patchids.indexOf(nodeonotherpatches[idx].patchID());
	    double otherz = surfaces[patchsurfidx]->getMeshPos(geomrowcol).z;
	    
	    if ( mIS_ZERO(otherz-pos.z) )
	    {
		if ( !surface->isLinked(posid, surfaces[patchsurfidx], posid ))
		{
		    surface->setLink(posid,surfaces[patchsurfidx],posid,true);
		    // Put to history?
		}
	    }
	}
    }

    poschnotifier.trigger( EM::PosID( id(), patch, rowCol2SubID(surfrc)), this);

    return true;
}


bool EM::Surface::setPos( const EM::PosID& posid, const Coord3& newpos,
			  bool addtohistory )
{
    if ( posid.objectID()!=id() ) return false;

    return setPos( posid.patchID(), subID2RowCol(posid.subID()),
	    	   newpos, false,addtohistory);
}


Coord3 EM::Surface::getPos( const EM::PosID& posid ) const
{
    return getPos( posid.patchID(), subID2RowCol(posid.subID()) );
}


Coord3 EM::Surface::getPos( const PatchID& patch, const RowCol& rc) const
{
    const int surfidx = patchids.indexOf( patch );
    RowCol geomnode;
    if ( !getMeshRowCol( rc, geomnode, patch ) )
	return Coord3( mUndefValue, mUndefValue, mUndefValue );

    return surfaces[surfidx]->getMeshPos( geomnode );
}


void EM::Surface::getPos( const RowCol& rc, TypeSet<Coord3>& crdset ) const
{
    const int nrsubsurf = nrPatches();
    for ( int surfidx=0; surfidx<nrsubsurf; surfidx++ )
    {
	Coord3 crd = getPos( patchID(surfidx), rc );
	if ( crd.isDefined() )
	    crdset += crd;
    }
}


bool EM::Surface::isDefined( const EM::PosID& posid ) const
{
    return isDefined( posid.patchID(), subID2RowCol(posid.subID()) );
}


bool EM::Surface::isDefined( const PatchID& patch, const RowCol& rc) const
{
    const int surfidx = patchids.indexOf( patch );
    RowCol geomnode;
    if ( !getMeshRowCol( rc, geomnode, patch ) )
	return false;

    return surfaces[surfidx]->isDefined( geomnode );
}


int EM::Surface::findPos( const RowCol& rowcol,
				  TypeSet<PosID>& res ) const
{
    TypeSet<Coord3> respos;
    const int nrsubsurf = nrPatches();
    for ( PatchID surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::MeshSurface* meshsurf = surfaces[surface];
	if ( !meshsurf->isDefined( rowcol ) )
	    continue;

	Coord3 pos = meshsurf->getMeshPos( rowcol );
	EM::SubID subid = rowCol2SubID( rowcol );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( subid!=res[idx].subID() )
		continue;

	    if ( mIS_ZERO( respos[idx].z-pos.z ) ) continue;

	    res += PosID(id(), patchID(surface), subid );
	    respos += pos;
	}
    }

    return res.size();
}

int EM::Surface::findPos( const EM::PatchID& patchid,
			  const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<EM::PosID>* res ) const	
{
    int idx = patchids.indexOf(patchid);
    if ( idx<0 ) return 0;

    TypeSet<EM::PosID> posids;
    TypeSet<Geometry::PosID> nodes;
    surfaces[idx]->findPos( x.center(), y.center(), z.center(),
			    x.width(), y.width(), z.width(), nodes );

    const int nrnodes = nodes.size();
    for ( int idy=0; idy<nrnodes; idy++ )
    {
	const PatchID patch = patchids[idx];
	const EM::PosID posid( id(), patchid, getSurfSubID(nodes[idy],patchid));

	TypeSet<EM::PosID> clones;
	getLinkedPos( posid, clones );
	clones += posid;

	const int nrclones = clones.size();
	bool found = false;
	for ( int idz=0; idz<nrclones; idz++ )
	{
	    if ( posids.indexOf(clones[idz]) != -1 )
	    { found = true; break; }
	}

	if ( !found )
	    posids += posid;
    }

    if ( res ) res->append(posids);
    return posids.size();
}


int EM::Surface::findPos( const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<EM::PosID>* res ) const	
{
    int sum = 0;
    const int nrpatches = nrPatches();
    for ( int idx=0; idx<nrpatches; idx++ )
	sum += findPos( patchID(idx), x, y, z, res );

    return sum;
}

    
int EM::Surface::findPos( const CubeSampling& cs,
			  TypeSet<EM::PosID>* res ) const
{
    Coord xypos = SI().transform(cs.hrg.start);
    Interval<float> xinterval( xypos.x, xypos.x );
    Interval<float> yinterval( xypos.y, xypos.y );

    xypos = SI().transform(cs.hrg.stop);
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    xypos = SI().transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    xypos = SI().transform( BinID(cs.hrg.stop.inl,cs.hrg.start.crl) );
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    TypeSet<EM::PosID> posids;
    findPos( xinterval, yinterval, cs.zrg, &posids );

    for ( int idx=0; idx<posids.size(); idx++ )
    {
	const EM::PosID& posid = posids[idx];
	const BinID nodebid = SI().transform(getPos(posid));

	if ( nodebid.inl<cs.hrg.start.inl || nodebid.inl>cs.hrg.stop.inl ||
	     nodebid.crl<cs.hrg.start.crl || nodebid.crl>cs.hrg.stop.crl )
	{
	    posids.removeFast( idx-- );
	    continue;
	}
    }

    if ( res ) res->append(posids);
    return posids.size();
}


int EM::Surface::getNeighbors( const EM::PosID& posid_, TypeSet<EM::PosID>* res,
				int maxradius, bool circle ) const
{
    ObjectSet< TypeSet<EM::PosID> > neigbors;
    const RowCol start = subID2RowCol(posid_.subID());
    neigbors += new TypeSet<EM::PosID>( 1, posid_ );

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	for ( int idz=0; idz<neigbors[idx]->size(); idz++ )
	{
	    EM::PosID currentposid = (*neigbors[idx])[idz];
	    const RowCol rowcol = subID2RowCol(currentposid.subID());

	    for ( int row=-step_.row; row<=step_.row; row+=step_.row )
	    {
		for ( int col=-step_.col; col<=step_.col; col+=step_.col )
		{
		    if ( !row && !col ) continue;

		    const RowCol neighborrowcol(rowcol.row+row,rowcol.col+col);
		    const int drow =abs(neighborrowcol.row-start.row)/step_.row;
		    const int dcol =abs(neighborrowcol.col-start.col)/step_.col;

		    if ( drow>maxradius || dcol>maxradius )
			continue;

		    if ( circle && (drow*drow+dcol*dcol)> maxradius*maxradius)
			continue;

		    if ( !isDefined(currentposid.patchID(),neighborrowcol) )
			continue;
		   
		    const EM::PosID
			    neighborposid(currentposid.objectID(),
			    currentposid.patchID(),
			    rowCol2SubID(neighborrowcol) );

		    bool found = false;
		    for ( int idy=0; idy<neigbors.size(); idy++ )
		    {
			const TypeSet<EM::PosID>& posids=*neigbors[idy];
			if ( posids.indexOf(neighborposid)!=-1 )
			{
			    found = true;
			    break;
			}
		    }

		    if ( found )
			continue;

		    TypeSet<EM::PosID>& posids = *new TypeSet<EM::PosID>;
		    getLinkedPos( neighborposid, posids );
		    posids.insert( 0, neighborposid );

		    neigbors += &posids;

		}
	    }
	}
    }


    if ( res )
    {
	// Leave out the fist one, since it's the origin
	for ( int idx=1; idx<neigbors.size(); idx++ )
	{
	    (*res) += (*neigbors[idx])[0];
	}
    }

    const int size = neigbors.size();
    deepErase( neigbors );

    // Leave out the fist one, since it's the origin
    return size-1;
}


void EM::Surface::getLinkedPos( const EM::PosID& posid,
				TypeSet<EM::PosID>& res ) const
{
    if ( posid.objectID()!=id() )
        return; //TODO: Implement handling for this case

    const EM::SubID subid = posid.subID();
    const RowCol rowcol = subID2RowCol(subid);
    const Geometry::MeshSurface* ownmeshsurf = getSurface( posid.patchID() );

    const int nrsubsurf = nrPatches();
    for ( int surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::MeshSurface* meshsurf = surfaces[surface];
	if ( ownmeshsurf->isLinked( subid, meshsurf, subid ) )
	{
	    res += EM::PosID( id(),patchids[surface], subid );
	}
    }
}


bool EM::Surface::isLoaded() const
{
    return nrPatches();
}


RowCol EM::Surface::loadedStep() const
{
    return loadedstep;
}


RowCol EM::Surface::step() const
{
    return step_;
}


void EM::Surface::setTranslatorData( const RowCol& step__,
					const RowCol& loadedstep_,
					const RowCol& origo_,
					const Interval<int>* rowrange_,
					const Interval<int>* colrange_ )
{
    step_ = step__;
    loadedstep = loadedstep_;
    origos += origo_;
    delete rowinterval;
    delete colinterval;
    rowinterval = rowrange_ ? new Interval<int>( *rowrange_ ) : 0;
    colinterval = colrange_ ? new Interval<int>( *colrange_ ) : 0;
}


RowCol EM::Surface::subID2RowCol( const EM::SubID& subid )
{
    return longlong2rc(subid);
}


EM::SubID EM::Surface::rowCol2SubID( const RowCol& rc )
{
    return rc2longlong(rc);
}


bool EM::Surface::isFullResolution() const
{
    return loadedstep == step_;
}


int EM::Surface::nrAuxData() const
{
    return auxdatanames.size();
}


const char* EM::Surface::auxDataName( int dataidx ) const
{
    if ( nrAuxData() && auxdatanames[dataidx] )
	return *auxdatanames[dataidx];

    return 0;
}


void EM::Surface::setAuxDataName( int dataidx, const char* name )
{
    if ( auxdatanames[dataidx] )
	auxdatanames.replace( new BufferString(name), dataidx );
}


int EM::Surface::auxDataIndex( const char* nm ) const
{
    for ( int idx=0; idx<auxdatanames.size(); idx++ )
	if ( *auxdatanames[idx] == nm ) return idx;
    return -1;
}


int EM::Surface::addAuxData( const char* name )
{
    auxdatanames += new BufferString( name );
    ObjectSet<TypeSet<float> >* newauxdata = new ObjectSet<TypeSet<float> >;
    auxdata += newauxdata;
    newauxdata->allowNull(true);

    for ( int idx=0; idx<nrPatches(); idx++ )
	(*newauxdata) += 0;

    return auxdatanames.size()-1;
}


void EM::Surface::removeAuxData( int dataidx )
{
    delete auxdatanames[dataidx];
    auxdatanames.replace( 0, dataidx );

    deepEraseArr( *auxdata[dataidx] );
    delete auxdata[dataidx];
    auxdata.replace( 0, dataidx );
}


void EM::Surface::removeAllAuxdata()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepErase( *auxdata[idx] );
    }

    deepErase( auxdata );
}


float EM::Surface::getAuxDataVal( int dataidx, const EM::PosID& posid ) const
{
    if ( !auxdata[dataidx] ) return mUndefValue;
    const int patchidx = patchids.indexOf( posid.patchID() );
    if ( patchidx==-1 ) return mUndefValue;

    const TypeSet<float>* patchauxdata = (*auxdata[dataidx])[patchidx];
    if ( !patchauxdata ) return mUndefValue;

    RowCol geomrc;
    getMeshRowCol( posid.subID(), geomrc, posid.patchID() );
    const int subidx = surfaces[patchidx]->indexOf( geomrc );
    if ( subidx==-1 ) return mUndefValue;
    return (*patchauxdata)[subidx];
}


void EM::Surface::setAuxDataVal(int dataidx,const EM::PosID& posid, float val)
{
    if ( !auxdata[dataidx] ) return;

    const int patchidx = patchids.indexOf( posid.patchID() );
    if ( patchidx==-1 ) return;

    RowCol geomrc; 
    getMeshRowCol( posid.subID(), geomrc, posid.patchID() );
    const int subidx = surfaces[patchidx]->indexOf( geomrc );
    if ( subidx==-1 ) return;

    TypeSet<float>* patchauxdata = (*auxdata[dataidx])[patchidx];
    if ( !patchauxdata )
    {
	const int sz = surfaces[patchidx]->size();
	auxdata[dataidx]->replace( new TypeSet<float>(sz,mUndefValue),patchidx);
	patchauxdata = (*auxdata[dataidx])[patchidx];
    }

    (*patchauxdata)[subidx] = val;
}


bool EM::Surface::getMeshRowCol( const EM::SubID& subid, RowCol& meshrowcol, 
				 const PatchID& patchid ) const
{
    return getMeshRowCol( subID2RowCol(subid), meshrowcol, patchid );
}


bool EM::Surface::getMeshRowCol( const RowCol& emrowcol, RowCol& meshrowcol,
       				 const PatchID& patchid ) const
{
    const int idx = patchids.indexOf( patchid );
    RowCol origo = origos.size() ? origos[idx] : RowCol(0,0);
    const RowCol relrowcol = emrowcol - origo;
    if ( relrowcol.row%loadedstep.row || relrowcol.col%loadedstep.col )
	return false;

    meshrowcol = relrowcol/loadedstep;
    return true;
}


EM::SubID EM::Surface::getSurfSubID( const RowCol& nodeid, 
				     const PatchID& patchid ) const
{
    const int idx = patchids.indexOf( patchid );
    RowCol origo = origos.size() ? origos[idx] : RowCol(0,0);
    return rowCol2SubID( origo+nodeid*loadedstep );
}


EM::SubID EM::Surface::getSurfSubID( const Geometry::PosID& gposid,
       				     const PatchID& patchid ) const
{
    const RowCol& nodeid = Geometry::MeshSurface::getMeshNode(gposid);
    return getSurfSubID( nodeid, patchid );
}



const Geometry::MeshSurface* EM::Surface::getSurface( PatchID patchid )const
{
    const int idx = patchids.indexOf( patchid );
    return idx==-1 ? 0 : surfaces[idx];
}


void EM::Surface::getRange( StepInterval<int>& rg, bool rowdir ) const
{
    const int nrpatches = nrPatches();
    for ( int idx=0; idx<nrpatches; idx++ )
    {
	const EM::PatchID patchid = patchID( idx );
	StepInterval<int> patchrg;
	getRange( patchID(idx), patchrg, rowdir );
	
	if ( !idx )
	    rg = patchrg;
	else
	{
	    rg.include( patchrg.start ); 
	    rg.include( patchrg.stop );
	}
    }
}


void EM::Surface::getRange( const EM::PatchID& patchid, StepInterval<int>& rg,
			    bool rowdir ) const
{
    const Geometry::MeshSurface& gsurf = *getSurface( patchid );
    if ( rowdir )
    {
	const RowCol firstrow(gsurf.firstRow(),0);
	const RowCol lastrow(gsurf.lastRow(),0);

	rg.start = subID2RowCol( getSurfSubID(firstrow,patchid)).row;
	rg.stop = subID2RowCol( getSurfSubID(lastrow,patchid)).row;
    }
    else
    {
	const Interval<int> colrg = gsurf.getColInterval();
	const RowCol firstrow(0,colrg.start);
	const RowCol lastrow(0,colrg.stop);

	rg.start = subID2RowCol( getSurfSubID(firstrow,patchid)).col;
	rg.stop = subID2RowCol( getSurfSubID(lastrow,patchid)).col;
    }

    rg.step = rowdir ? loadedStep().row : loadedStep().col;
}
