// thread.h by sgchoi@cs.umd.edu

#ifndef __THREAD_H__BY_SGCHOI  
#define __THREAD_H__BY_SGCHOI 

#include "typedefs.h"

#ifdef WIN32 
 
#include <process.h>

class CEvent  
{
// Constructor
public:
	CEvent(BOOL bManualReset = FALSE, BOOL bInitialSet = FALSE)
	{
		m_hHandle = ::CreateEvent(0, bManualReset, bInitialSet, 0);
	}

	~CEvent()
	{ 
		CloseHandle(m_hHandle);	
	}

// Operations
public:
	BOOL Set(){ return SetEvent(m_hHandle); }
	BOOL Reset(){ return ResetEvent(m_hHandle); }
	BOOL Wait(){ return WaitForSingleObject(m_hHandle, INFINITE) == WAIT_OBJECT_0; }

private:
	HANDLE	m_hHandle;
};


/////////////////////////////////////////////////////////////////////////////
// CLock

	
// Operations
class CLock
{
// Constructor
public:
	CLock(){ InitializeCriticalSection(&m_cs); }
	~CLock(){ DeleteCriticalSection(&m_cs); }

public:
	void Lock(){  EnterCriticalSection(&m_cs);  }
	void Unlock(){ LeaveCriticalSection(&m_cs); }
	 
private:
	CRITICAL_SECTION m_cs;  
};

class CThread
{
public:
	CThread(){m_bRunning = FALSE; m_hHandle = NULL;}
	virtual ~CThread(){ if(m_hHandle != NULL) CloseHandle(m_hHandle);}

public:
    BOOL Start()
	{
		m_bRunning = TRUE;
		m_hHandle = CreateThread(0, 0, ThreadMainHandler, this,0,0);
		if( m_hHandle == NULL )
			m_bRunning = FALSE;
	
		return m_bRunning;
	}

    BOOL Wait()
    {
		if( !m_bRunning ) return TRUE;
		return WaitForSingleObject(m_hHandle, INFINITE) == WAIT_OBJECT_0;
    }

	BOOL Kill()
	{
		if( !m_bRunning) return TRUE;

		m_bRunning = !(TerminateThread(m_hHandle, 0));
		return !m_bRunning;
	}

	BOOL IsRunning()
	{
		return m_bRunning;
	}


protected:
	virtual void ThreadMain() = 0;

    static DWORD __stdcall ThreadMainHandler( void* p )
    {
		CThread* pThis = (CThread*) p;
		pThis->ThreadMain();
		pThis->m_bRunning = FALSE;
		return 0;
	    }

protected:
 	BOOL	m_bRunning;
	HANDLE	m_hHandle;
};
 

#else // NOT WIN32 
#include <pthread.h>
class CThread
{
public:
	CThread(){m_bRunning = FALSE; }
	virtual ~CThread(){}

public:
    BOOL Start()
	{
		m_bRunning = !pthread_create(&m_pThread, NULL, 
						ThreadMainHandler, (void*) this);
		return m_bRunning;
	}

    BOOL Wait()
    {
		if( !m_bRunning ) return TRUE;
		return pthread_join(m_pThread, NULL)==0;
    }

	BOOL Kill()
	{
		if( !m_bRunning) return TRUE;
		pthread_exit(NULL);
		return TRUE;
	}

	BOOL IsRunning()
	{
		return m_bRunning;
	}

protected:
	virtual void ThreadMain() = 0;
	static void* ThreadMainHandler( void* p )
    {
		CThread* pThis = (CThread*) p;
		pThis->ThreadMain();
		pThis->m_bRunning = FALSE;
		return 0;
    }

protected:
 	BOOL		m_bRunning;
	pthread_t	m_pThread;
};


class CLock
{
// Constructor
public:
	CLock(){ pthread_mutex_init(&m_mtx, NULL); }
	~CLock(){ pthread_mutex_destroy(&m_mtx); }

public:
	void Lock(){ pthread_mutex_lock(&m_mtx);}
	void Unlock(){ pthread_mutex_unlock(&m_mtx); } 
	 
private:
	pthread_mutex_t m_mtx;
};



class CEvent  
{
// Constructor
public:
	CEvent(BOOL bManualReset = FALSE, BOOL bInitialSet = FALSE)
	{
		pthread_mutex_init(&m_mtx, NULL);
		pthread_cond_init(&m_cnd, NULL);
		m_bManual = bManualReset;
		m_bSet = bInitialSet;
 	}

	~CEvent()
	{ 
		pthread_mutex_destroy(&m_mtx);
		pthread_cond_destroy(&m_cnd);
	}

// Operations
public:
	BOOL Set()
	{   
		pthread_mutex_lock(&m_mtx);
        if(!m_bSet)  
        {
			m_bSet = TRUE;
			pthread_cond_signal(&m_cnd);
        }
        pthread_mutex_unlock(&m_mtx);
		return TRUE;
	} 

	BOOL Wait()
	{ 
		pthread_mutex_lock( &m_mtx );

        while(!m_bSet) 
        {
            pthread_cond_wait( &m_cnd, &m_mtx );
        }

        if ( !m_bManual ) m_bSet = FALSE; 
        pthread_mutex_unlock( &m_mtx );
		return TRUE;
	}

	BOOL Reset()
	{
		pthread_mutex_lock( &m_mtx );
		m_bSet = FALSE; 
        pthread_mutex_unlock( &m_mtx );
		return TRUE;
	}
private:
	pthread_cond_t  m_cnd;
	pthread_mutex_t m_mtx;
	BOOL			m_bManual;
	BOOL			m_bSet;
 };

#endif //WIN32



class CGrabLock
{
public:
	CGrabLock(CLock& l):lock(l){ lock.Lock(); }
	~CGrabLock(){ lock.Unlock(); }
public:
	CLock& lock;
};

#endif //__THREAD_H__BY_SGCHOI


