
#include "mutantspider.h"

std::string persistent_name = "/persistent";

#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

static void mkdir_p(const std::string& path)
{
	size_t pos = 0;
	std::string dir("/");
	while (pos != path.npos) {
		pos = path.find(dir,pos+1);
		std::string sp = path.substr(0,pos);
		struct stat st;
		
		// as of pepper35, errno is not propagated well through
		// the fuse api.  So we don't check errno being ENOENT
		// the way one might expect here, and just assume that
		// any case of stat returning -1 implies that the underlying
		// reason is ENOENT
		
		if (stat(sp.c_str(), &st) == -1)
		{
			if (mkdir(sp.c_str(),0777) != 0)
				fprintf(stderr, "mkdir(\"%s\") failed, errno: %d\n", sp.c_str(), errno);
		}
	}
}

#if defined(__native_client__)

#include <pnacl/sys/mount.h>
#include <nacl_io/nacl_io.h>
#include <nacl_io/fuse.h>
#include <future>
#include <list>
#include <fcntl.h>
#include <unistd.h>


namespace {

std::string html5_shadow_name = "/.html5fs_shadow";
std::string mem_shadow_name = "/.memfs_shadow";

std::list<std::pair<void (*)(void*), void*> >	msf_task_list;
std::mutex										msf_mtx;
std::condition_variable							msf_cnd;

void fs_worker()
{
	while (true)
	{
		std::pair<void (*)(void*), void*> task;
		{
			std::unique_lock<std::mutex> lk(msf_mtx);
			msf_cnd.wait(lk, []{return !msf_task_list.empty();});
			task = msf_task_list.front();
			msf_task_list.pop_front();
		}
		task.first(task.second);
	}
}

template<typename F, typename ...Args>
auto do_call(F&& f, Args&&... args) -> typename std::result_of<F (Args...)>::type
{
	// note, that while we could have a special case for when this is called off the main thread,
	// so that it just executed the function, we wouldn't have a way to ensure thread correctness
	// if a caller used its own synchronization mechanism while accessing a file from multiple threads.
	// For now, we _always_ implement these calls using this task_list, which ensures that they
	// are executed in the order that the client expected.
	auto b = std::bind(f, std::forward<Args>(args)...);
	using function_type = decltype(b);
	auto p = new function_type(b);
	
	std::unique_lock<std::mutex> lk(msf_mtx);
	msf_task_list.push_back(std::make_pair<void (*)(void*), void*>([] (void* _f)
																		{
																			function_type* f = static_cast<function_type*>(_f);
																			(*f)();
																			delete f;
																		}, p));
	msf_cnd.notify_one();
}

struct file_ref
{
	int	memfs_fd_;
	int shadow_fd_;
	
