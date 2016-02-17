
#pragma once

#include <sstream>
#include <iomanip>
#include <vector>

#if defined(__native_client__)
  #include "ppapi/cpp/module.h"
  #include "ppapi/cpp/instance.h"
  #include "ppapi/cpp/completion_callback.h"
  extern pp::Instance* gGlobalPPInstance;
  extern std::string gBrowserLanguage;
  extern int gModuleID;
#endif

#if defined(EMSCRIPTEN)
  #include "SDL/SDL.h"
#endif

extern "C" void MS_SetLocale(const char*);
extern "C" void MS_AsyncStartupComplete();
extern "C" void MS_Init(const char* args);

typedef void (*ms_free_buffer)(void*);

#ifdef EMSCRIPTEN

  extern "C" void ms_heap_initialized();

#endif

// helper class used in resource file system code
namespace mutantspider
{
    struct rez_dir;
}

class ms_transfered_buffer;
extern "C" void ms_free_transfered_buffer(ms_transfered_buffer* tb, void* ptr);

extern "C" void ms_consolelog(const char* message);
extern "C" void ms_async_startup_complete(const char* err = 0);
extern "C" void ms_mkdir(const char* path);
extern "C" void ms_persist_mount(const char* path);
extern "C" void ms_rez_mount(const char* path, const mutantspider::rez_dir* root_addr);
extern "C" void ms_syncfs_from_persistent();
extern "C" int  ms_browser_supports_persistent_storage();

#if defined(EMSCRIPTEN)

struct ms_callback_base
{
  virtual ~ms_callback_base() {}
  virtual void exec() = 0;
};

template<typename T>
struct ms_callback_struct : public ms_callback_base
{
  ms_callback_struct(T* f) : f_(f) {}
  virtual ~ms_callback_struct() { delete f_; }
  virtual void exec() { (*f_)(); }
  T* f_;
};

extern "C" void ms_timed_callback_js(int milli, ms_callback_base* cb);

// after, "milli" milliseconds, call function "f" with remaining args.
// for example:
//
//    ms_timed_callback(728, foo, "hello world" 17.9);
//
// will execute:
//
//    foo("hello world", 17.9);
//
// after 728 milliseconds
template<typename F, typename ...Args>
void ms_timed_callback(int milli, F&& f, Args&&... args)
{
  auto b = std::bind(f, std::forward<Args>(args)...);
  using function_type = decltype(b);
  auto p = new function_type(b);
  ms_timed_callback_js(milli, new ms_callback_struct<function_type>(p));
}

extern "C" const char* ms_get_browser_language();
#endif

namespace mutantspider
{
  const char* locale();
  
  template<typename Func>
  static void output(const char* file_name, int line_num, Func func)
  {
    std::ostringstream format;
   
    // (file_name, line_num)
    std::ostringstream loc;
    loc << "("<< file_name << ", " << line_num << ")";
    const int width = 75;
    format << std::setiosflags(std::ios_base::left) << std::setfill(' ') << std::setw(width) << loc.str();

    // the "body" of the anon_log message
    func(format);
    
    ms_consolelog(format.str().c_str());
  }


