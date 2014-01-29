/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odgraphicsitem.h"

#include "enums.h"
#include "geometry.h"
#include "pixmap.h"
#include "uifont.h"
#include "uimain.h"

#include <math.h>

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QRectF>
#include <QRgb>
#include <QStyleOption>
#include <QTextDocument>
#include <QMetaObject>
#include <QGraphicsScene>

mUseQtnamespace

QRectF ODGraphicsPointItem::boundingRect() const
{
    return highlight_ ? QRectF( -2, -2, 4, 4 )
		      : QRectF( -1,-1, 2, 2 );
}


void ODGraphicsPointItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget *widget )
{
    painter->setPen( pen() );
    drawPoint( painter );

    if ( option->state & QStyle::State_Selected )
    {
	painter->setPen( QPen(option->palette.text(),1.0,
		    	 		Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}
	

void ODGraphicsPointItem::drawPoint( QPainter* painter )
{
    painter->setPen( pen() );
    QPoint pts[13]; int ptnr = 0;
    #define mSetPt(ox,oy) pts[ptnr].setX(ox); pts[ptnr].setY(oy); ptnr++;
    mSetPt( 0, 0 );
    mSetPt( -1, 0 ); mSetPt( 1, 0 );
    mSetPt( 0, -1 ); mSetPt( 0, 1 );
    if ( highlight_ )
    {
	mSetPt( -1, -1 ); mSetPt( 1, -1 );
	mSetPt( -1, 1 ); mSetPt( 1, 1 );
	mSetPt( 2, 0 ); mSetPt( -2, 0 );
	mSetPt( 0, 2 ); mSetPt( 0, -2 );
    }

    for ( int idx=0; idx<13; idx++ )	
	painter->drawPoint( pts[idx] );
}



ODGraphicsMarkerItem::ODGraphicsMarkerItem()
    : QAbstractGraphicsShapeItem()
    , mstyle_( new MarkerStyle2D() )
    , fill_(false)
{}


ODGraphicsMarkerItem::~ODGraphicsMarkerItem()
{ delete mstyle_; }


void ODGraphicsMarkerItem::setMarkerStyle( const MarkerStyle2D& mstyle )
{
    const char* typestr = MarkerStyle2D::getTypeString( mstyle.type_ );
    if ( mstyle.isVisible() || mstyle.size_ != 0 || !typestr || !*typestr )
	*mstyle_ = mstyle;
}


QRectF ODGraphicsMarkerItem::boundingRect() const
{
    return QRectF( -mstyle_->size_, -mstyle_->size_, 
	    	             2*mstyle_->size_, 2*mstyle_->size_ );
}


void ODGraphicsMarkerItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget* widget )
{
   /* if ( side_ != 0 )
    pErrMsg( "TODO: implement single-sided markers" );
    if ( !mIsZero(angle_,1e-3) )
    pErrMsg( "TODO: implement tilted markers" );*/

    const QPointF p00 = mapToScene( QPointF(0,0) );
    const QPointF d01 = mapToScene( QPointF(0,1) )-p00;
    const QPointF d10 = mapToScene( QPointF(1,0) )-p00;

    const float xdist = Math::Sqrt(d10.x()*d10.x()+d10.y()*d10.y() );
    const float ydist = Math::Sqrt(d01.x()*d01.x()+d01.y()*d01.y() );
    if ( !xdist || !ydist )
	return;

    const float szx = mstyle_->size_ / xdist;
    const float szy = mstyle_->size_ / ydist;

    painter->setPen( pen() );
    if ( fill_ )
	painter->setBrush( QColor(QRgb(fillcolor_.rgb())) );

    drawMarker( *painter, mstyle_->type_, szx, szy );

    if ( option->state & QStyle::State_Selected )
    {
	painter->setPen( QPen(option->palette.text(),1.0,
		    			Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}


void ODGraphicsMarkerItem::drawMarker( QPainter& painter,
		    MarkerStyle2D::Type typ, float szx, float szy )
{
    switch ( typ )
    {
	case MarkerStyle2D::Square:
	    painter.drawRect( QRectF(-szx, -szy, 2*szx, 2*szy) );
	    break;
	
	case MarkerStyle2D::Target:
	    szx /=2;
	    szy /=2;
	case MarkerStyle2D::Circle:
	    painter.drawEllipse( QRectF( -szx, -szy, 2*szx, 2*szy) );
	    break;

	case MarkerStyle2D::Cross:
	    painter.drawLine( QLineF(-szx, -szy, +szx, +szy) );
	    painter.drawLine( QLineF(-szx, +szy, +szx, -szy) );
	    break;

	case MarkerStyle2D::HLine:
	    painter.drawLine( QLineF( -szx, 0, +szx, 0 ) );
	    break;

	case MarkerStyle2D::VLine:
	    painter.drawLine( QLineF( 0, -szy, 0, +szy ) );
	    break;

	case MarkerStyle2D::Plus:
	    drawMarker( painter, MarkerStyle2D::HLine, szx, szy );
	    drawMarker( painter, MarkerStyle2D::VLine, szx, szy );
	    break;

	case MarkerStyle2D::Plane:
	    painter.drawRect( QRectF(-3*szx, -szy/2, 6*szx, szy) );
	    break;

	case MarkerStyle2D::Triangle: {
	    QPolygonF triangle;
	    triangle += QPointF( -szx, 0 );
	    triangle += QPointF( 0, -2*szy );
	    triangle += QPointF( +szx, 0 );
	    painter.drawPolygon( triangle );
	    } break;

	case MarkerStyle2D::Arrow:
	    drawMarker( painter, MarkerStyle2D::VLine, 2*szx, 2*szy );
	    drawMarker( painter, MarkerStyle2D::Triangle, -szx, -szy );
	    break;
	case MarkerStyle2D::None:
	    break;
    }
}


ODGraphicsArrowItem::ODGraphicsArrowItem()
    : QAbstractGraphicsShapeItem()
{
}


QRectF ODGraphicsArrowItem::boundingRect() const
{
    return QRectF( -arrowsz_, -arrowsz_/2, arrowsz_, arrowsz_ );
}


void ODGraphicsArrowItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget* widget )
{
    painter->setClipRect( option->exposedRect );
    painter->setPen( pen() );
    drawArrow( *painter );

    if (option->state & QStyle::State_Selected)
    {
	painter->setPen( QPen(option->palette.text(),1.0,
		    			Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}


void ODGraphicsArrowItem::drawArrow( QPainter& painter )
{
    setLineStyle( painter, arrowstyle_.linestyle_ );

    QPoint qpointtail( -arrowsz_, 0 );
    QPoint qpointhead( 0, 0 );
    painter.drawLine( qpointtail.x(), qpointtail.y(), qpointhead.x(),
	    	      qpointhead.y() ); 
    if ( arrowstyle_.hasHead() )
	drawArrowHead( painter, qpointhead, qpointtail );
    if ( arrowstyle_.hasTail() )
	drawArrowHead( painter, qpointtail, qpointhead );
}


void ODGraphicsArrowItem::setLineStyle( QPainter& painter,
					const LineStyle& ls )
{
    pen().setStyle( (Qt::PenStyle)ls.type_ );
    pen().setColor( QColor(QRgb(ls.color_.rgb())) );
    pen().setWidth( ls.width_ );

    painter.setPen( pen() );
}


void ODGraphicsArrowItem::drawArrowHead( QPainter& painter,
					 const QPoint& qpt,
					 const QPoint& comingfrom )
{
    const float headangfac = .82; // bigger => lines closer to main line

    // In UI, Y is positive downward
    const QPoint relvec( qpt.x() - comingfrom.x(),
	    			   comingfrom.y() - qpt.y() );
    const double ang( atan2((double)relvec.y(),(double)relvec.x()) );

    const ArrowHeadStyle& headstyle = arrowstyle_.headstyle_;
    if ( headstyle.handedness_ == ArrowHeadStyle::TwoHanded )
    {
	switch ( headstyle.type_ )
	{
	    case ArrowHeadStyle::Square:
	    {
	        TypeSet<QPoint> polypts;
		polypts += qpt;
	        const QPoint pt1=getEndPoint(qpt,M_PI,headstyle.sz_);
	        const QPoint pt2 = getEndPoint(qpt, -(M_PI),headstyle.sz_);
		polypts += pt1;
		polypts += pt2;
		painter.drawPolygon( polypts.arr(), 3 );
		break;
	    }
	    case ArrowHeadStyle::Cross:
	    {
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,.25),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,.75),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,-.25),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,-.75),headstyle.sz_/2)) );
		break;
	    }
	    case ArrowHeadStyle::Triangle:
	    case ArrowHeadStyle::Line:
	    {
		const QPoint rightend = getEndPoint( qpt,
		    getAddedAngle( ang,headangfac), headstyle.sz_ );
		const QPoint leftend = getEndPoint( qpt,
		    getAddedAngle( ang,-headangfac), headstyle.sz_ );
		painter.drawLine( qpt, rightend );
		painter.drawLine( qpt, leftend );
		if ( headstyle.type_ == ArrowHeadStyle::Triangle )
		    painter.drawLine( leftend, rightend );
		break;
	    }
	}
    }
}


