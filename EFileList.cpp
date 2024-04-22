#include "EFileList.h"
#include "ASCIIFile.h"
#include "Timer.h" // CScopedCPUTimerLog
#include "Parse.h"

bool EFileList::load(const wchar_t* fileName)
{
    assert(fileName);

    CASCIIFile file;
    {
        CScopedCPUTimerLog scope("loadFile");

        if (!file.IO_LoadASCIIFile(fileName))
            return false;
    }

    const Char* p = (const Char*)file.GetDataPtr();

    std::string line;

//    int deviceId = (int)driveData.size();
//    DriveData& data = *(new DriveData());
//    driveData.push_back(&data);
//    data.csvName = to_string(internalName);
//    data.driveId = deviceId;

    bool error = false;

    std::string sPath;

    {
        CScopedCPUTimerLog scope("parse");

        // Everything EFU file format
        // https://www.voidtools.com/support/everything/file_lists/#:~:text=Everything%20File%20List%20(EFU)%20are,%2C%20sizes%2C%20dates%20and%20attributes
        //
        // EFU files are comma-separated values (CSV) files.
        // A header is required to define the columns in the file list.
        // The Filename column is required.
        // The Size, Date Modified, Date Created and Attributes columns are optional.
        // File size is specified in bytes.
        // Dates are FILETIMEs(100 - nanosecond intervals since January 1, 1601.) in decimal or ISO 8601 dates.
        // Attributes can be zero or more of the Windows File Attributes in decimal(or hexidecimal with 0x prefix).
        // EFU files are encoded with UTF - 8.
        //
        // Example:
        // Filename,Size,Date Modified,Date Created,Attributes
        // "C:\Program Files\Image-Line\FL Studio 20\Data\Patches\Misc\Used by demo projects\!step on stage.wav", 22482, 132198419340000000, 132198419340000000, 32

        std::string firstLine;
        if(!parseLine(p, firstLine))
            error = true;

        // this is the only format we support at the moment, todo: see above, some data is optional
        // we assume the file was written by Everything and not modified by a human (that might use whitespace)
        if(!error)
        if(firstLine != "Filename,Size,Date Modified,Date Created,Attributes")
            error = true;

        if(!error)
        while (*p)
        {
            parseWhiteSpaceOrLF(p);
            
            // faster than data.entries.push_back(FileEntry()); 4400ms -> 2400ms
            entries.resize(entries.size() + 1);
            FileEntry& entry = entries.back();

            if (!parseStartsWith(p, "\""))
            {
                int32_t errLine, errCol;
                computeLocationInFile((const Char *)file.GetDataPtr(), p, errLine, errCol, 4);

                error = true;
                assert(0);
                break;
            }

            if (!parseLine(p, sPath, '\"') ||
                !parseStartsWith(p, ","))
            {
                error = true;
                assert(0);
                break;
            }

            // splat sPath into path and filename
            {
                char* name = sPath.data();
                for(char* f = name; *f; ++f)
                {
                    if(*f == ':' || *f == '/' || *f == '\\')
                        name = f + 1;
                }

                if(*name == 0)
                {
                    error = true;
                    assert(0);
                    break;
                }

                entry.key.fileName = stringPool.push(name);
                // if there is a path, terminate and remove the last character
                if(name != sPath.data())
                    name[-1] = 0;
                entry.value.path = stringPool.push(sPath.c_str());
            }

            if (!parseInt64(p, entry.key.size) ||
                !parseStartsWith(p, ","))
            {
                error = true;
                assert(0);
                break;
            }

            if (!parseInt64(p, entry.key.time_write) ||
                !parseStartsWith(p, ","))
            {
                error = true;
                assert(0);
                break;
            }

            if(parseStartsWith(p, ","))
            {
                entry.value.time_create = -1;
            }
            else
            if (!parseInt64(p, entry.value.time_create) ||
                !parseStartsWith(p, ","))
            {
                int32_t errLine, errCol;
                computeLocationInFile((const Char*)file.GetDataPtr(), p, errLine, errCol, 4);


                error = true;
                assert(0);
                break;
            }

            int flags = 0;
            if (!parseInt(p, flags))
            {
                error = true;
                assert(0);
                break;
            }

            parseLineFeed(p);
        }
    }

//    DriveInfo driveInfo;
//    driveInfo.drivePath = FilePath(to_wstring(data.drivePath).c_str());
    //    driveInfo.deviceName = data.;
    //   driveInfo.internalName = data.;
//    driveInfo.volumeName = to_wstring(data.volumeName).c_str();
//    driveInfo.driveFlags = data.driveFlags;

//    data.csvName = to_string(driveInfo.generateKeyName()).c_str();

    return !error;
}

void EFileList::test()
{
    EFileList l;

    l.load(L"E:\\EverythingEFUs\\Ryzen4202024.efu");
}


// todo
// * file list UI
