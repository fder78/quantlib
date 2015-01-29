#include "StdAfx.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "CQuery.h"

#include "StringUtil.h"

// MFC 프로젝트에 사용될 경우는 stdafx.h에 CQuery.h를 포함시켜 주고 다음 한줄만
// 있으면 된다.
// #include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
/*
CQuery 클래스:ODBC의 래퍼 클래스. API 수준에서 재사용 가능하도록 작성하였다.
SQL문을 편리하게 실행하고 결과를 쉽게 읽는데 초점을 맞추었으며 SQL 서버와 
액세스에서 완벽하게 테스트되었다.
255자 이하의 정수, 문자열 컬럼에 대해서만 사용 가능하며 그 이상의 길이를 가
지는 필드는 "오른쪽 잘림" 진단 경고가 발생하므로 이 클래스로 읽을 수 없으며
ODBC API 함수를 직접 사용해야 한다.
최대 컬럼 개수는 100개이다. 초과시 사정없이 에러 메시지 박스를 출력하므로 이 
한도를 알아서 넘지 않도록 주의해야 한다. 특히 필드수가 많은 테이블을 select *
로 읽는 것은 삼가하는 것이 좋다.
전진 전용 커서를 사용하므로 이미 읽은 레코드는 쿼리를 다시 실행해야만 읽을 수
있다. 

사용법
1.CQuery 클래스의 객체를 생성한다. 가급적이면 전역으로 선언하는 것이 좋으며
  필요한 수만큼 객체를 만든다. 보통 세 개 정도면 충분하다.
2.적절한 Connect 함수를 호출하여 데이터 소스에 연결한다.
3.Exec 함수로 SQL문을 실행한다. 에러 처리는 Exec 함수 내부에서 처리한다.
4.Fetch 함수로 결과 셋 하나를 가져온다. 여러개의 결과셋이 있는 경우는 while
  루프를 돌면서 차례대로 꺼내올 수 있다.
5.Get* 함수로 컬럼 값을 읽는다.
6.Clear 함수로 명령 핸들을 닫는다.(Select문일 경우만)
7.객체를 파괴한다. 전역 객체인 경우는 일부러 파괴할 필요가 없다.
*/

// 생성자:각종 초기화를 담당한다.
CQuery::CQuery()
{
	AffectCount=-1;
	ret=SQL_SUCCESS;

	// 환경 핸들을 할당하고 버전 속성을 설정한다.
	SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv);
	SQLSetEnvAttr(hEnv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,SQL_IS_INTEGER);
}

// 파괴자:연결 핸들을 해제한다.
CQuery::~CQuery()
{
	Clear();

	if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
	if (hDbc) SQLDisconnect(hDbc);
	if (hDbc) SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
	if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
}