	file_ref(int memfs_fd)
		: memfs_fd_(memfs_fd),
		  shadow_fd_(-1)
	{}
};

bool set_fh(struct fuse_file_info* finfo, mode_t mode, int fd)
{
	if ((mode & O_ACCMODE) != O_RDONLY)
	{
		// either O_WRONLY or O_RDWR, so it is possible that it will be written to
		auto fr = new file_ref(fd);
		finfo->fh = reinterpret_cast<decltype(finfo->fh)>(fr);
		return true;
	}
	else
	{
		finfo->fh = (fd << 1) | 1;	// malloc'ed pointers can't have 1 in the low bit, so use that to distinguish
		return false;
	}
}

file_ref* get_fr(struct fuse_file_info* finfo)
{
	if (finfo->fh & 1)
		return 0;
	return reinterpret_cast<file_ref*>(finfo->fh);
}

int get_fd(struct fuse_file_info* finfo)
{
	auto fr = get_fr(finfo);
	return fr ? fr->memfs_fd_ : finfo->fh >> 1;
}

// Called when a filesystem of this type is initialized.
void* msf_init(struct fuse_conn_info* conn)
{
	return 0;
}

// Called when a filesystem of this type is unmounted.
void msf_destroy(void* p)
{
	printf("msf_destroy(%p)\n", p);
}

// Called by access()
int msf_access(const char* path, int mode)
{
	return access((mem_shadow_name + path).c_str(),mode);
}

void _open(std::string path, mode_t mode, file_ref* fr)
{
	//printf("  _open(%s)\n", path.c_str());
	int ret = open(path.c_str(),mode);
	if (ret)
	{
		//printf("  _open succeeded, setting (%p)->shadow_fd_ to %d\n", fr, ret);
		fr->shadow_fd_ = ret;
	}
	else
		fprintf(stderr, "open(%s, %d) failed with errno: %d\n", path.c_str(), mode, errno);
}

// Called when O_CREAT is passed to open()
int msf_create(const char* _path, mode_t mode, struct fuse_file_info* finfo)
{
	//printf("msf_create(\"/persistent%s\"), finfo->flags: 0x%x\n", _path, finfo->flags);
	std::string path(_path);
	int ret = open((mem_shadow_name + path).c_str(),/*mode*/O_CREAT | O_RDWR);
	//printf("msf_create, open(%s, 0x%x) returned %d, errno: %d\n", (mem_shadow_name + path).c_str(), /*mode*/O_CREAT | O_RDWR, ret, errno);
	if (ret >= 0)
	{
		if (set_fh(finfo, mode, ret))
			do_call(_open,html5_shadow_name + path,/*mode*/O_CREAT | O_RDWR,get_fr(finfo));
	}
	return ret;
}

int msf_fgetattr(const char* path, struct stat* st, struct fuse_file_info* finfo)
{
	//printf("msf_fgetattr(\"/persistent%s\") calling fstat(%d)\n", path, get_fd(finfo));
	return fstat(get_fd(finfo), st);
}

// Called by fsync(). The datasync paramater is not currently supported.
int msf_fsync(const char* path, int datasync, struct fuse_file_info* finfo)
{
	return 0;	// this can be a no-op for us
}

void _ftruncate(off_t pos, file_ref* fr)
{
	if (ftruncate(fr->shadow_fd_,pos))
		fprintf(stderr, "ftruncate(%d, %d) failed with errno: %d\n", fr->shadow_fd_, pos, errno);
}

// Called by ftruncate()
int msf_ftruncate(const char* _path, off_t pos, struct fuse_file_info* finfo)
{
	int ret = ftruncate(get_fd(finfo), pos);
	if (ret == 0)
		do_call(_ftruncate,pos,get_fr(finfo));
	return ret;
}

// Called by stat()/fstat(), but only when fuse_operations.fgetattr is NULL.
// Also called by open() to determine if the path is a directory or a regular
// file.
int msf_getattr(const char* path, struct stat* st)
{
	int ret = stat((mem_shadow_name + path).c_str(), st);
	//printf("msf_getattr(\"/persistent%s\"), calling stat(\"%s\"), returned %d, errno: %d, st->st_mode: %o\n", path, (mem_shadow_name+path).c_str(), ret, errno, st->st_mode);
	return ret;
}

void _mkdir(std::string path, mode_t mode)
{
	if (mkdir(path.c_str(), mode) != 0)
		fprintf(stderr, "mkdir(%s, %d) failed with errno: %d\n", path.c_str(), mode, errno);
}

// Called by mkdir()
int msf_mkdir(const char* _path, mode_t mode)
{
	std::string path(_path);
	//printf("msf_mkdir(\"/persistent%s\"), calling mkdir(\"%s\")\n", _path, (mem_shadow_name + path).c_str());
	int ret = mkdir((mem_shadow_name + path).c_str(), mode);
	if (ret == 0)
		do_call(_mkdir,html5_shadow_name + path,mode);
	return ret;
}

// Called when O_CREAT is passed to open(), but only if fuse_operations.create
// is non-NULL.
int msf_mknod(const char* path, mode_t mode, dev_t dev)
{
	fprintf(stderr, "msf_mknod(\"%s\", %d, %d)\n", path, mode, dev);
	return 0;
}

// Called by open()
int msf_open(const char* _path, struct fuse_file_info* finfo)
{
	//printf("msf_open(\"/persistent%s\", %p), finfo->flags: 0x%x\n", _path, finfo, finfo->flags);
	std::string path(_path);
	int ret = open((mem_shadow_name + path).c_str(),O_RDWR);
	if (ret >= 0)
	{
		set_fh(finfo, O_RDWR, ret);
		do_call(_open,html5_shadow_name + path,O_RDWR,get_fr(finfo));
	}
	return ret;
}

// Called by getdents(), which is called by the more standard functions
// opendir()/readdir().
int msf_opendir(const char* path, struct fuse_file_info* finfo)
{
	//printf("msf_opendir(\"/persistent%s\", %p), finfo->fh: %ld\n", path, finfo, finfo->fh);
	if (finfo->fh == 0)
		finfo->fh = reinterpret_cast<decltype(finfo->fh)>(opendir((mem_shadow_name + path).c_str()));
	return 0;
}

// Called by read(). Note that FUSE specifies that all reads will fill the
// entire requested buffer. If this function returns less than that, the
// remainder of the buffer is zeroed.
int msf_read(const char* path, char* buf, size_t count, off_t pos,
		  struct fuse_file_info* finfo)
{
	//printf("msf_read(%s, %p, %d)\n", path, buf, count);
	return pread(get_fd(finfo), buf, count, pos);
}

// Called by getdents(), which is called by the more standard function
// readdir().
//
// NOTE: it is the responsibility of this function to add the "." and ".."
// entries.
//
// This function can be implemented one of two ways:
// 1) Ignore the offset, and always write every entry in a given directory.
//    In this case, you should always call filler() with an offset of 0. You
//    can ignore the return value of the filler.
//
//   int my_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
//                  off_t offset, struct fuse_file_info*) {
//     ...
//     filler(buf, ".", NULL, 0);
//     filler(buf, "..", NULL, 0);
//     filler(buf, "file1", &file1stat, 0);
//     filler(buf, "file2", &file2stat, 0);
//     return 0;
//   }
//
// 2) Only write entries starting from offset. Always pass the correct offset
//    to the filler function. When the filler function returns 1, the buffer
//    is full so you can exit readdir.
//
//   int my_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
//                  off_t offset, struct fuse_file_info*) {
//     ...
//     size_t kNumEntries = 4;
//     const char* my_entries[] = { ".", "..", "file1", "file2" };
//     int entry_index = offset / sizeof(dirent);
//     offset = entry_index * sizeof(dirent);
//     while (entry_index < kNumEntries) {
//       int result = filler(buf, my_entries[entry_index], NULL, offset);
//       if (filler == 1) {
//         // buffer filled, we're done.
//         return 0;
//       }
//       offset += sizeof(dirent);
//       entry_index++;
//     }
//
//     // No more entries, we're done.
//     return 0;
//   }
//
int msf_readdir(const char* path, void* buf, fuse_fill_dir_t filldir, off_t pos,
			 struct fuse_file_info* finfo)
{
	//printf("  msf_readdidr(\"/persistent%s\", %p, %p, %d, %p) (start index = %d)\n", path, buf, (void*)filldir, (int)pos, finfo, (int)(pos/sizeof(dirent)));
	DIR* dir = (DIR*)finfo->fh;
	struct dirent *ent = readdir(dir);
	if (ent)
	{
		struct stat st;
		if (stat((mem_shadow_name + path + "/" + ent->d_name).c_str(),&st) == 0)
		{
			int ret = (*filldir)(buf, ent->d_name, &st, pos);
			//printf("  filldir(\"%s\") returned %d\n", ent->d_name, ret);
		}
	}
	
