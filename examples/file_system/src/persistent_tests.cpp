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
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

namespace {

void PostError(MS_AppInstance* inst, const std::string& message)
{
    inst->PostMessage(std::string("<span class=\"error\">") + message + "</span>");
}

void PostHeading(MS_AppInstance* inst, const std::string& message)
{
    inst->PostMessage(std::string("<span class=\"heading\">") + message + "</span>");
}

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
    
    int ret = rmdir(dir_name.c_str());
    print_indents(numIndents);
    printf("rmdir(%s) returned %d, errno: %d\n", dir_name.c_str(), ret, errno);
}

std::string errno_string()
{
    switch (errno)
    {
        case EPERM:
            return "EPERM";
        case ENOENT:
            return "ENOENT";
        case EBADF:
            return "EBADF";
        case EACCES:
            return "EACCES";
        default:
            return std::to_string(errno);
    }
}

}

#define kReallyBigFileSize 1000000
#define kReallyBigFileString "hotdogs and sausages and "

std::pair<int,int> persistent_tests(MS_AppInstance* inst)
{
    int num_tests_run = 0;
    int num_tests_failed = 0;
    
    if (mutantspider::browser_supports_persistent_storage())
    {
        // does it appear that this test code has run in the past?
        struct stat st;
        if (stat("/persistent/file_system_example/root", &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                PostHeading(inst, "Persistent File Tests: Data previously stored, running validate and delete tests");
                
                // there should be exactly 2 files in persistent/file_system_example/root,
                // one named small_file and one named big_file (plus '.' and '..')
                ++num_tests_run;
                DIR* dir = opendir("/persistent/file_system_example/root");
                if (dir == 0)
                {
                    ++num_tests_failed;
                    PostError(inst, std::string("opendir(\"/persistent/file_system_example/root\") failed with errno: ") + errno_string());
                }
                else
                {
                    inst->PostMessage("opendir(\"/persistent/file_system_example/root\")");
                    struct dirent *ent;
                    while ((ent = readdir(dir)) != 0)
                    {
                        ++num_tests_run;
                        inst->PostMessage(std::string("readdir returned \"") + ent->d_name + "\"");
                        if (strcmp(ent->d_name,".") && strcmp(ent->d_name,".."))
                        {
                            if (strcmp(ent->d_name,"small_file") == 0)
                            {
                                ++num_tests_run;
                                struct stat st;
                                if (stat("/persistent/file_system_example/root/small_file",&st) != 0)
                                {
                                    ++num_tests_failed;
                                    PostError(inst, std::string("stat(\"/persistent/file_system_example/root/small_file\",&st) failed with errno: ") + errno_string());
                                }
                                else
                                {
                                    inst->PostMessage("stat(\"/persistent/file_system_example/root/small_file\",&st)");
                                    ++num_tests_run;
                                    if (S_ISREG(st.st_mode))
                                    {
                                        inst->PostMessage("S_ISREG reported \"/persistent/file_system_example/root/small_file\" as a file (not directory)");
                                        ++num_tests_run;
                                        if (st.st_size == 11)
                                            inst->PostMessage("stat correctly reported the file size for \"/persistent/file_system_example/root/small_file\" as 11 bytes");
                                        else
                                        {
                                            ++num_tests_failed;
                                            PostError(inst, std::string("stat incorrectly reported the file size for \"/persistent/file_system_example/root/small_file\" as ") + std::to_string(st.st_size) + " bytes");
                                        }
                                    }
                                    else
                                    {
                                        ++num_tests_failed;
                                        PostError(inst, "S_ISREG incorrectly reported \"/persistent/file_system_example/root/small_file\" as something other than a file");
                                    }
                                }
                            }
                            else if (strcmp(ent->d_name,"big_file") == 0)
                            {
                                ++num_tests_run;
                                struct stat st;
                                if (stat("/persistent/file_system_example/root/big_file",&st) != 0)
                                {
                                    ++num_tests_failed;
                                    PostError(inst, std::string("stat(\"/persistent/file_system_example/root/big_file\",&st) failed with errno: ") + errno_string());
                                }
                                else
                                {
                                    inst->PostMessage("stat(\"/persistent/file_system_example/root/big_file\",&st)");
                                    ++num_tests_run;
                                    if (S_ISREG(st.st_mode))
                                    {
                                        inst->PostMessage("S_ISREG reported \"/persistent/file_system_example/root/big_file\" as a file (not directory)");
                                        ++num_tests_run;
                                        if (st.st_size == kReallyBigFileSize*strlen(kReallyBigFileString))
                                            inst->PostMessage(std::string("stat correctly reported the file size for \"/persistent/file_system_example/root/big_file\" as ") + std::to_string(kReallyBigFileSize*strlen(kReallyBigFileString)) + " bytes");
                                        else
                                        {
                                            ++num_tests_failed;
                                            PostError(inst, std::string("stat incorrectly reported the file size for \"/persistent/file_system_example/root/big_file\" as ") + std::to_string(st.st_size) + " bytes");
                                        }
                                    }
                                    else
                                    {
                                        ++num_tests_failed;
                                        PostError(inst, "S_ISREG incorrectly reported \"/persistent/file_system_example/root/big_file\" as something other than a file");
                                    }
                                }
                            }
                        }
                    }
                    closedir(dir);
                
                }
                
                ++num_tests_run;
                if (truncate("/persistent/file_system_example/root/big_file",10) != 0)
                {
                    ++num_tests_failed;
                    PostError(inst, std::string("truncate(\"/persistent/file_system_example/root/big_file\",10) failed with errno: ") + errno_string());
                }
                else
                {
                    inst->PostMessage("truncate(\"/persistent/file_system_example/root/big_file\",10)");
                    
                    ++num_tests_run;
                    struct stat st;
                    if (stat("/persistent/file_system_example/root/big_file",&st) != 0)
                    {
                        ++num_tests_failed;
                        PostError(inst, std::string("stat(\"/persistent/file_system_example/root/big_file\",&st) failed with errno: ") + errno_string());
                    }
                    else
                    {
                        inst->PostMessage("stat(\"/persistent/file_system_example/root/big_file\",&st)");
                    
                        ++num_tests_run;
                        if (st.st_size == 10)
                            inst->PostMessage("stat correctly reported the new, truncated file size for \"/persistent/file_system_example/root/big_file\" as 10 bytes");
                        else
                        {
                            ++num_tests_failed;
                            PostError(inst, std::string("stat incorrectly reported the file new, truncated size for \"/persistent/file_system_example/root/big_file\" as ") + std::to_string(st.st_size) + " bytes");
                        }
                    }
                }
                
                clear_all_r("/persistent/file_system_example/root",0);
            }
            else
            {
                PostError(inst, "/persistent/file_system_example/root exists but does not appear to be a directory, attempting to delete");
                clear_all_r("/persistent/file_system_example/root",0);
            }
        }
        else
        {
            PostHeading(inst, "Persistent File Tests: No data previously stored, running create and store tests");
            
            // can we make our root directory?
            // we handle failure a little differently on this one.  If this fails we don't
            // attempt to run further tests
            ++num_tests_run;
            if (mkdir("/persistent/file_system_example/root",0777) != 0)
            {
                ++num_tests_failed;
                PostError(inst, std::string("mkdir(\"/persistent/file_system_example/root\",0777) failed with errno: ") + errno_string());
                return std::make_pair(num_tests_run, num_tests_failed);
            }
            inst->PostMessage("mkdir(\"/persistent/file_system_example/root\",0777)");
            
            // does a file which does not exist fail to open as expected?
            ++num_tests_run;
            auto fd = open("/persistent/file_system_example/root/does_not_exist", O_RDONLY);
            if (fd != -1 || errno != ENOENT)
            {
                ++num_tests_failed;
                PostError(inst, std::string("open(\"/persistent/file_system_example/root/does_not_exist\",O_RDONLY) should have failed with errno = ENOENT, but did not.  It returned ") + std::to_string(fd) + ", and errno: " + errno_string());
            }
            else
                inst->PostMessage("open(\"/persistent/file_system_example/root/does_not_exist\", O_RDONLY)");
            
            // can we create a new file?
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/small_file",O_RDONLY | O_CREAT, 0666);
            if (fd == -1)
            {
                ++num_tests_failed;
                PostError(inst, std::string("open(\"/persistent/file_system_example/root/small_file\",O_RDONLY | O_CREAT, 0666) should have succeeded, but did not.  It returned -1, and errno: ") + errno_string());
            }
            else
            {
                close(fd);
                inst->PostMessage("open(\"/persistent/file_system_example/root/small_file\", O_RDONLY | O_CREAT, 0666)");
            }
            
            // can we open, read-only, an existing file (the one we just created above)
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/small_file",O_RDONLY);
            if (fd == 1)
            {
                ++num_tests_failed;
                PostError(inst, std::string("open(\"/persistent/file_system_example/root/small_file\",O_RDONLY) should have succeeded, but did not.  It returned -1, and errno: ") + errno_string());
            }
            else
            {
                inst->PostMessage("open(\"/persistent/file_system_example/root/small_file\", O_RDONLY)");
                
                // are we correctly blocked from writing to this file - we opened it O_RDONLY
                ++num_tests_run;
                const char* buf = "hello";
                auto written = write(fd, buf, strlen(buf));
                if (written != -1 || (errno != EBADF && errno != EACCES))
                {
                    ++num_tests_failed;
                    PostError(inst, std::string("write(read-only-fd, \"hello\", 5) should have failed with EBADF, but did not.  It returned ") + std::to_string(written) + ", and errno: " + errno_string());
                }
                else
                     inst->PostMessage("write(read-only-fd, \"hello\", 5)");
                close(fd);
            }
            
            const char* hello = "hello";
            const char* hello_world = "hello world";
            
            // can we open, write-only, an existing file
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/small_file",O_WRONLY);
            if (fd == -1)
            {
                ++num_tests_failed;
                PostError(inst, std::string("open(\"/persistent/file_system_example/root/small_file\",O_WRONLY) should have succeeded, but did not.  It returned -1, and errno: ") + errno_string());
            }
            else
            {
                inst->PostMessage("open(\"/persistent/file_system_example/root/small_file\", O_WRONLY)");
                
                // are we correctly blocked from reading from this file?
                ++num_tests_run;
                char b;
                auto bytes_read = read(fd, &b, 1);
                if (bytes_read != -1 || (errno != EBADF && errno != EACCES))
                {
                    ++num_tests_failed;
                    PostError(inst, std::string("read(write-only-fd, &b, 1) should have failed with errno EBADF, but did not.  It returned ") + std::to_string(bytes_read) + ", and errno: " + errno_string());
                }
                else
                    inst->PostMessage("read(write-only-fd, &b, 1)");
                
                // can we write to this file?
                ++num_tests_run;
                auto written = write(fd, hello, strlen(hello));
                if (written != strlen(hello))
                {
                    ++num_tests_failed;
                    PostError(inst, std::string("write(write-only-fd, \"hello\", 5) should have succeeded, but did not.  It returned ") + std::to_string(written) + ", and errno: " + errno_string());
                }
                else
                    inst->PostMessage("write(write-only-fd, \"hello\", 5)");
                close(fd);
            }
            
            // can we open, read-write, an existing file
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/small_file",O_RDWR);
            if (fd == -1)
            {
                ++num_tests_failed;
                PostError(inst, std::string("open(\"/persistent/file_system_example/root/small_file\",O_RDWR) should have succeeded, but did not.  It returned -1, and errno: ") + errno_string());
            }
            else
            {
                inst->PostMessage("open(\"/persistent/file_system_example/root/small_file\", O_RDWR)");
                
                // can we read from this file, and does it contain "hello"?
                ++num_tests_run;
                char rd_buf[16] = {0};
                auto bytes_read = read(fd, &rd_buf[0], sizeof(rd_buf));
                if (bytes_read != strlen(hello))
                {
                    ++num_tests_failed;
                    PostError(inst, std::string("read(read-write-fd, &buf, sizeof(buf)) should have returned 5, but did not.  It returned ") + std::to_string(bytes_read) + ", and errno: " + errno_string());
                }
                else if (memcmp(rd_buf, hello, strlen(hello)) != 0)
                {
                    ++num_tests_failed;
                    PostError(inst, std::string("read(read-write-fd, &buf, sizeof(buf)) should have read \"hello\", but did not.  It read: ") + &rd_buf[0]);
                }
                else
                    inst->PostMessage("read(read-write-fd, &buf, sizeof(buf))");
                
                // can we also write (append) to this file?
                ++num_tests_run;
                const char* buf = " world";
                auto written = write(fd, buf, strlen(buf));
                if (written != strlen(buf))
                {
                    ++num_tests_failed;
                    PostError(inst, std::string("write(read-write-fd, \" world\", 6) should have succeeded, but did not.  It returned ") + std::to_string(written) + ", and errno: " + errno_string());
                }
                else
                    inst->PostMessage("write(read-write-fd, \" world\", 6)");
                close(fd);
            }
            
            // can we create and write to a really big file?
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/big_file",O_RDWR | O_CREAT, 0666);
            if (fd == -1)
            {
                ++num_tests_failed;
                PostError(inst, std::string("open(\"/persistent/file_system_example/root/big_file\",O_RDWR | O_CREAT, 0666) should have succeeded, but did not.  It returned -1, and errno: ") + errno_string());
            }
            else
            {
                auto len = strlen(kReallyBigFileString);
                for (int i = 0; i < kReallyBigFileSize; i++)
                {
                    auto bytes_written = write(fd, kReallyBigFileString, len);
                    if (bytes_written != len)
                    {
                        ++num_tests_failed;
                        PostError(inst, std::string("write to really big file failed after writing ") + std::to_string(i*len) + " bytes");
                        break;
                    }
                }
                inst->PostMessage(std::string("wrote ") + std::to_string(kReallyBigFileSize*len) + " bytes to a really big file");
                close(fd);
            }
            
        }
    }
    else
    {
        PostError(inst, "This browser does not support persistent storage. Persistent tests will not be tried.");
    }
    
    return std::make_pair(num_tests_run, num_tests_failed);
}