// 연결 핸들을 할당하고 연결한 후 명령핸들까지 같이 할당한다.
// Type=1:ConStr은 MDB 파일의 경로를 가진다. 경로 생략시 현재 디렉토리에서 MDB를 찾는다.
// Type=2:ConStr은 SQL 서버의 연결 정보를 가지는 DSN 파일의 경로를 가진다. 
//        경로는 반드시 완전 경로로 지정해야 한다.
// Type=3:SQLConnect 함수로 DSN에 직접 연결한다.
// 연결 또는 명령 핸들 할당에 실패하면 FALSE를 리턴한다.
BOOL CQuery::Connect(int Type, const char *ConStr, const char *UID, const char *PWD)
{
	SQLCHAR InCon[8192];
	SQLWCHAR InConW[8192];
	SQLWCHAR OutConW[8192];
	SQLWCHAR ConStrW[8192], UIDW[8192], PWDW[8192];
    SQLSMALLINT cbOutCon;

	char_to_wchar(ConStrW, (char*)ConStr);
	char_to_wchar(UIDW, (char*)UID);
	char_to_wchar(PWDW, (char*)PWD);

	int ii=1;
	SQLRETURN Ret;
	SQLINTEGER NativeError;
	SQLWCHAR SqlState[6], Msg[8192];
	SQLSMALLINT MsgLen;
	wchar_t str[256];

	// 연결 타입에 따라 MDB 또는 SQL 서버, 또는 DSN에 연결한다.
	SQLAllocHandle(SQL_HANDLE_DBC,hEnv,&hDbc);
	switch (Type) {
	case 1:
		sprintf((char *)InCon,"DRIVER={Microsoft Access Driver (*.mdb)};DBQ=%s;",ConStr);
		char_to_wchar(InConW, (char*)InCon);
		ret=SQLDriverConnect(hDbc,NULL,InConW,sizeof(InCon),OutConW,
			sizeof(SQLCHAR)*8192,&cbOutCon, SQL_DRIVER_NOPROMPT);
		break;
	case 2:
		sprintf((char *)InCon, "FileDsn=%s",ConStr);
		char_to_wchar(InConW, (char*)InCon);
		ret=SQLDriverConnect(hDbc,NULL,InConW,sizeof(InCon),OutConW,
			sizeof(SQLCHAR)*8192,&cbOutCon, SQL_DRIVER_NOPROMPT);
		break;
	case 3:
		ret=SQLConnect(hDbc,ConStrW,SQL_NTS,UIDW,SQL_NTS,PWDW,SQL_NTS);
		break;
	case 4:
		sprintf((char *)InCon,"Driver={MySQL ODBC 3.51 Driver};DataBase=;Pwd=%s;UID=%s;Server=%s;OPTION=3;STMT=SET NAMES EUCKR", PWD, UID, ConStr);
		char_to_wchar(InConW, (char*)InCon);
		ret=SQLDriverConnect(hDbc,NULL,InConW,sizeof(InCon),OutConW,
			sizeof(SQLCHAR)*8192,&cbOutCon, SQL_DRIVER_NOPROMPT);

		break;
	}

	// 접속 에러시 진단 정보를 보여준다.
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		while (Ret=SQLGetDiagRec(SQL_HANDLE_DBC, hDbc, ii, SqlState, &NativeError, 
			Msg, sizeof(Msg), &MsgLen)!=SQL_NO_DATA) {
			wsprintf(str, TEXT("SQLSTATE:%s, 진단정보:%s"),SqlState,Msg);
			::MessageBox(NULL,str,TEXT("진단 정보"),0);
			ii++;
		}
		return FALSE;
	}

	// 명령 핸들을 할당한다.
	ret=SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		hStmt=0;
		return FALSE;
	}
	return TRUE;
}

// SQL문을 실행한다. 실패시 진단 정보를 출력하고 FALSE를 리턴한다.
BOOL CQuery::Exec( const std::wstring& query )
{
//	printf("%s\n", szSQL);

//	std::cout << szSQL << "\n";
//	std::wcout << szSQLW << "\n";

	int c;

	// SQL문을 실행한다. SQL_NO_DATA를 리턴한 경우도 일단 성공으로 취급한다. 
	// 이 경우 Fetch에서 EOF를 리턴하도록 되어 있기 때문에 진단 정보를 출력할 필요는 없다.
	ret=SQLExecDirect(hStmt,const_cast<SQLWCHAR*>( query.c_str() ), SQL_NTS);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO) && (ret != SQL_NO_DATA)) {
		PrintDiag();
		return FALSE;
	}

	// Update, Delete, Insert 명령시 영향받은 레코드 개수를 구해 놓는다.
	SQLRowCount(hStmt,&AffectCount);

	SQLNumResultCols(hStmt,&nCol);
	if (nCol > MAXCOL) {
		::MessageBox(NULL,TEXT("최대 컬럼 수를 초과했습니다"),TEXT("CQuery 에러"),MB_OK);
		return FALSE;
	}

	// nCol이 0인 경우는 Select문 이외의 다른 명령을 실행한 경우이므로 
	// 바인딩을 할 필요가 없다.
	if (nCol == 0) {
		Clear();
		return TRUE;
	}

	SQLRowCount( hStmt, &nRow );

	// 모든 컬럼을 문자열로 바인딩해 놓는다. Col배열은 zero base, 
	// 컬럼 번호는 one base임에 유의할 것
	for (c=0;c<nCol;c++) {
		SQLBindCol(hStmt,c+1,SQL_C_CHAR,Col[c],255,&lCol[c]);
		SQLDescribeCol(hStmt,c+1,ColNameW[c],30,NULL,NULL,NULL,NULL,NULL);
	}
	return TRUE;
}

