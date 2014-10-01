/*
 Copyright (c) 2014 Mutantspider authors, see AUTHORS file.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#include "mutantspider.h"

// caller sees the files they have requested in this location of the file system
std::string persistent_name = "/persistent";

#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

/*
 like "mkdir -p", create all subdirectories and ignore errors
 */
static void mkdir_p(const std::string& path)
{
    size_t pos = 0;
    std::string dir("/");
    while (pos != path.npos) {
        pos = path.find(dir,pos+1);
        std::string sp = path.substr(0,pos);
        struct stat st;
        if (stat(sp.c_str(), &st) == -1)
        {
            if (mkdir(sp.c_str(),0777) != 0)
                fprintf(stderr, "mkdir(\"%s\") failed, errno: %d\n", sp.c_str(), errno);
        }
    }
}

#if defined(__native_client__)

#include <sys/mount.h>
#include <nacl_io/nacl_io.h>
#include <nacl_io/fuse.h>
#include <future>
#include <list>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <ppapi/c/pp_macros.h>

namespace {

// where we mount the html5fs file system
std::string html5_shadow_name = "/.html5fs_shadow";

// the file data itself for /persistent is actually mounted here.
// our fuse-based code supporting the /persistent directory is a
// redirect to this location, and then a background thread mirrors
// everything to /.html5fs_shadow
std::string mem_shadow_name = "/.memfs_shadow";

// data structures we use to coordinate tasks on the background thread
std::list<std::pair<void (*)(void*), void*> >   pbmemfs_task_list;
std::mutex                                      pbmemfs_mtx;
std::condition_variable                         pbmemfs_cnd;

// thread proc that runs forever, waiting for new "tasks"
// to show up in the pbmemfs_task_list.  It executes them
// when it gets them.
void pbmemfs_worker()
{
    bool printed_sleep = true;
    while (true)
    {
        std::pair<void (*)(void*), void*> task;
        {
            std::unique_lock<std::mutex> lk(pbmemfs_mtx);
#if 1
            pbmemfs_cnd.wait(lk, []{return !pbmemfs_task_list.empty();});
#else
            while (pbmemfs_task_list.empty())
            {
                if (!printed_sleep)
                {
                    printed_sleep = true;
                    printf("no more tasks waiting, sleeping\n");
                }
                pbmemfs_cnd.wait(lk);
            }
            
            if (printed_sleep)
            {
                printed_sleep = false;
                printf("tasks queued, working on them\n");
            }
#endif
            task = pbmemfs_task_list.front();
            pbmemfs_task_list.pop_front();
        }
        task.first(task.second);
    }
}

// given an arbitrary callable function 'f', along with an arbitrary
// list of (copyable) arguments, add a task that will execute
// "f(args...)", to pbmemfs_task_list and then signal pbmemfs_worker
// to pick up and execute that task.
//
// for example:
//
//    bkg_call(foo,100,j);
//
// causes "foo(100,j)" to execute in the background thread
//
template<typename F, typename ...Args>
auto bkg_call(F&& f, Args&&... args) -> typename std::result_of<F (Args...)>::type
{
    auto b = std::bind(f, std::forward<Args>(args)...);
    using function_type = decltype(b);
    auto p = new function_type(b);
    
    std::unique_lock<std::mutex> lk(pbmemfs_mtx);
    pbmemfs_task_list.push_back(std::make_pair<void (*)(void*), void*>(
        [] (void* _f)
        {
            function_type* f = static_cast<function_type*>(_f);
            (*f)();
            delete f;
        }, p)
                            );
    pbmemfs_cnd.notify_one();
}

// simple data structure for when we need to keep track
// of both the file descriptor in /.memfs_shadow as well
// as the one in /.html5fs_shadow
struct file_ref
{
    int	memfs_fd_;
    int html5fs_fd_;
    
