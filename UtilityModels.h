// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __UTILITYMODELS_H
#define __UTILITYMODELS_H

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QPair>
#include <QStringList>
#include <QListView>
#include <QTreeView>
#include <QTableView>
#include <QMap>
#include <QStringListModel>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

class QAbstractButton;
class QXmlQuery;
class QDir;

class CMoveStringListModel : public QStringListModel
{
    Q_OBJECT;
public:
    CMoveStringListModel( QObject * parent );
    bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent ) override;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    void clear(){ setStringList( QStringList() ); }
    void addRow( const QString & value );
};

class CStringListModel : public QStringListModel
{
    Q_OBJECT;
public:
    CStringListModel( QObject * parent );
    void clear(){ setStringList( QStringList() ); }
    void addRow( const QString & value );
    void addRow();

    void copy( const CStringListModel * rhs );
    bool operator==( const CStringListModel & rhs ) const;
    QString at( int ii ) const;
};

class CStringTupleModel : public QAbstractTableModel
{
    Q_OBJECT;
public:
    CStringTupleModel( const QStringList & columnNames, QObject * parent );
    CStringTupleModel( int numColumns, QObject * parent ); // "Key", Value1 Value2 value3... valueN-1

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override { if ( parent.isValid() ) return 0; return fData.count(); }
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override { if ( parent.isValid() ) return 0; return fColumnNames.count(); }

    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    void setRow( const QString & key, const QStringList & data );
    void addRow( const QStringList & data );
    void addRow();
    virtual bool removeRows( int row, int count, const QModelIndex & parent = QModelIndex() );
    virtual bool removeRow( int row, const QModelIndex & parent = QModelIndex() );

    virtual Qt::ItemFlags flags( const QModelIndex & index ) const override;

    void copy( const CStringTupleModel * rhs );
    bool operator==( const CStringTupleModel & rhs ) const;
    void clear();
    void setValues( const QList< QStringList > & values );
    const QStringList & at( int ii ) const{ return fData.at( ii ); }

    void setKeyEditable( bool isEditable ){ fIsKeyEditable = isEditable; }
    QList< QStringList > data() const{ return fData; }

    bool loadXML( QXmlQuery & query, const std::list< std::pair< QString, bool > > & queries, const QDir & relToDir, QString & msg, bool clearFirst );
protected:
    QStringList fColumnNames;
    bool fIsKeyEditable{ true };
    QList< QStringList > fData;
};

class CKeyValuePairModel : public CStringTupleModel
{
    Q_OBJECT;
public:
    CKeyValuePairModel( const QString & keyName, const QString & valueName, QObject * parent );
    CKeyValuePairModel( QObject * parent );

    void setRow( const QString & key, const QString & value );

    void addRow( const QString & key, const QString & value );
    void addRow( const QString & pair );
    void addRow(){ CStringTupleModel::addRow(); }

    void setValues( const QList< QPair< QString, QString > > & values );
    QString getPair( int ii ) const; // key=value
    QPair< QString, QString > at( int ii ) const;
    QList< QPair< QString, QString > > data() const;
};

class CCheckableStringListModel : public QStringListModel
{
Q_OBJECT
public:
    CCheckableStringListModel( QObject * parent = nullptr );

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    virtual QVariant data( const QModelIndex & index, int role ) const override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
    virtual Qt::ItemFlags flags( const QModelIndex & index ) const override;

    bool setChecked( QString string, bool checked, bool update );
    void setChecked( const QStringList & strings, bool checked, bool update );

    QStringList getCheckedStrings( bool & allChecked ) const;
    QStringList getCheckedStrings() const;
    void uncheckAll( bool update );
    void checkAll( bool update );

    bool operator==( const CCheckableStringListModel & rhs ) const;
    void copy( const CCheckableStringListModel & rhs );
    bool isChecked( int rowNum ) const;

    void setStringList( const std::list< std::pair< QString, bool > > & values );
    void setStringList( const QStringList &strings ){ QStringListModel::setStringList( strings ); }

    void setAliasMap( const std::map< QString, QString > & map );
signals:
    void sigBlockFilterUpdates( bool block );
    void sigChanged();
private:
    QString getAlias( const QString & key ) const;
    std::map< QString, std::pair< QString, bool > > fEnabled;
    std::map< QString, QString > fAliasMap;
    std::map< QString, QString > fReverseAliasMap;
};

class CStringFilterModel : public QSortFilterProxyModel
{
Q_OBJECT
public:
    CStringFilterModel( QAbstractItemModel * sourceModel, QLineEdit * filter, QObject * parent = nullptr );
    CStringFilterModel( QAbstractItemModel * sourceModel, QLineEdit * filter, QAbstractButton * pb, QObject * parent = nullptr );
public slots:
    void slotFilterChanged();
private:
    QLineEdit * fFilter;
    QTimer * fTimer;
};

class CCheckableListView : public QListView
{
Q_OBJECT
public:
    CCheckableListView( QWidget * parent = nullptr );
    void keyPressEvent(QKeyEvent *event);
signals:
    void sigBlockFilterUpdates( bool block );
};


class CCheckableTableView : public QTableView
{
Q_OBJECT
public:
    CCheckableTableView( QWidget * parent = nullptr );
    void keyPressEvent(QKeyEvent *event);
signals:
    void sigBlockFilterUpdates( bool block );
};

class CCheckableTreeView : public QTreeView
{
Q_OBJECT
public:
    CCheckableTreeView( QWidget * parent = nullptr );
    void keyPressEvent(QKeyEvent *event);
signals:
    void sigBlockFilterUpdates( bool block );
};

#endif