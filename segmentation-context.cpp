
// Windows
#include <windows.data.json.h>

// OpenCV
#include <opencv2/core.hpp>

// std
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

// using
using namespace std;

// to be implemented
void generate_json();
void generate_bat();

// work in progress
// trying to get recursive file reader up
void convert_segments(const string path)
{
	vector<const char *> files;
	WIN32_FIND_DATA file;
	HANDLE handle;
	char buffer[_MAX_DIR];

	// format string
	sprintf_s(buffer, _MAX_DIR, "%s/*", path.c_str());

	// find first file in path
	handle = FindFirstFile(buffer, &file); // capture .
	FindNextFile(handle, &file); // capture .. 
	if (handle == INVALID_HANDLE_VALUE)
	{
		printf("FindFirstFile failed (%d)\n", GetLastError());
	}
	else
	{
		DWORD file_type;

		do
		{
			FindNextFile(handle, &file);
			// need to get the size of each string then use strcpy_s
			file_type = GetFileAttributes(file.cFileName);
			if (file_type == FILE_ATTRIBUTE_DIRECTORY) printf("foundone\n");
			printf("%ld\n", file_type);
			printf("%s\n", file.cFileName);
			
		} while (FindNextFileA(handle, &file));
	}
	FindClose(handle);
}

int main(int argc, char** argv)
{	
	//convert_segments("segments");
	return 0;
}
