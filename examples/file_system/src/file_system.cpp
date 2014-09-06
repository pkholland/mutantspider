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

#include "file_system.h"
#include "persistent_tests.h"
#include "resource_tests.h"

#include <sys/stat.h>

// defining this skips the fuse-based implementation of the
// persistent storage file system, allowing you to see what
// the test code does when running on a "normal" file system
//#define NO_FUSE

FileSystemInstance::FileSystemInstance(MS_Instance instance)
    : MS_AppInstance(instance)
{}

bool FileSystemInstance::Init(uint32_t argc, const char* argn[], const char* argv[])
{
    // the set of directories that we want to be "persistent"
    // that is, file io done in these directories (or subdirectories of them)
    // will persist across page loads in the browser.  It is recommended
    // that all such directories have a high-level directory name that is
    // reasonably unique to the web app itself.  So here we use
    // "file_system_example".  This will result in the directory
    // "/persistent/file_system_example" being our main persistent directory.
    std::vector<std::string> persistent_dirs;
    #if !defined(NO_FUSE)
    persistent_dirs.push_back("file_system_example");
    #endif
    mutantspider::init_fs(this, persistent_dirs);

    return true;
}

void FileSystemInstance::AsyncStartupComplete()
{

    #if defined(NO_FUSE)
    mkdir("/persistent", 0777);
    mkdir("/persistent/file_system_example", 0777);
    #endif

    // file system, and in particular the _async_ persistent part,
    // is now ready to use.  So start the testing code.
    int num_tests_run = 0;
    int num_tests_failed = 0;
    
    auto ret = persistent_tests(this);
    num_tests_run += ret.first;
    num_tests_failed += ret.second;
    
    ret = resource_tests(this);
    num_tests_run += ret.first;
    num_tests_failed += ret.second;
    
    PostMessage("");
    PostMessage("File System Tests Completed: " + std::to_string(num_tests_run) + " tests run, " + std::to_string(num_tests_failed) + " tests failed");
}


/*
    standard MS_Module, whose CreateInstance is called during startup
    to create the Instance object which the browser interacts with.
    Mutantspider follows the Pepper design here of a two-stage
    initialization sequence.  pp::CreateModule is the first stage,
    that MS_Module's (this FileSystemModule's) CreateInstance is the
    second stage.
*/
class FileSystemModule : public MS_Module
{
public:
    virtual MS_AppInstancePtr CreateInstance(MS_Instance instance)
    {
        return new FileSystemInstance(instance);
    }
};


/*
    a bit of crappy, almost boilerplate code.
    
    This is the one place in mutantspider where it leaves the Pepper
    "pp" namespace intact, and just follows the pepper model for how
    the basic initialization works.  To be a functioning mutantspider
    component, you have to implement pp::CreateModule.  It will be called
    as the first thing during initialization of your component.  It is
    required to return an allocated instance of an MS_Module.  The (virtual)
    CreateInstance method of that MS_Module is then called to create an
    Instance.  That Instance's virtual methods are then called to
    interact with the web page.
*/
    
namespace pp {

MS_Module* CreateModule() { return new FileSystemModule(); }

}

