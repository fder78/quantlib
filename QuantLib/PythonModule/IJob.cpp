#include "StdAfx.h"

#include "IJob.h"

#include "StringUtil.h"

DWORD WINAPI ThreadProc( LPVOID lpParam )
{
	try
	{
		IJob* job = reinterpret_cast<IJob*>( lpParam );
		job->Calculate();
	}
	catch( const std::exception& e )
	{
		//if( showMsgBox )
		{
			::MessageBox( NULL, ::ToWString( e.what() ).c_str(), TEXT( "���� ����" ), 0 );
		}
		//else
		{
			//std::cout << e.what();
		}


		return E_FAIL;
	}
	catch( ... )
	{
		//if( showMsgBox )
		{ 
			::MessageBox( NULL, L"�� �� ���� ���� �߻�!", TEXT( "���� ����" ), 0 );
		}
		//else
		{
			//std::cout << "�� �� ���� ���� �߻�!";
		}

		return E_FAIL;
	}

	return 0;
}


void IJob::CreateThread()
{
	DWORD id;
	while( true )
	{
		m_thread = ::CreateThread( NULL, 0, ThreadProc, reinterpret_cast<LPVOID>( this ), 0, &id );
		if( !!m_thread )
		{
			break;
		}
		Sleep( 1000 );
	}	
}

void IJob::WaitForTerminate()
{
	::WaitForSingleObject( m_thread, INFINITE );
}
