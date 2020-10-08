#ifndef _FLOWNAVIGATOR_
#define _FLOWNAVIGATOR_

#include "basicWindow.h"
#include "windowsExports.h"
#include <QFrame>
#include <memory>

class QScrollArea;
class CFlowWidget;
class CFlowWidgetHeader;
class QVBoxLayout;
class QStyleOptionToolBox;
class QJsonObject;
class QDir;

class CFlowWidget;
class CFlowWidgetItem;
class CFlowWidgetItemImpl;
class CFlowWidgetImpl;
class CFlowWidgetHeaderImpl;

struct WINDOWS_QDECLSPEC SRegisteredStatusInfo
{
    SRegisteredStatusInfo(){};
    SRegisteredStatusInfo( int xStateID, const QString & xStateDesc, const QIcon & xIcon, bool xIsInternal ) :
        dStateID( xStateID ),
        dStateDesc( xStateDesc ),
        dIcon( xIcon ),
        dIsInternal( xIsInternal )
    {
    }

    int dStateID{-1};
    QString dStateDesc;
    QIcon dIcon;
    bool dIsInternal{false};
};

// The Flow Navigator system is currently under LFK VIS_2021_1_BETA as well as the environmental variable ENABLE_FLOWNAV
// if enabled the window "flownavigator" is enabled and be opened via "view flownavigator"
// view flownavigator takes in two optional parameters
//    [-xml <filename>] the XML file path (QRC : based paths are allowed
//    [-init] Allows the opening of the flownavigator without a filename but before the gui startup has finished
//    due to the window manager restoring the windows without their parameters, this enables the navigator to be created and loaded later

class WINDOWS_QDECLSPEC CFlowNavigator : public basicWindow 
    // derived from QDockWidget
    // contains a QMainWindow
    // CFlowWidget -> is the Central Widget of the mainwindow and takes ownership
{
    Q_OBJECT

public:
    CFlowNavigator();
    ~CFlowNavigator();

    CFlowWidget * mGetFlowWidget() const{ return dFlowWidget; }

    static QObject * mCreateWindow(int xId, const QStringList& xArgs );
    static void mSetInitFunction( const std::function< CFlowWidget*() > & xFunction );
    static bool mIsEnabled(); // retuns true if the LFK for 2021.1 is enabled and the envvar ENABLE_FLOWNAV is set

public Q_SLOTS:
    // no slots

Q_SIGNALS: // you can connect to the FlowNavigator or the owned CFlowWidget
     // item information signals (changed and inserted) are sent out even if 
     void sigFlowWidgetItemChanged( CFlowWidgetItem * xItem ); // data change
     void sigFlowWidgetItemInserted( CFlowWidgetItem* xItem ); // item was in
 
     // item based selection signals are NOT sent out if the item is disabled
     void sigFlowWidgetItemSelected( CFlowWidgetItem* xItem, bool xSelected );
     void sigFlowWidgetItemDoubleClicked( CFlowWidgetItem* xItem );
     void sigFlowWidgetItemHovered( CFlowWidgetItem* xItem ); // can be null 
protected:
    CFlowWidget * dFlowWidget{nullptr}; // memory is owned by basicWindow
    static std::function< CFlowWidget *() > dInitFunction;
};

class WINDOWS_QDECLSPEC CFlowWidget : public QFrame
{
    friend class CFlowWidgetHeader;
    friend class CFlowWidgetImpl;
    friend class CFlowWidgetItem;
    friend class CFlowWidgetItemImpl;

    Q_OBJECT;
    Q_PROPERTY( int count READ mTopLevelItemCount )

public:
    enum EStates
    {
        eNone,
        eDisabled,
        eReadyToRun, // ready to run
        eRunning,
        eRunCompletedWithWarning,
        eRunCompletedWithError,
        eRunCompletedWithInfo,
        eRunPassed,
        eLastState = eRunPassed
    };

    CFlowWidget& operator=( const CFlowWidget& rhs ) = delete;
    CFlowWidget( const CFlowWidget& rhs ) = delete;
    ~CFlowWidget();

    explicit CFlowWidget( QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags() );

    virtual std::pair< bool, QString > mLoadFromXML( const QString & xFileName );
    // The returned CFLowWidgetItems are OWNED by the flow widget, and should not be deleted by the user without removing them from thw CFlowWidget or parent CFlowWidgetItem first
    virtual CFlowWidgetItem* mAddTopLevelItem( const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon ) { return mInsertTopLevelItem( -1, xStepID, xFlowName, xDescIcon ); }
    virtual CFlowWidgetItem* mAddTopLevelItem( const QString & xStepID, const QString& xFlowName );
    virtual CFlowWidgetItem* mAddTopLevelItem( const QString & xStepID, const QIcon& xDescIcon );

    virtual CFlowWidgetItem* mInsertTopLevelItem( int xIndex, const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon );
    virtual CFlowWidgetItem* mInsertTopLevelItem( int xIndex, const QString & xStepID, const QString& xFlowName );
    virtual CFlowWidgetItem* mInsertTopLevelItem( int xIndex, const QString & xStepID, const QIcon& xDescIcon ) { return mInsertTopLevelItem( xIndex, xStepID, QString(), xDescIcon ); }