    file_ref(int memfs_fd)
        : memfs_fd_(memfs_fd),
          html5fs_fd_(-1)
    {}
};

// set the 'fh' field of finfo.  If the file is writable
// then we use an allocated datastructure (file_ref) to keep
// track of both the file descriptor in /.memfs_shadow as well
// as /.html5fs_shadow.  Otherwise we just keep track of the
// the one in /.memfs_shadow
bool set_fh(struct fuse_file_info* finfo, int flags, int fd)
{
    if ((flags & O_ACCMODE) != O_RDONLY)
    {
        // it is possible that it will be written to
        auto fr = new file_ref(fd);
        finfo->fh = reinterpret_cast<decltype(finfo->fh)>(fr);
        return true;
    }
    else
    {
        // malloc'ed pointers can't have 1 in the low bit, so use that
        // to distinguish between the two cases.
        finfo->fh = (fd << 1) | 1;
        return false;
    }
}

// get the file_ref if there is one (the file is writable)
// or 0 if not.
file_ref* get_fr(struct fuse_file_info* finfo)
{
    if (finfo->fh & 1)
        return 0;
    return reinterpret_cast<file_ref*>(finfo->fh);
}

// get the primary file descriptor (for the file in
// in /.memfs_shadow)
int get_fd(struct fuse_file_info* finfo)
{
    auto fr = get_fr(finfo);
    return fr ? fr->memfs_fd_ : finfo->fh >> 1;
}
    
///////////////////////////////////////////////////////////

// Called when a filesystem of this type is initialized.
void* pbmemfs_init(struct fuse_conn_info* conn)
{
    return 0;
}

// Called when a filesystem of this type is unmounted.
void pbmemfs_destroy(void* p)
{
}

// Called by access()
int pbmemfs_access(const char* path, int mode)
{
    return access((mem_shadow_name + path).c_str(),mode);
}

// Called when O_CREAT is passed to open()
int pbmemfs_create(const char* _path, mode_t mode, struct fuse_file_info* finfo)
{
    std::string path(_path);
    int fd = open((mem_shadow_name + path).c_str(),finfo->flags,mode);
    if (fd >= 0)
    {
        set_fh(finfo, finfo->flags, fd);
        bkg_call([](std::string path, int flags, mode_t mode, file_ref* fr)
            {
                int fd = open(path.c_str(), flags, mode);
                if (fd >= 0)
                {
                    // fr will be null if the caller called open(path, O_RDONLY | O_CREAT, mode);
                    if (fr)
                        fr->html5fs_fd_ = fd;
                    else
                        close(fd);
                }
                else
                    fprintf(stderr, "open(%s, %o, %o) failed with errno: %d\n", path.c_str(), (int)flags, (int)mode, errno);
            },
            html5_shadow_name + path, finfo->flags, mode,get_fr(finfo));
        return 0;
    }
    return -errno;
}

// Called by stat()/fstat(), but only when fuse_operations.fgetattr is NULL.
// Also called by open() to determine if the path is a directory or a regular
// file.
int pbmemfs_getattr(const char* path, struct stat* st)
{
    if (stat((mem_shadow_name + path).c_str(), st) == 0)
        return 0;
    return -errno;
}

// Called by fstat()
int pbmemfs_fgetattr(const char* path, struct stat* st, struct fuse_file_info* finfo)
{
    if (finfo->fh != 0)
    {
        if (fstat(get_fd(finfo), st) == 0)
            return 0;
        return -errno;
    }
    else
        return pbmemfs_getattr(path, st);
}

// Called by fsync(). The datasync paramater is not currently supported.
int pbmemfs_fsync(const char* path, int datasync, struct fuse_file_info* finfo)
{
    // this can be a no-op for us, we always sync when
    // the io operation happens
    return 0;
}

// Called by ftruncate()
int pbmemfs_ftruncate(const char* _path, off_t pos, struct fuse_file_info* finfo)
{
    if (ftruncate(get_fd(finfo), pos) == 0)
    {
        bkg_call([](off_t pos, file_ref* fr)
            {
                if (ftruncate(fr->html5fs_fd_,pos))
                    fprintf(stderr, "ftruncate(%d, %d) failed with errno: %d\n", fr->html5fs_fd_, (int)pos, errno);
            },
            pos, get_fr(finfo));
        return 0;
    }
    return -errno;
}

// Called by mkdir()
int pbmemfs_mkdir(const char* _path, mode_t mode)
{
    std::string path(_path);
    if (mkdir((mem_shadow_name + path).c_str(), mode) == 0)
    {
        bkg_call([](std::string path, mode_t mode)
            {
                if (mkdir(path.c_str(), mode) != 0)
                    fprintf(stderr, "mkdir(%s, %d) failed with errno: %d\n", path.c_str(), (int)mode, errno);
            },
            html5_shadow_name + path, mode);
        return 0;
    }
    return -errno;
}

// Here is the comment in fuse.h from pepper35:
//
// Called when O_CREAT is passed to open(), but only if fuse_operations.create
// is non-NULL.
//
// But this comment is incorrect -- this is only called if fuse_operations.create
// _is_ NULL.  We set 'create' to pbmemfs_create, and so this will never be called.
int pbmemfs_mknod(const char* path, mode_t mode, dev_t dev)
{
    printf("shouldn't be here!!!  pbmemfs_mknod(\"%s\", %d, %d)\n", path, (int)mode, (int)dev);
    return -1;
}

// Called by open()
int pbmemfs_open(const char* _path, struct fuse_file_info* finfo)
{
    std::string path(_path);
    int fd = open((mem_shadow_name + path).c_str(),finfo->flags);
    if (fd >= 0)
    {
        if (set_fh(finfo, finfo->flags, fd))
            bkg_call([](std::string path, int flags, file_ref* fr)
                {
                    int fd = open(path.c_str(), flags);
                    if (fd >= 0)
                        fr->html5fs_fd_ = fd;
                    else
                        fprintf(stderr, "open(%s, %o) failed with errno: %d\n", path.c_str(), (int)flags, errno);
                },
                html5_shadow_name + path, finfo->flags, get_fr(finfo));

        return 0;
    }
    return -errno;
}

// Called by getdents(), which is called by the more standard functions
// opendir()/readdir().  NaCl's fuse implementation calls our pbmemfs_readdir
// once for each file/dir being enumerated.  So we just call opendir
// here, keep the DIR*, and then call readdir (once) in our pbmemfs_readdir.
// Unfortunately, NaCl calls this function (pbmemfs_opendir) once for each
// file/dir too, so we test to see whether we have already done the opendir
// step.
int pbmemfs_opendir(const char* path, struct fuse_file_info* finfo)
{
    if (finfo->fh == 0)
        finfo->fh = reinterpret_cast<decltype(finfo->fh)>(opendir((mem_shadow_name + path).c_str()));
    return 0;
}

// Called by read(). Note that FUSE specifies that all reads will fill the
// entire requested buffer. If this function returns less than that, the
// remainder of the buffer is zeroed.
int pbmemfs_read(const char* path, char* buf, size_t count, off_t pos,
             struct fuse_file_info* finfo)
{
    size_t bytesRead = 0;
    while (bytesRead < count)
    {
        auto bytes = pread(get_fd(finfo), &buf[bytesRead], count - bytesRead, pos + bytesRead);
        if (bytes == 0)
            return bytesRead;
        if (bytes < 0)
            return -errno;
        bytesRead += bytes;
    }
    return bytesRead;
}

// (big, long comment from fuse.h omitted)
int pbmemfs_readdir(const char* path, void* buf, fuse_fill_dir_t filldir, off_t pos,
                struct fuse_file_info* finfo)
{
    DIR* dir = (DIR*)finfo->fh; // see pbmemfs_opendir
    
