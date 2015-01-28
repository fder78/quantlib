#pragma once

#include <sql.h>
#include <sqlext.h>

class CQuery
{
public:
	// �ִ� �÷���, BLOB ����� ����, NULL �ʵ尪
	enum { MAXCOL=100, BLOBBATCH=10000, CQUERYNULL=-100, CQUERYEOF=-101, 
		CQUERYNOCOL=-102, CQUERYERROR=-103 };
private:
	SQLHENV hEnv;							// ȯ�� �ڵ�
	SQLHDBC hDbc;							// ���� �ڵ�
	SQLRETURN ret;							// ���� ������ SQL���� �����
	char Col[MAXCOL][512];					// ���ε��� �÷� ����

	int FindCol( const std::wstring& name ) const;				// �÷��� �̸����κ��� ��ȣ�� ã���ش�.

private:
	SQLLEN AffectCount;					// ������� ���ڵ� ����
	SQLHSTMT hStmt;							// ��� �ڵ�. ���� ����� ���� �����Ƿ� public���� ����
	SQLSMALLINT nCol;						// �÷� ����
	SQLLEN nRow;
	SQLWCHAR ColNameW[MAXCOL][50];			// �÷��� �̸��� W
	SQLLEN lCol[MAXCOL];				// �÷��� ����/���� ����

public:
	void PrintDiag();						// ���� ���� ���
	CQuery();								// ������
	~CQuery();								// �ı���:���� �ڵ��� �����Ѵ�.

	BOOL Connect(int Type, const char *ConStr, const char *UID=NULL, const char *PWD=NULL);	// ������ �ҽ��� �����Ѵ�.
	BOOL Exec( const std::wstring& query );				// SQL���� �����Ѵ�.
	int ExecGetInt( const std::wstring& szSQL);			// SQL���� �����ϰ� ù��° �÷��� ���� ����
	std::wstring ExecGetStr(const std::wstring& szSQL );		// SQL���� �����ϰ� ù��° �÷��� ���ڿ� ����
	SQLRETURN Fetch();						// �� �� ��������
	void Clear();							// Ŀ�� ���� �� �� ���ε�
	int GetInt(int nCol);					// ������ �÷� �б�
	int GetInt(const std::wstring& sCol);					// ������ �÷� �б�
	std::wstring GetStr(int nCol ) const;		// ���ڿ��� �÷� �б�
	std::wstring GetStr( const std::wstring& sCol ) const;		// ���ڿ��� �÷� �б�
	SQLLEN ReadBlob(LPCTSTR szSQL, void *buf);
	void WriteBlob(LPCTSTR szSQL, void *buf, SQLLEN size);

	SQLLEN GetNumRow() const;
};
