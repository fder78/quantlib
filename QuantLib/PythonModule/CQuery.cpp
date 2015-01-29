#include "StdAfx.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "CQuery.h"

#include "StringUtil.h"

// MFC ������Ʈ�� ���� ���� stdafx.h�� CQuery.h�� ���Խ��� �ְ� ���� ���ٸ�
// ������ �ȴ�.
// #include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
/*
CQuery Ŭ����:ODBC�� ���� Ŭ����. API ���ؿ��� ���� �����ϵ��� �ۼ��Ͽ���.
SQL���� ���ϰ� �����ϰ� ����� ���� �дµ� ������ ���߾����� SQL ������ 
�׼������� �Ϻ��ϰ� �׽�Ʈ�Ǿ���.
255�� ������ ����, ���ڿ� �÷��� ���ؼ��� ��� �����ϸ� �� �̻��� ���̸� ��
���� �ʵ�� "������ �߸�" ���� ��� �߻��ϹǷ� �� Ŭ������ ���� �� ������
ODBC API �Լ��� ���� ����ؾ� �Ѵ�.
�ִ� �÷� ������ 100���̴�. �ʰ��� �������� ���� �޽��� �ڽ��� ����ϹǷ� �� 
�ѵ��� �˾Ƽ� ���� �ʵ��� �����ؾ� �Ѵ�. Ư�� �ʵ���� ���� ���̺��� select *
�� �д� ���� �ﰡ�ϴ� ���� ����.
���� ���� Ŀ���� ����ϹǷ� �̹� ���� ���ڵ�� ������ �ٽ� �����ؾ߸� ���� ��
�ִ�. 

����
1.CQuery Ŭ������ ��ü�� �����Ѵ�. �������̸� �������� �����ϴ� ���� ������
  �ʿ��� ����ŭ ��ü�� �����. ���� �� �� ������ ����ϴ�.
2.������ Connect �Լ��� ȣ���Ͽ� ������ �ҽ��� �����Ѵ�.
3.Exec �Լ��� SQL���� �����Ѵ�. ���� ó���� Exec �Լ� ���ο��� ó���Ѵ�.
4.Fetch �Լ��� ��� �� �ϳ��� �����´�. �������� ������� �ִ� ���� while
  ������ ���鼭 ���ʴ�� ������ �� �ִ�.
5.Get* �Լ��� �÷� ���� �д´�.
6.Clear �Լ��� ��� �ڵ��� �ݴ´�.(Select���� ��츸)
7.��ü�� �ı��Ѵ�. ���� ��ü�� ���� �Ϻη� �ı��� �ʿ䰡 ����.
*/

// ������:���� �ʱ�ȭ�� ����Ѵ�.
CQuery::CQuery()
{
	AffectCount=-1;
	ret=SQL_SUCCESS;

	// ȯ�� �ڵ��� �Ҵ��ϰ� ���� �Ӽ��� �����Ѵ�.
	SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv);
	SQLSetEnvAttr(hEnv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,SQL_IS_INTEGER);
}

// �ı���:���� �ڵ��� �����Ѵ�.
CQuery::~CQuery()
{
	Clear();

	if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
	if (hDbc) SQLDisconnect(hDbc);
	if (hDbc) SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
	if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
}

// ���� �ڵ��� �Ҵ��ϰ� ������ �� ����ڵ���� ���� �Ҵ��Ѵ�.
// Type=1:ConStr�� MDB ������ ��θ� ������. ��� ������ ���� ���丮���� MDB�� ã�´�.
// Type=2:ConStr�� SQL ������ ���� ������ ������ DSN ������ ��θ� ������. 
//        ��δ� �ݵ�� ���� ��η� �����ؾ� �Ѵ�.
// Type=3:SQLConnect �Լ��� DSN�� ���� �����Ѵ�.
// ���� �Ǵ� ��� �ڵ� �Ҵ翡 �����ϸ� FALSE�� �����Ѵ�.
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

	// ���� Ÿ�Կ� ���� MDB �Ǵ� SQL ����, �Ǵ� DSN�� �����Ѵ�.
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

	// ���� ������ ���� ������ �����ش�.
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		while (Ret=SQLGetDiagRec(SQL_HANDLE_DBC, hDbc, ii, SqlState, &NativeError, 
			Msg, sizeof(Msg), &MsgLen)!=SQL_NO_DATA) {
			wsprintf(str, TEXT("SQLSTATE:%s, ��������:%s"),SqlState,Msg);
			::MessageBox(NULL,str,TEXT("���� ����"),0);
			ii++;
		}
		return FALSE;
	}

	// ��� �ڵ��� �Ҵ��Ѵ�.
	ret=SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		hStmt=0;
		return FALSE;
	}
	return TRUE;
}