    struct dirent *ent = readdir(dir);
    if (ent)
    {
        struct stat st;
        if (stat((mem_shadow_name + path + "/" + ent->d_name).c_str(),&st) == 0)
            (*filldir)(buf, ent->d_name, &st, pos);
    }
    
    return 0;
}

// Called when the last reference to this node is released. This is only
// called for regular files. For directories, fuse_operations.releasedir is
// called instead.
int pbmemfs_release(const char* path, struct fuse_file_info* finfo)
{
    if (close(get_fd(finfo)) == 0)
    {
        file_ref* fr = get_fr(finfo);
        if (fr)
            bkg_call([](file_ref* fr)
                {
                    if (close(fr->html5fs_fd_) != 0)
                        fprintf(stderr, "close(%d) failed, errno: %d\n", fr->html5fs_fd_, errno);
                    delete fr;
                },
                fr);
        return 0;
    }
    return -errno;
}

// Called when the last reference to this node is released. This is only
// called for directories. For regular files, fuse_operations.release is
// called instead.
int pbmemfs_releasedir(const char* path, struct fuse_file_info* finfo)
{
    // see pbmemfs_opendir
    if (finfo->fh)
    {
        closedir((DIR*)finfo->fh);
        finfo->fh = 0;
    }
    
    return 0;
}

// Called by rename()
int pbmemfs_rename(const char* _path, const char* _new_path)
{
    std::string path(_path);
    std::string new_path(_new_path);
    
    if (rename((mem_shadow_name + path).c_str(), (mem_shadow_name + new_path).c_str()) == 0)
    {
        bkg_call([](std::string path, std::string new_path)
            {
                if (rename(path.c_str(), new_path.c_str()) != 0)
                    fprintf(stderr, "rename(%s, %s) failed with errno: %d\n", path.c_str(), new_path.c_str(), errno);
            },
            html5_shadow_name + path, html5_shadow_name + new_path);
        return 0;
    }
    return -errno;
}

#if PPAPI_RELEASE >= 39
// called by utime()/utimes()/futimes()/futimens() etc
int pbmemfs_utimens(const char* _path, const struct timespec _tv[2])
{
    std::string path(_path);
    struct timeval tv[2];
    if (_tv != 0)
    {
        tv[0].tv_sec  = _tv[0].tv_sec;
        tv[0].tv_usec = _tv[0].tv_nsec / 1000;
        tv[1].tv_sec  = _tv[1].tv_sec;
        tv[1].tv_usec = _tv[1].tv_nsec / 1000;
    }
    auto tvp = &tv;
    if (!_tv)
        tvp = 0;
    
    if (utimes((mem_shadow_name + path).c_str(), *tvp) == 0)
    {
        bkg_call([](std::string path, const struct timeval tv[2])
            {
                if (utimes(path.c_str(), tv) != 0)
                    fprintf(stderr, "utimes(%s, (timespec)) failed with errno: %d\n", path.c_str(), errno);
            },
            html5_shadow_name + path, *tvp);
        return 0;
    }
    
    return -errno;
}

int pbmemfs_chmod(const char* _path, mode_t mode)
{
    std::string path(_path);
    if (chmod((mem_shadow_name + path).c_str(), mode) == 0)
    {
        bkg_call([](std::string path, mode_t mode)
            {
                if (chmod(path.c_str(), mode) != 0)
                    fprintf(stderr, "chmod(%s, 0%o) failed with errno: %d\n", path.c_str(), mode, errno);
            },
            html5_shadow_name + path, mode);
        return 0;
    }
    return -errno;
}
#endif

// Called by rmdir()
int pbmemfs_rmdir(const char* _path)
{
    std::string path(_path);
    if (rmdir((mem_shadow_name + path).c_str()) == 0)
    {
        bkg_call([](std::string path)
            {
                if (rmdir(path.c_str()) != 0)
                    fprintf(stderr, "rmdir(\"%s\") failed with errno: %d\n", path.c_str(), errno);
            },
            html5_shadow_name + path);
        return 0;
    }
    return -errno;
}

// Called by truncate(), as well as open() when O_TRUNC is passed.
int pbmemfs_truncate(const char* _path, off_t pos)
{
    std::string	path(_path);
    if (truncate((mem_shadow_name + path).c_str(),pos) == 0)
    {
        bkg_call([](std::string path, off_t pos)
            {
                if (truncate(path.c_str(),pos) != 0)
                    fprintf(stderr, "truncate(%s, %d) failed with errno: %d\n", path.c_str(), (int)pos, errno);
            },
            html5_shadow_name + path, pos);
        return 0;
    }
    return -errno;
}

// Called by unlink()
int pbmemfs_unlink(const char* _path)
{
    std::string path(_path);
    if (unlink((mem_shadow_name + path).c_str()) == 0)
    {
        bkg_call([](std::string path)
            {
                if (unlink(path.c_str()) != 0)
                    fprintf(stderr, "unlink(%s) failed with errno: %d\n", path.c_str(), errno);
            },
            html5_shadow_name + path);
        return 0;
    }
    return -errno;
}

// Called by write(). Note that FUSE specifies that a write should always
// return the full count, unless an error occurs.
int pbmemfs_write(const char* path, const char* buf, size_t count, off_t pos,
              struct fuse_file_info* finfo)
{
    int ret = pwrite(get_fd(finfo), buf, count, pos);
    if (ret != -1)
        bkg_call([](file_ref* fr, std::vector<char> buf, off_t pos)
            {
                int ret;
                if ((ret = pwrite(fr->html5fs_fd_, &buf.front(), buf.size(), pos)) != buf.size())
                    fprintf(stderr, "pwrite(%d, %p, %d, %d) returned unexpected value (%d instead of %d), errno: %d\n",
                            fr->html5fs_fd_, &buf.front(), (int)buf.size(), (int)pos, ret, (int)buf.size(), errno);
            },
            get_fr(finfo), std::vector<char>(buf,&buf[ret]), pos);
    return ret;
}

// the data structure we give to fuse
struct fuse_operations pbmemfs_ops = {

