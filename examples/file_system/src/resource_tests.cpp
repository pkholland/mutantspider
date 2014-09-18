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

#include "resource_tests.h"
#include "file_system.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

static std::string Indents(int indents)
{
    std::string ind;
    for (int i = 0; i < indents; i++)
        ind += "&nbsp;&nbsp;";
    return ind;
}

static void list_dir(FileSystemInstance* inst, const char* dir_name, int& num_tests_run, int& num_tests_failed, int indents)
{
     ++num_tests_run;
    DIR* dir = opendir(dir_name);
    if (dir == 0)
    {
        ++num_tests_failed;
        inst->PostError(LINE_PFX + Indents(indents) + "opendir(\"" + dir_name + "\") failed with errno: " + errno_string());
    }
    else
    {
        inst->PostMessage(LINE_PFX + Indents(indents) + "opendir(\"" + dir_name + "\")");
        struct dirent *ent;
        while ((ent = readdir(dir)) != 0)
        {
            inst->PostMessage(LINE_PFX + Indents(indents+1) + "readdir returned: \"" + ent->d_name + "\"");
            
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
                continue;
            
            ++num_tests_run;
            struct stat st;
            std::string full_path = std::string(dir_name) + "/" + ent->d_name;
            if (stat( full_path.c_str(), &st) != 0)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + Indents(indents+1) + "stat(\"" + full_path + "\",&st) failed with errno: " + errno_string());
            }
            else
            {
                if (S_ISDIR(st.st_mode))
                {
                    inst->PostMessage(LINE_PFX + Indents(indents+1) + "stat(\"" + full_path + "\",&st) reported directory");
                    list_dir(inst, full_path.c_str(), num_tests_run, num_tests_failed, indents+1);
                }
                else if (S_ISREG(st.st_mode))
                    inst->PostMessage(LINE_PFX + Indents(indents+1) + "stat(\"" + full_path + "\",&st) reported regular file");
                else
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + Indents(indents+1) + "stat(\"" + full_path + "\",&st) reported unknown st.st_mode of: " + to_octal_string(st.st_mode));
                }
                
            }
        }
        
    }
}