	return 0;
}

void _close(file_ref* fr)
{
	//printf("  _close, calling close(%d)\n", fr->shadow_fd_);
	if (close(fr->shadow_fd_) != 0)
		fprintf(stderr, "close(%d) failed, errno: %d\n", fr->shadow_fd_, errno);
	delete fr;
}

// Called when the last reference to this node is released. This is only
// called for regular files. For directories, fuse_operations.releasedir is
// called instead.
int msf_release(const char* path, struct fuse_file_info* finfo)
{
	//printf("msf_release(\"/persistent%s\", %p) on file %s file_ref, calling close(%d)\n", path, finfo, get_fr(finfo) != 0 ? "with" : "without", get_fd(finfo));
	int ret = close(get_fd(finfo));
	if (ret == 0)
	{
		file_ref* fr = get_fr(finfo);
		if (fr)
			do_call(_close,fr);
	}
	return ret;
}

// Called when the last reference to this node is released. This is only
// called for directories. For regular files, fuse_operations.release is
// called instead.
int msf_releasedir(const char* path, struct fuse_file_info* finfo)
{
	//printf("msf_releasedir(\"/persistent%s\", %p), finfo->fh: %d\n", path, finfo, (int)finfo->fh);
	
	if (finfo->fh)
	{
		closedir((DIR*)finfo->fh);
		finfo->fh = 0;
	}
	
	return 0;
}

void _rename(std::string path, std::string new_path)
{
	if (rename(path.c_str(), new_path.c_str()) != 0)
		fprintf(stderr, "rename(%s, %s) failed with errno: %d\n", path.c_str(), new_path.c_str(), errno);
}

// Called by rename()
int msf_rename(const char* _path, const char* _new_path)
{
	std::string path(_path);
	std::string new_path(_new_path);
	
	int ret = rename((mem_shadow_name + path).c_str(), (mem_shadow_name + new_path).c_str());
	if (ret == 0)
		do_call(_rename,html5_shadow_name + path, html5_shadow_name + new_path);
	return ret;
}

void _rmdir(std::string path)
{
	if (rmdir(path.c_str()) != 0)
		fprintf(stderr, "rmdir(\"%s\") failed with errno: %d\n", path.c_str(), errno);
}

// Called by rmdir()
int msf_rmdir(const char* _path)
{
	std::string path(_path);
	//printf("msf_rmdir(\"/persistent%s\"), calling rmdir(\"%s\")\n", _path, (mem_shadow_name + path).c_str());
	int ret = rmdir((mem_shadow_name + path).c_str());
	if (ret == 0)
		do_call(_rmdir,html5_shadow_name + path);
	return ret;
}

void _truncate(std::string path, off_t pos)
{
	if (truncate(path.c_str(),pos) != 0)
		fprintf(stderr, "truncate(%s, %d) failed with errno: %d\n", path.c_str(), pos, errno);
}

// Called by truncate(), as well as open() when O_TRUNC is passed.
int msf_truncate(const char* _path, off_t pos)
{
	std::string	path(_path);
	int ret = truncate((mem_shadow_name + path).c_str(),pos);
	if (ret == 0)
		do_call(_truncate,html5_shadow_name + path,pos);
	return ret;
}

void _unlink(std::string path)
{
	if (unlink(path.c_str()) != 0)
		fprintf(stderr, "unlink(%s) failed with errno: %d\n", path.c_str(), errno);
}

// Called by unlink()
int msf_unlink(const char* _path)
{
	std::string path(_path);
	int ret = unlink((mem_shadow_name + path).c_str());
	if (ret == 0)
		do_call(_unlink,html5_shadow_name + path);
	return ret;
}

void _write(file_ref* fr, std::vector<char> buf, off_t pos)
{
	//printf("  _write(%d, %p, %d) (\"%s\")\n", fr->shadow_fd_, &buf.front(), buf.size(), &buf.front());
	int ret;
	if ((ret = pwrite(fr->shadow_fd_, &buf.front(), buf.size(), pos)) != buf.size())
		fprintf(stderr, "pwrite(%d, %p, %d, %d) returned unexpected value (%d), errno: %d\n", fr->shadow_fd_, buf.front(), buf.size(), pos, ret, errno);
}

// Called by write(). Note that FUSE specifies that a write should always
// return the full count, unless an error occurs.
int msf_write(const char* path, const char* buf, size_t count, off_t pos,
		   struct fuse_file_info* finfo)
{
	int ret = pwrite(get_fd(finfo), buf, count, pos);
	//printf("msf_write(\"/persistent%s\", %p, %d) returned: %d, fd: %d, errno: %d\n", path, buf, count, ret, get_fd(finfo), errno);
	if (ret == count)
		do_call(_write,get_fr(finfo), std::vector<char>(buf,&buf[count]),pos);
	else
		fprintf(stderr, "pwrite returned %d instead of %d\n", ret, (int)count);
	return ret;
}


struct fuse_operations msf_ops = {