  /*
    init_fs MUST be called prior to any other file operations.

    If non-empty, 'persistent_dirs' will be interpreted as a list of directory path names.  These SHOULD NOT start
    with a leading '/', and you are encouraged to use a single, relatively unique "application name" as the first
    directory in each.  For example, if your application name is "my_image_editor" then persistent_dirs might be a
    list looking something like:

      "my_image_editor/resources"
      "my_image_editor/users/john_smith"

    When such a list is given to init_fs, it causes:

      /persistent/my_image_editor/resources
      /persistent/my_image_editor/users/john_smith

    to be mounted into the file system as "persistent" directories.  Any file IO that is done inside of these
    directories will persist beween page views for the particular URL you are on.  The persisting mechanism
    underlying this support is based on the browser's general support for "local storage".  This is not the same
    thing as true local files on the OS that the browser is running on.  In the current implementation this will use
    "html5fs" for nacl builds, and IndexedDB for asm.js builds. In both of these cases if the user clears the
    browser's cookies, or history (it depends on the browser) this data will be erased.  Different browsers also have
    different mechanisms to let the user grant or deny permission to store data this way.  You can read about these
    issues by searching for documentation for IndexedDB.

    In the current implementation, the /persistent/... files are all read into memory at start up, and stay there.
    These files are essentially a memory-based file system.  So there can be performance issues if you store enormous
    amounts of data this way.

    Loading the data in /persistent/..., and in fact the general preperation of the /persistent directory, is done
    asynchronously. The call to fs_init simply initiates this logic.  When the directories and data are avaiable your
    MS_AsyncStartupComplete will be called.  It is only legal to perform file IO operations in the /persistent/...
    directories once this method has been called.
        
        
    Note about error reporting:

    POSIX file IO is (at least in these examples) a blocking API set.  That is, when you return from fread the data
    is in the buffer you supplied.  The underlying persistence mechanisms are, in many cases, non-blocking and/or
    asynchronous.  In lots of cases this can make it impossible to _return_ error conditions to the caller.  In these
    cases, errors, if they occur, are detected long after the initiating IO function has returned.  The general
    strategy used by mutantspider for these is just to emit an error message to the web console when these asynchronous
    errors occur. The expectation is that this is really only useful during debugging when an engineer can examin the
    error.


    "Resources"

    Assuming your project is being compiled with the help of mutantspider.mk, files whose names are listed in the
    RESOURCES make variable will be available, read-only, in the /resources section of the file system.  For example,
    if your makefile contains:

      RESOURCES+=../../some_pic.jpg

    then after init_fs has returned:

      FILE* f = fopen("/resources/some_pic.jpg", "r" );

    will succeed in opening the file, allowing you to read its contents.  See README.makefile for details on how to
    use this feature.
  */
  void init_fs();
  void mount_fs(const std::vector<std::string>& persistent_dirs = std::vector<std::string>());
}

#define ms_log(_body) mutantspider::output(__FILE__, __LINE__, [&](std::ostream& formatter) {formatter << _body;})


template<typename Fn>
void ms_url_fetch(const char* url, Fn f)
{
  ms_url_fetch_glue(url, [](void* user_data, void* param1, size_t param2, const char* msg){
    Fn* f = (Fn*)user_data;
    (*f)(param1, param2, msg);
    delete f;
  }, new Fn(f));
}

#if defined(EMSCRIPTEN)

template<typename Fn>
void ms_on_main_thread(Fn&& f)
{
  f();
}

#elif defined(__native_client__)

template<typename Fn>
void ms_on_main_thread(Fn&& f)
{
  auto c = pp::Module::Get()->core();
  if (c->IsMainThread())
    f();
  else {
    c->CallOnMainThread(0,pp::CompletionCallback([](void* user_data, int32_t result){
      Fn* f = static_cast<Fn*>(user_data);
      (*f)();
      delete f;
    }, new Fn(std::move(f))));
  }
}

#else
  #error unsupportd build environment
#endif

template<typename T>
void ms_update_primary_surface(T& views)
{
  ms_on_main_thread([&views]{ms_update_primary_surface_main(views);});
}

#if defined(MS_HAS_RESOURCES)

// careful!
// in the emscripten builds these data structures are directly read out of memory
// by the javascript support code in library_rezfs.js.  If you change anything about
// these, or the way the makefile generates the code in resource_list.cpp, then you
// will need to make matching changes to the js code in library_rezfs.js.  Typically
// these will be statements involving 'makeGetValue'
namespace mutantspider
{
struct rez_file_ent
{
  const unsigned char*  file_data;
  size_t                file_data_sz;
};

struct rez_dir_ent
{
  const char* d_name;
  union {
    const rez_file_ent* file;
    const struct rez_dir* dir;
  } ptr;
  int is_dir;
};

struct rez_dir {
  size_t  num_ents;
  const rez_dir_ent* ents;
};

extern const rez_dir rez_root_dir;
extern const rez_dir_ent rez_root_dir_ent;
}

#endif

