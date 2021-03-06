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

#include "StringUtils.h"
#include "FromString.h"

#include <QString>
#include <QRegExp>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <functional>
#include <algorithm>
#include <cctype>
#include <cmath>

#ifdef _WIN32
#define vscprintf _vscprintf
#define vsnprintf _vsnprintf
#if _MSC_VER == 1500
#define va_copy( a, b ) a=b
#endif
#else
#define vscprintf( f, a ) vsnprintf( nullptr, 0, f, a )
#endif

#ifdef _LINUX
#include <errno.h>
#endif
#include <locale>
#include <map>

using namespace std;
namespace NStringUtils
{
    double round( double value, int numDecimalPlaces )
    {
        if ( numDecimalPlaces < 0 )
            numDecimalPlaces = 0;
        auto mult = std::pow( 10, numDecimalPlaces );
        return std::round( mult*value ) / mult;
    }


    bool is_number( const std::string & str )
    {
        if ( str.empty() )
            return false;
        for ( auto ch : str )
        {
            if ( !isdigit( ch ) )
            {
                return false;
            }
        }
        return true;
    }

    std::string left( std::string inString, size_t len )
    {
        if ( len >= inString.size() )
            return inString;
        return inString.substr( 0, len );
    }

    std::string right( std::string inString, size_t len )
    {
        if ( len >= inString.size() )
            return inString;
        return inString.substr( inString.size() - len );
    }

    std::string stripHierName( std::string objectName, const std::string & hierSep, bool stripArrayInfo )
    {
        size_t pos = objectName.find_last_of( "./" + hierSep ); // for flat nets
        if ( pos != std::string::npos )
            objectName = objectName.erase( 0, pos + 1 );

        if ( stripArrayInfo )
        {
            pos = objectName.find_last_of( "[" );
            if ( pos != std::string::npos )
                objectName = objectName.erase( pos, objectName.length() );
        }
        return objectName;
    }

    std::string addToRegEx( std::string oldRegEx, const std::string & regEx )
    {
        auto subExps = splitString( oldRegEx, "|" );
        auto newSubExps = splitString( regEx, "|" );
        subExps.insert( subExps.end(), newSubExps.cbegin(), newSubExps.cend() );
        if ( subExps.size() > 1 )
        {
            for ( auto & ii : subExps )
            {
                if ( *ii.begin() != '(' && *ii.rbegin() != ')' )
                    ii = "(" + ii + ")";
            }
        }

        auto uniqueSub = std::set< std::string >( subExps.begin(), subExps.end() );
        oldRegEx = joinString( uniqueSub, "|" );
        return oldRegEx;
    }

    void replaceAll( char * str, char from, char to )
    {
        while ( *str )
        {
            if ( *str == from )
                *str = to;
            str++;
        }
    }

    std::string replaceAll( const std::string& str, char from, char to )
    {
        std::string retVal = str;
        return replaceAll( retVal, from, to );
    }

    std::string replaceAll( std::string& str, char from, char to )
    {
        for ( std::string::iterator ii = str.begin(); ii != str.end(); ++ii )
        {
            if ( *ii == from )
            {
                *ii = to;
            }
        }
        return str;
    }

    std::string replaceAll( const std::string& str, const std::string& from, const std::string& to )
    {
        std::string retVal = str;
        return replaceAll( retVal, from, to );
    }

    std::string replaceAll( std::string& str, const std::string& from, const std::string& to )
    {
        size_t pos = str.find( from );
        while ( pos != std::string::npos )
        {
            str.replace( pos, from.length(), to );
            pos = str.find( from, pos + to.length() );
        }
        return str;
    }

    std::string replaceAll( const std::string& str, const std::string& from, char to )
    {
        std::string retVal = str;
        return replaceAll( retVal, from, to );
    }

    std::string replaceAll( std::string& str, const std::string& from, char to )
    {
        size_t pos = str.find( from );
        while ( pos != std::string::npos )
        {
            str.replace( pos, from.length(), 1, to );
            pos = str.find( from, pos + 1 );
        }
        return str;
    }

    std::string replaceAll( const std::string& str, char from, const std::string & to )
    {
        std::string retVal = str;
        return replaceAll( retVal, from, to );
    }

    std::string replaceAll( std::string& str, char from, const std::string & to )
    {
        size_t pos = str.find( from );
        while ( pos != std::string::npos )
        {
            str.replace( pos, 1, to );
            pos = str.find( from, pos + to.length() );
        }
        return str;
    }

    std::string stripBlanksHead( const std::string & inStr )
    {
        static std::string whitespaces( " \t\f\v\n\r" );

        std::string::size_type startIdx = inStr.find_first_not_of( whitespaces );
        if ( startIdx == std::string::npos )
            return std::string();
        std::string retVal = inStr;
        retVal.erase( 0, startIdx );
        return retVal;
    }

    std::string stripBlanksTail( const std::string & inStr )
    {
        static std::string whitespaces( " \t\f\v\n\r" );

        std::string::size_type endIdx = inStr.find_last_not_of( whitespaces );

        std::string retVal = inStr;
        retVal.erase( endIdx + 1 );

        return retVal;
    }

    void getBlankIndexes( const std::string & str, size_t & startIdx, size_t & endIdx )
    {
        static std::string whitespaces( " \t\f\v\n\r" );

        startIdx = endIdx = std::string::npos;
        startIdx = str.find_first_not_of( whitespaces );
        if ( startIdx == std::string::npos )
            return;

        endIdx = str.find_last_not_of( whitespaces );
    }

    void stripBlanksInline( std::string & str )
    {
        std::string::size_type startIdx;
        std::string::size_type endIdx;
        getBlankIndexes( str, startIdx, endIdx );

        if ( startIdx == std::string::npos )
        {
            str.clear();
            return;
        }
        str.erase( endIdx + 1 );
        str.erase( 0, startIdx );
    }


    std::string stripBlanks( const std::string & inStr )
    {
        std::string::size_type startIdx;
        std::string::size_type endIdx;
        getBlankIndexes( inStr, startIdx, endIdx );

        if ( startIdx == std::string::npos )
            return std::string();

        std::string retVal = inStr.substr( startIdx, endIdx - startIdx + 1 );
        return retVal;
    }

    //////////////////////////////////////////////////////////////////////////
    // stripQuotes
    // 
    // Strips quotes "" from beginning and ending of string
    //
    // Returns new string
    //////////////////////////////////////////////////////////////////////////

    std::string stripQuotes( const char * text, char quote )
    {
        char tmp[ 2 ] = { 0, 0 };
        tmp[ 0 ] = quote;
        return stripQuotes( text, tmp );
    }

    std::string stripQuotes( const std::string & text, char quote )
    {
        char tmp[ 2 ] = { 0, 0 };
        tmp[ 0 ] = quote;
        return stripQuotes( text, tmp );
    }

    QString stripQuotes( const QString & text, char quote )
    {
        char tmp[ 2 ] = { 0, 0 };
        tmp[ 0 ] = quote;
        return stripQuotes( text, tmp );
    }

    std::string stripQuotes( const char * text, const char * quotes )
    {
        if ( !text )
            return std::string();
        return stripQuotes( std::string( text ), quotes );
    }

    std::string stripQuotes( const std::string & text, const char * quotes )
    {
        std::string retVal = stripBlanks( text ); // Get rid of leading/trailing spaces
        for( auto currQuote = quotes; *currQuote; ++currQuote )
        {
            if ( ( retVal.length() >= 2 ) && *retVal.begin() == *currQuote && *retVal.rbegin() == *currQuote )
                retVal = retVal.substr( 1, retVal.length() - 2 );
            if ( ( retVal.length() >= 3 ) && *retVal.begin() == *currQuote && ( ( *( retVal.rbegin() ) == '\\' ) || ( *( retVal.rbegin() ) == '/' ) ) && *( retVal.rbegin() + 1 ) == *currQuote )
                retVal = retVal.substr( 1, retVal.length() - 3 ) + *( retVal.rbegin() );
        }

        return retVal;
    }

    QString stripQuotes( const QString & text, const char * quotes )
    {
        QString retVal = text.trimmed();
        for ( auto currQuote = quotes; *currQuote; ++currQuote )
        {
            if ( ( retVal.length() >= 2 ) && retVal.leftRef( 1 ).at( 0 ) == *currQuote && retVal.rightRef( 1 ).at( 0 ) == *currQuote )
                retVal = retVal.mid( 1, retVal.length() - 2 );
            if ( ( retVal.length() >= 3 ) && retVal.leftRef( 1 ).at( 0 ) == *currQuote && ( ( retVal.rightRef( 1 ) == "\\" ) || ( retVal.rightRef( 1 ) == "/" ) ) && retVal.midRef( retVal.length() - 2, 1 ).at( 0 ) == *currQuote )
                retVal = retVal.mid( 1, retVal.length() - 3 ) + retVal.rightRef( 1 ).toString();
        }
        return retVal;
    }

    bool isQuoted( const char * text, char quote )
    {
        char tmp[ 2 ] = {0,0};
        tmp[ 0 ] = quote;
        return isQuoted( text, tmp );
    }

