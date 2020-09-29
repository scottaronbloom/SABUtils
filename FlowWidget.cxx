#undef QT_NO_DEBUG_OUTPUT
#include "FlowWidget.h"

#include <unordered_map>
#include <QAbstractButton>
#include <QStyleOptionToolBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QApplication>
#include <QPainter>
#include <QTreeWidget>
#include <QHeaderView>
#include <QHelpEvent>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QItemDelegate>
#include <QTextOption>
#include <QTextLayout>
#include <QSizeF>
#include <QToolTip>
#include <QHelpEvent>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QDir>
#include <QScrollBar>
#include <cmath>

using TFlowWidgetItems = std::vector< std::unique_ptr< CFlowWidgetItem > >;
using TDataMap = std::map< int, QVariant >;

#ifdef _DEBUG
static QString dumpRect( const QRect & xRect )
{
    QString lRetVal = QString( "QRect((%1,%2),(%3,%4) %5x%6)").arg( xRect.left() ).arg( xRect.top() ).arg( xRect.right() ).arg( xRect.bottom() ).arg( xRect.size().width() ).arg( xRect.size().height() );
    return lRetVal;
}
#endif

// from QItemDelegatePrivate::textLayoutBounds
static QRect mTextLayoutBounds( const QRect& xRect, int lMargin, bool xElideText )
{
    auto lRetVal = xRect;
    lRetVal.adjust( lMargin, 0, -lMargin, 0 );
    if ( !xElideText )
        lRetVal.setWidth( INT_MAX / 256 );
    return lRetVal;
}

static QSizeF mDoTextLayout( QTextLayout& lTextLayout, int lLineWidth )
{
    qreal height = 0;
    qreal widthUsed = 0;
    lTextLayout.beginLayout();
    while ( true )
    {
        QTextLine line = lTextLayout.createLine();
        if ( !line.isValid() )
            break;
        line.setLineWidth( lLineWidth );
        line.setPosition( QPointF( 0, height ) );
        height += line.height();
        widthUsed = qMax( widthUsed, line.naturalTextWidth() );
    }
    lTextLayout.endLayout();
    return QSizeF( std::min( widthUsed, 1.0*lLineWidth ), height );
}


static int mComputeMaxAllowedTextWidth( int xFullWidth, const QPixmap& xIdentityPM, const QList< QPixmap >& xStatusPMs )
{
    auto lRetVal = xFullWidth;
    if ( !xIdentityPM.isNull() )
        lRetVal -= (xIdentityPM.width() / xIdentityPM.devicePixelRatio()) + 4;
    for ( int ii = 0; ii < xStatusPMs.count(); ++ii )
        lRetVal -= (xStatusPMs[ ii ].width() / xStatusPMs[ ii ].devicePixelRatio()) + 4;
    return lRetVal;
}

static std::pair< int, QString > mComputeTextWidth( const QFontMetrics & lFM, const QString & xText, const std::pair< bool, int >& xMaxWidth )
{
    auto lRetVal = lFM.boundingRect( xText );
    if ( xMaxWidth.first && (lRetVal.width() > xMaxWidth.second) )
        lRetVal.setWidth( xMaxWidth.second );
    auto lElidedText = lFM.elidedText( xText, Qt::ElideRight, lRetVal.width() );
    while ( (lElidedText != xText) && (!xMaxWidth.first || (lRetVal.width() < xMaxWidth.second)) )
    {
        lRetVal.setWidth( lRetVal.width() * 1.1 );
        lElidedText = lFM.elidedText( xText, Qt::ElideRight, lRetVal.width() );
    }
    return std::make_pair( lRetVal.width(), lElidedText );
}

static std::pair< int, QString > mComputeTextWidth( QStyleOptionToolBox& lOption, const std::pair< bool, int >& xMaxWidth )
{
    return mComputeTextWidth( lOption.fontMetrics, lOption.text, xMaxWidth );
}

static std::pair< QRect, QString > mCalcDisplayRect( const QFont & lFont, QString lText, bool xElide, const QRect & xRect )
{
    QTextOption lTextOption;
    lTextOption.setWrapMode( xElide ? QTextOption::NoWrap :QTextOption::WordWrap );

    QTextLayout lTextLayout;
    lTextLayout.setTextOption( lTextOption );
    lTextLayout.setFont( lFont );
    lTextLayout.setText( lText.replace( '\n', QChar::LineSeparator ) );
    const int lTextMargin = QApplication::style()->pixelMetric( QStyle::PM_FocusFrameHMargin, nullptr ) + 1;
    auto lFPSize = mDoTextLayout( lTextLayout, mTextLayoutBounds( xRect, lTextMargin, xElide ).width() );
    const QSize lSize = QSize( std::ceil( lFPSize.width() ), std::ceil( lFPSize.height() ) );
    std::pair< int, QString > lTextWidth( lSize.width() + 2*lTextMargin, lText );
    if ( xElide )
        lTextWidth =  mComputeTextWidth( QFontMetrics( lFont ), lText, std::make_pair( xElide, lSize.width() ) );
    auto lRect = QRect( 0, 0, lTextWidth.first, lSize.height() );
    return std::make_pair( lRect, lTextWidth.second );
}


static void mExpandAll( QTreeWidgetItem * xItem )
{
    if ( !xItem )
        return;
    xItem->setExpanded( true );
    for( int ii = 0; ii < xItem->childCount(); ++ii )
    {
        auto xChild = xItem->child( ii );
        if ( !xChild )
            return;
        mExpandAll( xChild );
    }
}

class CFlowWidgetItemDelegate;
class CFlowWidgetHeader : public QAbstractButton
{
    Q_OBJECT
public:
    friend class CFlowWidgetImpl;
    friend class CFlowWidgetItemImpl;
    friend class CFlowWidgetItemDelegate;

    CFlowWidgetHeader( CFlowWidgetItem* xContainer, CFlowWidget* xParent );
    ~CFlowWidgetHeader();

    CFlowWidget* mFlowWidget() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void mSetText( const QString& text ) 
    { 
        setText( text ); 
        mSetObjectName();
    }
    QString mText() const { return text(); }

    void mSetIcon( const QIcon& icon ) { return setIcon( icon ); }
    QIcon mIcon() const { return icon(); }

    void mSetToolTip( const QString& tooltip ) { return setToolTip( tooltip ); }

    void mAddChild( CFlowWidgetItem* xChild ) { mInsertChild( -1, xChild ); }
    int mInsertChild( int xIndex, CFlowWidgetItem* xChild );
    void mAddChildren( const TFlowWidgetItems & xChildren );

    int mIndexOfChild( const CFlowWidgetItem* xItem ) const;
    CFlowWidgetItem* mTakeChild( CFlowWidgetItem* xItem );
    int mChildCount() const;
    CFlowWidgetItem* mGetChild( int xIndex ) const;

    void mSetVisible( bool xVisible, bool xExpandIfShow );
    bool mIsVisible() const;

    void mSetDisabled( bool xDisabled ) { return mSetEnabled( !xDisabled ); }
    bool mIsDisabled() const { return !mIsEnabled(); }

    void mSetEnabled( bool xEnabled );
    bool mIsEnabled() const { return dEnabled.first && isEnabled(); }

    bool mSelected() const;
    void mSetSelected( bool b ) { mSetExpanded( b ); }

    void mSetExpanded( bool xExpanded );
    bool mIsExpanded() const;

    void mUpdate();

    void mSetObjectName()
    {
        QAbstractButton::setObjectName( QLatin1String( "flowwidgetitem_flowwidgetheader" ) + "_" + mText() );
        dTreeWidget->setObjectName( QLatin1String( "flowwidgetheader_treewidget" ) + "_" + mText() );
    }

    CFlowWidgetItem* mSelectedItem() const
    {
        if ( !dTreeWidget )
            return nullptr;
        auto lSelectedItems = dTreeWidget->selectedItems();
        if ( lSelectedItems.isEmpty() )
            return nullptr;
        return mFindFlowItem( lSelectedItems.front() );
    }

    CFlowWidgetItem* mFindFlowItem( QTreeWidgetItem* xItem ) const;
    void mAddToMap( QTreeWidgetItem* xItem, CFlowWidgetItem* xFlowItem )
    {
        dAllChildItemMap[ xItem ] = xFlowItem;
    }

    virtual void mouseDoubleClickEvent( QMouseEvent* /*xEvent*/ ) override
    {
        emit sigDoubleClicked();
    }

    void mSetElideText( bool xElideText );
Q_SIGNALS:
    void sigDoubleClicked();
protected:
    bool event( QEvent* xEvent ) override;
    bool eventFilter( QObject* xWatched, QEvent* xEvent ) override;

    void mSetIndex( int newIndex );

    void mAddToLayout( QVBoxLayout* xLayout );
    void mTakeFromLayout( QVBoxLayout* xLayout );

    void initStyleOption( QStyleOptionToolBox* opt ) const;
    void paintEvent( QPaintEvent* ) override;

    void mCollectDisabledTreeWidgetItems( QTreeWidgetItem* xItem );
    void mCollectDisabledTreeWidgetItems();
    void mSetTreeWidgetItemsEnabled( QTreeWidgetItem* xItem, bool xEnabled );
    void mSetTreeWidgetItemsEnabled( bool xEnabled );

private:
    CFlowWidgetItem* dContainer{ nullptr }; // not owned no delete
    QScrollArea* dScrollArea{ nullptr }; // child of the parent FlowWidget
    QTreeWidget* dTreeWidget{ nullptr }; // child of dScrollArea
    CFlowWidgetItemDelegate * dDelegate{ nullptr };
    bool dElideText{ false };
    bool dSelected{ false };
    std::pair< bool, std::unordered_map< QTreeWidgetItem *, bool > > dEnabled{ true, {} };
    int dIndexInPage{ -1 };
    std::vector< CFlowWidgetItem* > dTopItems; // owned in the dContainers dChildren
    std::map< QTreeWidgetItem*, CFlowWidgetItem* > dTopItemMap; // only the top level items
    std::map< QTreeWidgetItem*, CFlowWidgetItem* > dAllChildItemMap; // the individual items own the memory
};

QIcon::Mode iconMode( QStyle::State xState ) 
{
    if ( !(xState & QStyle::State_Enabled) ) 
        return QIcon::Disabled;
    if ( xState & QStyle::State_Selected ) 
        return QIcon::Selected;
    return QIcon::Normal;
}

QIcon::State iconState( QStyle::State xState )
{ 
    return xState & QStyle::State_Open ? QIcon::On : QIcon::Off; 
}

class CFlowWidgetItemDelegate : public QItemDelegate
{
public:
    CFlowWidgetItemDelegate( bool xElide, QObject * xParent ) :
        QItemDelegate( xParent ),
        dElideText( xElide )
    {
    }

    void mSetElideText( bool xElide )
    {
        dElideText = xElide;
    }

    QRect mIconSizeHint( const QStyleOptionViewItem& xOption, const QModelIndex& xIndex ) const
    {
        auto lMode = iconMode( xOption.state );
        auto lState = iconState( xOption.state );
        auto lIcon = xIndex.data( Qt::DecorationRole ).value< QIcon >();
        auto lSize = lIcon.actualSize( xOption.decorationSize, lMode, lState );
        return QRect( QPoint( 0, 0 ), lSize );
    }