std::pair<int,int> resource_tests(FileSystemInstance* inst)
{
    int num_tests_run = 0;
    int num_tests_failed = 0;
    
//return std::make_pair(num_tests_run, num_tests_failed);
 
    inst->PostMessage("");
    inst->PostHeading("Resource File Tests (see RESOURCES definition in Makefile for file layout):");
    
    list_dir(inst, "/resources", num_tests_run, num_tests_failed, 0);
 
    inst->PostMessage("");
    
    // is /resources correctly reported as a read-only directory?
    ++num_tests_run;
    struct stat st;
    if (stat("/resources", &st) != 0)
    {
        ++num_tests_failed;
        inst->PostError(LINE_PFX + "stat(\"/resources\", &st) failed with: " + errno_string());
    }
    else
    {
        inst->PostMessage(LINE_PFX + "stat(\"/resources\", &st)");
        
        if (st.st_mode != (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH))
        {
            ++num_tests_failed;
            inst->PostError(LINE_PFX + "stat(\"/resources\", &st) should have reported read-only directory, but st.st_mode = " + to_octal_string(st.st_mode));
        }
        else
            inst->PostMessage(LINE_PFX + "stat(\"/resources\", &st) correctly reported as a read-only directory");
    }
    
    // is resources/file2.txt reported as a read-only file?
    ++num_tests_run;
    if (stat("/resources/file2.txt", &st) != 0)
    {
        ++num_tests_failed;
        inst->PostError(LINE_PFX + "stat(\"/resources/file2.txt\", &st) failed with errno: " + errno_string());
    }
    else
    {
        inst->PostMessage(LINE_PFX + "stat(\"/resources/file2.txt\", &st)");
        
        ++num_tests_run;
        if (st.st_mode != (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH))
        {
            ++num_tests_failed;
            inst->PostError(LINE_PFX + "stat(\"/resources/file2.txt\", &st) should have reported read-only file (" + to_octal_string(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH) + "), but st.st_mode = " + to_octal_string(st.st_mode));
        }
            inst->PostMessage(LINE_PFX + "stat(\"/resources/file2.txt\", &st) correctly reported as read-only file");
    }
   
    // are we correctly blocked from opening resources/file2.txt with write access
    ++num_tests_run;
    auto fd = open("/resources/file2.txt", O_RDWR);
    if ((fd != -1) || (errno != EROFS))
    {
        ++num_tests_failed;
        inst->PostError(LINE_PFX + "open(\"/resources/file2.txt\", O_RDWR) should have returned -1 with errno = EROFS but did not, it returned: " + std::to_string(fd) + ", with errno = " + errno_string());
        if (fd != -1)
            close(fd);
    }
    else
        inst->PostMessage(LINE_PFX + "open(\"/resources/file2.txt\", O_RDWR) correctly failed with errno = EROFS");
    
    // are we correctly blocked from trying to chmod resources/file2.txt to something writable?
    ++num_tests_run;
    auto rslt = chmod("/resources/file2.txt", 0666);
    if ((rslt != -1) || (errno != EROFS))
    {
        ++num_tests_failed;
        inst->PostError(LINE_PFX + "chmod(\"/resources/file2.txt\", 0666) should have returned -1 with errno = EROFS but did not, it returned: " + std::to_string(rslt) + ", with errno = " + errno_string());

        // if chmod didn't fail, can we open for writing and then write?
        if (rslt == 0)
        {
            fd = open("/resources/file2.txt", O_RDWR);
            if (fd == -1)
                inst->PostMessage(LINE_PFX + "previous chmod reported that it worked, but we still cannot open the file with read access -- this is a good thing" );
            else
                inst->PostError(LINE_PFX + "previous chmod reported that it worked, and we can now open the file with read access -- this is bad" );
        }
    }
    else
        inst->PostMessage(LINE_PFX + "chmod(\"/resources/file2.txt\", 0666) correctly failed with errno = EROFS");
    
    // are we correctly blocked from creating a new file in /resources?
    ++num_tests_run;
    fd = open("/resources/does_not_exist.txt", O_RDWR | O_CREAT, 0666);
    if ((fd != -1) || (errno != EROFS))
    {
        ++num_tests_failed;
        inst->PostError(LINE_PFX + "open(\"/resources/does_not_exist.txt\", O_RDWR | O_CREAT, 0666) should have returned -1 with errno = EROFS but did not, it returned: " + std::to_string(fd) + ", with errno = " + errno_string());
        if (fd != -1)
            close(fd);
    }
    else
        inst->PostMessage(LINE_PFX + "open(\"/resources/does_not_exist.txt\", O_RDWR | O_CREAT, 0666) correctly failed with errno = EROFS");
    
    // does a read-only attempt at opening a new file correctly generate ENOENT?
    ++num_tests_run;
    fd = open("/resources/does_not_exist.txt", O_RDONLY);
    if ((fd != -1) || (errno != ENOENT))
    {
        ++num_tests_failed;
        inst->PostError(LINE_PFX + "open(\"/resources/does_not_exist.txt\", O_RDONLY) should have returned -1 with errno = ENOENT but did not, it returned: " + std::to_string(fd) + ", with errno = " + errno_string());
        if (fd != -1)
            close(fd);
    }
    else
        inst->PostMessage(LINE_PFX + "open(\"/resources/does_not_exist.txt\", O_RDONLY) correctly failed with errno = ENOENT");
    
    // can we open (read-only) our file2.txt and read from it?
    ++num_tests_run;
    fd = open("/resources/file2.txt", O_RDONLY);
    if (fd == -1)
    {
        ++num_tests_failed;
        inst->PostError(LINE_PFX + "open(\"/resources/file2.txt\") failed with errno = " + errno_string());
    }
    else
    {
        inst->PostMessage(LINE_PFX + "open(\"/resources/file2.txt\", O_RDONLY)");
        
        ++num_tests_run;
        char    rd_buf[32];
        auto bytes_read = read(fd, &rd_buf[0], sizeof(rd_buf) - 1);
        if (bytes_read != sizeof(rd_buf) - 1)
        {
            ++num_tests_failed;
            inst->PostError(LINE_PFX + "read(read-only-fd, &rd_buf[0], sizeof(rd_buf) - 1) failed returned: " + std::to_string(bytes_read) + ", with errno: " + errno_string());
        }
        else
        {
            inst->PostMessage(LINE_PFX + "read(read-only-fd, &rd_buf[0], sizeof(rd_buf) - 1)");
            
            ++num_tests_run;
            const char* reqrd = "This is the content of file2.  ";
            rd_buf[sizeof(rd_buf) - 1] = 0;
            if (strcmp(&rd_buf[0], reqrd) != 0)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "/resources/file2.txt does not seem to contain the right data, it starts with: \"" + &rd_buf[0] + "\"");
            }
            else
                inst->PostMessage(LINE_PFX + "/resources/file2.txt correctly starts with: \"" + reqrd + "\"");
            
        }
        
        close(fd);
    }
    
    
   
    return std::make_pair(num_tests_run, num_tests_failed);
}