    0,
    0,

#if PPAPI_RELEASE < 39
    
    pbmemfs_init,
    pbmemfs_destroy,
    pbmemfs_access,
    pbmemfs_create,
    pbmemfs_fgetattr,
    pbmemfs_fsync,
    pbmemfs_ftruncate,
    pbmemfs_getattr,
    pbmemfs_mkdir,
    pbmemfs_mknod,
    pbmemfs_open,
    pbmemfs_opendir,
    pbmemfs_read,
    pbmemfs_readdir,
    pbmemfs_release,
    pbmemfs_releasedir,
    pbmemfs_rename,
    pbmemfs_rmdir,
    pbmemfs_truncate,
    pbmemfs_unlink,
    pbmemfs_write
    
#else

    pbmemfs_getattr,
    0,                  // readlink
    pbmemfs_mknod,
    pbmemfs_mkdir,
    pbmemfs_unlink,
    pbmemfs_rmdir,
    0,                  // symlink
    pbmemfs_rename,
    0,                  // link
    pbmemfs_chmod,
    0,                  // chown
    pbmemfs_truncate,
    pbmemfs_open,
    pbmemfs_read,
    pbmemfs_write,
    0,                  // statfs
    0,                  // flush
    pbmemfs_release,
    pbmemfs_fsync,
    0,                  // setxattr
    0,                  // getxattr
    0,                  // listxattr
    0,                  // removexattr
    pbmemfs_opendir,
    pbmemfs_readdir,
    pbmemfs_releasedir,
    0,                  // fsyncdir
    pbmemfs_init,
    pbmemfs_destroy,
    pbmemfs_access,
    pbmemfs_create,
    pbmemfs_ftruncate,
    pbmemfs_fgetattr,
    0,                  // lock
    pbmemfs_utimens

#endif

};

// copy the contents of 'from' to 'to'
void file_cp(const char *to, const char *from)
{
    auto from_f = open(from, O_RDONLY);
    if (from_f == -1)
    {
        fprintf(stderr, "open(\"%s\", O_RDONLY) failed with errno: %d\n", from, errno);
        return;
    }
    
    auto to_f = open(to, O_CREAT | O_WRONLY, 0666);
    if (to_f == -1)
    {
        fprintf(stderr, "open(\"%s\", O_CREAT | O_WRONLY) failed with errno: %d\n", to, errno);
        close(from_f);
        return;
    }
    
    while (true)
    {
        char buf[4096];
        auto nread = read(from_f, buf, sizeof(buf));
        if (nread == 0)
            break;
        if (nread == -1)
        {
            fprintf(stderr, "read(%d, %p, %d) failed with errno: %d\n", from_f, buf, (int)sizeof(buf), errno);
            return;
        }
        int written = 0;
        while (written < nread)
        {
            auto bytes = write(to_f, &buf[written], nread - written);
            if (bytes == -1)
            {
                fprintf(stderr, "write(%d, %p, %d) failed with errno: %d\n", to_f, &buf[written], nread-written, errno);
                return;
            }
            written += bytes;
        }
    }
    
    close(to_f);
    close(from_f);
    
    struct stat st;
    if (stat(from, &st) != 0)
    {
        fprintf(stderr, "stat(%s, %p) failed with errno: %d\n", from, &st, errno);
        return;
    }
    if ((st.st_mode & 0777) != 0666)
    {
        if (chmod(to, st.st_mode & 0777) != 0)
            fprintf(stderr, "chmod(%s, 0%o) failed with errno: %d\n", to, (int)(st.st_mode & O_ACCMODE), errno);
    }
}

// copy the contents of /.html5fs_shadow/dirName to /.memfs_shadow/dirName (recursively)
void do_sync(const std::string& dirName)
{
    std::string html5_dir = html5_shadow_name + "/" + dirName;
    std::string mem_dir = mem_shadow_name + "/" + dirName;
    
    // walk the html5 directory.
    DIR	*dir;
    if ((dir = opendir(html5_dir.c_str())) != 0)
    {
        struct dirent *ent;
        while ((ent = readdir(dir)) != 0)
        {
            if (strcmp(ent->d_name,".") && strcmp(ent->d_name,".."))
            {
                std::string html5_path = html5_dir + "/" + ent->d_name;
                std::string mem_path = mem_dir + "/" + ent->d_name;
                struct stat st;
                if (stat(html5_path.c_str(),&st) == 0)
                {
                    if (S_ISDIR(st.st_mode))
                    {
                        mkdir(mem_path.c_str(),0777);
                        do_sync(dirName + "/" + ent->d_name);
                    }
                    else
                    {
                        // copy the contents
                        file_cp(mem_path.c_str(),html5_path.c_str());
                    }
                }
            }
        }
        closedir(dir);
    }
}

// assumes that the target directory is currently empty, and
// duplicates the entire directory structure under /.html5fs_shadow/...
// to /.memfs_shadow/...  We run this in a background thread because
// /.html5fs_shadow is one of nacl's html5fs mounts, which can
// only be read from (or written to) from a non-main thread (while the
// main thread is not blocked, waiting for this to complete)
void populate_memfs(MS_AppInstance* inst, std::vector<std::string> persistent_dirs)
{
    for (auto dir : persistent_dirs)
    {
        mkdir_p(html5_shadow_name + "/" + dir);
        mkdir_p(mem_shadow_name + "/" + dir);
        do_sync(dir);
    }
   
    inst->AsyncStartupComplete();
    inst->PostCommand("async_startup_complete:");
    
    pbmemfs_worker();   // note, this never returns
}

//#define _do_clear_

#if defined(_do_clear_)
void print_indents(int numIndents)
{
    for (int i = 0; i < numIndents; i++)
        printf("    ");
}

void clear_all_r(const std::string& dir_name, int numIndents)
{
    print_indents(numIndents);
    printf("clear_all_r(\"%s\")\n", dir_name.c_str());
    DIR	*dir;
    bool unlinked;
    do {
        unlinked = false;
        if ((dir = opendir(dir_name.c_str())) != 0)
        {
            struct dirent *ent;
            while ((ent = readdir(dir)) != 0)
            {
                if (strcmp(ent->d_name,".") && strcmp(ent->d_name,".."))
                {
                    std::string path = dir_name + "/" + ent->d_name;
                    print_indents(numIndents);
                    printf("found file/path %s\n", path.c_str());
                    struct stat st;
                    if (stat(path.c_str(),&st) == 0)
                    {
                        if (S_ISDIR(st.st_mode))
                        {
                            unlinked = true;
                            clear_all_r(path, numIndents+1);
                            int ret = rmdir(path.c_str());
                            print_indents(numIndents);
                            printf("rmdir(%s) returned %d, errno: %d\n", path.c_str(), ret, errno);
                        }
                        else
                        {
                            unlinked = true;
                            int ret = unlink(path.c_str());
                            print_indents(numIndents);
                            printf("unlink(%s) returned %d, errno: %d\n", path.c_str(), ret, errno);
                        }
                    }
                }
            }
            closedir(dir);
        }
    } while (unlinked);
}

void clear_all()
{
    clear_all_r(html5_shadow_name, 0);
}
#endif

#if defined(MUTANTSPIDER_HAS_RESOURCES)

const mutantspider::rez_dir_ent* get_dir_ent(const std::string& path,const mutantspider::rez_dir* dir)
{
    // path always starts with a '/'
    
    auto pos = path.find("/",1);
    auto d_name = path.substr(1,pos == std::string::npos ? path.length() - 1 : pos - 1);
    for (size_t i = 0; i < dir->num_ents; i++)
    {
        if (d_name == dir->ents[i].d_name)
        {
            if (pos == std::string::npos)
                return &dir->ents[i];
            if (dir->ents[i].is_dir != 0)
                return get_dir_ent(path.substr(pos, path.length() - pos), dir->ents[i].ptr.dir);
            return 0;
        }
    }
    return 0;
}

const mutantspider::rez_dir_ent* get_dir_ent(const std::string& path)
{
    return path == "/" ? &mutantspider::rez_root_dir_ent : get_dir_ent(path, &mutantspider::rez_root_dir);
}

// Called when a filesystem of this type is initialized.
void* rezfs_init(struct fuse_conn_info* conn)
{
    return 0;
}

// Called when a filesystem of this type is unmounted.
void rezfs_destroy(void* p)
{
}

// Called by access()
int rezfs_access(const char* path, int mode)
{
    auto ent = get_dir_ent(path);
    if (!ent)
        return -ENOENT;
    if ((mode & O_ACCMODE) != O_RDONLY)
        return -EROFS;
    return 0;
}

// Called when O_CREAT is passed to open()
int rezfs_create(const char* _path, mode_t /*mode*/, struct fuse_file_info* finfo)
{
    return -EROFS;
}

int rezfs_setattr(const mutantspider::rez_dir_ent* ent, struct stat* st)
{
    memset(st, 0, sizeof(*st));
    st->st_nlink = 1;
    if (ent->is_dir)
        st->st_mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;
    else
    {
        st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
        st->st_size = ent->ptr.file->file_data_sz;
    }
    return 0;
}

// Called by stat()/fstat(), but only when fuse_operations.fgetattr is NULL.
// Also called by open() to determine if the path is a directory or a regular
// file.
int rezfs_getattr(const char* path, struct stat* st)
{
    auto ent = get_dir_ent(path);
    if (ent)
        return rezfs_setattr(ent, st);
    else
        return -ENOENT;
}

// Called by fstat()
int rezfs_fgetattr(const char* path, struct stat* st, struct fuse_file_info* finfo)
{
    if (finfo->fh)
        return rezfs_setattr(reinterpret_cast<const mutantspider::rez_dir_ent*>(finfo->fh), st);
    else
        return rezfs_getattr(path, st);
}

// Called by fsync(). The datasync paramater is not currently supported.
int rezfs_fsync(const char* path, int datasync, struct fuse_file_info* finfo)
{
    return 0;
}

// Called by ftruncate()
int rezfs_ftruncate(const char* _path, off_t pos, struct fuse_file_info* finfo)
{
    return -EROFS;
}

// Called by mkdir()
int rezfs_mkdir(const char* _path, mode_t mode)
{
    return -EROFS;
}

// Here is the comment in fuse.h from pepper35:
//
// Called when O_CREAT is passed to open(), but only if fuse_operations.create
// is non-NULL.
//
// But this comment is incorrect -- this is only called if fuse_operations.create
// _is_ NULL.  We set 'create' to rezfs_create, and so this will never be called.
int rezfs_mknod(const char* path, mode_t mode, dev_t dev)
{
    return -EROFS;
}

// Called by open()
int rezfs_open(const char* path, struct fuse_file_info* finfo)
{
    auto ent = get_dir_ent(path);
    if (ent)
    {
        if ((finfo->flags & O_ACCMODE) != O_RDONLY)
            return -EROFS;
        finfo->fh = reinterpret_cast<decltype(finfo->fh)>(ent);
        return 0;
    }
    else
        return -ENOENT;
}

struct rez_dir_iter
{
    rez_dir_iter(const mutantspider::rez_dir* dir) : dir_(dir), index_(-2) {}
    
