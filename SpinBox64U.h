// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
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

#ifndef __SPINBOX64U_H
#define __SPINBOX64U_H
#include <QAbstractSpinBox>
#include <QString>
#include <QSize>

#include <cinttypes>

class CSpinBox64UImpl;
class CSpinBox64U : public QAbstractSpinBox
{
    friend class CSpinBox64UImpl;
    Q_OBJECT;
    Q_PROPERTY( bool wrapping READ wrapping WRITE setWrapping )
    Q_PROPERTY( QString suffix READ suffix WRITE setSuffix )
    Q_PROPERTY( QString prefix READ prefix WRITE setPrefix )
    Q_PROPERTY( QString cleanText READ cleanText )
    Q_PROPERTY( uint64_t minimum READ minimum WRITE setMinimum )
    Q_PROPERTY( uint64_t maximum READ maximum WRITE setMaximum )
    Q_PROPERTY( uint64_t singleStep READ singleStep WRITE setSingleStep )
    Q_PROPERTY( StepType stepType READ stepType WRITE setStepType )
    Q_PROPERTY( uint64_t value READ value WRITE setValue NOTIFY valueChanged USER true )
    Q_PROPERTY( uint64_t displayIntegerBase READ displayIntegerBase WRITE setDisplayIntegerBase )
public:
    explicit CSpinBox64U( QWidget* parent = nullptr );
    CSpinBox64U( const CSpinBox64U& ) = delete;
    CSpinBox64U& operator=( const CSpinBox64U& ) = delete;
    ~CSpinBox64U();

    uint64_t value() const ;

    QString prefix() const;
    void setPrefix( const QString& prefix );

    QString suffix() const;
    void setSuffix( const QString& suffix );

    bool wrapping() const;
    void setWrapping( bool wrapping );

    QString cleanText() const;

    uint64_t singleStep() const;
    void setSingleStep( uint64_t val );

    uint64_t minimum() const;
    void setMinimum( uint64_t min );

    uint64_t maximum() const;
    void setMaximum( uint64_t max );

    void setRange( uint64_t min, uint64_t max );

    StepType stepType() const;
    void setStepType( StepType stepType );

    int displayIntegerBase() const;
    void setDisplayIntegerBase( int base );

    QSize sizeHint() const override;

    virtual void stepBy( int steps ) override;

    static uint64_t maxAllowed() { return std::numeric_limits< uint64_t >::max(); }
    static uint64_t minAllowed() { return std::numeric_limits< uint64_t >::min(); }
    QValidator::State validate( QString& input, int& pos ) const override;
    virtual void fixup( QString& str ) const override;
    virtual QAbstractSpinBox::StepEnabled stepEnabled() const override;

public Q_SLOTS:
    void setValue( uint64_t val );
    void slotEditorCursorPositionChanged( int oldpos, int newpos );
    void slotEditorTextChanged( const QString& t );
Q_SIGNALS:
    void valueChanged( uint64_t v );
    void valueChanged( const QString & v );

protected:
    void connectLineEdit();

private:
    CSpinBox64UImpl * fImpl;
};
#endif
