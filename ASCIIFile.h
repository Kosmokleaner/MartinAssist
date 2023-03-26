#pragma once

#include "global.h"

//! 0 byte terminted char ASCII file
class CASCIIFile  
{
public:
	CASCIIFile();

	CASCIIFile( CASCIIFile &a );

	virtual ~CASCIIFile();												

	const CASCIIFile operator=(const CASCIIFile &a);

	// release old, don't make copy
	// @param ptr has not be 0 terminated, allocated with malloc
	// @param size without ohne 0 termination
	void CoverThisData(char *ptr, const uint32 size);
	
	uint32 GetDataSize();

	char * GetDataPtr();

	void ReleaseData();

	static bool IO_GetAvailability( const char *pathname );

	bool IO_SaveASCIIFile( const char *pathname );	
	bool IO_LoadASCIIFile( const char *pathname );

protected: // ------------------------------------------------------------------------

	char * m_Data; //!< zero terminated text
	uint32 m_Size; //!< including the zero termination (otherwise 0)
};
