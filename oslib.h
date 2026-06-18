
#if !defined(__nnoslib_header_h__)
#define __nnoslib_header_h__

#include <stdbool.h>
#include <time.h>

#if defined(__unix__) || defined(__linux__)
    #define OSFN_ISUNIXLIKE 1
#endif

#if defined(__unix__) || defined(__linux__)
    #define OSFN_ISLINUX
#elif defined(_WIN32) || defined(_WIN64)
    #define OSFN_ISWINNT
#endif

#if defined(OSFN_ISLINUX)
    #include <unistd.h>
    #include <dirent.h>
    #include <libgen.h>
    #include <sys/time.h>
#else
    #if defined(OSFN_ISWINNT)
        #include <windows.h>
    #endif
#endif

#if !defined(OSLIB_CONF_OSPATHSIZE)
    #define OSLIB_CONF_OSPATHSIZE 1024
#endif

#ifndef S_IREAD
    #define S_IREAD     0400
#endif /* S_IREAD */
#ifndef S_IWRITE
    #define S_IWRITE    0200
#endif /* S_IWRITE */
#ifndef S_IEXEC
    #define S_IEXEC     0100
#endif /* S_IEXEC */


#if !defined(S_IRUSR)
    #define S_IRUSR (S_IREAD)
#endif
#if !defined(S_IWUSR)
    #define S_IWUSR (S_IWRITE)
#endif
#if !defined(S_IXUSR)
    #define S_IXUSR (S_IEXEC)
#endif
#if !defined(S_IRGRP)
    #define S_IRGRP (S_IRUSR >> 3)
#endif
#if !defined(S_IWGRP)
    #define S_IWGRP (S_IWUSR >> 3)
#endif
#if !defined(S_IXGRP)
    #define S_IXGRP (S_IXUSR >> 3)
#endif
#if !defined(S_IROTH)
    #define S_IROTH (S_IRUSR >> 6)
#endif
#if !defined(S_IWOTH)
    #define S_IWOTH (S_IWUSR >> 6)
#endif
#if !defined(S_IXOTH)
    #define S_IXOTH (S_IXUSR >> 6)
#endif
#if !defined(S_IRWXU)
    #define S_IRWXU (S_IRUSR|S_IWUSR|S_IXUSR)
#endif
#if !defined(S_IRWXG)
    #define S_IRWXG (S_IRGRP|S_IWGRP|S_IXGRP)
#endif
#if !defined(S_IRWXO)
    #define S_IRWXO (S_IROTH|S_IWOTH|S_IXOTH)
#endif

#if !defined(S_IFLNK)
    #define S_IFLNK 0120000
#endif


#if !defined(S_IFMT)
    #define S_IFMT  00170000
#endif


#if !defined (S_ISDIR)
    #define	S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)	/* directory */
#endif

#if !defined (S_ISREG)
    #define	S_ISREG(m)	(((m)&S_IFMT) == S_IFREG)	/* file */
#endif

#if !defined(S_ISLNK)
    #define S_ISLNK(m)    (((m) & S_IFMT) == S_IFLNK)
#endif

#if !defined(DT_DIR)
    #define DT_DIR 4
#endif
#if !defined(DT_REG)
    #define DT_REG 8
#endif

#if !defined(PATH_MAX)
    #define PATH_MAX 1024
#endif


extern int kill(int, int);

typedef struct LitFSStat LitFSStat;
typedef struct FSDirReader FSDirReader;
typedef struct FSDirItem FSDirItem;

struct LitFSStat
{
    struct stat rawstbuf;
    int mode;
    int inode;
    int numlinks;
    int owneruid;
    int ownergid;
    const char* modename;
    bool isfile;
    size_t blocksize;
    size_t blockcount;
    size_t filesize;
    const time_t* tmlastchanged;
    const time_t* tmlastaccessed;
    const time_t* tmlastmodified;
};

struct FSDirReader
{
    #if defined(OSFN_ISWINNT)
        HANDLE handle;
        WIN32_FIND_DATA fdfile;
    #elif defined(OSFN_ISUNIXLIKE)
        DIR* handle;
    #endif
        
};

struct FSDirItem
{
    char name[OSLIB_CONF_OSPATHSIZE + 1];
    bool isdir;
    bool isfile;
};

