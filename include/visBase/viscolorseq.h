#ifndef viscolorseq_h
#define viscolorseq_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscolorseq.h,v 1.10 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "visdata.h"

namespace ColTab { class Sequence; }


namespace visBase
{

/*!\brief
ColorSequence describes a basic sequence of colors on a scale ranging from zero
to one.
*/

mClass ColorSequence : public DataObject
{
public:
    static ColorSequence*	create()
				mCreateDataObj(ColorSequence); 

    void			loadFromStorage(const char*);

    ColTab::Sequence&		colors();
    const ColTab::Sequence&	colors() const;
    void			colorsChanged();
				/*!< If you change the Colortable, notify me
				     with this function */

    Notifier<ColorSequence>	change;

    int				usePar(const IOPar&);
    void			fillPar(IOPar&,TypeSet<int>&) const;

protected:
    virtual			~ColorSequence();

    ColTab::Sequence&		coltabsequence_;
};

}; // Namespace


#endif