  0,
  0,

  msf_init,
  msf_destroy,
  msf_access,
  msf_create,
  msf_fgetattr,
  msf_fsync,
  msf_ftruncate,
  msf_getattr,
  msf_mkdir,
  msf_mknod,
  msf_open,
  msf_opendir,
  msf_read,
  msf_readdir,
  msf_release,
  msf_releasedir,
  msf_rename,
  msf_rmdir,
  msf_truncate,
  msf_unlink,
  msf_write

#if 0
  // The following functions are not currently called by the nacl_io
  // implementation of FUSE.
  int (*bmap)(const char*, size_t blocksize, uint64_t* idx);
  int (*chmod)(const char*, mode_t);
  int (*chown)(const char*, uid_t, gid_t);
  int (*fallocate)(const char*, int, off_t, off_t, struct fuse_file_info*);
  int (*flock)(const char*, struct fuse_file_info*, int op);
  int (*flush)(const char*, struct fuse_file_info*);
  int (*fsyncdir)(const char*, int, struct fuse_file_info*);
  int (*getxattr)(const char*, const char*, char*, size_t);
  int (*ioctl)(const char*, int cmd, void* arg, struct fuse_file_info*,
               unsigned int flags, void* data);
  int (*link)(const char*, const char*);
  int (*listxattr)(const char*, char*, size_t);
  int (*lock)(const char*, struct fuse_file_info*, int cmd, struct flock*);
  int (*poll)(const char*, struct fuse_file_info*, struct fuse_pollhandle* ph,
              unsigned* reventsp);
  int (*read_buf)(const char*, struct fuse_bufvec** bufp, size_t size,
                  off_t off, struct fuse_file_info*);
  int (*readlink)(const char*, char*, size_t);
  int (*removexattr)(const char*, const char*);
  int (*setxattr)(const char*, const char*, const char*, size_t, int);
  int (*statfs)(const char*, struct statvfs*);
  int (*symlink)(const char*, const char*);
  int (*utimens)(const char*, const struct timespec tv[2]);
  int (*write_buf)(const char*, struct fuse_bufvec* buf, off_t off,
                   struct fuse_file_info*);
#endif

};

static void file_cp(const char *to, const char *from)
{
    auto from_f = fopen(from, "r");
    if (from_f == 0)
        return;

    auto to_f = fopen(to, "w");
    if (to_f == 0)
	{
        fclose(from_f);
		return;
	}

	while (true)
	{
		char buf[4096];
		auto nread = fread(buf, 1, sizeof(buf), from_f);
		if (nread == 0)
			break;
		fwrite(buf, 1, nread, to_f);
		if (nread < sizeof(buf))
			break;
	}
	
	fclose(to_f);
	fclose(from_f);
}

static void do_sync(const std::string& dirName)
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
// duplicates the entire directory structure under /.html5fs_shadow/*
// to /.memfs_shadow/*.  We run this in a background thread because
// /.html5fs_shadow is one of nacl's html5fs mounts, which can
// only be read from (or written to) from a non-main thread (while the
// main thread is not blocked, waiting for this to complete)
static void populate_memfs(MS_AppInstance* inst, std::vector<std::string> persistent_dirs)
{
	for (auto dir : persistent_dirs)
	{
		mkdir_p(html5_shadow_name + "/" + dir);
		mkdir_p(mem_shadow_name + "/" + dir);
		do_sync(dir);
	}
		
	inst->PostCommand("async_startup_complete:");
	
	fs_worker();
}

//#define _do_clear_

void print_indents(int numIndents)
{
	for (int i = 0; i < numIndents; i++)
		printf("    ");
}

#if defined(_do_clear_)
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

}

