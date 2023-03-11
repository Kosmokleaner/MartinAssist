#pragma once

#include "global.h"

//! 0 byte terminted char ASCII file
class CASCIIFile  
{
public:
	//! default constructor
	CASCIIFile();

	//! copy construtor
	CASCIIFile( CASCIIFile &a );

	//! destructor
	virtual ~CASCIIFile();												

	//! assignment operator
	const CASCIIFile operator=( const CASCIIFile &a );

	//! altes freigeben, neues übernehmen, keine Kopie machen,
	//! size ist ohne Nullterm., Data soll nullterminiert sein
	void CoverThisData( char *ptr, const uint32 size );
	
	//! get data size (without zero termination byte)
	uint32 GetDataSize();

	//! get memory access
	char * GetDataPtr();

	//! free data
	void ReleaseData();

	static bool IO_GetAvailability( const char *pathname );

	bool IO_SaveASCIIFile( const char *pathname );	
	bool IO_LoadASCIIFile( const char *pathname );

protected: // ------------------------------------------------------------------------

	char *				m_Data;						//!< zero terminated text
	uint32				m_Size;						//!< including the zero termination (otherwise 0)
};