    QSize sizeHint( const QStyleOptionViewItem& xOption, const QModelIndex& xIndex ) const override
    {
        QVariant lValue = xIndex.data( Qt::SizeHintRole );
        if ( lValue.isValid() )
            return qvariant_cast<QSize>(lValue);

        if ( !xIndex.isValid() )
            return QItemDelegate::sizeHint( xOption, xIndex );

        auto xDesc = xIndex.data( Qt::DisplayRole ).toString();
        auto lTreeWidgetItem = static_cast<QTreeWidgetItem*>(xIndex.internalPointer());
        if ( !lTreeWidgetItem )
            return QItemDelegate::sizeHint( xOption, xIndex );

        auto xIdentityIcon = xIndex.data( Qt::DecorationRole ).value< QIcon >();
        auto xStateIcons = lTreeWidgetItem->data( 0, CFlowWidgetItem::ERoles::eStateIconsRole ).value< QList< QIcon > >();

        bool lEnabled = xOption.state & QStyle::State_Enabled;

        auto xIdentityPixmap = xIdentityIcon.pixmap( xOption.decorationSize, lEnabled ? QIcon::Normal : QIcon::Disabled );
        QList< QPixmap > lPixmaps;
        for ( auto&& ii : xStateIcons )
        {
            if ( ii.isNull() )
                continue;
            auto lPixMap = ii.pixmap( xOption.decorationSize, lEnabled ? QIcon::Normal : QIcon::Disabled );
            if ( lPixMap.isNull() )
                continue;
            lPixmaps << lPixMap;
        }

        auto lIdentityRect = mIconSizeHint( xOption, xIndex );

        auto lDescRect = mCalcDisplayRect( xIndex, xOption, dElideText, xIdentityPixmap, lPixmaps );
        QList< QRect > lStatusRects;
        for ( int ii = 0; ii < lPixmaps.count(); ++ii )
        {
            auto lRect = mCalcDecorationRect( xOption, lPixmaps[ ii ].devicePixelRatio() );
            lStatusRects << lRect;
        }

        mDoLayout( xOption, lDescRect.first, lIdentityRect, lStatusRects, true );

        QRect lOverAllRect = lIdentityRect | lDescRect.first;
        for( auto && ii : lStatusRects )
            lOverAllRect = lOverAllRect | ii;

        return lOverAllRect.size();
    }

    virtual void paint( QPainter * xPainter, const QStyleOptionViewItem& xOption, const QModelIndex & xIndex ) const override
    {
        if ( !xIndex.isValid() )
        {
            QItemDelegate::paint( xPainter, xOption, xIndex );
            return;
        }

        auto lTreeWidgetItem = static_cast<QTreeWidgetItem*>(xIndex.internalPointer());
        if ( !lTreeWidgetItem )
        {
            QItemDelegate::paint( xPainter, xOption, xIndex );
            return;
        }
        auto lIdentityIcon = xIndex.data( Qt::DecorationRole ).value< QIcon >();
        auto lStateIcons = lTreeWidgetItem->data( 0, CFlowWidgetItem::ERoles::eStateIconsRole ).value< QList< QIcon > >();

        bool lEnabled = xOption.state & QStyle::State_Enabled;
        auto xIdentityPixmap = lIdentityIcon.pixmap( xOption.decorationSize, lEnabled ? QIcon::Normal : QIcon::Disabled );
        QList< QPixmap > lPixmaps;
        for( auto && ii : lStateIcons )
        {
            if ( ii.isNull() )
                continue;
            auto lPixMap = ii.pixmap( xOption.decorationSize, lEnabled ? QIcon::Normal : QIcon::Disabled );
            if ( lPixMap.isNull() )
                continue;
            lPixmaps << lPixMap;
        }

        QString lDescription;
        QRect lDescRect;
        std::tie( lDescRect, lDescription ) = mCalcDisplayRect( xIndex, xOption, dElideText, xIdentityPixmap, lPixmaps );

        // order is IdentityIcon Text StatusIcons
        QRect lIdentityRect = xIdentityPixmap.isNull() ? QRect() : mCalcDecorationRect( xOption, xIdentityPixmap.devicePixelRatio() );

        QList< QRect > lStatusRects;
        for( int ii = 0; ii < lPixmaps.count(); ++ii )
        {
            auto lRect = mCalcDecorationRect( xOption, lPixmaps[ ii ].devicePixelRatio() );
            lStatusRects << lRect;
        }

        mDoLayout( xOption, lDescRect, lIdentityRect, lStatusRects, false );

        xPainter->save();
        if ( lDescRect.right() > xOption.rect.right() )
        {
            lDescRect.adjust( 0, 0, xOption.rect.right() - lDescRect.right(), 0 );
        }
        drawBackground( xPainter, xOption, xIndex );
        drawDecoration( xPainter, xOption, lIdentityRect, xIdentityPixmap );
        for ( int ii = 0; ii < lStatusRects.count(); ++ii )
        {
            drawDecoration( xPainter, xOption, lStatusRects[ ii ], lPixmaps[ ii ] );
        }
        if ( lDescRect.isValid() )
        {
            auto lOption = xOption;
            lOption.textElideMode = Qt::TextElideMode::ElideNone;
            drawDisplay( xPainter, lOption, lDescRect, lDescription );
        }
        drawFocus( xPainter, xOption, xOption.rect );
        xPainter->restore();
    }

    void mDoLayout( const QStyleOptionViewItem& xOption, QRect & xTextRect, QRect & xIdentityRect, QList< QRect > & xStatusRects, bool xHint ) const
    {
        const auto lOrigTextWidth = xTextRect.width();

        QRect lCheckRect; // unused
        QItemDelegate::doLayout( xOption, &lCheckRect, &xIdentityRect, &xTextRect, xHint );
        xTextRect.setWidth( lOrigTextWidth );

        if ( !xIdentityRect.isValid() )
        {
            auto lHeight = std::max( xTextRect.height(), xOption.decorationSize.height() );
            for( auto && ii : xStatusRects )
            {
                lHeight = std::max( lHeight, ii.height() );
            }
            if ( lHeight != xTextRect.height() )
                xTextRect.setHeight( lHeight );
        }

        const QWidget* widget = xOption.widget;
        QStyle* style = widget ? widget->style() : QApplication::style();
        const int lMargin = style->pixelMetric( QStyle::PM_FocusFrameHMargin, nullptr, widget ) + 1;
        for( int ii = 0; ii < xStatusRects.count(); ++ii )
        {
            auto lCurr = xStatusRects[ ii ]; // x,y = 0;
            int x1 = 0;
            if ( ii == 0 )
                x1 = std::max( xTextRect.right(), xIdentityRect.right() ) + 2 * lMargin;
            else
                x1 = xStatusRects[ ii - 1 ].right() + 2*lMargin;
            
            int y1 = xIdentityRect.isValid() ? xIdentityRect.top() : ( xTextRect.isValid() ? xTextRect.top() : 0 );

            lCurr.setRect( x1, y1, lCurr.width(), lCurr.height() );

            xStatusRects[ ii ] = lCurr;
        }
    }

    QRect mCalcDecorationRect( const QStyleOptionViewItem& xOption, qreal xPixelRatio ) const
    {
        QRect rect;

        rect.setX( xOption.rect.x() + 2 );
        rect.setWidth( xOption.decorationSize.width() / xPixelRatio );

        rect.setY( xOption.rect.y() + (xOption.rect.height() - xOption.decorationSize.height()/xPixelRatio )/2 );
        rect.setHeight( xOption.decorationSize.height()/xPixelRatio );
        return rect;
    }

    std::pair< QRect, QString > mCalcDisplayRect( const QModelIndex & xIndex, const QStyleOptionViewItem& xOption, bool xElideText, const QPixmap & xPM, const QList< QPixmap > & xPMs ) const
    {
        auto xText = xIndex.data( Qt::DisplayRole );
        if ( !xIndex.isValid() || xText.isNull() )
            return std::make_pair( QRect(), QString() );

        auto lRect = xOption.rect;
        if ( xElideText )
        {
            auto lMaxWidth = mComputeMaxAllowedTextWidth( xOption.rect.width(), xPM, xPMs );
            lRect.setWidth( lMaxWidth );
        }

        QString lText = xText.toString();
        const QVariant lFontVal = xIndex.data( Qt::FontRole );
        const QFont lFont = qvariant_cast<QFont>(lFontVal).resolve( xOption.font );

        return ::mCalcDisplayRect( lFont, lText, dElideText, lRect );
    }

private:
    bool dElideText{ false };
};

class CFlowWidgetItemImpl
{
public:
    CFlowWidgetItemImpl( CFlowWidgetItem* xContainer ) :
        dContainer( xContainer )
    {
    }
    ~CFlowWidgetItemImpl()
    {
        delete dHeader;
    }

    friend class CFlowWidget;
    void deleteLater()
    {
        if ( dHeader )
            dHeader->deleteLater();
        else if ( dTreeWidgetItem )
            delete dTreeWidgetItem;
        delete this;
    }

    void mSetStepID( const QString & xStepID )
    {
        dStepID = xStepID;
    }
    QString mStepID() const { return dStepID; }
    void mSetIcon( const QIcon& icon )
    {
        dIcon = icon;

        if ( dHeader )
            dHeader->mSetIcon( icon );
        else if ( dTreeWidgetItem )
            dTreeWidgetItem->setIcon( 0, icon );
    }

    QIcon mIcon() const
    {
        return dIcon;
    }

    void mSetToolTip( const QString& tip )
    {
        dToolTip = tip;
        //if ( dHeader )
        //    dHeader->mSetToolTip( tip );
        //else if ( dTreeWidgetItem )
        //    dTreeWidgetItem->setToolTip( 0, tip );
    }

    QString mToolTip( bool xStoredDataOnly ) const
    {
        if ( xStoredDataOnly )
            return dToolTip;

        QStringList lRetVal;
        if ( dToolTip.isEmpty() )
            lRetVal << mText();
        else
            lRetVal << dToolTip;
        auto lStates = mData( CFlowWidgetItem::eStateStatusRole ).value< QList< int > >();
        for ( auto&& ii : lStates )
        {
            lRetVal << mFlowWidget()->mGetStateStatus( ii ).first;
        }
        return lRetVal.join( "\n" );
    }

    void mSetAttribute( const QString& xAttributeName, const QString& xValue )
    {
        auto lCurrentAttributes = mData( CFlowWidgetItem::eAttributesRole ).toMap();
        lCurrentAttributes.insert( xAttributeName, xValue );
        mSetData( CFlowWidgetItem::eAttributesRole, lCurrentAttributes );
    }

    QString mGetAttribute( const QString& xAttributeName ) const
    {
        auto lCurrentAttributes = mData( CFlowWidgetItem::eAttributesRole ).toMap();
        auto pos = lCurrentAttributes.find( xAttributeName );
        if ( pos == lCurrentAttributes.end() )
            return QString();
        return pos.value().toString();
    }

    std::list< std::pair< QString, QString > > mGetAttributes() const
    {
        auto lCurrentAttributes = mData( CFlowWidgetItem::eAttributesRole ).toMap();

        std::list< std::pair< QString, QString > > lRetVal;
        for( auto ii = lCurrentAttributes.begin(); ii != lCurrentAttributes.end(); ++ii )
        {
            lRetVal.push_back( std::make_pair( ii.key(), ii.value().toString() ) );
        }
        return lRetVal;
    }

    bool mIsExpanded() const
    {
        if ( dHeader )
            return dHeader->mIsExpanded();
        else if ( dTreeWidgetItem )
            return dTreeWidgetItem->isExpanded();
        return false;
    }

    inline bool operator==( const CFlowWidgetItemImpl& xOther ) const
    {
        return (dHeader == xOther.dHeader) && (dTreeWidgetItem == xOther.dTreeWidgetItem);
    }

    CFlowWidget* mFlowWidget() const
    {
        if ( dHeader )
            return dynamic_cast<CFlowWidget*>(dHeader->mFlowWidget());
        else if ( dParent )
            return dParent->mGetFlowWidget();
        return nullptr;
    }

    CFlowWidgetItem* mSelectedItem() const
    {
        if ( mHeader() )
            return mHeader()->mSelectedItem();
        return nullptr;
    }
    bool mBeenPlaced() const;

    int mIndexOfChild( const CFlowWidgetItem* xChild )
    {
        if ( !xChild )
            return -1;
        if ( dHeader )
        {
            auto lRetVal = dHeader->mIndexOfChild( xChild );
            if ( lRetVal != -1 )
                return lRetVal;
        }
        else if ( dTreeWidgetItem )
        {
            return dTreeWidgetItem->indexOfChild( xChild->dImpl->dTreeWidgetItem );
        }
        return -1;
    }

    CFlowWidgetItem* mTakeChild( CFlowWidgetItem* xChild )
    {
        if ( !xChild )
            return nullptr;

        if ( dHeader )
        {
            return dHeader->mTakeChild( xChild );
        }
        else if ( dTreeWidgetItem )
        {
            dTreeWidgetItem->takeChild( dTreeWidgetItem->indexOfChild( xChild->dImpl->dTreeWidgetItem ) );
            return xChild;
        }
        return nullptr;
    }

