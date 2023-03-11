#include <iostream>
#include "ASCIIFile.h"
#include "Parse.h"


int main()
{
    CASCIIFile file;

    file.IO_LoadASCIIFile("MartinAssist.cpp");

    const Char *p = (Char*)file.GetDataPtr();

    

} 