    const mutantspider::rez_dir*    dir_;
    int                             index_;
};
    

// Called by getdents(), which is called by the more standard functions
// opendir()/readdir().  NaCl's fuse implementation calls our rezfs_readdir
// once for each file/dir being enumerated.
int rezfs_opendir(const char* path, struct fuse_file_info* finfo)
{
    if (finfo->fh == 0)
    {
        auto ent = get_dir_ent(path);
        if (ent)
        {
            if (ent->is_dir)
            {
                finfo->fh = reinterpret_cast<decltype(finfo->fh)>(new rez_dir_iter(ent->ptr.dir));
                return 0;
            }
            return -ENOTDIR;
        }
        return -ENOENT;
    }
    return 0;
}

// Called by read(). Note that FUSE specifies that all reads will fill the
// entire requested buffer. If this function returns less than that, the
// remainder of the buffer is zeroed.
int rezfs_read(const char* path, char* buf, size_t count, off_t pos,
             struct fuse_file_info* finfo)
{
    const mutantspider::rez_dir_ent* ent = reinterpret_cast<const mutantspider::rez_dir_ent*>(finfo->fh);
    if ((size_t)pos > ent->ptr.file->file_data_sz)
        pos = (off_t)ent->ptr.file->file_data_sz;
    size_t mx_count = ent->ptr.file->file_data_sz - (size_t)pos;
    if (count > mx_count)
        count = mx_count;
    memcpy(buf, &ent->ptr.file->file_data[pos], count);
    return (int)count;
}

// (big, long comment from fuse.h omitted)
int rezfs_readdir(const char* path, void* buf, fuse_fill_dir_t filldir, off_t pos,
                struct fuse_file_info* finfo)
{
    rez_dir_iter* it = reinterpret_cast<rez_dir_iter*>(finfo->fh);
    if (it->index_ < (int)it->dir_->num_ents)
    {
        bool        is_dir;
        const char* d_name;
        
        struct stat st;
        memset(&st, 0, sizeof(st));
        
        switch(it->index_)
        {
            case -2:
                is_dir = true;
                d_name = ".";
                break;
            case -1:
                is_dir = true;
                d_name = "..";
                break;
            default:
            {
                auto ent = &it->dir_->ents[it->index_];
                is_dir = ent->is_dir;
                if (!is_dir)
                    st.st_size = ent->ptr.file->file_data_sz;
                d_name = ent->d_name;
            }   break;
        }
        
        st.st_ino = it->index_ + 3;
        st.st_nlink = 1;
        if (is_dir)
            st.st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        else
            st.st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        
        ++it->index_;
        (*filldir)(buf, d_name, &st, pos);
    }
    return 0;
}

// Called when the last reference to this node is released. This is only
// called for regular files. For directories, fuse_operations.releasedir is
// called instead.
int rezfs_release(const char* path, struct fuse_file_info* finfo)
{
    finfo->fh = 0;
    return 0;
}

// Called when the last reference to this node is released. This is only
// called for directories. For regular files, fuse_operations.release is
// called instead.
int rezfs_releasedir(const char* path, struct fuse_file_info* finfo)
{
    delete reinterpret_cast<rez_dir_iter*>(finfo->fh);
    return 0;
}

// Called by rename()
int rezfs_rename(const char* _path, const char* _new_path)
{
    return -EROFS;
}

#if PPAPI_RELEASE >= 39
// called by utime()/utimes()/futimes()/futimens() etc
int rezfs_utimens(const char* _path, const struct timespec _tv[2])
{
    return -EROFS;
}

int rezfs_chmod(const char* _path, mode_t mode)
{
    return -EROFS;
}
#endif


// Called by rmdir()
int rezfs_rmdir(const char* _path)
{
    return -EROFS;
}

// Called by truncate(), as well as open() when O_TRUNC is passed.
int rezfs_truncate(const char* _path, off_t pos)
{
    return -EROFS;
}

// Called by unlink()
int rezfs_unlink(const char* _path)
{
    return -EROFS;
}

// Called by write(). Note that FUSE specifies that a write should always
// return the full count, unless an error occurs.
int rezfs_write(const char* path, const char* buf, size_t count, off_t pos,
              struct fuse_file_info* finfo)
{
    return -EBADF;
}

// the data structure we give to fuse
struct fuse_operations rezfs_ops = {
    