    int mCreateTreeWidgetItem( int xIndex )
    {
        if ( !dParent || !dParent->dImpl )
            return -1;

        auto lTreeWidgetItem = new QTreeWidgetItem( QStringList() << mText() );
        lTreeWidgetItem->setIcon( 0, mIcon() );
        dTreeWidgetItem = lTreeWidgetItem;

        int lRetVal = -1;
        if ( dParent->dImpl->dHeader ) // its a top level, thus it becomes a top level to the tree
        {
            lRetVal = dParent->dImpl->dHeader->mInsertChild( xIndex, dContainer );
        }
        else if ( dParent->dImpl->dTreeWidgetItem )
        {
            Q_ASSERT( xIndex >= 0 && (xIndex <= dParent->dImpl->dTreeWidgetItem->childCount()) );

            dParent->dImpl->dTreeWidgetItem->insertChild( xIndex, dTreeWidgetItem );
            if ( dParent->dImpl->mHeader() )
                dParent->dImpl->mHeader()->mAddToMap( dTreeWidgetItem, dContainer );
            dChildItemMap[ dTreeWidgetItem ] = dContainer;
            lRetVal = xIndex;
        }
        for ( auto ii = 0; ii < static_cast<int>(dChildren.size()); ++ii )
        {
            auto && lChild = dChildren[ ii ];
            lChild->dImpl->mCreateTreeWidgetItem( ii );
        }
        return lRetVal;
    }

    int mInsertChild( int xIndex, std::unique_ptr< CFlowWidgetItem > xChild )
    {
        auto lChildPtr = xChild.get();
        if ( xIndex < 0 || (xIndex > static_cast<int>(dChildren.size())) )
            xIndex = static_cast<int>(dChildren.size());
        dChildren.insert( dChildren.begin() + xIndex, std::move( xChild ) );

        Q_ASSERT( !lChildPtr->dImpl->dParent || (lChildPtr->dImpl->dParent == dContainer) );
        if ( !lChildPtr->dImpl->dParent )
            lChildPtr->dImpl->dParent = dContainer;

        return lChildPtr->dImpl->mCreateTreeWidgetItem( xIndex );
    }

    int mInsertChild( int xIndex, CFlowWidgetItem* xChild )
    {
        auto lUniquePtr = std::unique_ptr< CFlowWidgetItem >( xChild );
        return mInsertChild( xIndex, std::move( lUniquePtr ) );
    }

    int mChildCount() const
    {
        return static_cast<int>(dChildren.size());
    }

    CFlowWidgetItem* mGetChild( int xIndex ) const
    {
        if ( xIndex < 0 || (xIndex >= mChildCount()) )
            return nullptr;

        return dChildren[ xIndex ].get();
    }

    bool mSetData( int xRole, const QVariant& xData, bool xSetState = true );

    QVariant mData( int xRole ) const
    {
        auto pos = dData.find( xRole );
        if ( pos == dData.end() )
            return QVariant();
        return (*pos).second;
    }

    void mSetText( const QString& text )
    {
        dText = text;
        if ( dHeader )
            dHeader->mSetText( text );
        else if ( dTreeWidgetItem )
            dTreeWidgetItem->setText( 0, text );
    }

    QString mText() const
    {
        return dText;
    }

    QString mFullText( const QChar& xSeparator ) const
    {
        QString lMyText = dText;

        QString lParentText;
        if ( dParent )
        {
            lParentText = dParent->mFullText( xSeparator ) + xSeparator;
        }
        lParentText += lMyText;
        return lParentText;
    }

    void mSetVisible( bool xVisible, bool xExpandIfShow ) const
    {
        if ( dHeader )
            dHeader->mSetVisible( xVisible, xExpandIfShow );
        else if ( dTreeWidgetItem )
        {
            dTreeWidgetItem->setHidden( !xVisible );
            if ( xVisible && xExpandIfShow )
                dTreeWidgetItem->setExpanded( true );
        }
    }

    bool mIsVisible() const
    {
        if ( dHeader )
            return dHeader->mIsVisible();
        else if ( dTreeWidgetItem )
            return !dTreeWidgetItem->isHidden();
        return false;
    }

    bool mSetStateStatus( QList< int > xStateStatus );

    bool mAddStateStatus( int xStateStatus )
    {
        if ( xStateStatus == CFlowWidget::EStates::eNone )
            return false;
        auto lStates = mData( CFlowWidgetItem::eStateStatusRole ).value< QList< int > >();
        if ( lStates.indexOf( xStateStatus ) == -1 )
        {
            lStates.push_back( xStateStatus );
            return mSetStateStatus( lStates );
        }
        else
            return false;
    }

    bool mRemoveStateStatus( int xStateStatus )
    {
        if ( xStateStatus == CFlowWidget::EStates::eNone )
            return false;

        auto lStates = mData( CFlowWidgetItem::eStateStatusRole ).value< QList< int > >();
        if ( lStates.removeAll( xStateStatus ) )
            return mSetStateStatus( lStates );
        else
            return false;
    }

    QList< int > mStateStatuses() const
    {
        auto lStates = mData( CFlowWidgetItem::eStateStatusRole ).value< QList< int > >();
        if ( lStates.isEmpty() )
        {
            lStates.push_back( CFlowWidget::eNone );
        }
        return lStates;
    }

    static bool mIsStateDisabled( const QList< int > & xStates )
    {
        return xStates.indexOf( CFlowWidget::EStates::eDisabled ) != -1;
    }

    static bool mIsStateDisabled( const QVariant & xStates )
    {
        return mIsStateDisabled( xStates.value< QList< int > >() );
    }

    bool mIsStateDisabled() const
    {
        return mIsStateDisabled( mData( CFlowWidgetItem::eStateStatusRole ) );
    }

    void mSetStateDisabled( bool xDisabled )
    {
        if ( xDisabled )
            mAddStateStatus( CFlowWidget::EStates::eDisabled );
        else
            mRemoveStateStatus( CFlowWidget::EStates::eDisabled );
    }

    void mSetDisabled( bool xDisabled )
    {
        if ( dHeader )
            dHeader->mSetDisabled( xDisabled );
        else if ( dTreeWidgetItem )
        {
            dTreeWidgetItem->setDisabled( xDisabled );
            if ( xDisabled )
                dTreeWidgetItem->setSelected( false );
        }

        if ( xDisabled != mIsStateDisabled() )
        {
            mSetStateDisabled( xDisabled );
        }
    }

    bool mIsDisabled() const
    {
        if ( dHeader )
            return dHeader->mIsDisabled();
        else if ( dTreeWidgetItem )
            return dTreeWidgetItem->isDisabled();
        return false;
    }

    void mSetSelected( bool xSelected )
    {
        if ( dHeader )
        {
            dHeader->mSetSelected( xSelected ); // for top leave headers, selected opens it always
        }
        else if ( dTreeWidgetItem )
        {
            dTreeWidgetItem->setSelected( xSelected );
        }
    }

    bool mSelected() const
    {
        if ( dHeader )
        {
            dHeader->mSelected();
        }
        else if ( dTreeWidgetItem )
        {
            dTreeWidgetItem->isSelected();
        }
        return false;
    }
    
    CFlowWidgetHeader* mHeader() const
    {
        if ( dHeader )
            return dHeader;
        if ( dParent )
            return dParent->dImpl->mHeader();
        return nullptr;
    }

    void mExpandAll()
    {
        if ( dHeader )
            dHeader->dTreeWidget->expandAll();
        else if ( dTreeWidgetItem )
        {
            ::mExpandAll( dTreeWidgetItem );
        }
    }
    void mSetExpanded( bool xExpanded )
    {
        if ( dHeader )
            dHeader->mSetExpanded( xExpanded );
        else if ( dTreeWidgetItem )
            dTreeWidgetItem->setExpanded( xExpanded );
    }
    void mRemoveWidgets();
    void mClearWidgets( bool xClearCurrent );

    void mCreateFlowWidget( CFlowWidget * xFlowWidget )
    {
        dHeader = new CFlowWidgetHeader( dContainer, xFlowWidget );        
        mSetIcon( mIcon() );
        mSetText( mText() );
        dHeader->mSetObjectName();
        dHeader->mAddChildren( dChildren );
    }

    void mDump( QJsonObject& xJSON, bool xRecursive ) const
    {
        xJSON[ "StepID" ] = dStepID;
        xJSON[ "Text" ] = dText;
        xJSON[ "ToolTip" ] = dToolTip;
        xJSON[ "HasIcon" ] = !dIcon.isNull();
        xJSON[ "Disabled" ] = mIsDisabled();
        xJSON[ "Visible" ] = mIsVisible();
        QJsonArray lDataArray;
        for( auto && ii : dData )
        {
            QJsonObject lCurr;
            lCurr[ QString::number( ii.first ) ] = ii.second.toString();
            lDataArray.push_back( lCurr );
        }
        if ( lDataArray.size() )
            xJSON[ "Data" ] = lDataArray;

        if ( xRecursive )
        {
            QJsonArray lChildren;
            for ( auto&& ii : dChildren )
            {
                QJsonObject lCurr;
                ii->mDump( lCurr, true );
                lChildren.push_back( lCurr );
            }
            if ( lChildren.size() )
                xJSON[ "Children" ] = lChildren;
        }
    }

    void mSetElideText( bool xElideText )
    {
        if ( dHeader )
        {
            dHeader->mSetElideText( xElideText );
        }
    }


    CFlowWidgetItem* dContainer{ nullptr };
    CFlowWidgetItem* dParent{ nullptr };
    CFlowWidgetHeader* dHeader{ nullptr };
    QTreeWidgetItem* dTreeWidgetItem{ nullptr };

    std::map< QTreeWidgetItem*, CFlowWidgetItem* > dChildItemMap;  // flowItems owned by dChildren

    QString dStepID;
    QString dText;
    QString dToolTip;
    QIcon dIcon;
    TDataMap dData;
    TFlowWidgetItems dChildren;
};

class CFlowWidgetImpl
{
public:
    inline CFlowWidgetImpl( CFlowWidget* parent ) :
        dFlowWidget( parent ),
        dCurrentTopLevelItem( nullptr )
    {
        mInitDefaultMap();
    }

    void mInitDefaultMap()
    {
        Q_INIT_RESOURCE( FlowWidget ); // make sure its initialized
        mRegisterStateStatus( CFlowWidget::EStates::eDisabled, QObject::tr( "Disable" ), QIcon( ":/FlowWidgetResources/Disabled.png" ), true );
        mRegisterStateStatus( CFlowWidget::EStates::eReadyToRun, QObject::tr( "Ready To Run" ), QIcon( ":/FlowWidgetResources/ReadyToRun.png" ), true );
        mRegisterStateStatus( CFlowWidget::EStates::eRunning, QObject::tr( "Running" ), QIcon( ":/FlowWidgetResources/Running.png" ), true );
        mRegisterStateStatus( CFlowWidget::EStates::eRunCompletedWithWarning, QObject::tr( "Run Completed With Warning" ), QIcon( ":/FlowWidgetResources/RunCompletedWithWarning.png" ), true );
        mRegisterStateStatus( CFlowWidget::EStates::eRunCompletedWithError, QObject::tr( "Run Completed With Error" ), QIcon( ":/FlowWidgetResources/RunCompletedWithError.png" ), true );
        mRegisterStateStatus( CFlowWidget::EStates::eRunCompletedWithInfo, QObject::tr( "Run Completed With Info" ), QIcon( ":/FlowWidgetResources/RunCompletedWithInfo.png" ), true );
        mRegisterStateStatus( CFlowWidget::EStates::eRunPassed, QObject::tr( "Run Passed" ), QIcon( ":/FlowWidgetResources/RunPassed.png" ), true );
    }

    void mClear()
    {
        dTopLevelItems.clear();
        dCurrentTopLevelItem = nullptr;
        mRelayout();
    }