namespace mutantspider
{

void init_fs(MS_AppInstance* inst, const std::vector<std::string>& persistent_dirs)
{
	nacl_io_init_ppapi(inst->pp_instance(), pp::Module::Get()->get_browser_interface());
	umount("/");
	mount("", "/", "memfs", 0, "");
	
	if (persistent_dirs.empty())
		inst->PostCommand("async_startup_complete:");
	else
	{
		int ret = nacl_io_register_fs_type("ms_persist_backed_mem_fs", &msf_ops);

		ret = mount("", html5_shadow_name.c_str(), "html5fs", 0, "type=PERSISTENT,expected_size=1048576");
				
		#if defined(_do_clear_)
		std::thread(clear_all).detach();
		return;
		#endif
		
		ret = mount("", persistent_name.c_str(), "ms_persist_backed_mem_fs", 0, "");
		
		std::thread(std::bind(populate_memfs,inst,persistent_dirs)).detach();
	}
}

void memfs_mount(const char* dir)
{
	mkdir(dir, 0777);
}

}

// #if defined(__native_client__)
#endif

#if defined(EMSCRIPTEN)

namespace mutantspider
{

void init_fs(MS_AppInstance* inst, const std::vector<std::string>& persistent_dirs)
{
	if (persistent_dirs.empty())
		inst->PostCommand("async_startup_complete:");
	else
	{
		for (auto dir : persistent_dirs)
		{
			std::string path = persistent_name + "/" + dir;
			mkdir_p(path);
			ms_mount(path.c_str(), true/*persistent*/);
		}
		ms_syncfs_from_persistent();
	}
}

void memfs_mount(const char* dir)
{
	mkdir(dir, 0777);
	ms_mount(dir, false/*persistent*/);
}

}

#endif