    bool isQuoted( const std::string & text, char quote )
    {
        char tmp[ 2 ] = { 0, 0 };
        tmp[ 0 ] = quote;
        return isQuoted( text, tmp );
    }

    bool isQuoted( const QString & text, char quote )
    {
        char tmp[ 2 ] = { 0, 0 };
        tmp[ 0 ] = quote;
        return isQuoted( text, tmp );
    }

    bool isQuoted( const char * text, const char * quotes )
    {
        if ( !text )
            return false;
        return isQuoted( std::string( text ), quotes );
    }

    bool isQuoted( const std::string & text, const char * quotes )
    {
        std::string retVal = stripBlanks( text ); // Get rid of leading/trailing spaces
        for ( auto currQuote = quotes; *currQuote; ++currQuote )
        {
            if ( ( retVal.length() >= 2 ) && *retVal.begin() == *currQuote && *retVal.rbegin() == *currQuote )
                return true;
            if ( ( retVal.length() >= 3 ) && *retVal.begin() == *currQuote && ( ( *( retVal.rbegin() ) == '\\' ) || ( *( retVal.rbegin() ) == '/' ) ) && *( retVal.rbegin() + 1 ) == *currQuote )
                return true;
        }

        return false;
    }

    bool isQuoted( const QString & text, const char * quotes )
    {
        QString retVal = text.trimmed();
        for ( auto currQuote = quotes; *currQuote; ++currQuote )
        {
            if ( ( retVal.length() >= 2 ) && retVal.leftRef( 1 ).at( 0 ) == *currQuote && retVal.rightRef( 1 ).at( 0 ) == *currQuote )
                return true;
            if ( ( retVal.length() >= 3 ) && retVal.leftRef( 1 ).at( 0 ) == *currQuote && ( ( retVal.rightRef( 1 ) == "\\" ) || ( retVal.rightRef( 1 ) == "/" ) ) && retVal.midRef( retVal.length() - 2, 1 ).at( 0 ) == *currQuote )
                return true;
        }
        return false;
    }

    string stripAllBlanksAndQuotes( const std::string &text )
    {
        static std::string whitespace( " \t\f\v\n\r" );
        static string quotes( "\"\'" );
        std::string retVal( "" );
        retVal.reserve( text.length() + 1 );
        for ( std::string::const_iterator si = text.begin(); si != text.end(); ++si )
        {
            if ( whitespace.find( *si ) != std::string::npos )
            {
                retVal.push_back( '_' );
            }
            else if ( quotes.find( *si ) == std::string::npos )
            {
                retVal.push_back( *si );
            }
        }
        return retVal;
    }


    // -----------------------------------------------------------
    // Used to strip topModule name from net full path name

    // Moved here from verity
    // -----------------------------------------------------------
    std::string stripHead( const std::string & head, const std::string & name, bool * found )
    {
        std::string retVal = name;
        size_t idx = retVal.find( head );
        if ( found )
            *found = false;
        if ( idx != std::string::npos && !idx && retVal[ head.length() ] == '.' )
        {
            if ( found )
                *found = true;
            retVal = retVal.substr( head.length() + 1 );
        }
        return retVal;
    }