    size_t mOpenCount() const
    {
        size_t retVal = 0;
        for ( auto&& ii : dTopLevelItems )
        {
            if ( ii->mIsVisible() )
                retVal++;
        }
        return retVal;
    }
    int mAddTopLevelItem( CFlowWidgetItem* xItem ) { return mInsertTopLevelItem( -1, xItem ); }
    int mAddTopLevelItem( std::unique_ptr< CFlowWidgetItem > xItem ) { return mInsertTopLevelItem( -1, std::move( xItem ) ); }

    int mAddItem( CFlowWidgetItem* xItem, CFlowWidgetItem* xParent ) { return mInsertItem( -1, xItem, xParent ); }
    int mAddItem( std::unique_ptr< CFlowWidgetItem > xItem, CFlowWidgetItem* xParent ) { return mInsertItem( -1, std::move( xItem ), xParent ); }

    int mInsertTopLevelItem( int xIndex, CFlowWidgetItem* xItem ); // the parent is "this"
    int mInsertTopLevelItem( int xIndex, std::unique_ptr< CFlowWidgetItem > xItem ); // the parent is "this"

    int mInsertItem( int xIndex, CFlowWidgetItem* xItem, CFlowWidgetItem* xParent );
    int mInsertItem( int xIndex, std::unique_ptr< CFlowWidgetItem > xItem, CFlowWidgetItem* xParent );

    void mSetCurrentTopLevelItem( int xIndex, bool xMakeVisible = true )
    {
        auto lTopLevelItem = mTopLevelItem( xIndex );
        mSetCurrentTopLevelItem( lTopLevelItem, xMakeVisible );
    }

    void mSetCurrentTopLevelItem( CFlowWidgetItem* xItem, bool xMakeVisible = true )
    {
        if ( dCurrentTopLevelItem )
            dCurrentTopLevelItem->dImpl->mSetSelected( false );

        if ( xItem )
        {
            xItem->dImpl->mSetSelected( true );
            if ( xMakeVisible )
                xItem->mSetVisible( true, true );
        }
        if ( xItem != dCurrentTopLevelItem )
        {
            dCurrentTopLevelItem = xItem;
            dFlowWidget->dImpl->mUpdateTabs();
            emit dFlowWidget->sigFlowWidgetItemSelected( xItem, true );
        }
    }

    int mIndexOfTopLevelItem( CFlowWidgetItem* xItem )
    {
        for ( size_t ii = 0; ii < dTopLevelItems.size(); ++ii )
        {
            if ( dTopLevelItems[ ii ].get() == xItem )
                return static_cast<int>(ii);
        }
        return -1;
    }

    CFlowWidgetItem* mGetTopLevelItem( int xIndex ) const
    {
        if ( (xIndex < 0) || (xIndex >= dTopLevelItems.size()) )
            return nullptr;
        return dTopLevelItems[ xIndex ].get();
    }

    CFlowWidgetItem* mCurrentTopLevelItem() const
    {
        return dCurrentTopLevelItem;
    }

    void mSetCurrentItemExpanded( bool xExpanded )
    {
        if ( dCurrentTopLevelItem )
        {
            dCurrentTopLevelItem->mSetExpanded( xExpanded );
        }
    }

    int mTopLevelItemCount() const
    {
        return static_cast<int>(dTopLevelItems.size());
    }

    int mFindReplacementHeader( CFlowWidgetItem* xItem )
    {
        if ( !xItem )
            return -1;
        if ( !xItem->dImpl->dHeader )
            return -1;


        auto lCurrentIndex = mIndexOfTopLevelItem( xItem );
        return mFindReplacementHeader( lCurrentIndex, false );
    }

    int mFindReplacementHeader( int xCurrentIndex, bool xPreIndexed )
    {
        int lRetVal = xCurrentIndex;
        int lCurIndexUp = xCurrentIndex;
        int lCurIndexDown = lCurIndexUp;
        const int lCount = mTopLevelItemCount();
        while ( lCurIndexUp > 0 || lCurIndexDown < lCount - 1 )
        {
            if ( lCurIndexDown < lCount - 1 )
            {
                auto lDownItem = mGetTopLevelItem( xPreIndexed ? lCurIndexDown : ++lCurIndexDown );
                if ( lDownItem->mIsEnabled() && lDownItem->mIsVisible() )
                {
                    lRetVal = lCurIndexDown;
                    break;
                }
                if ( xPreIndexed )
                    lCurIndexDown++;
            }
            if ( lCurIndexUp > 0 )
            {
                auto lUpItem = mGetTopLevelItem( xPreIndexed ? lCurIndexUp : (--lCurIndexUp) );
                if ( lUpItem->mIsEnabled() && lUpItem->mIsVisible() )
                {
                    lRetVal = lCurIndexUp;
                    break;
                }
                if ( xPreIndexed )
                    lCurIndexUp--;
            }
        }
        return lRetVal;
    }

    const CFlowWidgetItem* mTopLevelItem( int xIndex ) const;
    CFlowWidgetItem* mTopLevelItem( int xIndex );

    CFlowWidgetItem* mTakeItem( CFlowWidgetItem* xItem );
    CFlowWidgetItem* mRemoveFromTopLevelItems( CFlowWidgetItem* xItem );
    void mRemoveTopLevelItem( int xIndex );

    void mToggleTopLevelItem( int index ); // if already open, close it, if closed open it
    void mDump( QJsonObject& xJSON ) const
    {
        if ( dCurrentTopLevelItem )
        {
            QJsonObject lCurrentObject;
            dCurrentTopLevelItem->mDump( lCurrentObject, false );
            xJSON[ "Current" ] = lCurrentObject;
        }
        else
        {
            xJSON[ "Current" ] = "nullptr";
        }

        QJsonArray lChildren;
        for ( auto && ii : dTopLevelItems )
        {
            QJsonObject lCurr;
            ii->mDump( lCurr, true );
            lChildren.append( lCurr );
        }
        if ( lChildren.size() )
            xJSON[ "Children" ] = lChildren;
    }

    void mUpdateTabs();
    void mRelayout();

    QList< std::tuple< int, QString, QIcon > > mGetRegisteredStatuses() const
    {
        QList< std::tuple< int, QString, QIcon > > lRetVal;
        for( auto && ii : dStateStatusMap )
        {
            lRetVal.push_back( std::make_tuple( ii.first, ii.second.first, ii.second.second ) );
        }
        return lRetVal;
    }

    void mRegisterStateStatus( int xState, const QString& xDescription, const QIcon& xIcon, bool xCheckForNullIcon )
    {
        Q_ASSERT( !xCheckForNullIcon || ( !xIcon.isNull() && !xIcon.pixmap( 16, 16 ).isNull() ) );
        dStateStatusMap[ xState ] = std::make_pair( xDescription, xIcon );
    }

    int mGetNextStatusID() const
    {
        auto lRetVal = CFlowWidget::EStates::eLastState + 1;
        for( auto && ii : dStateStatusMap )
        {
            lRetVal = std::max( lRetVal, ii.first );
        }
        return lRetVal;
    }

    std::pair< QString, QIcon > mGetStateStatus( int xState ) const
    {
        auto pos = dStateStatusMap.find( xState );
        if ( pos == dStateStatusMap.end() )
            return std::make_pair( QString(), QIcon() );
        return (*pos).second;
    }

    bool mIsRegistered( int xState )
    {
        return dStateStatusMap.find( xState ) != dStateStatusMap.end();
    }

    std::pair< bool, QString > mLoadFromXML( const QString & xfileName );
    std::pair< bool, QString > mLoadFromXML( const QDomElement & xStepElement, const QDir& xRelToDir, CFlowWidgetItem * xParent ); // if nullptr then load it as a top level element
    void mSetElideText( bool xElideText )
    {
        dElideText = xElideText;
        for( auto && ii : dTopLevelItems )
            ii->dImpl->mSetElideText( xElideText );
        dFlowWidget->repaint();
    }
    bool mElideText() const
    {
        return dElideText;
    }

    std::pair<bool, QString> mFindIcon( const QDir& lDir, const QString& xFileName ) const;

    QVBoxLayout* fTopLayout{ nullptr };
    CFlowWidget* dFlowWidget{ nullptr };

    CFlowWidgetItem * dCurrentTopLevelItem{ nullptr };

    TFlowWidgetItems dTopLevelItems;

    std::unordered_map< int, std::pair< QString, QIcon > > dStateStatusMap;
    bool dElideText{ false };
    std::function< std::pair< bool, QString >( const QDir& xRelToDir, const QString& xFileName ) > dFindIcon;
};

CFlowWidgetItem::CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon )
{
    dImpl = new CFlowWidgetItemImpl( this );
    dImpl->mSetStepID( xStepID );
    dImpl->mSetText( xFlowName );
    dImpl->mSetIcon( xDescIcon );
}

CFlowWidgetItem::CFlowWidgetItem() :
    CFlowWidgetItem( QString(), QString(), QIcon() )
{
}

CFlowWidgetItem::CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, CFlowWidget* xParent ) :
    CFlowWidgetItem( xStepID, xFlowName, QIcon(), xParent )
{
}

CFlowWidgetItem::CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, CFlowWidgetItem* xParent ) :
    CFlowWidgetItem( xStepID, xFlowName, QIcon(), xParent )
{
}

CFlowWidgetItem::CFlowWidgetItem( const QString & xStepID, const QIcon& xDescIcon, CFlowWidget* xParent ) :
    CFlowWidgetItem( xStepID, QString(), xDescIcon, xParent )
{
}

CFlowWidgetItem::CFlowWidgetItem( const QString & xStepID, const QIcon& xDescIcon, CFlowWidgetItem* xParent ) :
    CFlowWidgetItem( xStepID, QString(), xDescIcon, xParent )
{
}

CFlowWidgetItem::CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon, CFlowWidget* xParent ) :
    CFlowWidgetItem( xStepID, xFlowName, xDescIcon )
{
    if ( xParent )
        xParent->dImpl->mAddTopLevelItem( this );
}

CFlowWidgetItem::CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon, CFlowWidgetItem* xParent ) :
    CFlowWidgetItem( xStepID, xFlowName, xDescIcon )
{
    dImpl->dParent = xParent;
    if ( xParent )
        xParent->mAddChild( this );
}

CFlowWidgetItem::CFlowWidgetItem( CFlowWidget * xParent ) :
    CFlowWidgetItem()
{
    if ( xParent )
        xParent->dImpl->mAddTopLevelItem( this );
}

CFlowWidgetItem::CFlowWidgetItem( CFlowWidgetItem* xParent ) :
    CFlowWidgetItem()
{
    dImpl->dParent = xParent;
    if ( xParent )
        xParent->mAddChild( this );
}

CFlowWidgetItem::~CFlowWidgetItem()
{
    delete dImpl;
    dImpl = nullptr;
}

void CFlowWidgetItem::deleteLater()
{
    auto lPtr = dImpl;
    lPtr->deleteLater();
    dImpl = nullptr;
    delete this;
}

CFlowWidgetItem* CFlowWidgetItem::mParentItem() const
{
    return dImpl->dParent;
}

CFlowWidget* CFlowWidgetItem::mGetFlowWidget() const
{
    return dImpl->mFlowWidget();
}

void CFlowWidgetItem::mAddChild( CFlowWidgetItem* xChild )
{
    mInsertChild( -1, xChild );
}

int CFlowWidgetItem::mInsertChild( CFlowWidgetItem* xPeer, CFlowWidgetItem* xItem, bool xBefore )
{
    if ( !xPeer || !xItem )
        return -1;

    auto lIndex = xPeer->mIndexInParent();
    if ( !xBefore )
        lIndex++;
    return mInsertChild( lIndex, xItem );
}

int CFlowWidgetItem::mInsertChild( int xIndex, CFlowWidgetItem* xChild )
{
    return dImpl->mInsertChild( xIndex, xChild );
}

int CFlowWidgetItem::mIndexOfChild( const CFlowWidgetItem* xChild )
{
    return dImpl->mIndexOfChild( xChild );
}

int CFlowWidgetItem::mChildCount() const
{
    return dImpl->mChildCount();
}

CFlowWidgetItem* CFlowWidgetItem::mGetChild( int xIndex ) const
{
    return dImpl->mGetChild( xIndex );
}

bool CFlowWidgetItem::mSetData( int xRole, const QVariant& xVariant )
{
    return dImpl->mSetData( xRole, xVariant );
}