double ODGraphicsArrowItem::getAddedAngle( double ang, float ratiopi )
{
    ang += ratiopi * M_PI;
    while ( ang < -M_PI ) ang += 2 * M_PI;
    while ( ang > M_PI ) ang -= 2 * M_PI;
    return ang;
}


QPoint ODGraphicsArrowItem::getEndPoint( const QPoint& pt,
				         double angle, double len )
{
    QPoint endpt( pt.x(), pt.y() );
    double delta = len * cos( angle );
    endpt.setX( pt.x() + mNINT32(delta) );
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.setY( pt.y() + mNINT32(delta) );
    return endpt;
}


void ODViewerTextItem::setText( const char* t )
{
    prepareGeometryChange();
    text_ = t;
}


QPointF ODViewerTextItem::getAlignment() const
{
    float movex = 0, movey = 0;
    
    switch ( hal_ )
    {
        case Qt::AlignRight:
            movex = -1.0f;
            break;
        case Qt::AlignHCenter:
            movex = -0.5f;
            break;
    }
    
    switch ( val_ )
    {
        case Qt::AlignBottom:
            movey = -1;
            break;
        case Qt::AlignVCenter:
            movey = -0.5f;
            break;
    }
    
    return QPointF( movex, movey );
}


QRectF ODViewerTextItem::boundingRect() const
{
    const QPointF alignment = getAlignment();
    
    QFontMetrics qfm( getFont() );
    const float txtwidth = qfm.width( QString(text_.buf()) );
    const float txtheight = qfm.height();
    
    const float movex = alignment.x() * txtwidth;
    const float movey = alignment.y() * txtheight;

    const QPointF paintpos = mapToScene( QPointF(0,0) );
    const QRectF scenerect( paintpos.x()+movex-5, paintpos.y()+movey-5,
			    txtwidth+10, txtheight+10 );//Extra space is added
    //to avoid clipping on some platforms & the number 5 is arbitrarily chosen.
    return mapRectFromScene( scenerect );
}