// SQL���� �����Ѵ�. ���н� ���� ������ ����ϰ� FALSE�� �����Ѵ�.
BOOL CQuery::Exec( const std::wstring& query )
{
//	printf("%s\n", szSQL);

//	std::cout << szSQL << "\n";
//	std::wcout << szSQLW << "\n";

	int c;

	// SQL���� �����Ѵ�. SQL_NO_DATA�� ������ ��쵵 �ϴ� �������� ����Ѵ�. 
	// �� ��� Fetch���� EOF�� �����ϵ��� �Ǿ� �ֱ� ������ ���� ������ ����� �ʿ�� ����.
	ret=SQLExecDirect(hStmt,const_cast<SQLWCHAR*>( query.c_str() ), SQL_NTS);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO) && (ret != SQL_NO_DATA)) {
		PrintDiag();
		return FALSE;
	}

	// Update, Delete, Insert ��ɽ� ������� ���ڵ� ������ ���� ���´�.
	SQLRowCount(hStmt,&AffectCount);

	SQLNumResultCols(hStmt,&nCol);
	if (nCol > MAXCOL) {
		::MessageBox(NULL,TEXT("�ִ� �÷� ���� �ʰ��߽��ϴ�"),TEXT("CQuery ����"),MB_OK);
		return FALSE;
	}

	// nCol�� 0�� ���� Select�� �̿��� �ٸ� ����� ������ ����̹Ƿ� 
	// ���ε��� �� �ʿ䰡 ����.
	if (nCol == 0) {
		Clear();
		return TRUE;
	}

	SQLRowCount( hStmt, &nRow );

	// ��� �÷��� ���ڿ��� ���ε��� ���´�. Col�迭�� zero base, 
	// �÷� ��ȣ�� one base�ӿ� ������ ��
	for (c=0;c<nCol;c++) {
		SQLBindCol(hStmt,c+1,SQL_C_CHAR,Col[c],255,&lCol[c]);
		SQLDescribeCol(hStmt,c+1,ColNameW[c],30,NULL,NULL,NULL,NULL,NULL);
	}
	return TRUE;
}

// SQL���� �����ϰ� ������� ù Į������ �������� �о� ������ �ش�. 
// ����� ���� �� �������� �� �ش�.
int CQuery::ExecGetInt( const std::wstring& szSQL)
{
	int i;

	if (Exec(szSQL) == FALSE) 
		return CQUERYERROR;

	// �����Ͱ� ���� ��� EOF�� �����Ѵ�.
	if (Fetch()==SQL_NO_DATA) {
		Clear();
		return CQUERYEOF;
	}
	i=GetInt(1);
	Clear();
	return i;
}

// SQL���� �����ϰ� ������� ù Į������ ���ڿ��� �о� ������ �ش�.
std::wstring CQuery::ExecGetStr(const std::wstring& szSQL )
{
	// SQL�� ������ ������ �߻��� ��� ���ڿ� ���ۿ� ������ ���� ������.
	if (Exec(szSQL) == FALSE) {
		return std::wstring();
	}

	// �����Ͱ� ���� ��� EOF�� �����Ѵ�.
	if (Fetch()==SQL_NO_DATA) {
		Clear();
		return std::wstring();
	}
	std::wstring res = GetStr( 1 );
	Clear();

	return res;
}

// ����¿��� �� ���� �����´�.
SQLRETURN CQuery::Fetch()
{
	ret=SQLFetch(hStmt);
	return ret;
}

// Ŀ���� �ݰ� ���ε� ������ �����Ѵ�.
void CQuery::Clear()
{
	SQLCloseCursor(hStmt);
	SQLFreeStmt(hStmt, SQL_UNBIND);
}

// �÷� �̸����κ��� �÷� �ε����� ã�´�. ���� ��� -1�� �����Ѵ�.
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

// nCol�� �÷����� ������ �о��ش�. NULL�� ��� CQUERYNULL�� �����Ѵ�.
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

// sCol�� �÷����� ������ �о��ش�.
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

// nCol�� �÷����� ���ڿ��� �о��ش�. NULL�� ��� ���ڿ��� NULL�� ä���ش�. 
// buf�� ���̴� �ּ��� 256�̾�� �ϸ� ���� ������ ���� �ʴ´�.
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

// sCol�� �÷����� ���ڿ��� �о��ش�.
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

// ���� �߻��� ���� ������ ����� �ش�.
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
		wsprintf(str, L"SQLSTATE:%s, ��������:%s",SqlState,Msg);
		::MessageBox(NULL,str,L"���� ����",0);
		ii++;
	}

	// ���� �Ұ����� ������ �߻��� ��� ���α׷��� �����Ѵ�. �ش��� ���� ó���� ���ʿ���
	// ���� �� �ڵ带 �ּ� ó���ϰų� �����ϰ� �ٸ� ��ƾ���� �ٲ�� �Ѵ�.
/*	if (ii > 1) {
		::MessageBox(NULL,"���� ������ ��µǾ����ϴ�. �� ������ ��µ� ���� ��Ʈ�� ����, DB ���� ���� ����\r\n"
			"���� �Ұ����� ������ �߻��� ����̸� ���α׷� ������ ����� �� �����ϴ�.\r\n"
			"������ �����Ͻ� �� ���α׷��� �ٽ� ������ �ֽʽÿ�.","Critical Error",MB_OK | MB_ICONERROR);

		// ���� �� �� �ϳ��� ������ ��
		PostQuitMessage(0);
		// ExitProcess(0);
	}
*/
}

// BLOB �����͸� buf�� ä���ش�. �̶� buf�� ����� ũ���� �޸𸮸� �̸� �Ҵ��� 
// ���ƾ� �Ѵ�. NULL�� ��� 0�� �����ϰ� ���� �߻��� -1�� �����Ѵ�. ������ ���� 
// �� ����Ʈ ���� �����Ѵ�. szSQL�� �ϳ��� BLOB �ʵ带 �д� Select SQL���̾�� �Ѵ�.
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

// buf�� BLOB �����͸� �����Ѵ�. szSQL�� �ϳ��� BLOB �����͸� �����ϴ� Update, Insert
// SQL���̾�� �Ѵ�.
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