QVariant CFlowWidgetItem::mData( int xRole ) const
{
    return dImpl->mData( xRole );
}

void CFlowWidgetItem::mSetText( const QString& xText )
{
    return dImpl->mSetText( xText );
}

QString CFlowWidgetItem::mText() const
{
    return dImpl->mText();
}

QString CFlowWidgetItem::mFullText( const QChar& xSeparator ) const
{
    return dImpl->mFullText( xSeparator );
}

void CFlowWidgetItem::mSetToolTip( const QString& xToolTip )
{
    return dImpl->mSetToolTip( xToolTip );
}

QString CFlowWidgetItem::mToolTip() const
{
    return dImpl->mToolTip( true );
}

void CFlowWidgetItem::mSetIcon( const QIcon& xIcon )
{
    return dImpl->mSetIcon( xIcon );
}

QIcon CFlowWidgetItem::mIcon() const
{
    return dImpl->mIcon();
}

void CFlowWidgetItem::mSetAttribute( const QString& xAttributeName, const QString& xValue )
{
    return dImpl->mSetAttribute( xAttributeName, xValue );
}

QString CFlowWidgetItem::mGetAttribute( const QString& xAttributeName ) const
{
    return dImpl->mGetAttribute( xAttributeName );
}

std::list< std::pair< QString, QString > > CFlowWidgetItem::mGetAttributes() const
{
    return dImpl->mGetAttributes();
}

void CFlowWidgetItem::mSetStepID( const QString & xStepID )
{
    return dImpl->mSetStepID( xStepID );
}

QString CFlowWidgetItem::mStepID() const
{
    return dImpl->mStepID();
}

void CFlowWidgetItem::mSetHidden( bool xHidden )
{
    return mSetVisible( !xHidden, false );
}

bool CFlowWidgetItem::mIsHidden() const
{
    return !mIsVisible();
}

void CFlowWidgetItem::mSetVisible( bool xVisible, bool xExpandIfShow )
{
    return dImpl->mSetVisible( xVisible, xExpandIfShow );
}

bool CFlowWidgetItem::mIsVisible() const
{
    return dImpl->mIsVisible();
}

void CFlowWidgetItem::mSetDisabled( bool xDisabled )
{
    return dImpl->mSetDisabled( xDisabled );
}

bool CFlowWidgetItem::mIsDisabled() const
{
    return dImpl->mIsDisabled();
}

void CFlowWidgetItem::mSetEnabled( bool xEnabled )
{
    return mSetDisabled( !xEnabled );
}

bool CFlowWidgetItem::mIsEnabled() const
{
    return !mIsDisabled();
}

void CFlowWidgetItem::mSetSelected( bool xSelected )
{
    dImpl->mFlowWidget()->mSelectFlowItem( this, xSelected, true );
}

bool CFlowWidgetItem::mSelected() const
{
    return dImpl->mSelected();
}

bool CFlowWidgetItemImpl::mBeenPlaced() const
{
    return (dHeader != nullptr) || (dTreeWidgetItem != nullptr);
}

void CFlowWidgetItemImpl::mClearWidgets( bool xClearCurrent )
{
    if ( xClearCurrent )
    {
        dHeader = nullptr;
        dTreeWidgetItem = nullptr;
    }

    dChildItemMap.clear();
    for ( int ii = 0; ii < mChildCount(); ++ii )
    {
        auto lCurr = mGetChild( ii );
        if ( !lCurr )
            continue;
        lCurr->dImpl->mClearWidgets( true ); // clears all tree widget items
    }
}

bool CFlowWidgetItemImpl::mSetData( int xRole, const QVariant& xData, bool xSetState /*= true */ )
{
    // header data comes from here
    auto lData = mData( xRole );
    if ( lData == xData )
        return false;

    if ( dTreeWidgetItem )
        dTreeWidgetItem->setData( 0, xRole, xData );

    dData[ xRole ] = xData;
    if ( !dTreeWidgetItem )
        emit mFlowWidget()->sigFlowWidgetItemChanged( dContainer );

    if ( xSetState && (xRole == CFlowWidgetItem::eStateStatusRole) )
    {
        auto lStates = xData.value< QList< int > >();
        mSetDisabled( mIsStateDisabled( xData ) );
    }

    if ( xRole == CFlowWidgetItem::eStateStatusRole )
    {
        QList< QIcon > lIcons;
        auto lStates = xData.value< QList< int > >();
        for ( auto ii : lStates )
        {
            auto lIcon = mFlowWidget()->dImpl->mGetStateStatus( ii ).second;
            if ( !lIcon.isNull() )
                lIcons.push_back( lIcon );
        }
        mSetData( CFlowWidgetItem::eStateIconsRole, QVariant::fromValue< QList< QIcon > >( lIcons ) );
    }

    if ( !dTreeWidgetItem )
    {
        dHeader->repaint();
        dHeader->dTreeWidget->viewport()->update();
    }
    return true;
}

bool CFlowWidgetItemImpl::mSetStateStatus( QList< int > xStateStatuses )
{
    xStateStatuses.removeAll( CFlowWidget::EStates::eNone );
    auto lCurrData = mStateStatuses();
    for ( auto&& ii : xStateStatuses )
    {
        if ( !mFlowWidget()->dImpl->mIsRegistered( ii ) )
            return false;
    }
    if ( xStateStatuses != lCurrData )
        return mSetData( CFlowWidgetItem::eStateStatusRole, QVariant::fromValue< QList< int > >( xStateStatuses ) );
    else
        return false;
}

void CFlowWidgetItemImpl::mRemoveWidgets()
{
    mClearWidgets( false );
    if ( dHeader )
    {
        delete dHeader;
        dHeader = nullptr;
    }
    else if ( dTreeWidgetItem )
    {
        delete dTreeWidgetItem;
        dTreeWidgetItem = nullptr;
    }
}

bool CFlowWidgetItem::mIsTopLevelItem() const
{
    return dImpl->mBeenPlaced() && (dImpl->dHeader != nullptr) && (dImpl->dTreeWidgetItem == nullptr);
}

bool CFlowWidgetItem::mSetStateStatus( int xStateStatus )
{
    return mSetStateStatus( QList< int >( { xStateStatus } ) );
}

bool CFlowWidgetItem::mSetStateStatus( const QList< int > & xStateStatus )
{
    return dImpl->mSetStateStatus( xStateStatus );
}

bool CFlowWidgetItem::mAddStateStatus( int xStateStatus )
{
    return dImpl->mAddStateStatus( xStateStatus );
}

bool CFlowWidgetItem::mRemoveStateStatus( int xStateStatus )
{
    return dImpl->mRemoveStateStatus( xStateStatus );
}

QList< int > CFlowWidgetItem::mStateStatuses() const
{
    return dImpl->mStateStatuses();
}

void CFlowWidgetItem::mExpandAll()
{
    return dImpl->mExpandAll();
}

void CFlowWidgetItem::mSetExpanded( bool xExpanded )
{
    return dImpl->mSetExpanded( xExpanded );
}

bool CFlowWidgetItem::mIsExpanded() const
{
    return dImpl->mIsExpanded();
}

int CFlowWidgetItem::mIndexInParent() const
{
    if ( !mParentItem() )
    {
        auto lFlowWidget = dImpl->mFlowWidget();
        if ( !lFlowWidget )
            return -1;

        return lFlowWidget->mIndexOfTopLevelItem( this );
    }
    else
        return mParentItem()->mIndexOfChild( this );
}
/*========================================================================================================*/

CFlowWidgetItem* CFlowWidgetImpl::mTopLevelItem( int xIndex )
{
    if ( xIndex >= 0 && xIndex < static_cast<int>(dTopLevelItems.size()) )
        return dTopLevelItems[ xIndex ].get();
    return nullptr;
}

const CFlowWidgetItem* CFlowWidgetImpl::mTopLevelItem( int xIndex ) const
{
    if ( xIndex >= 0 && xIndex < static_cast<int>(dTopLevelItems.size()) )
        return dTopLevelItems[ xIndex ].get();
    return nullptr;
}

void CFlowWidgetImpl::mUpdateTabs()
{
    CFlowWidgetHeader* lLastButton = dCurrentTopLevelItem ? dCurrentTopLevelItem->dImpl->dHeader : nullptr;
    bool lAfter = false;
    int lIndex = 0;
    for ( const auto& lCurrItem : dTopLevelItems )
    {
        auto lHeader = lCurrItem->dImpl->dHeader;
        // update indexes, since the updates are delayed, the indexes will be correct
        // when we actually paint.
        lHeader->mSetIndex( lIndex );
        lHeader->mUpdate();
        lAfter = lHeader == lLastButton;
        ++lIndex;
    }
}

CFlowWidgetHeader::CFlowWidgetHeader( CFlowWidgetItem* xContainer, CFlowWidget* xParent ) :
    QAbstractButton( xParent ),
    dContainer( xContainer )
{
    setBackgroundRole( QPalette::Window );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
    setFocusPolicy( Qt::NoFocus );

    dTreeWidget = new QTreeWidget;
    dTreeWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarPolicy::ScrollBarAlwaysOff );
    dDelegate = new CFlowWidgetItemDelegate( xParent->mElideText(), this );
    dTreeWidget->setItemDelegate( dDelegate );
    dTreeWidget->setObjectName( "flowwidgetheader_treewidget" );
    dTreeWidget->setExpandsOnDoubleClick( false );
    dTreeWidget->header()->setHidden( true );
    connect( dTreeWidget, &QTreeWidget::itemChanged,
             [this, xParent]( QTreeWidgetItem * xItem, int /*xColumn*/ )
    {
        auto pos = dAllChildItemMap.find( xItem );
        if ( pos != dAllChildItemMap.end() )
            emit xParent->sigFlowWidgetItemChanged( (*pos).second );
    }
    );
    connect( dTreeWidget, &QTreeWidget::itemDoubleClicked,
             [this]()
    {
        auto lSelected = mSelectedItem();
        mFlowWidget()->sigFlowWidgetItemDoubleClicked( lSelected );
    }
    );
    connect( dTreeWidget, &QTreeWidget::itemSelectionChanged,
             [this]()
    {
        auto lSelected = mSelectedItem();
        mFlowWidget()->sigFlowWidgetItemSelected( lSelected, true );
    }
    );

    dScrollArea = new QScrollArea( xParent );
    dScrollArea->setObjectName( "flowwidgetheader_scrollarea" );
    dScrollArea->setWidget( dTreeWidget );
    dScrollArea->setWidgetResizable( true );
    dScrollArea->hide();
    dScrollArea->setFrameStyle( QFrame::NoFrame );
    dTreeWidget->installEventFilter( this );
    dTreeWidget->viewport()->installEventFilter( this );
}

CFlowWidgetHeader::~CFlowWidgetHeader()
{
    delete dScrollArea;
}

bool CFlowWidgetHeader::eventFilter( QObject* xWatched, QEvent* xEvent )
{
    if ( ( xWatched == dTreeWidget ) || ( xWatched == dTreeWidget->viewport() ) )
    {
        if ( xEvent->type() == QEvent::ToolTip )
        {
            auto lHelpEvent = dynamic_cast< QHelpEvent * >( xEvent );
            auto pos = lHelpEvent->pos();
            if ( xWatched == dTreeWidget->viewport() )
            {
                pos = dTreeWidget->mapFromGlobal( lHelpEvent->globalPos() );
            }
            auto xItem = dTreeWidget->itemAt( lHelpEvent->pos() );
            auto xFlowItem = mFindFlowItem( xItem );

            emit mFlowWidget()->sigFlowWidgetItemHovered( xFlowItem );

            if ( xFlowItem )
            {
                auto lText = xFlowItem->dImpl->mToolTip( false );
                if ( !lText.isEmpty() )
                {
                    QToolTip::showText( static_cast<QHelpEvent*>(xEvent)->globalPos(), lText, this, QRect(), toolTipDuration() );
                    return true;
                }
            }
        }
    }
    return QAbstractButton::eventFilter( xWatched, xEvent );
}