void ODViewerTextItem::paint( QPainter* painter,
			      const QStyleOptionGraphicsItem *option,
			      QWidget *widget )
{
    if ( option )
	painter->setClipRect( option->exposedRect );
    
    QPointF paintpos( 0, 0 );
    
    paintpos = painter->worldTransform().map( paintpos );

    painter->save();
    painter->resetTransform();
    
    const QString text( text_.buf() );
    
    const QPointF alignment = getAlignment();
    
    QFontMetrics qfm( getFont() );
    const float txtwidth = qfm.width( text );
    const float txtheight = qfm.height();
    
    const float movex = alignment.x() * txtwidth;
    const float movey = alignment.y() * txtheight;
    
    painter->setPen( pen() );
    painter->setFont( font_ );
    
    //Nice for debugging
    //painter->drawPoint( paintpos.x(), paintpos.y() );
    
    painter->drawText(
	    QPointF(paintpos.x() + movex, paintpos.y()+movey+txtheight), text );
		  
    painter->restore();
    
    //Nice for debugging
    //painter->drawRect( boundingRect() );
}


ODGraphicsPixmapItem::ODGraphicsPixmapItem()
    : QGraphicsPixmapItem()
{}


ODGraphicsPixmapItem::ODGraphicsPixmapItem( const ioPixmap& pm )
    : QGraphicsPixmapItem(*pm.qpixmap())
{}


void ODGraphicsPixmapItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget* widget )
{
    painter->setClipRect( option->exposedRect );
    QGraphicsPixmapItem::paint( painter, option, widget );
}



ODGraphicsPolyLineItem::ODGraphicsPolyLineItem()
    : QAbstractGraphicsShapeItem()
    , closed_( false )
{}


QRectF ODGraphicsPolyLineItem::boundingRect() const
{
    return qpolygon_.boundingRect();
}


void ODGraphicsPolyLineItem::paint( QPainter* painter,
			       const QStyleOptionGraphicsItem* option,
			       QWidget* widget )
{
    const QTransform trans = painter->worldTransform();
    
    QPolygonF paintpolygon( qpolygon_.size() );
    for ( int idx=qpolygon_.size()-1; idx>=0; idx-- )
	paintpolygon[idx] = trans.map( qpolygon_[idx] );
    
    painter->save();
    painter->resetTransform();

    
    painter->setPen( pen() );
    
    if ( closed_ )
    {
	painter->setBrush( brush() );
	painter->drawPolygon( paintpolygon, fillrule_ );
    }
    else
    {
    	painter->drawPolyline( paintpolygon );
    }
    
    painter->restore();
}


ODGraphicsDynamicImageItem::ODGraphicsDynamicImageItem()
    : wantsData( this )
    , bbox_( 0, 0, 1, 1 )
    , updatedynpixmap_( false )
    , updatebasepixmap_( false )
{}


ODGraphicsDynamicImageItem::~ODGraphicsDynamicImageItem()
{
    
}

#if QT_VERSION>=0x040700
# define mImage2PixmapImpl( image, pixmap ) pixmap->convertFromImage( image )
#else
# define mImage2PixmapImpl( image, pixmap ) \
   *pixmap = QPixmap::fromImage( image, Qt::OrderedAlphaDither )