    virtual std::pair< CFlowWidgetItem*, bool > mAddTopLevelItem( CFlowWidgetItem* xItem ) { return mInsertTopLevelItem( -1, xItem ); }
    virtual std::pair< CFlowWidgetItem*, bool > mInsertTopLevelItem( int xIndex, CFlowWidgetItem* xItem );
    virtual std::pair< CFlowWidgetItem*, bool > mInsertTopLevelItem( CFlowWidgetItem* xPeer, CFlowWidgetItem* xItem, bool xBefore );

    virtual CFlowWidgetItem* mAddItem( const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon, CFlowWidgetItem* xParent ) { return mInsertItem( -1, xStepID, xFlowName, xDescIcon, xParent ); }
    virtual CFlowWidgetItem* mAddItem( const QString & xStepID, const QString& xFlowName, CFlowWidgetItem* xParent );
    virtual CFlowWidgetItem* mAddItem( const QString & xStepID, const QIcon& xDescIcon, CFlowWidgetItem* xParent ) { return mInsertItem( -1, xStepID, QString(), xDescIcon, xParent ); }

    virtual CFlowWidgetItem* mInsertItem( int xIndex, const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon, CFlowWidgetItem* xParent );
    virtual CFlowWidgetItem* mInsertItem( int xIndex, const QString & xStepID, const QString& xFlowName, CFlowWidgetItem* xParent );
    virtual CFlowWidgetItem* mInsertItem( int xIndex, const QString & xStepID, const QIcon& xDescIcon, CFlowWidgetItem* xParent ) { return mInsertItem( xIndex, xStepID, QString(), xDescIcon, xParent ); }

    virtual int mTopLevelItemCount() const;
    virtual CFlowWidgetItem* mGetTopLevelItem( int xIndex ) const;
    virtual CFlowWidgetItem* mGetTopLevelItem( const QString & xStepID ) const; // returns the first top level item with the stepID
    virtual int mIndexOfTopLevelItem( const CFlowWidgetItem* xItem ) const; // returns -1 if not found as a top level widget

    virtual void mRemoveTopLevelItem( int xIndex ); // deletes the CFlowStateDefItem
    virtual CFlowWidgetItem* mTakeTopLevelItem( int xIndex );

    virtual void mClear();
    virtual CFlowWidgetItem* mTakeItem( CFlowWidgetItem* xItem );   // does the work of "get parent, parent->take item
    virtual void mRemoveItem( CFlowWidgetItem* xItem ); // does the work of "get parent, parent->remove item

    virtual void mSelectFlowItem( CFlowWidgetItem* xItem, bool xSelect, bool xExpand );

    virtual CFlowWidgetItem* mSelectedItem() const; // only one item is selectable at a time.

    void mSetSummarizeStatus( bool xSummarizeStatus );
    bool mSummarizeStatus() const;

    void mSetAlignStatus( bool xAlignStatus );
    bool mAlignStatus() const;

    virtual void mRegisterStateStatus( int xStateID, const QString & xDescription, const QIcon& xIcon );  // xIcon can be null if so, nothing painted
    virtual int mGetNextStatusID() const;
    virtual QList< SRegisteredStatusInfo > mGetRegisteredStatuses() const; 
    virtual SRegisteredStatusInfo mGetStateStatus( int xState ) const; 

    virtual void mSetElideText( bool xElide );
    virtual bool mElideText() const;

    virtual bool mSummarizeStatus() const;
    virtual void mSetSummarizeStatus( bool xSummarizeStatus );

    virtual bool mAlignStatus() const;
    virtual void mSetAlignStatus( bool xAlignStatus );

    virtual QString mDump( bool xCompacted ) const;
    virtual void mDump( QJsonObject& xTS ) const;
protected Q_SLOTS:
    virtual void slotOpenTopLevelItem( int xIndex );
    virtual void slotExpandItem( CFlowWidgetItem* xItem, bool xExpand );
Q_SIGNALS: 
    // item information signals (changed and inserted) are sent out even if the item is disabled
    void sigFlowWidgetItemChanged( CFlowWidgetItem * xItem ); // data changed
    void sigFlowWidgetItemInserted( CFlowWidgetItem* xItem ); // item was inserted into the flow widget

    // item based selection signals are NOT sent out if the item is disabled
    void sigFlowWidgetItemSelected( CFlowWidgetItem* xItem, bool xSelected ); // can be null if all items removed 
    void sigFlowWidgetItemDoubleClicked( CFlowWidgetItem* xItem );            // often will follow a Selected signal (single click is the same as selected) 
    void sigFlowWidgetItemHovered( CFlowWidgetItem* xItem ); // can be null if hovered over a non item
protected:
    bool event( QEvent* e ) override;
    void showEvent( QShowEvent* e ) override;
    void changeEvent( QEvent* ) override;

protected:
    virtual void mSetCurrentItemExpanded( bool xExpanded );
    CFlowWidgetItem* mCurrentTopLevelItem() const;
    CFlowWidgetImpl * dImpl{nullptr};
};
Q_DECLARE_METATYPE( CFlowWidget::EStates );