bool CFlowWidgetHeader::event( QEvent* xEvent )
{
    if ( xEvent->type() == QEvent::ToolTip )
    {
        emit mFlowWidget()->sigFlowWidgetItemHovered( dContainer );
        auto lText = dContainer->dImpl->mToolTip( false );
        if ( lText.isEmpty() )
            xEvent->ignore();
        else
        {
            QToolTip::showText( static_cast<QHelpEvent*>(xEvent)->globalPos(), lText, this, QRect(), toolTipDuration() );
            xEvent->ignore();
        }
    }
    return QAbstractButton::event( xEvent );
}

CFlowWidgetItem* CFlowWidgetHeader::mFindFlowItem( QTreeWidgetItem* xItem ) const
{
    if ( !xItem )
        return nullptr;
    auto pos = dTopItemMap.find( xItem );
    if ( pos != dTopItemMap.end() )
        return (*pos).second;

    auto pos2 = dAllChildItemMap.find( xItem );
    if ( pos2 != dAllChildItemMap.end() )
        return (*pos2).second;

    return nullptr;
}

void CFlowWidgetHeader::mSetElideText( bool xElideText )
{
    dDelegate->mSetElideText( xElideText );
    dElideText = xElideText;
    repaint();
    dTreeWidget->viewport()->update();
}

CFlowWidget* CFlowWidgetHeader::mFlowWidget() const
{
    return dynamic_cast<CFlowWidget*>(parentWidget());
}

int CFlowWidgetHeader::mIndexOfChild( const CFlowWidgetItem* xItem ) const
{
    if ( !xItem )
        return -1;

    for ( auto ii = 0; ii < dTreeWidget->topLevelItemCount(); ++ii )
    {
        auto lCurrItem = dTreeWidget->topLevelItem( ii );
        if ( !lCurrItem )
            continue;
        if ( lCurrItem == xItem->dImpl->dTreeWidgetItem )
            return ii;
    }
    return -1;
}

int CFlowWidgetHeader::mInsertChild( int xIndex, CFlowWidgetItem* xItem )
{
    Q_ASSERT( xItem->dImpl->dTreeWidgetItem );
    if ( xIndex < 0 || xIndex > dTreeWidget->topLevelItemCount() )
        xIndex = dTreeWidget->topLevelItemCount();
    dTreeWidget->insertTopLevelItem( xIndex, xItem->dImpl->dTreeWidgetItem );

    dTopItems.insert( dTopItems.begin() + xIndex, xItem );
    dTopItemMap[ xItem->dImpl->dTreeWidgetItem ] = xItem;
    return xIndex;
}

void CFlowWidgetHeader::mAddChildren( const TFlowWidgetItems& xChildren )
{
    for ( auto ii = 0; ii < static_cast<int>(xChildren.size()); ++ii )
    {
        auto && lChild = xChildren[ ii ];
        lChild->dImpl->dParent = dContainer;
        lChild->dImpl->mCreateTreeWidgetItem( ii );
    }
}

QString CFlowWidgetItem::mDump( bool xRecursive, bool xCompacted ) const
{
    QJsonObject lObject;
    mDump( lObject, xRecursive );
    auto lRetVal = QJsonDocument( lObject ).toJson( xCompacted ? QJsonDocument::Compact : QJsonDocument::Indented );
    return lRetVal;
}

void CFlowWidgetItem::mDump( QJsonObject& xJSON, bool xRecursive ) const
{
    dImpl->mDump( xJSON, xRecursive );
}

CFlowWidgetItem* CFlowWidgetItem::mTakeChild( CFlowWidgetItem* xItem )
{
    return dImpl->mTakeChild( xItem );
}

CFlowWidgetItem* CFlowWidgetHeader::mTakeChild( CFlowWidgetItem* xItem )
{
    if ( !dTreeWidget )
        return nullptr;
    auto lIndex = dTreeWidget->indexOfTopLevelItem( xItem->dImpl->dTreeWidgetItem );
    if ( lIndex != -1 )
    {
        dTreeWidget->takeTopLevelItem( lIndex );
        auto pos = dTopItemMap.find( xItem->dImpl->dTreeWidgetItem );
        if ( pos != dTopItemMap.end() )
            dTopItemMap.erase( pos );
        auto lItem = std::move( dTopItems[ lIndex ] );
        (void)lItem;

        return xItem;
    }
    else
    {
        auto lParent = xItem->mParentItem();
        if ( !lParent )
            return nullptr;
        return lParent->dImpl->mTakeChild( xItem );
    }
}

int CFlowWidgetHeader::mChildCount() const
{
    if ( dTreeWidget )
        return dTreeWidget->topLevelItemCount();
    return 0;
}

CFlowWidgetItem* CFlowWidgetHeader::mGetChild( int xIndex ) const
{
    if ( (xIndex < 0) || (xIndex > static_cast<int>(dTopItems.size())) )
        return nullptr;
    return dTopItems[ xIndex ];
}

void CFlowWidgetHeader::initStyleOption( QStyleOptionToolBox* xOption ) const
{
    if ( !xOption )
        return;
    xOption->initFrom( this );
    if ( !mIsEnabled() )
    {
        xOption->state &= ~QStyle::State_Enabled;
        xOption->palette.setCurrentColorGroup( QPalette::Disabled );
    }
    if ( dSelected )
        xOption->state |= QStyle::State_Selected;

    if ( isDown() )
        xOption->state |= QStyle::State_Sunken;
    xOption->text = text();
    xOption->icon = icon();

    auto lFlowWidget = mFlowWidget();
    const auto lTopLevelCount = lFlowWidget->mTopLevelItemCount();
    const auto lCurrIndex = lFlowWidget->mIndexOfTopLevelItem( lFlowWidget->mCurrentTopLevelItem() );
    if ( lTopLevelCount == 1 )
    {
        xOption->position = QStyleOptionToolBox::OnlyOneTab;
    }
    else if ( dIndexInPage == 0 )
    {
        xOption->position = QStyleOptionToolBox::Beginning;
    }
    else if ( dIndexInPage == lTopLevelCount - 1 )
    {
        xOption->position = QStyleOptionToolBox::End;
    }
    else
    {
        xOption->position = QStyleOptionToolBox::Middle;
    }
    if ( lCurrIndex == dIndexInPage - 1 )
    {
        xOption->selectedPosition = QStyleOptionToolBox::PreviousIsSelected;
    }
    else if ( lCurrIndex == dIndexInPage + 1 )
    {
        xOption->selectedPosition = QStyleOptionToolBox::NextIsSelected;
    }
    else
    {
        xOption->selectedPosition = QStyleOptionToolBox::NotAdjacent;
    }
}

void CFlowWidgetHeader::mSetVisible( bool xVisible, bool xExpandIfShow )
{
    if ( isVisible() != xVisible )
    {
        if ( dScrollArea )
        {
            if ( xVisible && !dScrollArea->isVisible() )
                dScrollArea->setVisible( xVisible && xExpandIfShow );
            else if ( !xVisible )
                dScrollArea->setVisible( false );
        }
        setVisible( xVisible );

        if ( !xVisible && mFlowWidget()->mCurrentTopLevelItem() == dContainer )
        {
            auto lNewIndex = mFlowWidget()->dImpl->mFindReplacementHeader( dContainer );
            mFlowWidget()->dImpl->mSetCurrentTopLevelItem( lNewIndex );
        }

        if ( xVisible && mFlowWidget()->mCurrentTopLevelItem() != dContainer )
        {
            mFlowWidget()->dImpl->mSetCurrentTopLevelItem( dContainer, false );
        }
    }
}

bool CFlowWidgetHeader::mIsVisible() const
{
    return isVisible();
}

void CFlowWidgetHeader::mSetExpanded( bool xExpanded )
{
    if ( xExpanded )
        mFlowWidget()->mSetCurrentItemExpanded( false );

    dScrollArea->setVisible( xExpanded );
    dSelected = xExpanded;
    mFlowWidget()->dImpl->mUpdateTabs();
    if ( dSelected )
    {
        auto lCurrSelected = dTreeWidget->selectedItems();
        for( auto && ii : lCurrSelected )
            ii->setSelected( false );
        emit mFlowWidget()->sigFlowWidgetItemSelected( dContainer, dSelected );
    }
}

bool CFlowWidgetHeader::mIsExpanded() const
{
    return (dScrollArea && dScrollArea->isVisible());
}

void CFlowWidgetHeader::mUpdate()
{
    auto lTW = dTreeWidget;
    if ( dIndexInPage )
    {
        QPalette lPalette = palette();
        lPalette.setColor( backgroundRole(), lTW->palette().color( lTW->backgroundRole() ) );
        setPalette( lPalette );
    }
    else if ( backgroundRole() != QPalette::Window )
    {
        setBackgroundRole( QPalette::Window );
    }
    update();
}

void CFlowWidgetHeader::mSetIndex( int newIndex )
{
    dIndexInPage = newIndex;
}

void CFlowWidgetHeader::mAddToLayout( QVBoxLayout* xLayout )
{
    xLayout->addWidget( this );
    xLayout->addWidget( dScrollArea );

    if ( dScrollArea )
        dScrollArea->setVisible( false );
}

void CFlowWidgetHeader::mTakeFromLayout( QVBoxLayout* xLayout )
{
    xLayout->removeWidget( dScrollArea );
    xLayout->removeWidget( this );

    if ( dScrollArea )
        dScrollArea->setVisible( false );
    setVisible( false );

}


QSize CFlowWidgetHeader::sizeHint() const
{
    QSize iconSize( 8, 8 );
    int icone = style()->pixelMetric( QStyle::PM_SmallIconSize, nullptr, mFlowWidget() );
    if ( !icon().isNull() )
    {
        iconSize += QSize( icone + 2, icone );
    }
    QSize textSize = fontMetrics().size( Qt::TextShowMnemonic, text() ) + QSize( 0, 8 );

    auto xStates = dContainer->mData( CFlowWidgetItem::ERoles::eStateStatusRole ).value< QList< int > >();
    QSize total( iconSize.width() + textSize.width(), qMax( iconSize.height(), textSize.height() ) );

    total.setWidth( total.width() + xStates.count() * (icone + 2) );

    return total.expandedTo( QApplication::globalStrut() );
}

QSize CFlowWidgetHeader::minimumSizeHint() const
{
    auto xStates = dContainer->mData( CFlowWidgetItem::ERoles::eStateStatusRole ).value< QList< int > >();
    if ( icon().isNull() && xStates.isEmpty() )
        return QSize();
    int icone = style()->pixelMetric( QStyle::PM_SmallIconSize, nullptr, mFlowWidget() );
    int lCount = icon().isNull() ? 0 : 1;
    lCount += xStates.count();
    return QSize( lCount * (icone + 8), icone + 8 );
}

