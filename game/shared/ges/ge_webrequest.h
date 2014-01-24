///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_webrequest.h
// Description:
//      Initializes a web request in an asynchronous thread calling a defined
//      callback when the load is finished for processing
//
// Created On: 09 Oct 2011
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_WEBREQUEST_H
#define GE_WEBREQUEST_H
#pragma once

#include "threadtools.h"
#include "utlbuffer.h"

typedef void (*WEBRQST_CLBK)( const char *result, const char *error );

class CGEWebRequest : public CThread
{
public:
	CGEWebRequest( const char *addr, WEBRQST_CLBK = NULL );
	~CGEWebRequest();

	void Destroy();

	bool IsFinished() { return m_bFinished; }

	const char* GetResult() { return (char*) m_Result.Base(); }
	bool		HadError()	{ return m_pError[0] != '\0'; }
	const char* GetError()	{ return m_pError; }

protected:
	static size_t WriteData( void *ptr, size_t size, size_t nmemb, void *userdata );

	virtual int Run();
	virtual void OnExit();

	bool m_bFinished;

	char *m_pAddress;
	char *m_pError;
	CUtlBuffer m_Result;

	WEBRQST_CLBK m_pCallback;
};

#endif