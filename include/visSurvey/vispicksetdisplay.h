#ifndef vissurvpickset_h
#define vissurvpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.13 2002-04-29 09:28:16 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "geompos.h"
#include "vissurvobj.h"

class Color;

namespace visBase { class SceneObjectGroup; class EventCatcher; };
namespace Geometry { class Pos; }

namespace visSurvey
{

class Scene;

/*!\brief


*/

class PickSetDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static PickSetDisplay*	create()
				mCreateDataObj0arg(PickSetDisplay);

    int				nrPicks() const;
    Geometry::Pos		getPick( int idx ) const;
    void			addPick( const Geometry::Pos& );
    void			removePick( const Geometry::Pos& );
    void			removeAll();

    float			getXSz() const { return xsz; }
    float			getYSz() const { return ysz; }
    float			getZSz() const { return zsz; }

    void			setSize( float inl, float crl, float t );
    Notifier<PickSetDisplay>	changed;

protected:
    virtual		~PickSetDisplay();

    void		pickCB( CallBacker* =0 );
    void		updateCubeSz( CallBacker* = 0);

    float		xsz;
    float		ysz;
    float		zsz;

    int			mousepressid;
    Geometry::Pos	mousepressposition;

    visBase::SceneObjectGroup*	group;
    visBase::EventCatcher*	eventcatcher;
};

};


#endif
