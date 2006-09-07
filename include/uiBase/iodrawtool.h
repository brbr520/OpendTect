#ifndef iodrawtool_h
#define iodrawtool_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawtool.h,v 1.13 2006-09-07 15:44:24 cvskris Exp $
________________________________________________________________________

-*/

#include "iodraw.h"
#include "uigeom.h"
#include "sets.h"
#include "color.h"

class QPaintDevice; 
class QPainter;
class QPen;

class ioPixmap;
class uiFont;
class Color;
class Alignment;
class LineStyle;
class MarkerStyle2D;

#ifndef USEQT4
class QPaintDeviceMetrics; 
#endif


//! Tool to draw on ioDrawArea's. Each ioDrawArea can give you a drawtool.
class ioDrawTool
{   

    friend class	ioDrawAreaImpl;
    friend class	uiScrollViewBody;

mProtected:
			ioDrawTool( QPaintDevice* handle, int x_0=0, int y_0=0);
public:

    virtual		~ioDrawTool(); 

    Color		backgroundColor() const;
    void		setBackgroundColor(const Color&);
    void		clear(const uiRect* r=0,const Color* c=0);

    void		drawLine( int x1, int y1, int x2, int y2 );
    inline void		drawLine( uiPoint p1, uiPoint p2 )
                        { drawLine ( p1.x, p1.y, p2.x, p2.y ); }

    void		drawLine(const TypeSet<uiPoint>&,int idx1=0,int nr=-1);
    			//<!Draws a line defined by 'nr' points starting at idx1
    void		drawPolygon(const TypeSet<uiPoint>&,
	    			    int idx1=0,int nr=-1);
    			/*<!Draws a polygon defined by 'nr' points starting 
    			    at idx1*/

    void		drawText( int x, int y, const char *, const Alignment&, 
				  bool over=true, bool erase=false);
    inline void		drawText(uiPoint p,const char * txt,const Alignment& al,
				 bool over=true,bool erase=false)
                        { drawText( p.x, p.y, txt, al, over, erase ); }

    void 		drawRect( int x, int y, int w, int h ); 
    inline void		drawRect( uiPoint topLeft, uiSize sz )
                        { drawRect( topLeft.x, topLeft.y, 
                                    sz.hNrPics(), sz.vNrPics()); }
    inline void		drawRect( uiRect r )
                        { drawRect( r.left(), r.top(), 
                                    r.hNrPics(), r.vNrPics()); }

    void 		drawEllipse( int x, int y, int w, int h ); 
    inline void		drawEllipse( uiPoint topLeft, uiSize sz )
                        { drawEllipse( topLeft.x, topLeft.y, 
                                       sz.hNrPics(), sz.vNrPics()); }
    inline void 	drawEllipse( uiRect r )
                        { drawEllipse( r.left(), r.top(), 
                                       r.hNrPics(), r.vNrPics()); }

    void		drawBackgroundPixmap(const Color* c=0);

    void 		drawPixmap( uiPoint destTopLeft,
				     ioPixmap*, 
				     uiRect srcAreaInPixmap );
    inline void		drawPixmap( int left, int top, 
				     ioPixmap* pm , 
                                     int sLeft, int sTop,
                                     int sRight, int sBottom )
                        { drawPixmap( uiPoint( left, top ), pm, 
                                      uiRect( sLeft, sTop, sRight, sBottom )); } 

    void		drawMarker(uiPoint,const MarkerStyle2D&,
	    			   const char* txt=0,bool below=true);

    int 		getDevHeight() const;
    int 		getDevWidth() const;

    void		setLineStyle( const LineStyle& );
    void		setPenColor( const Color& );
    void		setFillColor( const Color& );
    void		setPenWidth( unsigned int );

    void		setFont( const uiFont& f);
    const uiFont*	font()				{ return font_; }

    inline void		setOrigin( uiPoint tl ) { setOrigin( tl.x, tl.y ); }
    void		setOrigin( int x_0, int y_0 ) { x0 = x_0; y0 = y_0; }

    bool 		active() const { return active_; }
    bool	        beginDraw(); 
    bool		endDraw();

    void		setRasterXor();
    void		setRasterNorm();

protected:

    bool		setActivePainter( QPainter* );

private:
    QPainter*		qpainter;
    QPen&		qpen;
    bool		freeqpainter;
    QPaintDevice*	qpaintdev;
#ifndef USEQT4
    QPaintDeviceMetrics* qpaintdevmetr;
#endif
    bool		active_;
    int			x0;
    int			y0;
    const uiFont*	font_;
};

#endif
