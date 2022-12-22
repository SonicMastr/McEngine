#ifdef __vita__

#include "VitaSDLEnvironment.h"

#ifdef MCENGINE_FEATURE_SDL

#include "ConVar.h"

#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>

#include <unistd.h>

VitaSDLEnvironment::VitaSDLEnvironment() : SDLEnvironment(NULL)
{

}

Environment::OS VitaSDLEnvironment::getOS()
{
	return Environment::OS::OS_VITA;
}

void VitaSDLEnvironment::sleep(unsigned int us)
{
	if (us) // sceKernelDelayThread causes errors on Devkits, but not retail (kinda need this for debugging)
		usleep(us);
}

void VitaSDLEnvironment::openURLInDefaultBrowser(UString url)
{
	// Gonna try to do this
}

UString VitaSDLEnvironment::getUsername()
{
	UString uUsername = convar->getConVarByName("name")->getString();

	uUsername = UString("Guest");

	return uUsername;
}

bool VitaSDLEnvironment::directoryExists(UString directoryName)
{
	DIR *dir = opendir(directoryName.toUtf8());
	if (dir)
	{
		closedir(dir);
		return true;
	}
	else if (ENOENT == errno) // not a directory
	{
	}
	else // something else broke
	{
	}
	return false;
}

bool VitaSDLEnvironment::createDirectory(UString directoryName)
{
	return mkdir(directoryName.toUtf8(), DEFFILEMODE) != -1;
}

std::vector<UString> VitaSDLEnvironment::getFilesInFolder(UString folder)
{
	std::vector<UString> files;

	DIR *dir;
	struct dirent *ent;

	dir = opendir(folder.toUtf8());
	if (dir == NULL)
		return files;

	while ((ent = readdir(dir)))
	{
		const char *name = ent->d_name;
		UString uName = UString(name);
		UString fullName = folder;
		fullName.append(uName);

		struct stat stDirInfo;
		int statret = stat(fullName.toUtf8(), &stDirInfo); // NOTE: lstat() always returns 0 in st_mode, seems broken, therefore using stat() for now
		if (statret < 0)
		{
			///perror (name);
			///debugLog("VitaSDLEnvironment::getFilesInFolder() error, stat() returned %i!\n", lstatret);
			continue;
		}

		if (!S_ISDIR(stDirInfo.st_mode))
			files.push_back(uName);
	}
	closedir(dir);

	return files;
}

std::vector<UString> VitaSDLEnvironment::getFoldersInFolder(UString folder)
{
	std::vector<UString> folders;

	DIR *dir;
	struct dirent *ent;

	dir = opendir(folder.toUtf8());
	if (dir == NULL)
		return folders;

	while ((ent = readdir(dir)))
	{
		const char *name = ent->d_name;
		UString uName = UString(name);
		UString fullName = folder;
		fullName.append(uName);

		struct stat stDirInfo;
		int statret = stat(fullName.toUtf8(), &stDirInfo); // NOTE: lstat() always returns 0 in st_mode, seems broken, therefore using stat() for now
		if (statret < 0)
		{
			///perror (name);
			///debugLog("VitaSDLEnvironment::getFoldersInFolder() error, stat() returned %i!\n", lstatret);
			continue;
		}

		if (S_ISDIR(stDirInfo.st_mode))
			folders.push_back(uName);
	}
	closedir(dir);

	return folders;
}

std::vector<UString> VitaSDLEnvironment::getLogicalDrives()
{
	std::vector<UString> drives;

	drives.push_back("app0");
	drives.push_back("ux0");
	drives.push_back("ur0");

	return drives;
}

UString VitaSDLEnvironment::getFolderFromFilePath(UString filepath)
{
	if (directoryExists(filepath)) // indirect check if this is already a valid directory (and not a file)
		return filepath;
	else
		return UString(dirname((char*)filepath.toUtf8()));
}



// helper functions

int VitaSDLEnvironment::getFilesInFolderFilter(const struct dirent *entry)
{
	return 1;
}

int VitaSDLEnvironment::getFoldersInFolderFilter(const struct dirent *entry)
{
	return 1;
}

#endif

#endif
