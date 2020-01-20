/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdlib.h>

#include "filesystem.h"
#include "strings.h"

#if defined(CONF_FAMILY_UNIX)
	#include <unistd.h>
	#include <dirent.h>
#elif defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <fcntl.h>
	#include <errno.h>
#else
	#error NOT IMPLEMENTED
#endif

#if defined(__cplusplus)
extern "C" {
#endif

void fs_listdir(const char *dir, FS_LISTDIR_CALLBACK cb, int type, void *user)
{
#if defined(CONF_FAMILY_WINDOWS)
	WIN32_FIND_DATA finddata;
	HANDLE handle;
	char buffer[1024*2];
	int length;
	str_format(buffer, sizeof(buffer), "%s/*", dir);

	handle = FindFirstFileA(buffer, &finddata);

	if (handle == INVALID_HANDLE_VALUE)
		return;

	str_format(buffer, sizeof(buffer), "%s/", dir);
	length = str_length(buffer);

	/* add all the entries */
	do
	{
		str_copy(buffer+length, finddata.cFileName, (int)sizeof(buffer)-length);
		if(cb(finddata.cFileName, fs_is_dir(buffer), type, user))
			break;
	}
	while (FindNextFileA(handle, &finddata));

	FindClose(handle);
	return;
#else
	struct dirent *entry;
	char buffer[1024*2];
	int length;
	DIR *d = opendir(dir);

	if(!d)
		return;

	str_format(buffer, sizeof(buffer), "%s/", dir);
	length = str_length(buffer);

	while((entry = readdir(d)) != NULL)
	{
		str_copy(buffer+length, entry->d_name, (int)sizeof(buffer)-length);
		if(cb(entry->d_name, fs_is_dir(buffer), type, user))
			break;
	}

	/* close the directory and return */
	closedir(d);
	return;
#endif
}

int fs_storage_path(const char *appname, char *path, int max)
{
#if defined(CONF_FAMILY_WINDOWS)
	char *home = getenv("APPDATA");
	if(!home)
		return -1;
	str_format(path, max, "%s/%s", home, appname);
	return 0;
#else
	char *home = getenv("HOME");
	int i;
	char *xdgdatahome = getenv("XDG_DATA_HOME");
	char xdgpath[max];

	if(!home)
		return -1;

#if defined(CONF_PLATFORM_MACOSX)
	str_format(path, max, "%s/Library/Application Support/%s", home, appname);
	return 0;
#endif

	/* old folder location */
	str_format(path, max, "%s/.%s", home, appname);
	for(i = strlen(home)+2; path[i]; i++)
		path[i] = tolower(path[i]);

	if(!xdgdatahome)
	{
		/* use default location */
		str_format(xdgpath, max, "%s/.local/share/%s", home, appname);
		for(i = strlen(home)+14; xdgpath[i]; i++)
			xdgpath[i] = tolower(xdgpath[i]);
	}
	else
	{
		str_format(xdgpath, max, "%s/%s", xdgdatahome, appname);
		for(i = strlen(xdgdatahome)+1; xdgpath[i]; i++)
			xdgpath[i] = tolower(xdgpath[i]);
	}

	/* check for old location / backward compatibility */
	if(fs_is_dir(path))
	{
		/* use old folder path */
		/* for backward compatibility */
		return 0;
	}

	str_format(path, max, "%s", xdgpath);

	return 0;
#endif
}

int fs_makedir(const char *path)
{
#if defined(CONF_FAMILY_WINDOWS)
	if(_mkdir(path) == 0)
			return 0;
	if(errno == EEXIST)
		return 0;
	return -1;
#else
	if(mkdir(path, 0755) == 0)
		return 0;
	if(errno == EEXIST)
		return 0;
	return -1;
#endif
}

int fs_makedir_recursive(const char *path)
{
	char buffer[2048];
	int len;
	int i;
	str_copy(buffer, path, sizeof(buffer));
	len = str_length(buffer);
	// ignore a leading slash
	for(i = 1; i < len; i++)
	{
		char b = buffer[i];
		if(b == '/' || (b == '\\' && buffer[i-1] != ':'))
		{
			buffer[i] = 0;
			if(fs_makedir(buffer) < 0)
			{
				return -1;
			}
			buffer[i] = b;

		}
	}
	return fs_makedir(path);
}

int fs_is_dir(const char *path)
{
#if defined(CONF_FAMILY_WINDOWS)
	/* TODO: do this smarter */
	WIN32_FIND_DATA finddata;
	HANDLE handle;
	char buffer[1024*2];
	str_format(buffer, sizeof(buffer), "%s/*", path);

	if ((handle = FindFirstFileA(buffer, &finddata)) == INVALID_HANDLE_VALUE)
		return 0;

	FindClose(handle);
	return 1;
#else
	struct stat sb;
	if (stat(path, &sb) == -1)
		return 0;

	if (S_ISDIR(sb.st_mode))
		return 1;
	else
		return 0;
#endif
}

int fs_chdir(const char *path)
{
	if(fs_is_dir(path))
	{
		if(chdir(path))
			return 1;
		else
			return 0;
	}
	else
		return 1;
}

char *fs_getcwd(char *buffer, int buffer_size)
{
	if(buffer == 0)
		return 0;
#if defined(CONF_FAMILY_WINDOWS)
	return _getcwd(buffer, buffer_size);
#else
	return getcwd(buffer, buffer_size);
#endif
}

int fs_parent_dir(char *path)
{
	char *parent = 0;
	for(; *path; ++path)
	{
		if(*path == '/' || *path == '\\')
			parent = path;
	}

	if(parent)
	{
		*parent = 0;
		return 0;
	}
	return 1;
}

int fs_remove(const char *filename)
{
	if(remove(filename) != 0)
		return 1;
	return 0;
}

int fs_rename(const char *oldname, const char *newname)
{
	if(rename(oldname, newname) != 0)
		return 1;
	return 0;
}

#if defined(__cplusplus)
}
#endif
