#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
________________________________________________________________________

-*/

#include "generalmod.h"
#include "draw.h"
#include "notify.h"
#include "namedmonitorable.h"
#include "odpresentationmgr.h"

namespace OD
{
    class RGBImage;
    class LineStyle;
    class MarkerStyle2D;
}


class MouseEvent;
class TaskRunnerProvider;
template <class T> class ODPolygon;

/*!Object that can be painted in a basemap. */


mExpClass(General) BaseMapObject : public NamedMonitorable
{
public:

    typedef Geom::PosRectangle<double>	BoundingBox;

				BaseMapObject(const char* nm);
				~BaseMapObject();

    int				ID() const		{ return id_; }

    Threads::Lock		lock_;
    virtual void		updateGeometry()			{}

    virtual void		setType(const char* tp) { typenm_ = tp;}
    virtual const char*		getType() const		{ return typenm_; }

    virtual void		setDepth(int val);
    virtual int			getDepth() const	{ return depth_; }
				/*!<Determines what should be painted ontop of
				    what */

    virtual int			nrShapes() const;
    virtual const char*		getShapeName(int shapeidx) const;
    virtual void		getPoints(int shapeidx,TypeSet<Coord>&) const;
				/*!<Returns a number of coordinates that
				    may form a be connected or filled. */
    virtual bool		getBoundingBox(BoundingBox&) const;
				/*!<Returns the Coord range. */
    virtual OD::Alignment	getAlignment(int shapeidx) const;
    virtual float		getTextRotation() const { return 0; }
    virtual Color		getColor() const;

    virtual void		setMarkerStyle(int idx,
					const OD::MarkerStyle2D&) {}
    virtual const OD::MarkerStyle2D* getMarkerStyle(int shapeidx) const
					{ return 0;}
    virtual BufferString	getImageFileName(int idx) const	{ return ""; }
    virtual void		getXYScale(int idx,float& scx,float& scy) const
				{ scx = scy = 1.f; }
    virtual void		setXYScale(int idx,float scx,float scy)	{}

    virtual void		setLineStyle(int shapeidx,
					const OD::LineStyle&) {}
    virtual const OD::LineStyle* getLineStyle(int shapeidx) const { return 0; }

    virtual void		setFillColor(int idx,const Color&)	    {}
    virtual const Color		getFillColor(int idx) const
						    { return Color::NoColor();}

    virtual bool		fill(int shapeidx) const	{ return false;}
    virtual bool		close(int shapeidx) const	{ return false;}
    virtual bool		multicolor(int shapeidx) const	{ return false;}

    virtual void		setImage(int idx,OD::RGBImage*)     {}
    virtual const OD::RGBImage* getImage(int shapeidx) const	{ return 0;}

    virtual const OD::RGBImage* createImage(Coord& origin,Coord& p11) const;
				/*!<Returns image in xy plane. p11 is the
				    coordinate of the corner opposite of the
				    origin. */
    virtual const OD::RGBImage* createPreview(int approxdiagonal) const;
				/*!<Returns a preview image that has
				    approximately the size of the specified
				    diagonal. */
    virtual bool		allowHoverEvent(int) const	{ return true; }

    virtual bool		canRemoveWithPolygon() const   { return false; }
    virtual void		removeWithPolygon(const ODPolygon<double>&) {}

    virtual void		getMousePosInfo(Coord3&,TrcKey&,float& val,
						BufferString& info) const     {}

    bool			fillPar(IOPar&) const;
    bool			usePar(const IOPar&,const TaskRunnerProvider&);

    CNotifier<BaseMapObject,const MouseEvent&>	leftClicked;
    CNotifier<BaseMapObject,const MouseEvent&>	rightClicked;
    Notifier<BaseMapObject>			changed;
    Notifier<BaseMapObject>			styleChanged;
    Notifier<BaseMapObject>			zvalueChanged;
    CNotifier<BaseMapObject,const uiString&>	nameChanged;

protected:

    int				depth_;
    int				id_;
    BufferString		typenm_;

    virtual bool		doFillPar( IOPar& ) const		= 0;
    virtual bool		doUsePar( const IOPar&, const TaskRunnerProvider& )
									= 0;

};

static OD::ViewerTypeID theViewerBasemapTypeID( OD::ViewerTypeID::get(2) );

/*!Base class for a Basemap. */
mExpClass(General) BaseMap : public OD::PresentationManagedViewer
{
public:

				BaseMap();
    OD::ViewerTypeID		viewerTypeID() const
				{ return theViewerTypeID(); }
    static OD::ViewerTypeID	theViewerTypeID()
				{ return theViewerBasemapTypeID; }
    virtual void		addObject(BaseMapObject*)		= 0;
				/*!<Object maintained by caller. Adding an
				    existing will trigger update */

    virtual void		removeObject(const BaseMapObject*)	= 0;

};
