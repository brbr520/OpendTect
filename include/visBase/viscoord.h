#ifndef viscoord_h
#define viscoord_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscoord.h,v 1.3 2003-01-07 10:20:14 kristofer Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "position.h"
#include "vissceneobj.h"

class SoCoordinate3;
class Executor;

namespace Geometry { class PosIdHolder; }

namespace visBase
{


/*!\brief
A set of coordinates.
*/

class Coordinates : public SceneObject
{
public:

    static Coordinates*	create()
			mCreateDataObj(Coordinates);
    friend		class CoordinatesBuilder;

    int			size(bool includedelete=false) const;
    int			addPos( const Coord3& );
    Coord3		getPos( int ) const;
    void		setPos( int,  const Coord3& );
    void		removePos( int );

    SoNode*		getData();
protected:

    			~Coordinates();
     SoCoordinate3*	coords;
     TypeSet<int>	unusedcoords;
};

/*
class CoordinateMessage
{
public:
    enum Event		{ ChangedPos, NrChanged } event;
    unsigned long	coordnr;
    unsigned long	newnr; //Only set when NrChanged
};
*/


};

#endif