    0,
    0,

#if PPAPI_RELEASE < 39
    
    rezfs_init,
    rezfs_destroy,
    rezfs_access,
    rezfs_create,
    rezfs_fgetattr,
    rezfs_fsync,
    rezfs_ftruncate,
    rezfs_getattr,
    rezfs_mkdir,
    rezfs_mknod,
    rezfs_open,
    rezfs_opendir,
    rezfs_read,
    rezfs_readdir,
    rezfs_release,
    rezfs_releasedir,
    rezfs_rename,
    rezfs_rmdir,
    rezfs_truncate,
    rezfs_unlink,
    rezfs_write
    
#else

    rezfs_getattr,
    0,                  // readlink
    rezfs_mknod,
    rezfs_mkdir,
    rezfs_unlink,
    rezfs_rmdir,
    0,                  // symlink
    rezfs_rename,
    0,                  // link
    rezfs_chmod,
    0,                  // chown
    rezfs_truncate,
    rezfs_open,
    rezfs_read,
    rezfs_write,
    0,                  // statfs
    0,                  // flush
    rezfs_release,
    rezfs_fsync,
    0,                  // setxattr
    0,                  // getxattr
    0,                  // listxattr
    0,                  // removexattr
    rezfs_opendir,
    rezfs_readdir,
    rezfs_releasedir,
    0,                  // fsyncdir
    rezfs_init,
    rezfs_destroy,
    rezfs_access,
    rezfs_create,
    rezfs_ftruncate,
    rezfs_fgetattr,
    0,                  // lock
    rezfs_utimens

#endif

};

#endif

}

