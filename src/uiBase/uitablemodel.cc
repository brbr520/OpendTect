/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2019
________________________________________________________________________

-*/


#include "uitablemodel.h"

#include "uiobjbody.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTableView>


class DoubleItemDelegate : public QStyledItemDelegate
{
public:
DoubleItemDelegate()
    : QStyledItemDelegate()
    , nrdecimals_(2)
{}

QString displayText( const QVariant& val, const QLocale& locale ) const
{
    bool ok;
    const double dval = val.toDouble( &ok );
    if ( !ok )
	return QStyledItemDelegate::displayText( val, locale );

    return QString( toString(dval,nrdecimals_) );
}

   int		nrdecimals_;
};


class ODAbstractTableModel : public QAbstractTableModel
{
public:
			ODAbstractTableModel( uiTableModel& mdl )
			    : model_(mdl)		{}

    Qt::ItemFlags	flags(const QModelIndex&) const;
    int			rowCount(const QModelIndex&) const;
    int			columnCount(const QModelIndex&) const;
    QVariant		data(const QModelIndex&,int role) const;
    QVariant		headerData(int rowcol,Qt::Orientation orientation,
				   int role=Qt::DisplayRole) const;
    bool		setData(const QModelIndex&,const QVariant&,int role);
    void		beginReset();
    void		endReset();

protected:
    uiTableModel&	model_;
};


Qt::ItemFlags ODAbstractTableModel::flags( const QModelIndex& qmodidx ) const
{
    if ( !qmodidx.isValid() )
	return Qt::NoItemFlags;

    const int modelflags = model_.flags( qmodidx.row(), qmodidx.column() );
    return sCast(Qt::ItemFlags,modelflags);
}


int ODAbstractTableModel::rowCount( const QModelIndex& ) const
{
    return model_.nrRows();
}


int ODAbstractTableModel::columnCount( const QModelIndex& ) const
{
    return model_.nrCols();
}


QVariant ODAbstractTableModel::data( const QModelIndex& qmodidx,
				     int role ) const
{
    if ( !qmodidx.isValid() )
	return QVariant();

    if ( role == Qt::DisplayRole )
    {
	uiTableModel::CellData cd =
		model_.text( qmodidx.row(), qmodidx.column() );
	return *cd.qvar_;
    }
    if ( role == Qt::BackgroundRole )
    {
	Color odcol = model_.color( qmodidx.row(), qmodidx.column() );
	if ( odcol==Color::NoColor() )
	    return QVariant();

	return QColor( odcol.rgb() );
    }

    if ( role == Qt::ForegroundRole )
    {
	Color odcol = model_.textColor( qmodidx.row(), qmodidx.column() );
	return QColor( odcol.rgb() );
    }

    if ( role == Qt::ToolTipRole )
    {
	const uiString tt =
	    model_.tooltip( qmodidx.row(), qmodidx.column() );
	return tt.getQString();
    }

    return QVariant();
}


bool ODAbstractTableModel::setData( const QModelIndex& qmodidx,
				    const QVariant& qvar, int role )
{
    if ( !qmodidx.isValid() )
	return false;

    return true;
}


QVariant ODAbstractTableModel::headerData( int rowcol, Qt::Orientation orient,
					   int role ) const
{
    if ( role == Qt::DisplayRole )
    {
	uiString str = model_.headerText( rowcol,
	    orient==Qt::Horizontal ? OD::Horizontal : OD::Vertical );
	return str.getQString();
    }

    return QVariant();
}


void ODAbstractTableModel::beginReset()
{ beginResetModel(); }

void ODAbstractTableModel::endReset()
{ endResetModel(); }



// uiTableModel
uiTableModel::uiTableModel()
{
    odtablemodel_ = new ODAbstractTableModel(*this);
}


uiTableModel::~uiTableModel()
{
    delete odtablemodel_;
}


uiTableModel::CellData::CellData() : qvar_(new QVariant())
{}