// SQL문을 실행하고 결과셋의 첫 칼럼에서 정수값을 읽어 리턴해 준다. 
// 결과를 얻은 후 정리까지 해 준다.
int CQuery::ExecGetInt( const std::wstring& szSQL)
{
	int i;

	if (Exec(szSQL) == FALSE) 
		return CQUERYERROR;

	// 데이터가 없는 경우 EOF를 리턴한다.
	if (Fetch()==SQL_NO_DATA) {
		Clear();
		return CQUERYEOF;
	}
	i=GetInt(1);
	Clear();
	return i;
}

// SQL문을 실행하고 결과셋의 첫 칼럼에서 문자열을 읽어 리턴해 준다.
std::wstring CQuery::ExecGetStr(const std::wstring& szSQL )
{
	// SQL문 실행중 에러가 발생한 경우 문자열 버퍼에 에러를 돌려 보낸다.
	if (Exec(szSQL) == FALSE) {
		return std::wstring();
	}

	// 데이터가 없는 경우 EOF를 리턴한다.
	if (Fetch()==SQL_NO_DATA) {
		Clear();
		return std::wstring();
	}
	std::wstring res = GetStr( 1 );
	Clear();

	return res;
}

// 결과셋에서 한 행을 가져온다.
SQLRETURN CQuery::Fetch()
{
	ret=SQLFetch(hStmt);
	return ret;
}

// 커서를 닫고 바인딩 정보를 해제한다.
void CQuery::Clear()
{
	SQLCloseCursor(hStmt);
	SQLFreeStmt(hStmt, SQL_UNBIND);
}

// 컬럼 이름으로부터 컬럼 인덱스를 찾는다. 없을 경우 -1을 리턴한다.
int CQuery::FindCol( const std::wstring& name) const
{
	int i;
	for( i = 0; i < nCol; i++ )
	{
		if ( name == ColNameW[ i ] )
			return i;
	}
	return -1;
}

// nCol의 컬럼값을 정수로 읽어준다. NULL일 경우 CQUERYNULL을 리턴한다.
int CQuery::GetInt(int nCol)
{
	if (nCol > this->nCol)
		return CQUERYNOCOL;

	if (lCol[nCol]==SQL_NULL_DATA) {
		return CQUERYNULL;
	} else {
		return boost::lexical_cast<int>( Col[ nCol ] );
	}
}

// sCol의 컬럼값을 정수로 읽어준다.
int CQuery::GetInt( const std::wstring& sCol )
{
	int n;
	n=FindCol(sCol);
	if (n==-1) {
		return CQUERYNOCOL;
	} else {
		return GetInt(n);
	}
}

// nCol의 컬럼값을 문자열로 읽어준다. NULL일 경우 문자열에 NULL을 채워준다. 
// buf의 길이는 최소한 256이어야 하며 길이 점검은 하지 않는다.
std::wstring CQuery::GetStr( int nCol ) const
{
	if (nCol > this->nCol) {
		return std::wstring();
	}

	if (lCol[nCol]==SQL_NULL_DATA) {
		return std::wstring();
	} else {
		return ToWString( Col[ nCol ] );
	}
}

// sCol의 컬럼값을 문자열로 읽어준다.
std::wstring CQuery::GetStr( const std::wstring& sCol ) const
{
	int n;
	n = FindCol(sCol);
	if (n==-1) {
		return std::wstring();
	} else {
		return GetStr( n );
	}
}