// each Item can have 2 icons
// the first is defined via the xDescIcon parameter during construction, is used to represent the action describe by the flow item, ie the flow item "load files" might use the file open icon
// the second is set via the state attribute on the Item, as a state changes, that icon is update to reflect the new state
//    there are 7 predefined states with associated pre-defined icons, as part of the initialization of the CFlowWidget itself, the client can add new flow states (int value and icon) to the system

// note, you do not own the memory for the CFlowWidgetItem once its added to a parent FlowWidget or FlowWidgetItem
class WINDOWS_QDECLSPEC CFlowWidgetItem
{
public:
    friend class CFlowWidgetItemImpl;
    friend class CFlowWidgetImpl;
    friend class CFlowWidget;
    friend class CFlowWidgetHeader;

    enum ERoles
    {
        eStateStatusRole = Qt::UserRole + 1,  // returns the states
        eStateIconsRole,                      // returns he icons
        eAttributesRole
    };

    CFlowWidgetItem();
    CFlowWidgetItem( CFlowWidgetItem * xParent );
    CFlowWidgetItem( CFlowWidget * xParent );

    CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, CFlowWidget* xParent );
    CFlowWidgetItem( const QString& xStepID, const QString& xFlowName, CFlowWidgetItem* xParent );

    CFlowWidgetItem( const QString & xStepID, const QIcon& xDescIcon, CFlowWidget* xParent );
    CFlowWidgetItem( const QString & xStepID, const QIcon& xDescIcon, CFlowWidgetItem* xParent );

    CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon, CFlowWidget* xParent );
    CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon, CFlowWidgetItem* xParent );
    ~CFlowWidgetItem();

    void deleteLater();

    CFlowWidgetItem* mParentItem() const;
    CFlowWidget* mGetFlowWidget() const;

    void mAddChild( CFlowWidgetItem* xChild );
    int mInsertChild( int xIndex, CFlowWidgetItem* xChild );
    int mInsertChild( CFlowWidgetItem* xPeer, CFlowWidgetItem* xChild, bool xBefore );

    int mIndexOfChild( const CFlowWidgetItem* xChild );

    int mChildCount() const;
    CFlowWidgetItem* mGetChild( int xIndex ) const;
    CFlowWidgetItem* mGetChild( const QString & xStepID ) const; // returns the first child with the stepID

    CFlowWidgetItem* mTakeChild( CFlowWidgetItem* xItem ); // ownership of the returned item is now the client

    bool mSetData( int xRole, const QVariant& xVariant ); // if the value for the role changes return true
    QVariant mData( int xRole ) const; // for top level items tied to QAbstract Button, for Sub Items tree widget item

    void mSetText( const QString& xText );
    QString mText() const;
    QString mFullText( const QChar& xSeparator = QChar( '.' ) ) const;

    void mSetIcon( const QIcon& xIcon );
    QIcon mIcon() const;

    void mSetStepID( const QString & xStepID );
    QString mStepID() const;

    void mSetToolTip( const QString& xToolTip );
    QString mToolTip() const;

    void mSetAttribute( const QString & xAttributeName, const QString & xValue );
    QString mGetAttribute( const QString & xAttributeName ) const;
    std::list< std::pair< QString, QString > > mGetAttributes() const;

    // order is kept of statuses, if you set via a list and only the order is changed, the item is considered changed
    bool mSetStateStatus( int xStateStatus ); // returns true if state changed if none is sent it, it clears all
    bool mSetStateStatus( const QList< int > & xStateStatuses ); // returns true if state changed, if none is the only item sent in, it clears, if individual it is ignored
    bool mAddStateStatus( int xStateStatus ); // returns true if state add, false if already set, none is sent in, its ignored
    bool mRemoveStateStatus( int xStateStatus ); // returns true if state removed, false if already set
    QList< int > mStateStatuses( bool xLocalOnly ) const; // guaranteed to never be empty, eNone is always returned if needed

    void mSetHidden( bool xHidden );
    bool mIsHidden() const;

    void mSetVisible( bool xVisible, bool xExpandIfVisible );
    bool mIsVisible() const;

    void mSetDisabled( bool xDisabled );
    bool mIsDisabled() const;

    void mSetEnabled( bool xEnabled );
    bool mIsEnabled() const;

    void mExpandAll();
    void mSetExpanded( bool xExpanded );
    bool mIsExpanded() const;

    void mSetSelected( bool xSelected );
    bool mSelected() const;

    bool mIsTopLevelItem() const;

    int mIndexInParent() const;

    QString mDump( bool xRecursive, bool xCompacted ) const;
    void mDump( QJsonObject& xJSON, bool xRecursive ) const;

private:
    CFlowWidgetItem( const QString & xStepID, const QString& xFlowName, const QIcon& xDescIcon );
    CFlowWidgetItemImpl * dImpl{nullptr};
};
Q_DECLARE_METATYPE( CFlowWidgetItem* );

#endif 
