#pragma once

#include <windows.h>
#include <ql/quantlib.hpp>
#include <boost/format.hpp>
#include <boost/scoped_array.hpp>

using namespace QuantLib;

namespace QuantLib {
	// HC Added
	typedef Integer Hour;
	typedef Integer Minute;
	typedef Integer Second;
}

// case insensitive�� �񱳸� ���� transform�� �ϱ� ������ �����ڷ� ���� �ʴ´�.
bool CompareString( std::wstring lhs, std::wstring rhs );

inline void char_to_wchar(wchar_t *dst, char *src)
{
	int n = MultiByteToWideChar(949, 0, src, (int)strlen(src), NULL, NULL);
	MultiByteToWideChar(949, 0, src, (int)strlen(src), dst, n);
	dst[n]=L'\0';
}

inline void wchar_to_char(char *dst, wchar_t *src)
{
	int n = (int)wcslen(src) + 1;
	wcstombs(dst, src, n);
}

inline std::wstring ToWString( const std::string& str )
{
	int n = ::MultiByteToWideChar(949, 0, str.c_str(), -1, NULL, 0 );
	boost::scoped_array<wchar_t> BUFF( new wchar_t[ n + 1 ] );

	::MultiByteToWideChar(949, 0, str.c_str(), -1, BUFF.get(), n );
	BUFF[ n ] = L'\0';

	return std::wstring( BUFF.get() );
}

inline std::wstring ToWString( double val )
{
	std::wostringstream buf;
	buf << boost::wformat( L"%.8f" ) % val;
	return buf.str();
}

inline std::wstring ToWString( int val )
{
	std::wostringstream buf;
	buf << boost::wformat( L"%d" ) % val;
	return buf.str();
}

inline std::wstring ToWString( unsigned int val )
{
	std::wostringstream buf;
	buf << boost::wformat( L"%u" ) % val;
	return buf.str();
}

#ifdef _WIN64
inline std::wstring ToWString( size_t val )
{
	std::wostringstream buf;
	buf << boost::wformat( L"%u" ) % val;
	return buf.str();
}
#endif

inline std::wstring TowString( bool val )
{
	return ( val ) ? L"1" : L"0";
}

inline std::wstring ToWString( const std::wstring& str )
{
	return str;
}

inline std::wstring ToWString( const Date& date )
{
	std::wostringstream buf;
	buf << boost::wformat( L"%04d-%02d-%02d" ) % date.year() % date.month() % date.dayOfMonth();
	return buf.str();
}

template<typename T>
inline std::wstring ToWString( const std::vector<T>& v )
{
	if( v.empty() )
	{
		return std::wstring();
	}
	std::wostringstream buf;
	buf << ::ToWString( v.front() );
	for( size_t i = 1; i < v.size(); i++ )
	{
		buf << L" /\n" << ::ToWString( v[ i ] );
	}

	return buf.str();
}

inline std::string ToString( const std::wstring& wstr )
{
	int iLen = ::WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
	boost::scoped_array<char> BUFF( new char[ iLen + 1 ] );

	::WideCharToMultiByte( CP_ACP, 0, wstr.c_str(), -1, BUFF.get(), iLen, NULL, NULL );

	BUFF[ iLen ]  = '\0';

	return std::string( BUFF.get() );
}

inline std::string ToString( double val )
{
	return ::ToString( ::ToWString( val ) );
}

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

