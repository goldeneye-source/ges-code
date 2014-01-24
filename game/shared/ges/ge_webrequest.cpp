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

#include "cbase.h"
#include "ge_webrequest.h"

#pragma warning( disable : 4005 )
#include "curl.h"
#pragma warning( default : 4005 )

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGEWebRequest::CGEWebRequest( const char *addr, WEBRQST_CLBK callback /*=NULL*/ )
{	
	m_pAddress = new char[Q_strlen(addr)+1];
	Q_strcpy( m_pAddress, addr );

	m_pError = new char[CURL_ERROR_SIZE+1];
	m_pError[0] = '\0';

	m_bFinished = false;

	m_pCallback = callback;

	// Start the thread
	SetName( "GEWebRequest" );
	Start();
}

CGEWebRequest::~CGEWebRequest()
{
	// Stop the thread
	if ( IsAlive() )
	{
		Terminate();
		Join();
	}
	
	// Clear the buffers
	delete [] m_pAddress;
	delete [] m_pError;
	m_Result.Purge();
}

void CGEWebRequest::Destroy()
{
	delete this;
}

size_t CGEWebRequest::WriteData( void *ptr, size_t size, size_t nmemb, void *userdata )
{
	CUtlBuffer *result = (CUtlBuffer*) userdata;
	result->PutString( (char*)ptr );
	return size * nmemb;
}

int CGEWebRequest::Run()
{
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if ( curl )
	{
		curl_easy_setopt( curl, CURLOPT_URL, m_pAddress );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &m_Result );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, &CGEWebRequest::WriteData );
		curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, m_pError );
		curl_easy_setopt( curl, CURLOPT_FAILONERROR, 1 );

		res = curl_easy_perform( curl );
		if ( res != CURLE_OK && res != CURLE_HTTP_RETURNED_ERROR )
			Q_strncpy( m_pError, curl_easy_strerror( res ), CURL_ERROR_SIZE );

		curl_easy_cleanup( curl );
	}
	else
	{
		Q_strncpy( m_pError, "Failed to start CURL", CURL_ERROR_SIZE );
	}

	m_bFinished = true;
	return 0;
}

void CGEWebRequest::OnExit()
{
	if ( m_pCallback )
		m_pCallback( (char*) m_Result.Base(), m_pError );
}