namespace mutantspider
{
    
void init_fs(MS_AppInstance* inst, const std::vector<std::string>& persistent_dirs)
{
    nacl_io_init_ppapi(inst->pp_instance(), pp::Module::Get()->get_browser_interface());
    
    umount("/");
    mount("", "/", "memfs", 0, "");
    
    #if defined(MUTANTSPIDER_HAS_RESOURCES)
    nacl_io_register_fs_type("rez_fs", &rezfs_ops);
    mount("", "/resources", "rez_fs", 0, "");
    #endif
    
    if (persistent_dirs.empty())
    {
        inst->AsyncStartupComplete();
        inst->PostCommand("async_startup_complete:");
    }
    else
    {
        nacl_io_register_fs_type("persist_backed_mem_fs", &pbmemfs_ops);
        
        mount("", html5_shadow_name.c_str(), "html5fs", 0, "type=PERSISTENT,expected_size=1048576");
        
        #if defined(_do_clear_)
        std::thread(clear_all).detach();
        return;
        #endif
        
        mount("", persistent_name.c_str(), "persist_backed_mem_fs", 0, "");
        
        std::thread(std::bind(populate_memfs,inst,persistent_dirs)).detach();
    }
}

// end of namespace mutantspider
}

// #if defined(__native_client__)
#endif

#if defined(EMSCRIPTEN)

namespace mutantspider
{
    
void init_fs(MS_AppInstance* inst, const std::vector<std::string>& persistent_dirs)
{
    #if defined(MUTANTSPIDER_HAS_RESOURCES)
    mkdir("/resources", 0777);
    ms_rez_mount("/resources", &rez_root_dir);
    #endif

    if (persistent_dirs.empty())
    {
        inst->AsyncStartupComplete();
        inst->PostCommand("async_startup_complete:");
    }
    else
    {
        for (auto dir : persistent_dirs)
        {
            std::string path = persistent_name + "/" + dir;
            mkdir_p(path);
            ms_persist_mount(path.c_str());
        }
        ms_syncfs_from_persistent();
    }
}

// end of namespace mutantspider
}

// #if defined(EMSCRIPTEN)
#endif