// 에러 발생시 진단 정보를 출력해 준다.
void CQuery::PrintDiag()
{
	int ii;
	SQLRETURN Ret;
	SQLINTEGER NativeError;
	SQLWCHAR SqlState[6], Msg[8192];
	SQLSMALLINT MsgLen;
	wchar_t str[256];

	ii=1;
	while (Ret=SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, ii, SqlState, &NativeError, 
		Msg, sizeof(Msg), &MsgLen)!=SQL_NO_DATA) {
		wsprintf(str, L"SQLSTATE:%s, 진단정보:%s",SqlState,Msg);
		::MessageBox(NULL,str,L"진단 정보",0);
		ii++;
	}

	// 복구 불가능한 에러가 발생한 경우 프로그램을 종료한다. 극단적 예외 처리가 불필요한
	// 경우는 이 코드를 주석 처리하거나 적당하게 다른 루틴으로 바꿔야 한다.
/*	if (ii > 1) {
		::MessageBox(NULL,"진단 정보가 출력되었습니다. 이 정보가 출력될 경우는 네트웍 끊김, DB 서버 중지 등의\r\n"
			"복구 불가능한 에러가 발생한 경우이며 프로그램 실행을 계속할 수 없습니다.\r\n"
			"에러를 수정하신 후 프로그램을 다시 실행해 주십시오.","Critical Error",MB_OK | MB_ICONERROR);

		// 다음 둘 중 하나를 선택할 것
		PostQuitMessage(0);
		// ExitProcess(0);
	}
*/
}

// BLOB 데이터를 buf에 채워준다. 이때 buf는 충분한 크기의 메모리를 미리 할당해 
// 놓아야 한다. NULL일 경우 0을 리턴하고 에러 발생시 -1을 리턴한다. 성공시 읽은 
// 총 바이트 수를 리턴한다. szSQL은 하나의 BLOB 필드를 읽는 Select SQL문이어야 한다.
int CQuery::ReadBlob(LPCTSTR szSQL, void *buf)
{
	SQLCHAR BinaryPtr[BLOBBATCH];
	SQLINTEGER LenBin;
	char *p;
	int nGet;
	int TotalGet=0;

	SQLWCHAR szSQLW[8192];
	char_to_wchar(szSQLW, (char*)szSQL);

	ret=SQLExecDirect(hStmt,szSQLW,SQL_NTS);
	if (ret!=SQL_SUCCESS) {
		PrintDiag();
		return -1;
	}

	while ((ret=SQLFetch(hStmt)) != SQL_NO_DATA) {
		p=(char *)buf;
		while ((ret=SQLGetData(hStmt,1,SQL_C_BINARY,BinaryPtr,sizeof(BinaryPtr),
			&LenBin))!=SQL_NO_DATA) {
			if (LenBin==SQL_NULL_DATA) {
				Clear();
				return 0;
			}
			if (ret==SQL_SUCCESS)
				nGet=LenBin;
			else
				nGet=BLOBBATCH;
			TotalGet+=nGet;
			memcpy(p,BinaryPtr,nGet);
			p+=nGet;
		}
	}
	Clear();
	return TotalGet;
}

// buf의 BLOB 데이터를 저장한다. szSQL은 하나의 BLOB 데이터를 저장하는 Update, Insert
// SQL문이어야 한다.
void CQuery::WriteBlob(LPCTSTR szSQL, void *buf, int size)
{
	SQLINTEGER cbBlob;
	char tmp[BLOBBATCH],*p;
	SQLPOINTER pToken;
	int nPut;

	SQLWCHAR szSQLW[8192];
	char_to_wchar(szSQLW, (char*)szSQL);

	cbBlob=SQL_LEN_DATA_AT_EXEC(size);
	SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_LONGVARBINARY,
		size,0,(SQLPOINTER)1,0,&cbBlob);
	SQLExecDirect(hStmt,szSQLW,SQL_NTS);
	ret=SQLParamData(hStmt, &pToken);
	while (ret==SQL_NEED_DATA) {
		if (ret==SQL_NEED_DATA) {
			if ((int)pToken==1) {
				for (p=(char *)buf;p<(char *)buf+size;p+=BLOBBATCH) {
					nPut=std::min((int)( BLOBBATCH ),(int)((char *)buf+size-p));
					memcpy(tmp,p,nPut);
					SQLPutData(hStmt,(PTR)tmp,nPut);
				}
			}
		}
		ret=SQLParamData(hStmt, &pToken);
	}
	Clear();
}
