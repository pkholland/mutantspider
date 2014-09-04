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
#include "persistent_tests.h"
#include "resource_tests.h"

/*
    Beginner's note:
    
    The first thing called is pp::CreateModule at the bottom of this file.
    Then FileSystemModule::CreateInstance is the next thing called
    (second to bottom of the file).  That creates the FileSystemInstance
    (immediately below this comment)
*/

/*
    An instance of FileSystemInstance is created by the initialization logic
    (pp::CreateModule()->CreateInstance()).  This instance is then associated
    with the component on the web page that initiated this code.  Virtual methods
    like DidChangeView and HandleInputEvent are called in response to event
    processing in the browser.
    
    This FileSystemInstance initializes the mutantspider file system servers
    in its Init method.  But this initialization is asynchronous, so it waits
    until it has been notified of completion before executing its testing
    logic.
*/
class FileSystemInstance : public MS_AppInstance
{
public:
    explicit FileSystemInstance(MS_Instance instance)
                : MS_AppInstance(instance)
            {}


    virtual bool Init(uint32_t argc, const char* argn[], const char* argv[])
    {
        // the set of directories that we want to be "persistent"
        // that is, file io done in these directories (or subdirectories of them)
        // will persist across page loads in the browser.  It is recommended
        // that all such directories have a high-level directory name that is
        // reasonably unique to the web app itself.  So here we use
        // "file_system_example".  This will result in the directory
        // "/persistent/file_system_example" being our main persistent directory.
        std::vector<std::string> persistent_dirs;
        persistent_dirs.push_back("file_system_example");
        mutantspider::init_fs(this, persistent_dirs);

        // start the initialization
        mutantspider::init_fs(this);

        return true;
    }
    
    void HandleMessage(const mutantspider::Var& var_message)
    {
		if (var_message.is_dictionary())
		{
			mutantspider::VarDictionary dict_message(var_message);
			mutantspider::Var var_command = dict_message.Get("cmd");
			if (var_command.is_string())
			{
				auto cmd = var_command.AsString();
				if (cmd == "post_asyncfs_init")
				{
					// file system, and in particular the _async_ persistent part,
                    // is now ready to use.  So start the testing code.
                    PostMessage("Async File Initialization Complete, starting tests");
                    persistent_tests(this);
                    resource_tests(this);
				}
            }
        }
    }

};

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