    //////////////////////////////////////////////////////////////////////////
    // containsSubString(string str, string substr)
    // 
    // 
    //
    // Returns true if string contains sub-string. Leading/trailing * is wild-card.
    // Revision History: Added , string::size_type *ind arg
    //////////////////////////////////////////////////////////////////////////
    bool containsSubString( const std::string & str, const std::string & subs, std::string::size_type *ind )
    {
        string::size_type idx;

        if ( str.empty() || subs.empty() )
            return false;

        idx = subs.find_first_not_of( "*" );
        if ( idx == string::npos )
            return true;

        std::string realSubs = subs.substr( idx );

        size_t endi = realSubs.length() - 1;

        if ( realSubs[ endi ] == '*' )
            realSubs = realSubs.substr( 0, endi );

        idx = str.find( realSubs );
        if ( idx == string::npos )
            return false;

        if ( ind )
            *ind = idx;
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // hasPrefixSubString(string str, string substr)
    // 
    // 
    //
    // Returns true if string contains sub-string. Ending * is wild-card
    //////////////////////////////////////////////////////////////////////////
    bool hasPrefixSubString( const std::string & str, const std::string & prefix )
    {
        if ( str.empty() || prefix.empty() )
            return false;

        QString pre = QString::fromStdString( prefix );
        if ( *pre.end() != '*' )
            pre += "*";
        QRegExp regEx( pre, Qt::CaseSensitive, QRegExp::Wildcard );
        return regEx.exactMatch( QString::fromStdString( str ) );
    }


    //////////////////////////////////////////////////////////////////////////
    // hasSuffixSubString(string str, string substr)
    // 
    // 
    //
    // Returns true if string contains sub-string. Starting * is wild-card
    //////////////////////////////////////////////////////////////////////////
    bool hasSuffixSubString( const std::string & str, const std::string & suffix )
    {
        if ( str.empty() || suffix.empty() )
            return false;

        QString suf = QString::fromStdString( suffix );
        if ( *suf.begin() != '*' )
            suf.insert( 0, "*" );
        QRegExp regEx( suf, Qt::CaseSensitive, QRegExp::Wildcard );
        return regEx.exactMatch( QString::fromStdString( str ) );
    }

    std::string replaceAllNot( const std::string & inString, const std::string & notOf, char to )
    {
        std::string string = inString;
        std::replace_if( string.begin(), string.end(), [ notOf ]( char x ) { return ( notOf.find( x ) == std::string::npos ); }, to );
        return string;
    }

    //////////////////////////////////////////////////////////////////////////
    // hasSuffixSubString(string str, string substr)
    // 
    // 
    //
    // Returns true if string contains wild-card * suffix

    //////////////////////////////////////////////////////////////////////////
    bool hasWildCardSuffixSubString( const std::string & str )
    {
        if ( str.empty() )
            return false;

        size_t j = str.length() - 1;

        if ( str[ j ] == '*' )
            return true;

        return false;
    }


    //////////////////////////////////////////////////////////////////////////
    // containsWildCardCharacters(string str)
    // 
    // 
    //
    // Returns true if string contains wild-card */? characters

    //////////////////////////////////////////////////////////////////////////
    bool containsWildCardCharacters( const std::string & str )
    {
        if ( str.empty() )
            return false;

        for ( size_t j = 0; j < str.length(); j++ )
        {
            if ( ( str[ j ] == '*' ) || ( str[ j ] == '?' ) )
                return true;
        }
        return false;
    }

    bool isLowerCaseString( const std::string & text )
    {
        for ( size_t i = 0; i < text.length(); i++ )
        {
            if ( isalpha( text[ i ] ) && !islower( text[ i ] ) )
                return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // hasLowerCaseChars()
    //
    // Returns true if there is a lower case characer
    //////////////////////////////////////////////////////////////////////////
    bool hasLowerCaseChars( const std::string & text )
    {
        for ( size_t i = 0; i < text.length(); i++ )
        {
            if ( isalpha( text[ i ] ) && islower( text[ i ] ) )
                return true;
        }
        return false;
    }


    int
        count_identifiers( const char *stmt )
    {
        int cnt = 0;

        while ( *stmt != '\0' )
        {
            while ( ( *stmt == ' ' ) || ( *stmt == '\t' ) || ( *stmt == '\n' ) )
                stmt++;  // Skip spaces
            if ( ( *stmt != '\0' ) && ( *stmt != ' ' ) &&
                 ( *stmt != '\t' ) && ( *stmt != '\n' ) )
                 cnt++; // Increment count at start of non-white space identifier
            while ( ( *stmt != '\0' ) && ( *stmt != ' ' ) &&
                    ( *stmt != '\t' ) && ( *stmt != '\n' ) )
                    stmt++; // Skip rest of identifier
        }


        return cnt;
    }


    int curr_file_line_no = 0;

    bool
        get_string_token( FILE *fp, char *s )
    {
        int fgetcValue;
        bool status = true;
        char last_c = 0;
        bool processing_token = false;

        while ( ( fgetcValue = fgetc( fp ) ) != EOF )
        {
            char c = (char)fgetcValue;
            if ( ( c == '/' ) && ( last_c == '/' ) )
            {
                *( s - 1 ) = '\0';
                while ( ( ( c = (char)fgetc( fp ) ) != EOF ) && c != '\n' )
                    ;
            }
            else if ( processing_token &&
                      ( ( c == '\t' ) || ( c == ' ' ) || ( c == '\n' ) ) )
            {
                processing_token = false;
                break;
            }
            else if ( ( c != '\t' ) && ( c != ' ' ) && ( c != '\n' ) )
            {
                processing_token = true;
                last_c = c;
                *s++ = c;
            }
        }

        if ( fgetcValue == '\n' )
            curr_file_line_no++;
        *s = '\0';
        if ( fgetcValue == EOF )
            status = false;

        return status;
    }


    bool has_suffix( const char *str, const char *suffix )
    {
        int endi;

        if ( suffix[ 0 ] == '*' )
            endi = 1;
        else
            endi = 0;
        size_t j = strlen( str ) - 1;
        for ( int64_t i = ( strlen( suffix ) - 1 ); i >= endi; i--, j-- )
        {
            if ( str[ j ] != suffix[ i ] )
                return false;
        }

        return true;
    }
    bool has_suffix( const std::string & str, const char * suffix )
    {
        return has_suffix( str.c_str(), suffix );
    }


    std::string hexToBin( char ch, bool * aOK )
    {
        if ( aOK )
            *aOK = true;
        switch ( std::tolower( ch ) )
        {
            case '0': return "0000";
            case '1': return "0001";
            case '2': return "0010";
            case '3': return "0011";
            case '4': return "0100";
            case '5': return "0101";
            case '6': return "0110";
            case '7': return "0111";
            case '8': return "1000";
            case '9': return "1001";
            case 'a': return "1010";
            case 'b': return "1011";
            case 'c': return "1100";
            case 'd': return "1101";
            case 'e': return "1110";
            case 'f': return "1111";
            case 'x':
            case 'z':
                return std::string( 4, ch );
            default:
                {
                    if ( aOK )
                        *aOK = false;
                    return std::string( 4, 'X' );
                }
        }
    }

    std::string octToBin( char ch, bool * aOK )
    {
        if ( aOK )
            *aOK = true;
        switch ( std::tolower( ch ) )
        {
            case '0': return "000";
            case '1': return "001";
            case '2': return "010";
            case '3': return "011";
            case '4': return "100";
            case '5': return "101";
            case '6': return "110";
            case '7': return "111";
            case 'x':
            case 'z':
                return std::string( 3, ch );
            default:
                {
                    if ( aOK )
                        *aOK = false;
                    return std::string( 3, 'X' );
                }
        }
    }

    std::string hexToBinXY( const std::string & in, bool * aOK )
    {
        std::string retVal;
        bool lclOK = true;
        for ( size_t ii = 0; ii < in.size(); ++ii )
        {
            bool tmpOK = true;
            retVal += hexToBin( in[ ii ], &tmpOK );
            lclOK = lclOK && tmpOK;
        }
        if ( aOK )
            *aOK = lclOK;
        return retVal;
    }

    std::string octToBinXY( const std::string & in, bool * aOK )
    {
        std::string retVal;
        bool lclOK = true;
        for ( size_t ii = 0; ii < in.size(); ++ii )
        {
            bool tmpOK = true;
            retVal += octToBin( in[ ii ], &tmpOK );
            lclOK = lclOK && tmpOK;
        }
        if ( aOK )
            *aOK = lclOK;
        return retVal;
    }

    std::string toBin( std::ios_base & ( *base )( std::ios_base & ), const std::string & in, size_t len, bool * aOK )
    {
        if ( aOK )
            *aOK = false;

        std::istringstream iss( in );
        int64_t val;
        iss >> base >> std::ws >> val;

        if ( !iss.eof() )
            iss >> std::ws;

        if ( iss.fail() || !iss.eof() )
            return "";
        if ( aOK )
            *aOK = true;
        return toBinString( val, len );
    }

    std::string hexToBin( const std::string & in, size_t len, bool * aOK )
    {
        if ( len > 64 )
            return hexToBinXY( in, aOK );

        return toBin( std::hex, in, len, aOK );
    }

    std::string hexToBin( const std::string & in, bool * aOK )
    {
        return hexToBin( in, in.length() * 4, aOK );
    }

    std::string octToBin( const std::string & in, bool * aOK )
    {
        return octToBin( in, in.length() * 4, aOK );
    }

    std::string octToBin( const std::string & in, size_t  len, bool * aOK )
    {
        if ( len > 64 )
            return octToBinXY( in, aOK );

        return toBin( std::oct, in, len, aOK );
    }

    std::string decToBin( const std::string & in, bool * aOK )
    {
        return decToBin( in, 32, aOK );
    }

    std::string decToBin( const std::string & in, size_t len, bool * aOK )
    {
        return toBin( std::dec, in, len, aOK );
    }

    std::string AsciiToBin( const std::string &in )
    {
        std::string result = "";
        result.reserve( in.length() * 8 );
        for ( size_t i = 0; i < in.length(); ++i )
        {
            result += toBinString( in[ i ], 8 );
        }
        return result;
    }

    void stripBlanks( std::string& str )
    {
        for ( int64_t i = str.length() - 1; i >= 0; --i )
        {
            if ( !isspace( str[ i ] ) ) break;
            str = str.substr( 0, i );
        }
    }


    int hexToInt( const char *id, bool * aOK )
    {
        if ( aOK )
            *aOK = false;
        if ( !id )
            return 0;

        int retVal;
        bool lclOK = hexToInt< int >( retVal, std::string( id ) );
        if ( aOK )
            *aOK = lclOK;
        return retVal;
    }

    std::string stripInline( const std::string & inStr, char value )
    {
        std::string retVal = inStr;
        strip( retVal, value );
        return retVal;
    }

    void strip( std::string & str, char ch )
    {
        str.erase( std::remove( str.begin(), str.end(), ch ), str.end() );
    }

    std::string getFMTString( const char * fmt, ... )
    {
        std::string retVal;
        if ( fmt != nullptr )
        {
            va_list marker;
            va_start( marker, fmt );
            retVal = getVAString( fmt, marker );
            va_end( marker );
        }
        return retVal;
    }

    std::string getVAString( const char * fmt, va_list marker )
    {
        if ( !fmt )
            return std::string();

        size_t len = 0;
        va_list tmp;
        va_copy( tmp, marker );
        len = vscprintf( fmt, tmp ) + 1;
        va_end( tmp );
        char * buff = new char[ len ];

        int written = 0;

        va_copy( tmp, marker );
        written = vsnprintf( buff, len, fmt, tmp );
        va_end( tmp );

        std::string retVal;
        if ( written )
            retVal = buff;
        delete[] buff;
        return retVal;
    }

    std::list< std::string > stripParen( const std::list< std::string > & list )
    {
        auto retVal = list;
        for ( auto & ii : retVal )
        {
            if ( ii.empty() )
                continue;

            if ( *ii.begin() == '(' && *ii.rbegin() == ')' )
            {
                ii.erase( 0, 1 );
                ii.erase( ii.length() - 1, 1 );
            }
        }
        return retVal;
    }

    bool regExEqual( const std::string & lhs, const std::string & rhs )
    {
        if ( lhs == rhs )
            return true;

        auto lhsSplit = stripParen( splitString( lhs, "|" ) );
        auto lhsExps = std::set< std::string >( lhsSplit.begin(), lhsSplit.end() );

        auto rhsSplit = stripParen( splitString( rhs, "|" ) );
        auto rhsExps = std::set< std::string >( rhsSplit.begin(), rhsSplit.end() );

        return lhsExps == rhsExps;
    }


    bool isExactMatchRegEx( const std::string & data, const std::string & pattern, bool nocase )
    {
        std::string regEx = "\\A(" + pattern + ")\\z";
        QRegularExpression regExp( QString::fromStdString( regEx ) );
        if ( !regExp.isValid() )
            return false;
        if ( nocase )
            regExp.setPatternOptions( QRegularExpression::CaseInsensitiveOption );
        auto match = regExp.match( QString::fromStdString( data ), 0, QRegularExpression::PartialPreferCompleteMatch );
        return match.hasMatch();
    }

    QString PadString( const QString & str, size_t max, EPadType padType, char padChar )
    {
        return QString::fromStdString( PadString( str.toStdString(), max, padType, padChar ) );
    }


    std::string PadString( const std::string & str, size_t max, EPadType padType, char padChar )
    {
        std::string retVal;
        std::string::size_type diff = max - str.length();
        if ( static_cast<size_t>( max ) < str.length() )
            diff = 0;

        switch ( padType )
        {
            case EPadType::eLeftJustify: // left justify add spaces in back of the string
                retVal = str + std::string( diff, padChar );
                break;
            case EPadType::eRightJustify: // right justify add sapces in fron of the string
                retVal = std::string( diff, padChar ) + str;
                break;
            default:
            case EPadType::eCenter:
                {
                    std::string::size_type leftSide = (int)( std::floor( diff / 2.0 ) );
                    std::string::size_type rightSide = 0;
                    if ( diff >= leftSide )
                        rightSide = diff - leftSide;
                    retVal = std::string( leftSide, padChar ) + str + std::string( rightSide, padChar );
                }

        }
        return retVal;
    }

    void stripLF( std::string & str )
    {
        if ( str.length() && ( *str.rbegin() == '\r' ) )
            str.erase( --str.end() );
    }

    void stripLF( char * str )
    {
        if ( str )
        {
            size_t len = strlen( str );
            if ( len && ( str[ len - 1 ] == '\r' ) )
                str[ len - 1 ] = '\0';
        }
    }

    std::string joinString( const char * lhs, const char * rhs, char delim )
    {
        return joinString( std::string( lhs ? lhs : "" ), std::string( rhs ? rhs : "" ), delim );
    }

    std::string joinString( const char * lhs, const char * rhs, const char * delim )
    {
        return joinString( std::string( lhs ? lhs : "" ), std::string( rhs ? rhs : "" ), delim );
    }

    std::string joinString( const std::string & lhs, const std::string & rhs, const char * delim )
    {
        if ( rhs.empty() )
            return lhs;
        if ( lhs.empty() )
            return rhs;
        if ( !delim )
            return lhs + rhs;

        std::string retVal = lhs;

        size_t delimLen = strlen( delim );
        bool lhsNeedsDelim = ( retVal.length() < delimLen );
        if ( !lhsNeedsDelim )
        {
            std::string::reverse_iterator ii = retVal.rbegin() + ( strlen( delim ) - 1 );
            const char * lshD = &*( ii );
            lhsNeedsDelim = strcmp( lshD, delim ) != 0;
        }
        bool rhsNeedsDelim = strncmp( rhs.c_str(), delim, strlen( delim ) ) != 0;
        if ( lhsNeedsDelim && rhsNeedsDelim )
            retVal += delim + rhs;
        else if ( !lhsNeedsDelim && !rhsNeedsDelim )
        {
            std::string newRHS = rhs;
            newRHS.erase( newRHS.begin(), newRHS.begin() + delimLen );
            retVal += newRHS;
        }
        else
            retVal += rhs;
        return retVal;
    }

    std::string joinString( const std::string & lhs, const std::string & rhs, char delim )
    {
        char d[ 2 ];
        d[ 0 ] = delim;
        d[ 1 ] = 0;
        return joinString( lhs, rhs, d );
    }

    std::string joinString( const std::string & lhs, const std::string & rhs, const std::string & delim )
    {
        return joinString( lhs, rhs, delim.c_str() );
    }

    template < typename T, typename S >
    std::string joinString( const T & list, const S & delim, bool condenseBlanks )
    {
        std::string retVal;
        for ( typename T::const_iterator ii = list.begin(); ii != list.end(); ++ii )
        {
            if ( ( !condenseBlanks || !( *ii ).empty() ) && !retVal.empty() )
                retVal += delim;
            retVal += *ii;
        }
        return retVal;
    }

    std::string joinString( const std::pair< std::string, std::string > & pair, const std::string & delim, bool condenseBlanks )
    {
        std::list< std::string > list = { pair.first, pair.second };
        return joinString< std::list< std::string >, std::string >( list, delim, condenseBlanks );
    }

    std::string joinString( const std::list< std::string > & list, const std::string & delim, bool condenseBlanks )
    {
        return joinString< std::list< std::string >, std::string >( list, delim, condenseBlanks );
    }

    std::string joinString( const std::list< std::string > & list, char delim, bool condenseBlanks )
    {
        return joinString< std::list< std::string >, char >( list, delim, condenseBlanks );
    }

    std::string joinString( const std::vector< std::string > & list, const std::string & delim, bool condenseBlanks )
    {
        return joinString< std::vector< std::string >, std::string >( list, delim, condenseBlanks );
    }

    std::string joinString( const std::vector< std::string > & list, char delim, bool condenseBlanks )
    {
        return joinString< std::vector< std::string >, char >( list, delim, condenseBlanks );
    }

    std::string joinString( const std::set< std::string > & list, const std::string & delim, bool condenseBlanks )
    {
        return joinString< std::set< std::string >, std::string >( list, delim, condenseBlanks );
    }

    std::string joinString( const std::set< std::string > & list, char delim, bool condenseBlanks )
    {
        return joinString< std::set< std::string >, char >( list, delim, condenseBlanks );
    }

    std::string joinString( const std::set< std::string, noCaseStringCmp > & list, const std::string & delim, bool condenseBlanks )
    {
        return joinString< std::set< std::string, noCaseStringCmp >, std::string >( list, delim, condenseBlanks );
    }

    std::string joinString( const std::set< std::string, noCaseStringCmp > & list, char delim, bool condenseBlanks )
    {
        return joinString< std::set< std::string, noCaseStringCmp >, char >( list, delim, condenseBlanks );
    }

    std::list< std::string > splitString( const std::string & string, const std::string & oneOfDelim, bool skipEmpty, bool keepQuoted, bool stripQuotes )
    {
        std::list< std::string > retVal;
        if ( string.empty() )
            return retVal;
        std::string delim = oneOfDelim;
        if ( keepQuoted )
            delim += "\"\'";
        std::string::size_type prevPos = 0;
        auto pos = string.find_first_of( delim );
        while ( pos != std::string::npos )
        {
            if ( keepQuoted && ( ( string[ pos ] == '"' ) || ( string[ pos ] == '\'' )  ) )
            {
                std::string prefix;
                if ( pos != prevPos )
                    prefix = string.substr( prevPos, pos - prevPos );

                prevPos = pos;
                pos = string.find_first_of( "'\"", prevPos + 1 );
                if ( pos == std::string::npos )
                {
                    std::string curr;
                    if ( prefix.empty() ) // it started with a quote
                    {
                        if ( stripQuotes )
                            curr = string.substr( prevPos + 1 );
                        else
                            curr = string.substr( prevPos );
                    }
                    else
                    {
                        // quote was embedded....
                        pos = string.find_first_of( delim, prevPos + 1 );
                        if ( pos == std::string::npos )
                            curr = prefix + string.substr( prevPos );
                        else
                        {
                            curr = prefix + string.substr( prevPos, pos - prevPos );
                            if ( stripQuotes )
                                curr = NStringUtils::stripQuotes( curr );
                            retVal.push_back( curr );
                            prevPos = pos + 1;
                            pos = string.find_first_of( delim, prevPos );
                            continue;
                        }
                    }


                    if ( stripQuotes )
                        curr = NStringUtils::stripQuotes( curr );
                    retVal.push_back( curr );
                    prevPos = std::string::npos;
                    break;
                }
                else
                {
                    auto curr = string.substr( prevPos, pos - prevPos + 1 );
                    if ( stripQuotes )
                        curr = NStringUtils::stripQuotes( curr );
                    retVal.push_back( prefix + curr );
                    prevPos = pos + 2;
                    pos = string.find_first_of( delim, prevPos );
                    continue;
                }
            }

            std::string curr = string.substr( prevPos, pos - prevPos );
            if ( !skipEmpty || !curr.empty() )
            {
                if ( stripQuotes )
                    curr = NStringUtils::stripQuotes( curr );
                retVal.push_back( curr );
            }
            prevPos = pos + 1;
            pos = string.find_first_of( delim, prevPos );
        }
        if ( prevPos < string.length() || ( oneOfDelim.find( *string.rbegin() ) != std::string::npos ) )
        {
            std::string curr = string.substr( prevPos );
            if ( !skipEmpty || !curr.empty() )
            {
                if ( stripQuotes )
                    curr = NStringUtils::stripQuotes( curr );
                retVal.push_back( curr );
            }
        }

        return retVal;
    }

    std::list< std::string > splitStringRegEx( const std::string & string, const std::string & pattern, bool nocase, bool skipEmpty )
    {
        QRegularExpression regExp( QString::fromStdString( pattern ) );
        if ( nocase )
            regExp.setPatternOptions( QRegularExpression::CaseInsensitiveOption );

        Q_ASSERT( regExp.isValid() );
        if ( !regExp.isValid() )
        {
            QString error = regExp.errorString();
            int offset = regExp.patternErrorOffset();
            (void)error;
            (void)offset;
            return{};
        }

        QStringList tmp = QString::fromStdString( string ).split( regExp, skipEmpty ? TSkipEmptyParts : TKeepEmptyParts );
        std::list< std::string > retVal;
        for ( auto & ii : tmp )
        {
            retVal.push_back( ii.toStdString() );
        }
        return retVal;
    }


    std::list< std::string > splitString( const std::string & string, char delimChar, bool skipEmpty, bool keepQuoted, bool stripQuotes )
    {
        if ( string.empty() )
            return std::list< std::string >();

        std::string delim( 1, delimChar );

        return splitString( string, delim, skipEmpty, keepQuoted, stripQuotes );
    }

    std::string toupper( std::string retVal )
    {
        std::transform( retVal.begin(), retVal.end(), retVal.begin(), ::toupper );
        return retVal;
    }

    char *
        convert_to_lower_case( char *string )
    {
        char *s;
        s = string;
        while ( !NStringUtils::isWhiteSpace( s ) &&
                ( *s != '\0' ) &&
                ( *s != '\n' ) )
        {
            if ( isalpha( *s ) && isupper( *s ) )
                *s = *s + 'a' - 'A'; //convert to lower
            s++;
        }
        return string;
    }

    std::string tolower( std::string retVal )
    {
        std::transform( retVal.begin(), retVal.end(), retVal.begin(), ::tolower );
        return retVal;
    }


    int strNCaseCmp( const char* s1, const char* s2, size_t n )
    {
#ifdef _WINDOWS
        return( _strnicmp( s1, s2, n ) );
#else
        return( strncasecmp( s1, s2, n ) );
#endif
    }

    int strNCaseCmp( const std::string & s1, const char* s2, size_t n )
    {
        return strNCaseCmp( s1.c_str(), s2, n );
    }

    int strNCaseCmp( const char* s1, const std::string & s2, size_t n )
    {
        return strNCaseCmp( s1, s2.c_str(), n );
    }

    int strNCaseCmp( const std::string & s1, const std::string & s2, size_t n )
    {
        return strNCaseCmp( s1.c_str(), s2.c_str(), n );
    }

    int strCaseCmp( const char* s1, const char* s2 )
    {
#ifdef _WINDOWS
        return( _stricmp( s1, s2 ) );
#else
        return( strcasecmp( s1, s2 ) );
#endif
    }

    int strCaseCmp( const std::string & s1, const char* s2 )
    {
        return strCaseCmp( s1.c_str(), s2 );
    }

    int strCaseCmp( const char* s1, const std::string & s2 )
    {
        return strCaseCmp( s1, s2.c_str() );
    }

    int strCaseCmp( const std::string & s1, const std::string & s2 )
    {
        return strCaseCmp( s1.c_str(), s2.c_str() );
    }

    std::string::size_type strCaseFind( const std::string & s1, const std::string & substr )
    {
        auto pos = std::search( s1.begin(), s1.end(), substr.begin(), substr.end(),
                                []( char ch1, char ch2 ) { return std::toupper( ch1, std::locale() ) == std::toupper( ch2, std::locale() ); }
        );
        if ( pos == s1.end() )
            return std::string::npos;
        return pos - s1.begin();
    }

    bool strCaseSuffix( const std::string & s1, const std::string & substr )
    {
        if ( s1.length() < substr.length() )
        {
            return false;
        }
        std::string::size_type retVal = s1.length() - substr.length();
        return strCaseCmp( s1.substr( retVal ), substr ) == 0;
    }

    bool stringCompare( const std::string & s1, const std::string & s2, bool caseInsentive )
    {
        if ( !caseInsentive )
            return s1 == s2;
        return ( s1.size() == s2.size() ) && std::equal( s1.begin(), s1.end(), s2.begin(), []( std::string::value_type c1, std::string::value_type c2 ) { return ::toupper( c1 ) == ::toupper( c2 ); } );
    }

    bool strEqual( const char* s1, const char* s2, bool caseInsensitive )
    {
        if ( caseInsensitive )
            return strCaseCmp( s1, s2 ) == 0;
        else
            return strcmp( s1, s2 ) == 0;
    }

    bool strEqual( const std::string & s1, const char* s2, bool caseInsensitive )
    {
        return strEqual( s1.c_str(), s2, caseInsensitive );
    }

    bool strEqual( const char* s1, const std::string & s2, bool caseInsensitive )
    {
        return strEqual( s1, s2.c_str(), caseInsensitive );
    }

    bool strEqual( const std::string & s1, const std::string & s2, bool caseInsensitive )
    {
        return strEqual( s1.c_str(), s2.c_str(), caseInsensitive );
    }

    bool strNEqual( const char* s1, const char* s2, size_t len, bool caseInsensitive )
    {
        if ( caseInsensitive )
            return strNCaseCmp( s1, s2, len ) == 0;
        else
            return strncmp( s1, s2, len ) == 0;
    }

    bool strNEqual( const std::string & s1, const char* s2, size_t len, bool caseInsensitive )
    {
        return strNEqual( s1.c_str(), s2, len, caseInsensitive );
    }

    bool strNEqual( const char* s1, const std::string & s2, size_t len, bool caseInsensitive )
    {
        return strNEqual( s1, s2.c_str(), len, caseInsensitive );
    }

    bool strNEqual( const std::string & s1, const std::string & s2, size_t len, bool caseInsensitive )
    {
        return strNEqual( s1.c_str(), s2.c_str(), len, caseInsensitive );
    }

    std::string writeEscaped( const std::string & s, bool escapeWhitespace )
    {
        std::string escaped;
        escaped.reserve( s.size() );
        for ( size_t ii = 0; ii < s.size(); ++ii )
        {
            char c = s.at( ii );
            if ( c == '<' )
                escaped += "&lt;";
            else if ( c == '>' )
                escaped.append( "&gt;" );
            else if ( c == '&' )
                escaped.append( "&amp;" );
            else if ( c == '\"' )
                escaped.append( "&quot;" );
            else if ( c == '\\' ) 
                escaped.append( "\\\\" );
            else if ( escapeWhitespace && ::isspace( c ) )
            {
                if ( c == '\n' )
                    escaped.append( "&#10;" );
                else if ( c == '\r' )
                    escaped.append( "&#13;" );
                else if ( c == '\t' )
                    escaped.append( "&#9;" );
                else if ( c == ' ' )
                    escaped.append( "%20" );
                else
                    escaped += c;
            }
            else
            {
                escaped += c;
            }
        }
        return escaped;
    }
    std::string writeQuotedStringForXml(const std::string & name)
    {
        auto retVal = NStringUtils::writeEscaped(name);
        if (!retVal.empty() && retVal.back() == '\\')
            retVal += " ";
        retVal = '\"' + retVal + '\"';
        return retVal;
    }
    bool extractEnvVariable( const std::string & inStr, std::string & pre, std::string & var, std::string & post )
    {
        std::string::size_type pos1 = inStr.find( '%' );
        std::string::size_type pos2 = std::string::npos;
        if ( pos1 != std::string::npos ) // has a % in it
        {
            pos2 = inStr.find( '%', pos1 + 1 );
            if ( pos2 == std::string::npos )
            {
                var = inStr;
                return true; // not an 
            }
            pre = inStr.substr( 0, pos1 );
            var = inStr.substr( pos1 + 1, pos2 - pos1 - 1 );
            post = inStr.substr( pos2 + 1 );
            return true;
        }
        pos1 = inStr.find( '$' );
        if ( pos1 == std::string::npos )
        {
            var = inStr;
            return true;
        }
        pre = inStr.substr( 0, pos1 );

        char otherChar = 0;
        if ( *( inStr.begin() + pos1 + 1 ) == '{' )
        {
            otherChar = '}';
        }
        else if ( *( inStr.begin() + pos1 + 1 ) == '(' )
        {
            otherChar = ')';
        }
        else
        {
            // is $VAR
            pos2 = inStr.find_first_of( " \t/\\$", pos1 + 1 );
            if ( pos2 == std::string::npos )
            {
                var = inStr.substr( pos1 + 1 );
                post = std::string();
                return true;
            }
            var = inStr.substr( pos1 + 1, pos2 - pos1 - 1 );
            post = inStr.substr( pos2 );
            return true;
        }

        // is $(VAR) or ${VAR}
        pos2 = inStr.find( otherChar, pos1 );
        if ( pos2 == std::string::npos )// malformed return false
            return false;

        var = inStr.substr( pos1 + 2, pos2 - pos1 - 2 );
        post = inStr.substr( pos2 + 1 );

        return true;
    }

    std::string expandEnvVariable( const std::string & string, std::string * msg, bool * aOK )
    {
        if ( aOK )
            *aOK = false;
        if ( string.empty() )
        {
            if ( aOK )
                *aOK = true;
            return string;
        }

        std::string pre;
        std::string envVar;
        std::string post;
        if ( !extractEnvVariable( string, pre, envVar, post ) )
        {
            if ( msg )
                *msg = "Malformed environmental variable";
            return string;
        }

        if ( envVar == string )
        {
            if ( aOK )
                *aOK = true;
            return string;
        }

        auto tmp = qgetenv( envVar.c_str() );
        if ( tmp == nullptr )
        {
            envVar = toupper( envVar );
            tmp = qgetenv( envVar.c_str() );
        }

        if ( aOK )
            *aOK = ( tmp != nullptr );
        std::string retVal;
        if ( tmp == nullptr )
        {
            if ( msg )
                *msg = "Environmental variable '" + envVar + "' not found";
            retVal = pre + "" + post;
            if ( aOK )
                *aOK = false;
            return retVal;
        }
        else
            retVal = pre + tmp.toStdString() + post;
        if ( retVal.find_first_of( "%$" ) != std::string::npos )
        {
            return expandEnvVariable( retVal, msg, aOK );
        }
        if ( aOK )
            *aOK = true;
        return retVal;
    }






    bool  matchRegExpr( const char* s1, const char *s2 )
    {
        const char *last_star = 0;
        while ( *s1 && *s2 )
            if ( *s1 == *s2 || *s2 == '?' )
            {
                s1++;
                s2++;
            }
            else if ( *s2 == '*' )
            {
                if ( !s2[ 1 ] ) return true;
                while ( *s1 && *s1 != s2[ 1 ] )
                    ++s1;
                last_star = s2;
                ++s2;
            }
            else if ( last_star )
            {
                s2 = last_star; // revert back to the last '*' in s2
                last_star = 0;
            }
            else break;

            if ( !*s1 )
            {
                return ( !*s2 || !strcmp( s2, "*" ) || !strcmp( s2, "?" ) ) ? true : false;
            }
            else return ( !strcmp( s2, "*" ) || ( strlen( s1 ) == 1 && !strcmp( s2, "?" ) ) ) ? true : false;
    }




    char * get_identifier_from_string( const char *string, char * id )
    {
        while ( NStringUtils::isWhiteSpace( string ) )
            string++;

        if ( *string == '"' )
        {
            *id++ = *string++;
            while ( *string != '\0' )
            {
                *id++ = *string;
                if ( *string++ == '"' )
                {
                    break;
                }
            }
        }
        else
        {
            //Allow a semi-colon terminator
            while ( !NStringUtils::isWhiteSpace( string ) && ( *string != '\0' ) &&
                    ( *string != '\n' ) && ( *string != ':' ) && ( *string != ';' ) )
            {
                *id++ = *string++;
            }
        }
        *id = '\0';

        if ( *string == '\0' )
        {
        }
        else if ( *string == ':' )
        {
            string++;
            while ( NStringUtils::isWhiteSpace( string ) )
                string++;
        }

        return (char *)string;
    }

    std::string get_identifier_from_string_std( const std::string & string, std::string & id )
    {
        id.clear();

        std::string retVal = string;

        size_t pos = retVal.find_first_not_of( " \t" );
        if ( pos != std::string::npos )
            retVal = retVal.substr( pos );

        if ( retVal.empty() )
            return retVal;

        if ( retVal[ 0 ] == '"' )
        {
            pos = retVal.find( '"', 1 );
            if ( pos != std::string::npos )
            {
                id = retVal.substr( 0, pos + 1 );
                retVal = retVal.substr( pos + 1 );
            }
            else
            {
                id = retVal;
                retVal.clear();
            }
        }
        else
        {
            pos = retVal.find_first_of( " \t\n:;" );
            if ( pos != std::string::npos )
            {
                id = retVal.substr( 0, pos );
                retVal = retVal.substr( pos );
            }
            else
            {
                id = retVal;
                retVal.clear();
            }
        }

        if ( !retVal.empty() && retVal[ 0 ] == ':' )
        {
            retVal = retVal.substr( 1 );
            size_t pos = retVal.find_first_not_of( " \t" );
            if ( pos != std::string::npos )
                retVal = retVal.substr( pos );
            else
                retVal.clear();
        }

        return retVal;
    }

    std::string strip_terminal( const std::string & token, const std::string & term )
    {
        if ( term.size() > token.size() )
        {
            return token;
        }

        size_t idx = token.rfind( term );
        if ( idx == std::string::npos )
            return token;
        return token.substr( 0, idx );
    }


    bool matchKeyWord( const std::string & line, const std::string & key )
    {
        return ( strNCaseCmp( line, key, key.length() ) == 0 ) && line.length() >= key.length();
    }


    std::string binToHex( const std::string & string )
    {
        if ( string.empty() )
            return "";
        std::string hexStr;
        hexStr.reserve( string.length() / 4 + ( string.length() % 4 > 0 ) );
        int bitsSeen = 0;
        int totalBits =
            std::count( string.begin(), string.end(), '0' ) + std::count( string.begin(), string.end(), '1' )
            + std::count( string.begin(), string.end(), 'Z' ) + std::count( string.begin(), string.end(), 'z' )
            + std::count( string.begin(), string.end(), 'X' ) + std::count( string.begin(), string.end(), 'x' )
            + std::count( string.begin(), string.end(), '?' );
        ;

        unsigned hexChar = 0;
        char xchar = 0;
        for ( unsigned idx = 0; idx < string.length(); ++idx )
        {
            switch ( string[ idx ] )
            {
                case '1':
                    hexChar <<= 1;
                    hexChar++;
                    bitsSeen++;
                    break;
                case '0':
                    hexChar <<= 1;
                    bitsSeen++;
                    break;
                case '?':
                case 'X':
                case 'x':
                case 'Z':
                case 'z':
                    xchar = string[ idx ];
                    bitsSeen++;
                    break;
                case ' ':
                    continue;
                    break;
                default:
                    return "";
                    break;
            }
            if ( ( totalBits - bitsSeen ) % 4 == 0 )
            {
                if ( xchar )
                    hexStr.push_back( xchar );
                else
                    hexStr.push_back( "0123456789abcdef"[ hexChar ] );
                xchar = 0;
                hexChar = 0;
            }
        }
        return hexStr;
    }

    bool getOnOffValue( const char * value, bool & aOK, bool defaultVal )
    {
        aOK = true;
        if ( !value || !*value )
            return defaultVal;
        size_t len = strlen( value );

        if ( value[ 0 ] == '"' && value[ strlen( value ) - 1 ] == '"' )
        {
            len -= 2;
            value++;
        }

        if ( !strNCaseCmp( "on", value, len ) || !strNCaseCmp( value, "yes", len ) || !strNCaseCmp( value, "true", len ) || !strNCaseCmp( value, "1", len ) )
            return true;
        if ( !strNCaseCmp( value, "no", len ) || !strNCaseCmp( value, "off", len ) || !strNCaseCmp( value, "false", len ) || !strNCaseCmp( value, "0", len ) )
            return false;
        aOK = false;
        return defaultVal;
    }

    bool getOnOffValue( const std::string & str, bool & aOK, bool defaultVal )
    {
        return getOnOffValue( str.c_str(), aOK, defaultVal );
    }

    long getSerialNum(const std::string &str, const std::string &prefix)
    {
        if (str.find(prefix) != 0)
            return -1;
        long val = 1;
        bool aOk = fromString(val, str.substr(prefix.length()));
        if (aOk)
            return val;
        return 1;
    }

    void padBinary(std::string &data, size_t maxSize, bool isSigned)
    {
        if ( maxSize != static_cast<size_t>( -1 ) )
        {
            if ( data.length() < maxSize )
            {
                char fillChar = isSigned ? ( ( data.length() > 0 ) ? data[ 0 ] : '0' ) : '0';
                std::string pad = std::string( maxSize - data.length(), fillChar );
                data = pad + data;
            }
            if ( data.length() > maxSize )
                data.erase( 0, data.length() - maxSize );
        }
    }

    std::string presentationFormat( const std::string & format )
    {
        std::string retVal = format;
        replaceAll( retVal, "\n", "" );
        size_t pos = retVal.find( '%' );
        while ( pos != std::string::npos )
        {
            std::string replaceString;
            std::string::iterator endPos = retVal.begin() + pos + 1;
            while ( endPos != retVal.end() && ( ::isdigit( *endPos ) || ( *endPos == '.' ) ) )
                endPos++;
            if ( endPos != retVal.end() )
            {
                if ( *endPos == 'l' )
                    endPos++;

                switch ( *endPos )
                {
                    case 's':
                        replaceString = "<string>";
                        break;

                    case 'd':
                    case 'f':
                    case 'g':
                    case 'e':
                        replaceString = "<number>";
                        break;
                    case 'x':
                    case 'X':
                        replaceString = "<hex number>";
                        break;
                    case 'o':
                        replaceString = "<octal number>";
                        break;
                    case 'c':
                        replaceString = "<character>";
                        break;
                }
            }
            if ( !replaceString.empty() )
                retVal.replace( retVal.begin() + pos, endPos + 1, replaceString );

            pos = retVal.find( '%', pos + ( replaceString.empty() ? 1 : replaceString.length() ) );
        }
        return retVal;
    }


    /**********************************************************************************************
      Function: binaryAttrToASCII()
      Description:
      Convert the binary string into ascii, with attribute prefix on the string.
      For example: 40b'0100011001000001010011000101001101000101 -> FALSE

      Revision:
      ***********************************************************************************************/
    std::string binaryAttrToASCII( const std::string & bString )
    {
        // Check the format
        size_t pos = bString.find( "'b" );
        if ( pos == string::npos )
            return bString; // Can't convert.

        // Check the provided-size prefix.
        string tmpString = bString.substr( 0, pos );
        size_t size;
        ::fromString( size, tmpString );
        if ( size % 8 != 0 )
            return bString;
        tmpString = bString.substr( pos + 2, bString.size() );
        if ( size != tmpString.size() )
            return bString;

        bool aOK;
        std::string retVal = binaryToASCII( tmpString, aOK );
        if ( !aOK )
            return bString;
        return retVal;
    }

    /**********************************************************************************************
      Function: binaryToASCII()
      Description:
      Convert binary string to ASCII, core function without attribute format checking.
      - For attribute or parameter, use binaryAttrToASCII()

      Revision:
      ***********************************************************************************************/
    std::string binaryToASCII( const std::string & bString, bool & aOK )
    {
        aOK = false;
        int hexIdx = 0; // 4-bits for hex data
        int hexCnt = 0; // 2-hexes for byte
        unsigned int hexNum = 0; // Keep two hex numbers each time
        const char *c = bString.c_str();
        string aString;
        for ( size_t idx = 0; idx < bString.size(); ++idx )
        {
            // Build the hex number
            switch ( c[ idx ] )
            {
                case '0':
                    // No need to do anything
                    break;
                case '1':
                    if ( hexIdx == 0 )
                        hexNum += 0x8; // Reverse significant
                    else if ( hexIdx == 1 )
                        hexNum += 0x4;
                    else if ( hexIdx == 2 )
                        hexNum += 0x2;
                    else if ( hexIdx == 3 )
                        hexNum += 0x1;
                    else
                        Q_ASSERT( 0 );
                    break;
                default:
                    Q_ASSERT( 0 );
                    break;
            }

            // Manage hex number
            if ( ++hexIdx == 4 )
            {
                hexIdx = 0; // Reset
                if ( hexCnt++ == 0 )
                    hexNum <<= 4; // Shift to upper hex
                else // Two hexes number
                {
                    hexCnt = 0; // Reset

                    // Get the character from hexNum
                    unsigned char uc = hexNum;
                    //printf("#Debug: char: %s\n", &uc);
                    aString += uc;
                    hexNum = 0; // Reset
                }
            }
        }

        //cout << "#Debug: aString: " << aString << endl;
        for ( std::string::const_iterator ii = aString.begin(); ii != aString.end(); ++ii )
        {
            int curr = *ii;
            if ( ( curr <= 0 ) || ( curr >= 255 ) || !isprint( *ii ) )
            {
                return bString;
            }
        }
        aOK = true;
        return aString;
    }
    
    bool isHSC( char ch, const char * hsc )
    {
        for( auto curr = hsc; *curr; ++curr )
        {
            if ( *curr == ch )
                return true;
        }
        return false;
    }

    std::list< std::pair< std::string, bool > > splitByEscape( const std::string & string, bool regExp, const char * hsc )
    {
        std::list< std::pair< std::string, bool > > tmp;
        std::string::size_type prevPos = 0;
        auto separator = regExp ? "\\\\" : "\\";
        std::string::size_type pos = string.find( separator );
        while ( pos != std::string::npos )
        {
            if ( pos != prevPos )
            {
                auto curr = string.substr( prevPos, pos - prevPos );
                if ( !curr.empty() )
                    tmp.push_back( std::make_pair( curr, false ) );
            }
            /* find the end of escaped...
             handle \\foo.\barfoo\
             leading escape is the count...
             */
            auto escStart = pos;
            int escCount = 0;
            while( ( pos < string.length() ) && ( string[ pos ] == '\\' ) )
            {
                escCount++;
                pos++;
            }
            while( ( pos < string.length() ) && ( escCount > 0 ) )
            {
                if ( string[ pos ] == '\\' )
                {
                    escCount--;
                    if ( escCount == 0 )
                        break;
                }
                pos++;
            }

            std::string curr;
            if ( escCount == 0 )
                curr = string.substr( escStart, pos - escStart + 1 );
            else
                curr = string.substr( escStart );
            if ( !curr.empty() )
                tmp.push_back( std::make_pair( curr, true ) );
            prevPos = pos+1;
            pos = string.find( separator, prevPos );
        }

        if ( prevPos < string.length() )
        {
            std::string curr = string.substr( prevPos );
            if ( !curr.empty() )
            {
                tmp.push_back( std::make_pair( curr, false ) );
            }
        }

        bool isFirst = true;
        for( auto ii = tmp.begin(); ii != tmp.end(); ++ii )
        {
            if ( (*ii).second )
            {
                isFirst = false;
                continue;
            }

            if ( ( (*ii).first.length() == 1 ) && isHSC( (*ii).first[ 0 ], hsc ) )
            {
                auto tmpIter = ii;
                tmpIter--;
                tmp.erase( ii );
                ii = tmpIter;
                continue;
            }

            if ( !isFirst )
            {
                if ( isHSC( *(*ii).first.begin(), hsc ) )
                    (*ii).first.erase( (*ii).first.begin() );
            }
            isFirst = false;
            if ( isHSC( *(*ii).first.rbegin(), hsc ) )
                (*ii).first.erase( (*ii).first.begin() + (*ii).first.length() - 1 );
        }
        return tmp;
    }

    std::list< std::string > splitSDCPattern( const std::string & pattern, bool regExp, const char * hsc, bool & aOK, std::string * msg )
    {
        std::list< std::string > retVal;
        // search for escape char;

        auto pos1 = pattern.find( regExp ? "\\\\" : "\\" );
        if ( pos1 != std::string::npos )
        {
            auto tmp = splitByEscape( pattern, regExp, hsc );

            for( auto && ii : tmp )
            {
                if ( !ii.second )
                {
                    auto curr = splitSDCPattern( ii.first, regExp, hsc, aOK, msg );
                    if ( !aOK )
                        return{};
                    retVal.insert( retVal.end(), curr.begin(), curr.end() );
                }
                else
                    retVal.push_back( ii.first );
            }
            return retVal;
        }
        if ( regExp )
        {
            QString origHSC = QString( "%1" ).arg( hsc );
            QString realHSC = QRegularExpression::escape( QString( "%1" ).arg( hsc ) );
            if ( realHSC != origHSC )
            {
                realHSC = QRegularExpression::escape( QString( "\\%1" ).arg( hsc ) );;
            }
            retVal = splitStringRegEx( pattern, realHSC.toStdString() );

            for ( auto ii : retVal )
            {
                QRegularExpression regExp( QString::fromStdString( ii ) );
                if ( !regExp.isValid() )
                {
                    if ( msg )
                    {
                        *msg = QString( "Invalid sub-regular expression '%1' post splitting on the hierarchy separator: '%2' - %3(%4)" ).arg( QString::fromStdString( ii ) ).arg( hsc ).arg( regExp.errorString() ).arg( regExp.patternErrorOffset() ).toStdString();
                        aOK = false;
                        return{};
                    }
                }
            }
        }
        else
        {
            retVal = splitString( pattern, hsc, false, true, false );
        }

        while ( !retVal.empty() && retVal.front().empty() )
        {
            retVal.pop_front();
        }

        aOK = true;
        return retVal;
    }

    QStringList splitSDCPattern( const QString & objName, bool regexp, const char * hsc, bool & aOK, QString * msg )
    {
        std::string lclMsg;
        auto list = splitSDCPattern( objName.toStdString(), regexp, hsc, aOK, &lclMsg );

        if ( msg )
            *msg = QString::fromStdString( lclMsg );

        QStringList retVal;
        for ( auto && ii : list )
        {
            retVal.push_back( QString::fromStdString( ii ) );
        }

        return retVal;
    }

    std::list< std::string > splitSDCPattern( const std::string & objName, bool regexp, char singleHSC, bool & aOK, std::string * msg )
    {
        char hsc[ 2 ];
        hsc[ 0 ] = singleHSC;
        hsc[ 1 ] = 0;
        return splitSDCPattern( objName, regexp, hsc, aOK, msg );
    }

    QStringList splitSDCPattern( const QString & objName, bool regexp, char singleHSC, bool & aOK, QString * msg )
    {
        char hsc[ 2 ];
        hsc[ 0 ] = singleHSC;
        hsc[ 1 ] = 0;
        return splitSDCPattern( objName, regexp, hsc, aOK, msg );
    }

    char IsSwitch( const std::string & str )
    {
        return IsSwitch( str.c_str() );
    }

    char IsSwitch( const char * str )
    {
        if ( !str )
            return 0;
        if ( !*str )
            return 0;
        if ( !str[ 1 ] )
            return 0;
        char switchChar = str[ 0 ];
        char nextChar = str[ 1 ];
        if ( switchChar != '-' && switchChar != '+' )
            switchChar = 0;
        else if ( nextChar == '-' || nextChar == '+' )
            return 0;
        else if ( switchChar == '-' )
        {
            int intVal = atoi( str );
            if ( intVal < 0 )
            {
                return 0;
            }
        }
        return switchChar;
    }

    bool isSeparatorEscaped( const std::string & name, char secondSep/*=0 */ )
    {
        bool inEscape = false;
        for ( auto ii = name.begin(); ii != name.end(); ++ii )
        {
            if ( *ii == '\\' )
                inEscape = !inEscape;
            else if ( ( *ii == '.' ) || ( secondSep && ( *ii == secondSep ) ) )
            {
                if ( !inEscape )
                    return false;
            }
        }

        return true;
    }

    std::string separatorEscape( const std::string & name, char secondSep/*=0 */ )
    {
        if ( name.size() > 2 && !isSeparatorEscaped( name, secondSep ) )
        {
            return '\\' + name + '\\';
        }
        return name;
    }

    size_t findLastSeparator( const std::string & str, char separator, size_t offset/*=std::string::npos */ )
    {
        if ( str.empty() )
            return std::string::npos;

        auto ii = ( offset != std::string::npos ) ? offset : ( str.size() - 1 );
        bool inEscape = false;
        for ( ; ( ii != std::string::npos ) && ( ii < str.length() ); ( ( ii == 0 ) ? ( ii = std::string::npos ) : --ii ) )
        {
            if ( str.at( ii ) == '\\' )
                inEscape = !inEscape;
            else if ( !inEscape && ( str.at( ii ) == separator ) )
                break;
        }
        return ii;
    }

    size_t findSeparator( const std::string & str, char separator, size_t offset/*=std::string::npos */ )
    {
        if ( str.empty() )
            return std::string::npos;

        auto ii = ( offset != std::string::npos ) ? offset : 0;
        bool inEscape = false;
        for ( ; ii < str.size(); ++ii )
        {
            if ( str.at( ii ) == '\\' )
                inEscape = !inEscape;
            else if ( !inEscape && ( str.at( ii ) == separator ) )
                break;
        }
        return ( ii == str.size() ) ? std::string::npos : ii;
    }

    QStringList asReport( const QStringList & columnNames, const QStringList & subHeader, const QList< QStringList > & data, bool sortData )
    {
        // first row is the columnNames
        std::vector< int > maxSize;
        maxSize.resize( columnNames.size() );
        for ( int ii = 0; ii < columnNames.size(); ++ii )
            maxSize[ ii ] = columnNames[ ii ].length() + 1;

        Q_ASSERT( subHeader.isEmpty() || ( subHeader.size() == columnNames.size() ) );
        for ( int ii = 0; ii < subHeader.size(); ++ii )
            maxSize[ ii ] = std::max( maxSize[ ii ], subHeader[ ii ].length() + 1 );

        for ( auto && ii : data )
        {
            Q_ASSERT( ii.size() == maxSize.size() );
            for ( int jj = 0; ( jj < static_cast< int >( maxSize.size() ) ) && ( jj < ii.size() ); jj++ )
            {
                maxSize[ jj ] = std::max( maxSize[ jj ], ii[ jj ].length() + 1 );
            }
        }

        QStringList headers;
        QString headerLine;
        QTextStream ts( &headerLine );
        for ( int ii = 0; ii < columnNames.size(); ++ii )
            ts << NStringUtils::PadString( columnNames[ ii ], maxSize[ ii ], NStringUtils::EPadType::eCenter ) << "  ";
        headers << headerLine;
        headerLine.clear();
        for ( int ii = 0; ii < columnNames.size(); ++ii )
            ts << QString( maxSize[ ii ], '-' ) << "  ";
        headers << headerLine;

        if ( !subHeader.isEmpty() )
        {
            QString headerLine;
            QTextStream ts( &headerLine );
            for ( int ii = 0; ( ii < subHeader.size() ) && ( ii < static_cast< int >( maxSize.size() ) ); ++ii )
                ts << NStringUtils::PadString( subHeader[ ii ], maxSize[ ii ], NStringUtils::EPadType::eLeftJustify ) << "  ";
            headers << headerLine;

            headerLine.clear();
            for ( int ii = 0; ii < columnNames.size(); ++ii )
                ts << QString( maxSize[ ii ], '-' ) << "  ";
            headers << headerLine;
        }
        
        QStringList retVal;
        for ( auto && ii : data )
        {
            Q_ASSERT( ii.size() == maxSize.size() );
            QString currLine;
            QTextStream ts( &currLine );
            for ( int jj = 0; ( jj < static_cast<int>( maxSize.size() ) ) && ( jj < ii.size() ); jj++ )
            {
                ts << NStringUtils::PadString( ii[ jj ], maxSize[ jj ], NStringUtils::EPadType::eLeftJustify ) << "  ";
            }
            retVal << currLine;
        }

        if ( sortData )
            retVal.sort();

        retVal = headers + retVal;
        return retVal;
    }

    QString encodeRegEx( QString retVal )
    {
        QRegularExpression regEx( "([\\^\\$\\.\\*\\+\\?\\|\\(\\)\\[\\]\\{\\}\\\\])" );
        retVal.replace( regEx, "\\\\1" );
        return retVal;
    }

    QString encodeRegEx( const char * inString )
    {
        return encodeRegEx( QString( inString ) );
    }

    std::string encodeRegEx( const std::string & inString )
    {
        return encodeRegEx( QString::fromStdString( inString ) ).toStdString();
    }

    bool isNumericString( const std::string &constString, uint64_t & val, unsigned int & numBits )
    {
        val = 0;
        numBits = 0;

        char bit = constString[ 0 ];
        bool allsame = true;
        for ( size_t i = 0; allsame && ( i < constString.size() ); i++ )
        {
            allsame = ( bit == constString[ i ] );
        }

        bool retVal = true;
        if ( allsame && ( ( constString[ 0 ] == '0' ) || ( constString.size() <= 64 && constString[ 0 ] == '1' ) ) )
        {
            numBits = static_cast<unsigned int>( constString.size() );
            retVal = true;
            if ( constString[ 0 ] == '0' )
                val = 0;
            else
            {
                auto isSigned = ( constString.size() == 64 ) && constString[ 0 ] == '1';
                val = NStringUtils::binToDec< uint64_t >( constString, isSigned, &retVal );
            }
        }
        else
        {
            // 0 1 constants ?
            size_t maxBit = 64;
            retVal = true;
            for ( size_t bitNum = 0; retVal && ( bitNum < constString.size() ) && ( bitNum < maxBit ); bitNum++ )
            {
                char bit = constString[ bitNum ];
                retVal = ( bit == '1' ) || ( bit == '0' );
                if ( !retVal )
                {
                    break;
                }
                val = ( val << 1 ) + ( bit == '1' );
                numBits++;
            }
            if ( numBits >= maxBit )
            {
                retVal = false;
            }
            numBits = static_cast<unsigned int>( constString.size() );
        }
        return retVal;
    }

    bool endsWith( const char * str, char ch )
    {
        if ( *str == ch )
        {
            for( const char * nextChar = str; *nextChar; ++nextChar )
            {
                if ( ( *nextChar == ch ) || !*nextChar )
                    return true;
                return false;
            }
        }
        return false;
    }

    bool isValidEncodeChar( char ch )
    {
        if ( ( ch >= 'A' && ch <= 'Z' )
             || ( ch >= 'a' && ch <= 'z' )
             || ( ch >= '0' && ch <= '9' )
             || ( ( ch == '+' ) || ( ch == '/' ) )
             )
             return true;
        return false;
    }

    bool validateBase64String( const char * str, size_t len )
    {
        if ( !str )
            return true;

        if ( len == std::string::npos )
            len = std::strlen( str );

        if ( len >= 2 )
        {
            if ( *str == '-' && ( *( str + 1 ) == '-' ) )
                return true;

            if ( *str == '/' && ( *( str + 1 ) == '/' ) )
                return true;
        }

        for ( const char * curr = str; *curr; ++curr )
        {
            if ( endsWith( curr, '=' ) )
                continue;
            if ( ( *curr == '\r' ) || ( *curr == '\n' ) )
                continue;

            if ( !isValidEncodeChar( *curr ) )
            {
                return false;
            }
        }
        return true;
    }

    bool validateBase64String( const std::string & str )
    {
        return validateBase64String( str.c_str(), str.length() );
    }

    bool validateUUEncodeString( const char * str, size_t len )
    {
        if ( !str )
            return true;

        if ( len == std::string::npos )
            len = std::strlen( str );

        if ( len >= 2 )
        {
            if ( *str == '-' && ( *( str + 1 ) == '-' ) )
                return true;

            if ( *str == '/' && ( *( str + 1 ) == '/' ) )
                return true;
        }

        if ( len > 76 )
            return false;

        for ( const char * curr = str; *curr; ++curr )
        {
            if ( endsWith( curr, '=' ) )
                continue;
            if ( ( *curr == '\r' ) || ( *curr == '\n' ) )
                continue;

            if ( !isValidEncodeChar( *curr ) )
            {
                return false;
            }
        }
        return true;
    }

    bool validateUUEncodeString( const std::string & str )
    {
        return validateBase64String( str.c_str(), str.length() );
    }

    bool validateQuotedPrintableString( const char * str, size_t len )
    {
        if ( !str )
            return true;

        if ( len == std::string::npos )
            len = std::strlen( str );

        if ( len > 76 )
            return false;

        if ( len >= 2 )
        {
            if ( *str == '-' && ( *( str + 1 ) == '-' ) )
                return true;

            if ( *str == '/' && ( *( str + 1 ) == '/' ) )
                return true;
        }

        for ( const char * curr = str; *curr; ++curr )
        {
            if ( ( *curr == '=' ) && *(curr+1) && *(curr+2) )
            {
                if ( ( *(curr+1) >= 'A' ) && ( *(curr+1) <= 'F' ) )
                    continue;
                if ( ( *( curr + 1 ) >= '0' ) && ( *( curr + 1 ) <= '9' ) )
                    continue;

                return false;
            }
            if ( ( *curr == '\r' ) || ( *curr == '\n' ) )
                continue;

            if ( ( *curr >= 33 ) && ( *curr <= 60 ) )
                continue;
            if ( ( *curr >= 62 ) && ( *curr <= 126 ) )
                continue;

            if ( ( *curr == 9 ) && ( *curr == 32 ) )
            {
                if ( !*(curr+1) ) // at the end of the line
                    return false;
            }
        }
        return true;
    }

    bool validateQuotedPrintableString( const std::string & str )
    {
        return validateQuotedPrintableString( str.c_str(), str.length() );
    }

    bool isSpecialRegExChar( char ch, bool includeDotSlash )
    {
        if ( ( ch == '\\' ) || ( ch == '.' ) )
            return includeDotSlash;
        return  ( ch == '[' )
            || ( ch == ']' )
            || ( ch == '^' )
            || ( ch == '$' )
            || ( ch == '|' )
            || ( ch == '?' )
            || ( ch == '*' )
            || ( ch == '+' )
            || ( ch == '(' )
            || ( ch == ')' )
            ;
    }

    bool isSpecialRegExChar( const QChar & ch, bool includeDotSlash )
    {
        return isSpecialRegExChar( ch.toLatin1(), includeDotSlash );
    }
}