uiTableModel::CellData::CellData( const char* txt ) : qvar_(new QVariant(txt))
{}

uiTableModel::CellData::CellData( int val ) : qvar_(new QVariant(val))
{}

uiTableModel::CellData::CellData( float val, int ) : qvar_(new QVariant(val))
{}

uiTableModel::CellData::CellData( double val, int ) : qvar_(new QVariant(val))
{}

uiTableModel::CellData::CellData( const CellData& cd )
    : qvar_(new QVariant(*cd.qvar_))
{}

uiTableModel::CellData::~CellData()
{ delete qvar_; }

void uiTableModel::beginReset()
{ odtablemodel_->beginReset(); }

void uiTableModel::endReset()
{ odtablemodel_->endReset(); }



class ODTableView : public uiObjBodyImpl<uiTableView,QTableView>
{
public:
ODTableView( uiTableView& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiTableView,QTableView>(hndl,p,nm)
{}

protected:
};


uiTableView::uiTableView( uiParent* p, const char* nm )
    : uiObject(p,nm,mkView(p,nm))
    , tablemodel_(nullptr)
    , qproxymodel_(nullptr)
{
}


uiTableView::~uiTableView()
{
}


ODTableView& uiTableView::mkView( uiParent* p, const char* nm )
{
    odtableview_ = new ODTableView( *this, p, nm );
    return *odtableview_;
}


void uiTableView::setModel( uiTableModel* mdl )
{
    tablemodel_ = mdl;
    if ( !tablemodel_ )
	return;

    delete qproxymodel_;
    qproxymodel_ = new QSortFilterProxyModel();
    qproxymodel_->setSourceModel(  tablemodel_->getAbstractModel() );
    odtableview_->setModel( qproxymodel_ );
}


void uiTableView::setSortingEnabled( bool yn )
{ odtableview_->setSortingEnabled( yn ); }

bool uiTableView::isSortingEnabled() const
{ return odtableview_->isSortingEnabled(); }


void uiTableView::sortByColumn( int col, bool asc )
{
    odtableview_->sortByColumn( col,
			asc ? Qt::AscendingOrder : Qt::DescendingOrder );
}


void uiTableView::setRowHidden( int row, bool yn )
{ odtableview_->setRowHidden( row, yn ); }

bool uiTableView::isRowHidden( int row ) const
{ return odtableview_->isRowHidden( row ); }

void uiTableView::setColumnHidden( int col, bool yn )
{ odtableview_->setColumnHidden( col, yn ); }

bool uiTableView::iscolumnHidden( int col ) const
{ return odtableview_->isColumnHidden( col ); }


RowCol uiTableView::mapFromSource( const RowCol& rc ) const
{
    QModelIndex sourceidx =
	tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
    QModelIndex qmi = qproxymodel_->mapFromSource( sourceidx );
    return RowCol( qmi.row(), qmi.column() );
}


RowCol uiTableView::mapToSource( const RowCol& rc ) const
{
    QModelIndex proxyidx = qproxymodel_->index( rc.row(), rc.col() );
    QModelIndex qmi = qproxymodel_->mapFromSource( proxyidx );
    return RowCol( qmi.row(), qmi.column() );
}


void uiTableView::setSelectionBehavior( SelectionBehavior sb )
{
    odtableview_->setSelectionBehavior(
		sCast(QAbstractItemView::SelectionBehavior,sCast(int,sb)) );
}


void uiTableView::setSelectionMode( SelectionMode sm )
{
    odtableview_->setSelectionMode(
		sCast(QAbstractItemView::SelectionMode,sCast(int,sm)) );
}


static DoubleItemDelegate* getDoubleDelegate()
{
    mDefineStaticLocalObject( PtrMan<DoubleItemDelegate>, del,
			      = new DoubleItemDelegate );
    return del;
}


void uiTableView::setColumnValueType( int col, CellType tp )
{
    if ( tp==NumD || tp==NumF )
	odtableview_->setItemDelegateForColumn( col, getDoubleDelegate() );
}