#endif


#ifdef __mac__
# define mImage2Pixmap( image, pixmap ) \
 pixmap = new QPixmap; \
 mImage2PixmapImpl( image, pixmap )
#else
# define mImage2Pixmap( image, pixmap ) \
 if ( !pixmap ) \
    pixmap = new QPixmap; \
 mImage2PixmapImpl( image, pixmap )
#endif

void ODGraphicsDynamicImageItem::setImage( bool isdynamic,
					   const QImage& image,
					   const QRectF& rect )
{
    imagelock_.lock();
    if ( isdynamic )
    {
	dynamicimage_ = image;
	dynamicimagebbox_ = rect;
	updatedynpixmap_ = true;
	dynamicrev_[0] = false;
	dynamicrev_[1] = false;
    }
    else
    {
	bbox_ = rect;
	baseimage_ = image;
	updatebasepixmap_ = true;
	baserev_[0] = false;
	baserev_[1] = false;
    }

    imagelock_.unlock();

    if ( isMainThreadCurrent() )
    {
	update();
    }
    else
    {
	QObject* qobj = scene();
	if ( qobj && !QMetaObject::invokeMethod( qobj, "update",
					     Qt::QueuedConnection ))
	{
	    pErrMsg("Cannot invoke method");
	}
    }
}


void ODGraphicsDynamicImageItem::paint(QPainter* painter,
			      const QStyleOptionGraphicsItem* option,
			      QWidget* widget )
{
    if ( updateResolution( painter ) )
	wantsData.trigger();
    
    const QTransform worldtrans = painter->worldTransform();
    
    imagelock_.lock();
    
    const QPointF pix00 = worldtrans.map( QPointF(0,0) );
    const QPointF pix11 = worldtrans.map( QPointF(1,1) );
    
    const bool revx = pix00.x()>pix11.x();
    const bool revy = pix00.y()>pix11.y();

    const bool dynmirror = (revx!=dynamicrev_[0] || revy!=dynamicrev_[1]);

    if ( updatedynpixmap_ || dynmirror )
    {
	if ( dynamicimagebbox_.isValid() )
	{
	    if ( dynmirror )
	    {
		mImage2Pixmap( dynamicimage_.mirrored( revx, revy ),
			       dynamicpixmap_ );
		dynamicrev_[0] = revx;
		dynamicrev_[1] = revy;
	    }
	    else
	    {
		mImage2Pixmap( dynamicimage_, dynamicpixmap_ );
		
		dynamicrev_[0] = false;
		dynamicrev_[1] = false;
	    }
	}
	else
	{
	    dynamicpixmap_ = 0;
	}
	   
	dynamicpixmapbbox_ = dynamicimagebbox_;
	updatedynpixmap_ = false;
    }

    bool paintbase = true;
    QRect dynamicscenerect;

    //Check if we cover everything
    if ( dynamicpixmap_ )
    {
	dynamicscenerect = worldtrans.mapRect(dynamicpixmapbbox_).toRect();
	paintbase = !dynamicscenerect.contains( painter->viewport() );
    }

    if ( paintbase )
    {
	const bool basemirror = (revx!=baserev_[0] || revy!=baserev_[1]);
	if ( !basepixmap_ || updatebasepixmap_ || basemirror )
	{
	    if ( basemirror )
	    {
		mImage2Pixmap( baseimage_.mirrored( revx, revy ), basepixmap_ );
		
		baserev_[0] = revx;
		baserev_[1] = revy;
	    }
	    else
	    {
	    	mImage2Pixmap( baseimage_, basepixmap_ );
		baserev_[0] = false;
		baserev_[1] = false;
	    }
	}

	updatebasepixmap_ = false;
    }

    imagelock_.unlock();

    painter->save();
    painter->resetTransform();

    if ( paintbase )
    {
	const QRect scenerect = worldtrans.mapRect(bbox_).toRect();
	painter->drawPixmap( scenerect, *basepixmap_ );
    }


    if ( dynamicpixmap_ )
	painter->drawPixmap( dynamicscenerect, *dynamicpixmap_ );

    painter->restore();
}


bool ODGraphicsDynamicImageItem::updateResolution( const QPainter* painter )
{
    const QRectF viewport = painter->viewport();
    const QRectF projectedwr =
	painter->worldTransform().inverted().mapRect( viewport );

    const QRectF wantedwr = projectedwr.intersected( bbox_ );
    if ( !wantedwr.isValid() )
	return false;
    
    const QSize wantedscreensz =
    	painter->worldTransform().mapRect(wantedwr).toRect().size();

    if ( wantedwr==wantedwr_ && wantedscreensz==wantedscreensz_)
	return false;

    wantedwr_ = wantedwr;
    wantedscreensz_ = wantedscreensz;
    return true;
}
