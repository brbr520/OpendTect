#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		May 2001
 Contents:	PickSet base classes
 RCS:		$Id: pickset.h,v 1.9 2003-07-24 11:54:43 nanne Exp $
________________________________________________________________________

-*/

#include <uidobj.h>
#include <sets.h>
#include <position.h>
#include <color.h>


/*!\brief Pick location in space */

class PickLocation
{
public:
		PickLocation( double x=0, double y=0, float f=0 )
		: pos(x,y), z(f)			{}
		PickLocation( const Coord& c, float f=0 )
		: pos(c), z(f)				{}
		PickLocation( const Coord3& c )
		: pos(c.x,c.y), z(c.z)			{}

    inline bool	operator ==( const PickLocation& pl ) const
		{ return pos == pl.pos && mIS_ZERO(z-pl.z); }
    inline bool	operator !=( const PickLocation& pl ) const
		{ return !(*this == pl); }

    bool	fromString(const char*);
    void	toString(char*);

    Coord	pos;
    float	z;

};


/*!\brief Group of picks with a common 'value' at the location */

class PickSet : public UserIDObject
	      , public TypeSet<PickLocation>
{
public:

			PickSet(const char* nm);

    Color		color;

};


/*!\brief Set of Pick Groups */

class PickSetGroup : public UserIDObject
{
public:

			PickSetGroup( const char* nm=0 )
			: UserIDObject(nm)	{}
			~PickSetGroup()		{ clear(); }

    int			nrSets() const		{ return sets.size(); }
    PickSet*		get( int nr )		{ return sets[nr]; }
    const PickSet*	get( int nr ) const	{ return sets[nr]; }
    void		add(PickSet*&);
			//!< PickSet becomes mine. Will merge if necessary.
			//!< So PickSet may be deleted (will be set to null)
    void		remove( int idx )
			{ delete sets[idx]; sets.remove(idx); }
    void		clear()			{ deepErase(sets); }

protected:

    ObjectSet<PickSet> sets;

};


#endif