inline Date ConvertToDateWTime( const std::wstring& str )
{
	std::vector<std::wstring> dt;
	boost::algorithm::split( dt, str, boost::is_any_of( L" " ), boost::algorithm::token_compress_on );
	std::vector<std::wstring> ymd;
	std::vector<std::wstring> hms;
	boost::algorithm::split( ymd, dt[ 0 ], boost::is_any_of( L"-" ), boost::algorithm::token_compress_on );
	boost::algorithm::split( hms, dt[ 1 ], boost::is_any_of( L":" ), boost::algorithm::token_compress_on );

	QL_ASSERT( ymd.size() == 3, ::ToString( str ) + "��¥�� �̻��մϴ�" );
	QL_ASSERT( hms.size() == 3, ::ToString( str ) + "�ð��� �̻��մϴ�" );

	BigInteger y = boost::lexical_cast<BigInteger>( ymd[ 0 ] );
	BigInteger m = boost::lexical_cast<BigInteger>( ymd[ 1 ] );
	BigInteger d = boost::lexical_cast<BigInteger>( ymd[ 2 ] );


	BigInteger h = boost::lexical_cast<BigInteger>( hms[ 0 ] );
	BigInteger mi = boost::lexical_cast<BigInteger>( hms[ 1 ] );
	BigInteger s = boost::lexical_cast<BigInteger>( hms[ 2 ] );

	if( y == 1900 )
	{
		y++;
	}

	return Date( Day( d ), Month( m ), Year( y ) ); //, Hour( h ), Minute( mi ), Second( s ) );
}

inline Date ConvertToDate( const std::wstring& str )
{
	std::vector<std::wstring> dt;
	boost::algorithm::split( dt, str, boost::is_any_of( L" " ), boost::algorithm::token_compress_on );
	if( dt.size() == 2 )
	{
		return ConvertToDateWTime( str );
	}

	std::vector<std::wstring> ymd;
	boost::algorithm::split( ymd, dt[ 0 ], boost::is_any_of( L"-" ), boost::algorithm::token_compress_on );

	QL_ASSERT( ymd.size() == 3, ::ToString( str ) + "��¥�� �̻��մϴ�" );

	BigInteger y = boost::lexical_cast<BigInteger>( ymd[ 0 ] );
	BigInteger m = boost::lexical_cast<BigInteger>( ymd[ 1 ] );
	BigInteger d = boost::lexical_cast<BigInteger>( ymd[ 2 ] );

	if( y == 1900 )
	{
		y++;
	}

	return Date( Day( d ), Month( m ), Year( y ) );
}

inline std::wstring ToWStringWTime( const Date& date )
{
	std::wostringstream buf;
	//BigInteger hour = date.second() / 3600;
	//BigInteger minute = date.second() / 60 % 60;
	//BigInteger second = date.second() % 60;
	BigInteger hour = 0;
	BigInteger minute = 0;
	BigInteger second = 0;
	buf << boost::wformat( L"%04d-%02d-%02d %02d:%02d:%02d" ) % date.year() % date.month() % date.dayOfMonth() % hour % minute % second;
	return buf.str();
}

template<typename strType, typename Pred>
inline std::vector<strType> Split( const strType& str, const Pred& pred )
{
	std::vector<strType> splitted;
	boost::algorithm::split( splitted, str, pred, boost::algorithm::token_compress_on );

	return splitted;
}

inline Date ConvertToDateFromBloomberg( const std::wstring& str )
{
	std::vector<std::wstring> mdy;
	boost::algorithm::split( mdy, str, boost::is_any_of( L"/" ), boost::algorithm::token_compress_on );

	QL_ASSERT( mdy.size() == 3, ::ToString( str ) + "��¥�� �̻��մϴ�" );

	BigInteger y = boost::lexical_cast<BigInteger>( mdy[ 2 ] );
	BigInteger m = boost::lexical_cast<BigInteger>( mdy[ 0 ] );
	BigInteger d = boost::lexical_cast<BigInteger>( mdy[ 1 ] );

	return Date( Day( d ), Month( m ), Year( y ) );
}

inline std::wstring ConvertToCurrencyCode( const std::wstring& code )
{
	if( code == L"U180" )
	{
		return L"KRW";
	}
	else if( code == L"SPX" )
	{
		return L"USD";
	}
	else if( code == L"HSCEI" )
	{
		return L"HKD";
	}
	else if( code == L"SX5E" )
	{
		return L"EUR";
	}
	else if( code == L"TWSE" )
	{
		return L"TWD";
	}
	else if( code == L"NKY Index" )
	{
		return L"NKY";
	}

	return L"KRW";
}