void CFlowWidgetHeader::paintEvent( QPaintEvent* )
{
    QPainter lPaint( this );
    QPainter* lPainter = &lPaint;
    QStyleOptionToolBox lOption;
    initStyleOption( &lOption );
   
    lPainter->save();
    style()->drawControl( QStyle::CE_ToolBoxTabShape, &lOption, lPainter, mFlowWidget() );

    bool lEnabled = lOption.state & QStyle::State_Enabled;
    bool lSelected = lOption.state & QStyle::State_Selected;
    int lIconExtent = style()->proxy()->pixelMetric( QStyle::PM_SmallIconSize, &lOption, this );
    QPixmap lIdentityPixmap = lOption.icon.pixmap( this->window()->windowHandle(), QSize( lIconExtent, lIconExtent ), lEnabled ? QIcon::Normal : QIcon::Disabled );

    auto xStateIcons = dContainer->mData( CFlowWidgetItem::ERoles::eStateIconsRole ).value< QList< QIcon > >();
    QList< QPixmap > lPixMaps;
    for( auto && ii : xStateIcons )
    {
        auto lPixmap = ii.pixmap( this->window()->windowHandle(), QSize( lIconExtent, lIconExtent ), lEnabled ? QIcon::Normal : QIcon::Disabled );
        if ( !lPixmap.isNull() )
            lPixMaps.push_back( lPixmap );
    }

    if ( lSelected && style()->proxy()->styleHint( QStyle::SH_ToolBox_SelectedPageTitleBold, &lOption, this ) )
    {
        QFont f( lPainter->font() );
        f.setBold( true );
        lPainter->setFont( f );
    }
    QRect cr = style()->subElementRect( QStyle::SE_ToolBoxTabContents, &lOption, this );
    int lMaxWidth = mComputeMaxAllowedTextWidth( cr.width(), lIdentityPixmap, lPixMaps );

    auto lElidedText = mComputeTextWidth( lOption, std::make_pair( dElideText, lMaxWidth - 4 ) );

    QRect lTextRect;
    QRect lIdentyIconRect;
    if ( lIdentityPixmap.isNull() )
    {
        lTextRect = QRect( cr.left() + 4, cr.top(), lElidedText.first, cr.height() );
    }
    else
    {
        int lIconWidth = ( lIdentityPixmap.width() / lIdentityPixmap.devicePixelRatio() + 4 );
        int lIconHeight = ( lIdentityPixmap.height() / lIdentityPixmap.devicePixelRatio() );

        lIdentyIconRect = QRect( cr.left() + 4, cr.top(), lIconWidth + 2, lIconHeight );
        lTextRect = QRect( lIdentyIconRect.right(), cr.top(), lElidedText.first, cr.height() );
        lPainter->drawPixmap( lIdentyIconRect.left(), (lOption.rect.height() - lIdentyIconRect.height()) / 2, lIdentityPixmap );
    }

    int lAlign = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic;
    if ( !style()->proxy()->styleHint( QStyle::SH_UnderlineShortcut, &lOption, this ) )
        lAlign |= Qt::TextHideMnemonic;
    style()->proxy()->drawItemText( lPainter, lTextRect, lAlign, lOption.palette, lEnabled, lElidedText.second, QPalette::ButtonText );

    auto x1 = std::max( lTextRect.right(), lIdentyIconRect.right() ) + 4;
    for ( int ii = 0; ii < lPixMaps.count(); ++ii )
    {
        auto y1 = lOption.rect.height();
        auto lIconHeight = lPixMaps[ ii ].height()/lIdentityPixmap.devicePixelRatio();

        lPainter->drawPixmap( x1, (y1 - lIconHeight) / 2, lPixMaps[ ii ] );
        x1 += lPixMaps[ ii ].width() / lIdentityPixmap.devicePixelRatio() + 4;
    }

    if ( !lElidedText.second.isEmpty() && lOption.state & QStyle::State_HasFocus )
    {
        QStyleOptionFocusRect lOption;
        lOption.rect = lTextRect;
        lOption.palette = lOption.palette;
        lOption.state = QStyle::State_None;
        style()->proxy()->drawPrimitive( QStyle::PE_FrameFocusRect, &lOption, lPainter, this );
    }

    lPainter->restore();
}

CFlowWidget::CFlowWidget( QWidget* xParent, Qt::WindowFlags xFlags )
    : QFrame( xParent, xFlags )
{
    dImpl = new CFlowWidgetImpl( this );
    dImpl->mRelayout();
    setBackgroundRole( QPalette::Button );
}

CFlowWidget::~CFlowWidget()
{
    delete dImpl;
    dImpl = nullptr;
}

int CFlowWidget::mTopLevelItemCount() const
{
    return dImpl->mTopLevelItemCount();
}

CFlowWidgetItem* CFlowWidget::mGetTopLevelItem( int xIndex ) const
{
    return dImpl->mGetTopLevelItem( xIndex );
}

void CFlowWidgetImpl::mToggleTopLevelItem( int index )
{
    auto lTopLevelItem = mTopLevelItem( index );
    if ( !lTopLevelItem )
        return;

    lTopLevelItem->mSetExpanded( !lTopLevelItem->mIsExpanded() );
}

void CFlowWidget::slotOpenTopLevelItem( int index )
{
    auto lTopLevelItem = dImpl->mTopLevelItem( index );
    if ( !lTopLevelItem )
        return;
    slotExpandItem( lTopLevelItem, true );
}

void CFlowWidget::slotExpandItem( CFlowWidgetItem* xItem, bool xExpand )
{
    if ( !xItem )
        return;

    xItem->mSetExpanded( xExpand );
}

void CFlowWidget::mSelectFlowItem( CFlowWidgetItem* xItem, bool xSelect, bool xExpand )
{
    auto lOrigItem = xItem;
    while ( xItem )
    {
        if ( xExpand )
            xItem->mSetExpanded( true );
        xItem->dImpl->mSetSelected( xSelect );
        xItem = xItem->mParentItem();
    }
    if ( lOrigItem->mIsTopLevelItem() )
    {
        dImpl->mSetCurrentTopLevelItem( lOrigItem );
    }
}

void CFlowWidget::mSetCurrentItemExpanded( bool xExpanded )
{
    dImpl->mSetCurrentItemExpanded( xExpanded );
}

void CFlowWidgetImpl::mRelayout()
{
    delete fTopLayout;
    fTopLayout = new QVBoxLayout;
    fTopLayout->setContentsMargins( QMargins() );
    for ( const auto& page : dTopLevelItems )
    {
        page->dImpl->dHeader->mAddToLayout( fTopLayout );
    }
    dFlowWidget->setLayout( fTopLayout );
}

auto gPageEquals = []( const CFlowWidgetItem* page )
{
    return [page]( const std::unique_ptr<CFlowWidgetItem>& ptr )
    {
        return ptr.get() == page;
    };
};

CFlowWidgetItem* CFlowWidgetImpl::mRemoveFromTopLevelItems( CFlowWidgetItem* xItem )
{
    auto first = std::find_if( dTopLevelItems.begin(), dTopLevelItems.end(), gPageEquals( xItem ) );

    std::list< CFlowWidgetItem* > lRetVal;
    if ( first != dTopLevelItems.end() )
    {
        lRetVal.push_back( (*first).get() );
        (*first).release();
        for ( auto ii = first; ++ii != dTopLevelItems.end(); )
        {
            if ( (*ii).get() != xItem )
            {
                *first++ = std::move( *ii );
            }
            else
            {
                lRetVal.push_back( (*ii).get() );
                (*first).release();
            }
        }
    }
    dTopLevelItems.erase( first, dTopLevelItems.end() );
    if ( lRetVal.empty() )
        return nullptr;
    auto lRetValItem = *lRetVal.begin();
    return lRetValItem;
}

CFlowWidgetItem* CFlowWidgetImpl::mTakeItem( CFlowWidgetItem* xItem )
{
    xItem->dImpl->mRemoveWidgets(); // removes all GUI items from item

    auto lParent = xItem->mParentItem();
    if ( lParent )
        return lParent->mTakeChild( xItem );

    auto lTopLevelIndex = mIndexOfTopLevelItem( xItem );
    if ( lTopLevelIndex == -1 )
        return nullptr;

    bool lRemoveCurrent = xItem == dCurrentTopLevelItem;

    auto lRetVal = mRemoveFromTopLevelItems( xItem );

    if ( dTopLevelItems.empty() )
    {
        dCurrentTopLevelItem = nullptr;
        emit dFlowWidget->sigFlowWidgetItemSelected( nullptr, false );
    }
    else if ( lRemoveCurrent )
    {
        auto lNewIndex = mFindReplacementHeader( lTopLevelIndex ? (lTopLevelIndex - 1) : 0, true );
        mSetCurrentTopLevelItem( lNewIndex );
        emit dFlowWidget->sigFlowWidgetItemSelected( xItem, false );
    }

    return lRetVal;
}

CFlowWidgetItem* CFlowWidget::mTakeTopLevelItem( int xIndex )
{
    auto lTopLevelItem = mGetTopLevelItem( xIndex );
    if ( !lTopLevelItem )
        return nullptr;

    return mTakeItem( lTopLevelItem );
}

void CFlowWidget::mClear()
{
    dImpl->mClear();
}

CFlowWidgetItem* CFlowWidget::mTakeItem( CFlowWidgetItem* xItem )
{
    if ( !xItem )
        return nullptr;

    return dImpl->mTakeItem( xItem );
}

void CFlowWidgetImpl::mRemoveTopLevelItem( int xIndex )
{
    auto lTopLevelItem = mTopLevelItem( xIndex );
    if ( !lTopLevelItem )
        return;

    return dFlowWidget->mRemoveItem( lTopLevelItem );
}

void CFlowWidget::mRemoveTopLevelItem( int xIndex )
{
    dImpl->mRemoveTopLevelItem( xIndex );
}
void CFlowWidget::mRemoveItem( CFlowWidgetItem* xItem )
{
    auto lTakenItem = mTakeItem( xItem );
    if ( lTakenItem )
        lTakenItem->deleteLater();
}

int CFlowWidget::mIndexOfTopLevelItem( const CFlowWidgetItem* xItem ) const
{
    if ( !xItem )
        return -1;
    // item can be a tree widget item or a parent item
    if ( xItem->mParentItem() )
        return -1;

    const auto it = std::find_if( dImpl->dTopLevelItems.cbegin(), dImpl->dTopLevelItems.cend(), gPageEquals( xItem ) );
    if ( it == dImpl->dTopLevelItems.cend() )
        return -1;
    return static_cast<int>(it - dImpl->dTopLevelItems.cbegin());
}

void CFlowWidgetHeader::mCollectDisabledTreeWidgetItems( QTreeWidgetItem * xItem )
{
    if ( !xItem )
        return;
    dEnabled.second[ xItem ] = xItem->isDisabled();
    for( auto ii = 0; ii < xItem->childCount(); ++ii )
    {
        mCollectDisabledTreeWidgetItems( xItem->child( ii ) );
    }
}

void CFlowWidgetHeader::mCollectDisabledTreeWidgetItems()
{
    for( auto ii = 0; ii < dTreeWidget->topLevelItemCount(); ++ii )
    {
        mCollectDisabledTreeWidgetItems( dTreeWidget->topLevelItem( ii ) );
    }
}

void CFlowWidgetHeader::mSetTreeWidgetItemsEnabled( QTreeWidgetItem* xItem, bool xEnabled )
{
    if ( !xItem )
        return;
    xItem->setDisabled( !xEnabled );
    for ( auto ii = 0; ii < xItem->childCount(); ++ii )
    {
        mSetTreeWidgetItemsEnabled( xItem->child( ii ), xEnabled );
    }
}

void CFlowWidgetHeader::mSetTreeWidgetItemsEnabled( bool xEnabled )
{
    for ( auto ii = 0; ii < dTreeWidget->topLevelItemCount(); ++ii )
    {
        mSetTreeWidgetItemsEnabled( dTreeWidget->topLevelItem( ii ), xEnabled );
    }
}

void CFlowWidgetHeader::mSetEnabled( bool xEnabled )
{
    if ( dEnabled.first != xEnabled )
    {
        dEnabled.first = xEnabled;
        // if we are disabling collect all items that are already disabled
        if ( !xEnabled )
        {
            dEnabled.second.clear();
            mCollectDisabledTreeWidgetItems();
        }
        mSetTreeWidgetItemsEnabled( xEnabled ); // set all items

        // if we are enabling, disable all that were previously disabled
        if ( xEnabled )
        {
            for ( auto&& ii : dEnabled.second )
            {
                ii.first->setDisabled( ii.second );
            }
        }
        repaint();
    }
}

bool CFlowWidgetHeader::mSelected() const
{
    return dSelected;
}

void CFlowWidget::showEvent( QShowEvent* e )
{
    QFrame::showEvent( e );
}

void CFlowWidget::changeEvent( QEvent* ev )
{
    if ( ev->type() == QEvent::StyleChange )
        dImpl->mUpdateTabs();
    QFrame::changeEvent( ev );
}

bool CFlowWidget::event( QEvent* e )
{
    return QFrame::event( e );
}