static bool fslib_diropen(FSDirReader* rd, const char* path)
{
    #if defined(OSFN_ISUNIXLIKE)
        if((rd->handle = opendir(path)) == NULL)
        {
            return false;
        }
        return true;
    #else
        /*
        * windows' directory reading api expects a glob pattern.
        * i wish i was making this up!
        */
        enum { kExtra = 5 };
        bool b;
        size_t pslen;
        size_t buflen;
        char* winsillypath;
        b = false;
        pslen = strlen(path);
        buflen = (pslen + kExtra);
        winsillypath = (char*)malloc(buflen);
        if(winsillypath == NULL)
        {
            return false;
        }
        memset(winsillypath, 0, buflen);
        strcat(winsillypath, path);
        strcat(winsillypath, "\\*.*");
        fprintf(stderr, "sillypath=%s\n", winsillypath);
        rd->handle = FindFirstFile(winsillypath, &rd->fdfile);
        if(rd->handle != INVALID_HANDLE_VALUE)
        {
            b = true;
        }
        free(winsillypath);
        return b;
    #endif
}

static bool fslib_dirread(FSDirReader* rd, FSDirItem* itm)
{
    itm->isdir = false;
    itm->isfile = false;
    memset(itm->name, 0, OSLIB_CONF_OSPATHSIZE);
    #if defined(OSFN_ISUNIXLIKE)
        struct dirent* ent;
        if((ent = readdir((DIR*)(rd->handle))) == NULL)
        {
            return false;
        }
        if(ent->d_type == DT_DIR)
        {
            itm->isdir = true;
        }
        if(ent->d_type == DT_REG)
        {
            itm->isfile = true;
        }
        strcpy(itm->name, ent->d_name);
        return true;
    #else
        if(FindNextFile(rd->handle, &rd->fdfile))
        {
            if((rd->fdfile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                itm->isdir = true;
            }
            else
            {
                itm->isfile = true;
            }
            strcpy(itm->name, rd->fdfile.cFileName);
            return true;            
        }
    #endif
    return false;
}

static bool fslib_dirclose(FSDirReader* rd)
{
    #if defined(OSFN_ISUNIXLIKE)
        closedir(rd->handle);
    #else
        FindClose(rd->handle);
    #endif
    return false;
}

static const char* lit_util_filestatinternmodetoname(int t)
{
    switch(t)
    {
        #if defined(S_IFBLK)
            case S_IFBLK:
                return "blockdevice";
                break;
        #endif
        #if defined(S_IFCHR)
            case S_IFCHR:
                return "characterdevice";
                break;
        #endif
        #if defined(S_IFDIR)
            case S_IFDIR:
                return "directory";
                break;
        #endif
        #if defined(S_IFIFO)
            case S_IFIFO:
                return "pipe";
                break;
        #endif
        #if defined(S_IFLNK)
            case S_IFLNK:
                return "symlink";
                break;
        #endif
        #if defined(S_IFREG)
            case S_IFREG:
                return "file";
                break;
        #endif
        #if defined(S_IFSOCK)
            case S_IFSOCK:
                return "socket";
                break;
        #endif
            default:
                break;
    }
    return "unknown";
}


bool lit_filestat_initempty(LitFSStat* nfs)
{
    memset(nfs, 0, sizeof(LitFSStat));
    return true;
}

bool lit_filestat_setup(LitFSStat* nfs)
{
    nfs->inode = nfs->rawstbuf.st_ino;
    nfs->mode = (nfs->rawstbuf.st_mode & S_IFMT);
    nfs->numlinks = nfs->rawstbuf.st_nlink;
    nfs->owneruid = nfs->rawstbuf.st_uid;
    nfs->ownergid = nfs->rawstbuf.st_gid;
    #if !defined(_WIN32) && !defined(_WIN64)
        nfs->blocksize = nfs->rawstbuf.st_blksize;
        nfs->blockcount = nfs->rawstbuf.st_blocks;
    #else
        nfs->blocksize = 8;
        nfs->blockcount = (1024 * 4);
    #endif
    nfs->filesize = nfs->rawstbuf.st_size;
    nfs->modename = lit_util_filestatinternmodetoname(nfs->mode);
    nfs->tmlastchanged = (&nfs->rawstbuf.st_ctime);
    nfs->tmlastaccessed = (&nfs->rawstbuf.st_atime);
    nfs->tmlastmodified = (&nfs->rawstbuf.st_mtime);
    return true;
}


bool lit_filestat_initfrompath(LitFSStat* nfs, const char* path)
{
    if(!lit_filestat_initempty(nfs))
    {
        return false;
    }
    if(stat(path, &nfs->rawstbuf) == -1)
    {
        return false;
    }
    return lit_filestat_setup(nfs);
}



#endif
