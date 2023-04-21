//#include "stdafx.h"														// for precompiled headers (has to be in first place)
#include "global.h"
#include <stdlib.h>
#include <malloc.h>														// malloc,free
#include <stdio.h>														// FILE
#include <fcntl.h>														// filesize
#include <io.h>																// filesize
#include <string.h>														// strlen
#include <assert.h>													  // assert

#include "ASCIIFile.h"

#pragma warning( disable : 4996 )

/// test0

/// {} = This is a test {key2}
///    = And it goes further {key3}y as expected
/// {key3} = trick
/// {key2} = (Another {key3,separator})

/// {key3} = AAA
/// {key3} = BBB
/// {separator} = ;

//					++fbuf; 				/* Skip overlay byte */


// {} ="     Application: {AppName}"
//    ="  date(mm/dd/yy): {__Date__}"
//    ="  time(hh:mm:ss): {__Time__}"
//    ="      line count: {__CodeLines__}"
//    =" code file count: {__CodeFiles__}"

// {AppName} = CppScanner




uint32 IO_GetFileSize( const char *Name )		// test1
{
	assert(Name);
	int handle,size;	// test2

	handle = open(Name,O_RDONLY);
	if(handle==-1)
	{
		return 0;
	}

	size=filelength(handle);
	close(handle);

	return size;
}



CASCIIFile::CASCIIFile()
{
	m_Size=0;
	m_Data=0;
}

// destructor
CASCIIFile::~CASCIIFile()
{
	ReleaseData();
}


// copy constructor
CASCIIFile::CASCIIFile( CASCIIFile &a )
{
	char *mem=(char *)malloc(a.m_Size);

	if(mem)
	{
		m_Data=mem;
		m_Size=a.m_Size;
	}
	else throw "low memory";
}


// assignment operator
const CASCIIFile CASCIIFile::operator=( const CASCIIFile &a )
{
	char *mem=(char *)malloc(a.m_Size);

	if(mem)
	{
		ReleaseData();
		m_Data=mem;
		m_Size=a.m_Size;
	}
	else throw "low memory";

	return *this;
}





void CASCIIFile::ReleaseData()
{
	free((void*)m_Data);
	m_Data=0;
	m_Size=0;
}

bool CASCIIFile::IO_GetAvailability( const char *pathname )
{
	int handle;

	handle=open(pathname,O_RDONLY);

	if(handle==-1)
		return false;

	close(handle);

	return true;
}





bool CASCIIFile::IO_SaveASCIIFile( const char *pathname )	 
{
	if(m_Data==0)
		return false;
	
	FILE *file;

	if((file = fopen(pathname,"wb")) != NULL)
	{
		fwrite(m_Data, m_Size - 1, 1, file);
		fclose(file);
		return true;
	}

	return false;
}


bool CASCIIFile::IO_LoadASCIIFile( const char *pathname )		// allociert 1+Filesize, liest Daten und 0 terminiert, free nicht vergessen !!
{
	if(*pathname==0)
		return false;

	uint32 size=IO_GetFileSize(pathname);
	char *ret=0;

	ret=(char *)malloc(size+1);

	if(ret==0)
		return false;	// no memory
	
	FILE *file;

	if((file = fopen(pathname,"rb")) != NULL)
	{
		fread(ret,size, 1, file);
		ret[size] = 0;				// 0 terminate

		m_Size = size + 1;
		m_Data = ret;

		fclose(file);
		return true;
	}

	free(ret);
	return false;
}

size_t CASCIIFile::GetDataSize() const
{
	if(m_Data)return m_Size-1;
		else return 0;
}

const char *CASCIIFile::GetDataPtr()
{
	return m_Data;
}

void CASCIIFile::CoverThisData(const char *ptr, const size_t size)
{
	ReleaseData();
	m_Data = ptr;
	m_Size = size + 1;
}