int CFlowWidgetImpl::mInsertTopLevelItem( int xIndex, std::unique_ptr< CFlowWidgetItem > xItem )
{
    if ( !xItem || !xItem->dImpl )
        return -1;
    if ( !xItem->dImpl->mBeenPlaced() )
    {
        xItem->dImpl->mCreateFlowWidget( dFlowWidget );
    }

    auto lFlowItem = xItem.get();
    bool lCurrentChanged = false;

    if ( xIndex < 0 || xIndex >= static_cast<int>(dTopLevelItems.size()) )
        xIndex = static_cast<int>(dTopLevelItems.size());

    dTopLevelItems.insert( dTopLevelItems.cbegin() + xIndex, std::move( xItem ) );
    mRelayout();
    if ( dCurrentTopLevelItem )
    {
        int oldindex = mIndexOfTopLevelItem( dCurrentTopLevelItem );
        if ( xIndex <= oldindex )
        {
            dCurrentTopLevelItem = nullptr; // trigger change
            lCurrentChanged = true;
            mSetCurrentTopLevelItem( oldindex );
        }
    }
    else
    {
        mSetCurrentTopLevelItem( xIndex );
        lCurrentChanged = true;
    }

    QObject::connect( lFlowItem->dImpl->dHeader, &CFlowWidgetHeader::clicked,
                      [this, lFlowItem]()
    {
        auto lIndex = mIndexOfTopLevelItem( lFlowItem );
        if ( (lIndex < 0) || (lIndex >= static_cast<int>(dTopLevelItems.size())) )
            return;
        dFlowWidget->dImpl->mSetCurrentTopLevelItem( dTopLevelItems[ lIndex ].get() );
        dFlowWidget->sigFlowWidgetItemSelected( dTopLevelItems[ lIndex ].get(), true );
    } );

    QObject::connect( lFlowItem->dImpl->dHeader, &CFlowWidgetHeader::sigDoubleClicked,
                      [this, lFlowItem]()
    {
        auto lIndex = mIndexOfTopLevelItem( lFlowItem );
        if ( (lIndex < 0) || (lIndex >= static_cast<int>(dTopLevelItems.size())) )
            return;
        dFlowWidget->dImpl->mSetCurrentTopLevelItem( dTopLevelItems[ lIndex ].get() );
        dFlowWidget->sigFlowWidgetItemDoubleClicked( dTopLevelItems[ lIndex ].get() );
    } );

    lFlowItem->dImpl->dHeader->mSetVisible( true, lCurrentChanged );

    mUpdateTabs();
    emit dFlowWidget->sigFlowWidgetItemInserted( dTopLevelItems[ xIndex ].get() );
    return xIndex;
}

int CFlowWidgetImpl::mInsertItem( int xIndex, std::unique_ptr< CFlowWidgetItem > xItem, CFlowWidgetItem* xParent )
{
    Q_ASSERT( xParent );
    if ( !xParent )
        return -1;

    auto lPtr = xItem.release();
    return xParent->mInsertChild( xIndex, lPtr );
}

int CFlowWidgetImpl::mInsertTopLevelItem( int xIndex, CFlowWidgetItem* xItem )
{
    auto lUniquePtr = std::unique_ptr< CFlowWidgetItem >( xItem );
    return mInsertTopLevelItem( xIndex, std::move( lUniquePtr ) );
}

CFlowWidgetItem* CFlowWidget::mInsertTopLevelItem( int xIndex, const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon )
{
    auto lItem = new CFlowWidgetItem( xStepID, xFlowName, xDescIcon );
    auto lRetVal = mInsertTopLevelItem( xIndex, lItem );
    return lRetVal.second ? lRetVal.first : nullptr;
}

std::pair< CFlowWidgetItem*, bool > CFlowWidget::mInsertTopLevelItem( CFlowWidgetItem * xPeer, CFlowWidgetItem* xItem, bool xBefore )
{
    if ( !xPeer || !xItem )
        return std::make_pair( xItem, false );

    auto lIndex = xPeer->mIndexInParent();
    if ( !xBefore )
        lIndex++;
    return mInsertTopLevelItem( lIndex, xItem );
}

std::pair< CFlowWidgetItem*, bool > CFlowWidget::mInsertTopLevelItem( int xIndex, CFlowWidgetItem* xItem )
{
    if ( xItem->dImpl->mBeenPlaced() && !xItem->mIsTopLevelItem() )
        return std::make_pair( xItem, false );

    dImpl->mInsertTopLevelItem( xIndex, xItem );
    return std::make_pair( xItem, true );
}

int CFlowWidgetImpl::mInsertItem( int xIndex, CFlowWidgetItem* xItem, CFlowWidgetItem* xParent )
{
    auto lUniquePtr = std::unique_ptr< CFlowWidgetItem >( xItem );
    return mInsertItem( xIndex, std::move( lUniquePtr ), xParent );
}

CFlowWidgetItem* CFlowWidget::mInsertItem( int xIndex, const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon, CFlowWidgetItem* xParent )
{
    auto lItem = new CFlowWidgetItem( xStepID, xFlowName, xDescIcon ); // no parent the mInsertItem sets the parent
    dImpl->mInsertItem( xIndex, lItem, xParent );
    return lItem;
}

CFlowWidgetItem* CFlowWidget::mCurrentTopLevelItem() const
{
    return dImpl->mCurrentTopLevelItem();
}

CFlowWidgetItem* CFlowWidget::mSelectedItem() const
{
    auto lCurrentTop = mCurrentTopLevelItem();
    if ( !lCurrentTop )
        return nullptr;

    auto lItemSelected = lCurrentTop->dImpl->mSelectedItem();
    return lItemSelected ? lItemSelected : lCurrentTop;
}

void CFlowWidget::mRegisterStateStatus( int xState, const QString & xDescription, const QIcon& xIcon )
{
    return dImpl->mRegisterStateStatus( xState, xDescription, xIcon, false );
}

int CFlowWidget::mGetNextStatusID() const
{
    return dImpl->mGetNextStatusID();
}

QList< std::tuple< int, QString, QIcon > > CFlowWidget::mGetRegisteredStatuses() const
{
    return dImpl->mGetRegisteredStatuses();
}

std::pair< QString, QIcon > CFlowWidget::mGetStateStatus( int xState ) const
{
    return dImpl->mGetStateStatus( xState );
}

void CFlowWidget::mSetElideText( bool xElide )
{
    return dImpl->mSetElideText( xElide );
}

bool CFlowWidget::mElideText() const
{
    return dImpl->mElideText();
}

void CFlowWidget::mDump( QJsonObject & xJSON ) const
{
    dImpl->mDump( xJSON );
}

QString CFlowWidget::mDump( bool xCompacted ) const
{
    QJsonObject lObject;
    mDump( lObject );
    auto lRetVal = QJsonDocument( lObject ).toJson( xCompacted ? QJsonDocument::Compact : QJsonDocument::Indented );
    return lRetVal;
}

void CFlowWidget::mSetFindIconFunc( const std::function< std::pair< bool, QString >( const QDir& xRelToDir, const QString& xFileName ) >& lFindIcon )
{
    dImpl->dFindIcon = lFindIcon;
}

std::pair< bool, QString > CFlowWidget::mLoadFromXML( const QString & xFileName )
{
    return dImpl->mLoadFromXML( xFileName );
}

std::pair< bool, QString > CFlowWidgetImpl::mFindIcon( const QDir& xRelToDir, const QString & xFileName ) const
{
    std::pair< bool, QString > lRetVal = { false, QString() };

    if ( dFindIcon )
        lRetVal = dFindIcon( xRelToDir, xFileName );
    
    if ( lRetVal.first )
        return lRetVal;

    if ( QFileInfo::exists( xRelToDir.absoluteFilePath( xFileName ) ) )
        return std::make_pair( true, xRelToDir.absoluteFilePath( xFileName ) );

    if ( QFileInfo::exists( QDir::current().absoluteFilePath( xFileName ) ) )
        return std::make_pair( true, QDir::current().absoluteFilePath( xFileName ) );

    return lRetVal;
}



std::pair< bool, QString > CFlowWidgetImpl::mLoadFromXML( const QDomElement& xStepElement, const QDir & xRelToDir, CFlowWidgetItem* xParent ) // if nullptr then load it as a top level element
{
    if ( xStepElement.tagName() != "Step" )
    {
        return std::make_pair( false, CFlowWidget::tr( "Invalid Element in XML (%1,%2): Expected 'Step' found '%3'" ).arg( xStepElement.lineNumber() ).arg( xStepElement.columnNumber() ).arg( xStepElement.tagName() ) );
    }

    auto lIDEle   = xStepElement.firstChildElement( "id" );
    auto lNameEle = xStepElement.firstChildElement( "name" );
    auto lIconEle = xStepElement.firstChildElement( "icon" ); // can be null error if non-null and file does not exist
    if ( lIDEle.isNull() )
        return std::make_pair( false, CFlowWidget::tr( "Invalid XML (%1,%2): Missing 'id' Element" ).arg( xStepElement.lineNumber() ).arg( xStepElement.columnNumber() ) );
    if ( lNameEle.isNull() )
        return std::make_pair( false, CFlowWidget::tr( "Invalid XML (%1,%2): Missing 'name' Element" ).arg( xStepElement.lineNumber() ).arg( xStepElement.columnNumber() ) );

    CFlowWidgetItem* xCurrItem = xParent ? new CFlowWidgetItem( xParent ) : new CFlowWidgetItem( dFlowWidget );
    xCurrItem->mSetStepID( lIDEle.text() );
    xCurrItem->mSetText( lNameEle.text() );

    auto lIconFileName = mFindIcon( xRelToDir, lIconEle.text() );
    if ( !lIconFileName.first )
    {
        return std::make_pair( false, CFlowWidget::tr( "Invalid Element in XML (%1,%2): Icon file '%3' not found" ).arg( lIconEle.lineNumber() ).arg( lIconEle.columnNumber() ).arg( lIconEle.text() ) );
    }
    xCurrItem->mSetIcon( QIcon( lIconFileName.second ) );

    for( auto ii = xStepElement.firstChildElement(); !ii.isNull(); ii = ii.nextSiblingElement() )
    {
        auto lTagName = ii.tagName();
        if ( ( lTagName == "name" ) ||
             ( lTagName == "icon" ) ||
             ( lTagName == "id" ) ||
             ( lTagName == "Step" ) )
            continue;

        auto lValue = ii.text();
        xCurrItem->mSetAttribute( lTagName, lValue );
    }

    for( auto ii = xStepElement.firstChildElement( "Step" ); !ii.isNull(); ii = ii.nextSiblingElement( "Step" ) )
    {
        auto lCurr = mLoadFromXML( ii, xRelToDir, xCurrItem );
        if ( !lCurr.first )
            return lCurr;
    }
    return std::make_pair( true, QString() );
}

std::pair< bool, QString > CFlowWidgetImpl::mLoadFromXML( const QString& xFileName )
{
    QFile lFile( xFileName );
    if ( !lFile.open( QFile::ReadOnly | QFile::Text ) )
    {
        return std::make_pair( false, CFlowWidget::tr( "Could not open file '%1' for reading" ).arg( xFileName ) );
    }

    QDomDocument lDoc;
    QString lErrorString;
    int lErrorLine;
    int lErrorColumn;
    if ( !lDoc.setContent( &lFile, false, &lErrorString, &lErrorLine, &lErrorColumn ) )
    {
        return std::make_pair( false, CFlowWidget::tr( "Error parsing XML file (%1,%2): %3" ).arg( lErrorLine ).arg( lErrorColumn ).arg( lErrorString ) );
    }

    mClear();

    auto lRoot = lDoc.documentElement();
    auto lTopStepsEle = lRoot.firstChildElement( "Steps" );
    if ( lTopStepsEle.isNull() )
    {
        return std::make_pair( false, CFlowWidget::tr( "Invalid XML (%1,%2): Missing 'Steps' element'" ).arg( lRoot.lineNumber() ).arg( lRoot.columnNumber() ) );
    }

    auto lRelToDir = QFileInfo( xFileName ).absoluteDir();
    for( auto lChild = lTopStepsEle.firstChildElement( "Step" ); !lChild.isNull(); lChild = lChild.nextSiblingElement( "Step" ) )
    {
        auto lCurr = mLoadFromXML( lChild.toElement(), lRelToDir, nullptr );
        if ( !lCurr.first )
        {
            mClear();
            return lCurr;
        }
    }

    for( auto && ii : dTopLevelItems )
        ii->mExpandAll();

    if ( dTopLevelItems.size() )
        dTopLevelItems[ 0 ]->mSetSelected( true );

    return std::make_pair( true, QString() );
}

#include "FlowWidget.moc"
