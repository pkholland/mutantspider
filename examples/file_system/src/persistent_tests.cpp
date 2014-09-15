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

#include "persistent_tests.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <utime.h>

namespace {

void clear_all(const std::string& dir_name)
{
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
                    struct stat st;
                    if (stat(path.c_str(),&st) == 0)
                    {
                        if (S_ISDIR(st.st_mode))
                        {
                            unlinked = true;
                            clear_all(path);
                         }
                        else
                        {
                            unlinked = true;
                            int ret = unlink(path.c_str());
                        }
                    }
                }
            }
            closedir(dir);
        }
    } while (unlinked);
    
    int ret = rmdir(dir_name.c_str());
}

}

#define kReallyBigFileSize 1000000
#define kReallyBigFileString "hotdogs and sausages and "

std::pair<int,int> persistent_tests(FileSystemInstance* inst)
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
                inst->PostHeading("Persistent File Tests: Data previously stored, running validate and delete tests");
                
                // is the access mode set to 0777?
                ++num_tests_run;
                if ((st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != (S_IRWXU | S_IRWXG | S_IRWXO))
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "stat(\"/persistent/file_system_example/root\", &st) incorrectly reported access as " + to_octal_string(st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) + " instead of 0777");
                }
                else
                    inst->PostMessage(LINE_PFX + "stat(\"/persistent/file_system_example/root\", &st) correctly reported access as 0777");
                
                // there should be exactly 3 files in persistent/file_system_example/root,
                // one named small_file, one named write_only_file, and one named big_file (plus '.' and '..')
                ++num_tests_run;
                DIR* dir = opendir("/persistent/file_system_example/root");
                if (dir == 0)
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "opendir(\"/persistent/file_system_example/root\") failed with errno: " + errno_string());
                }
                else
                {
                    inst->PostMessage(LINE_PFX + "opendir(\"/persistent/file_system_example/root\")");
                    struct dirent *ent;
                    while ((ent = readdir(dir)) != 0)
                    {
                        ++num_tests_run;
                        inst->PostMessage(LINE_PFX + "readdir returned \"" + ent->d_name + "\"");
                        if (strcmp(ent->d_name,".") && strcmp(ent->d_name,".."))
                        {
                            if (strcmp(ent->d_name,"small_file") == 0)
                            {
                                ++num_tests_run;
                                struct stat st;
                                if (stat("/persistent/file_system_example/root/small_file",&st) != 0)
                                {
                                    ++num_tests_failed;
                                    inst->PostError(LINE_PFX + "stat(\"/persistent/file_system_example/root/small_file\",&st) failed with errno: " + errno_string());
                                }
                                else
                                {
                                    inst->PostMessage(LINE_PFX + "stat(\"/persistent/file_system_example/root/small_file\",&st)");
                                    ++num_tests_run;
                                    if (S_ISREG(st.st_mode))
                                    {
                                        inst->PostMessage(LINE_PFX + "S_ISREG reported \"/persistent/file_system_example/root/small_file\" as a file (not directory)");
                                        ++num_tests_run;
                                        if (st.st_size == 20)
                                        {
                                            inst->PostMessage(LINE_PFX + "stat correctly reported the file size for \"/persistent/file_system_example/root/small_file\" as 11 bytes");
        
                                            ++num_tests_run;
                                            char rd_buf[32];
                                            char b[32];
                                            memcpy(&b[0], "hello world  goodbye", 20);
                                            b[11] = b[12] = 0;
                                            auto fd = open("/persistent/file_system_example/root/small_file", O_RDONLY);
                                            read(fd, &rd_buf, sizeof(rd_buf));
                                            if (memcmp(&b[0], &rd_buf[0], 20) != 0)
                                            {
                                                ++num_tests_failed;
                                                rd_buf[20] = 0;
                                                for (int i = 0; i < 20; i++)
                                                    if (rd_buf[i] == 0)
                                                        rd_buf[i] = 'Z';
                                                inst->PostError(LINE_PFX + "file should have contained \"hello worldZZgoodbye\", but it did not.  It contained: \"" + &rd_buf[0] + "\"");
                                            }
                                            else
                                                inst->PostMessage(LINE_PFX + "file correctly contained \"hello worldZZgoodbye\" (with the Z's as NULL bytes)");
                                        }
                                        else
                                        {
                                            ++num_tests_failed;
                                            inst->PostError(LINE_PFX + "stat incorrectly reported the file size for \"/persistent/file_system_example/root/small_file\" as " + std::to_string(st.st_size) + " bytes instead of 11");
                                        }
                                    }
                                    else
                                    {
                                        ++num_tests_failed;
                                        inst->PostError(LINE_PFX + "S_ISREG incorrectly reported \"/persistent/file_system_example/root/small_file\" as something other than a file");
                                    }
                                    
                                    
                                }
                            }
                            else if (strcmp(ent->d_name,"write_only_file") == 0)
                            {
                                ++num_tests_run;
                                struct stat st;
                                if (stat("/persistent/file_system_example/root/write_only_file",&st) != 0)
                                {
                                    ++num_tests_failed;
                                    inst->PostError(LINE_PFX + "stat(\"/persistent/file_system_example/root/write_only_file\",&st) failed with errno: " + errno_string());
                                }
                                else
                                {
                                    inst->PostMessage(LINE_PFX + "stat(\"/persistent/file_system_example/root/write_only_file\",&st)");
                                    ++num_tests_run;
                                    if ((st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != (S_IWUSR | S_IWGRP | S_IWOTH))
                                    {
                                        ++num_tests_failed;
                                        inst->PostError(LINE_PFX + "stat(\"/persistent/file_system_example/root/write_only_file\",&st) incorrectly reported access as " + to_octal_string(st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) + " instead of " + to_octal_string(S_IWUSR | S_IWGRP | S_IWOTH));
                                    }
                                    else
                                        inst->PostMessage(LINE_PFX + "stat(\"/persistent/file_system_example/root/write_only_file\",&st) correctly reported access as " + to_octal_string(S_IWUSR | S_IWGRP | S_IWOTH));
                                    
                                    ++num_tests_run;
                                    if (st.st_mtime != 17)
                                    {
                                        ++num_tests_failed;
                                        inst->PostError(LINE_PFX + "write_only_file mod time incorrectly reported as " + std::to_string(st.st_mtime) + ", instead of 17");
                                    }
                                    else
                                        inst->PostMessage(LINE_PFX + "write_only_file mod time correctly reported as 17");

                                }
                            }
                            else if (strcmp(ent->d_name,"big_file") == 0)
                            {
                                ++num_tests_run;
                                struct stat st;
                                if (stat("/persistent/file_system_example/root/big_file",&st) != 0)
                                {
                                    ++num_tests_failed;
                                    inst->PostError(LINE_PFX + "stat(\"/persistent/file_system_example/root/big_file\",&st) failed with errno: " + errno_string());
                                }
                                else
                                {
                                    inst->PostMessage(LINE_PFX + "stat(\"/persistent/file_system_example/root/big_file\",&st)");
                                    ++num_tests_run;
                                    if (S_ISREG(st.st_mode))
                                    {
                                        inst->PostMessage(LINE_PFX + "S_ISREG reported \"/persistent/file_system_example/root/big_file\" as a file (not directory)");
                                        ++num_tests_run;
                                        if (st.st_size == kReallyBigFileSize*strlen(kReallyBigFileString))
                                            inst->PostMessage(LINE_PFX + "stat correctly reported the file size of \"/persistent/file_system_example/root/big_file\" as " + std::to_string(kReallyBigFileSize*strlen(kReallyBigFileString)) + " bytes");
                                        else
                                        {
                                            ++num_tests_failed;
                                            inst->PostError(LINE_PFX + "stat incorrectly reported the file size of \"/persistent/file_system_example/root/big_file\" as " + std::to_string(st.st_size) + " bytes");
                                        }
                                    }
                                    else
                                    {
                                        ++num_tests_failed;
                                        inst->PostError(LINE_PFX + "S_ISREG incorrectly reported \"/persistent/file_system_example/root/big_file\" as something other than a file");
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
                    inst->PostError(LINE_PFX + "truncate(\"/persistent/file_system_example/root/big_file\",10) failed with errno: " + errno_string());
                }
                else
                {
                    inst->PostMessage(LINE_PFX + "truncate(\"/persistent/file_system_example/root/big_file\",10)");
                    
                    ++num_tests_run;
                    struct stat st;
                    if (stat("/persistent/file_system_example/root/big_file",&st) != 0)
                    {
                        ++num_tests_failed;
                        inst->PostError(LINE_PFX + "stat(\"/persistent/file_system_example/root/big_file\",&st) failed with errno: " + errno_string());
                    }
                    else
                    {
                        inst->PostMessage(LINE_PFX + "stat(\"/persistent/file_system_example/root/big_file\",&st)");
                    
                        ++num_tests_run;
                        if (st.st_size == 10)
                            inst->PostMessage(LINE_PFX + "stat correctly reported the new, truncated file size of \"/persistent/file_system_example/root/big_file\" as 10 bytes");
                        else
                        {
                            ++num_tests_failed;
                            inst->PostError(LINE_PFX + "stat incorrectly reported the file new, truncated size of \"/persistent/file_system_example/root/big_file\" as " + std::to_string(st.st_size) + " bytes");
                        }
                    }
                }
                
                clear_all("/persistent/file_system_example/root");
            }
            else
            {
                inst->PostError(LINE_PFX + "/persistent/file_system_example/root exists but does not appear to be a directory, attempting to delete");
                unlink("/persistent/file_system_example/root");
            }
        }
        else
        {
            inst->PostHeading("Persistent File Tests: No data previously stored, running create and store tests");
            
            // can we make our root directory?
            // we handle failure a little differently on this one.  If this fails we don't
            // attempt to run further tests
            ++num_tests_run;
            if (mkdir("/persistent/file_system_example/root",0777) != 0)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "mkdir(\"/persistent/file_system_example/root\",0777) failed with errno: " + errno_string());
                return std::make_pair(num_tests_run, num_tests_failed);
            }
            inst->PostMessage(LINE_PFX + "mkdir(\"/persistent/file_system_example/root\",0777)");
            
            // does a file which does not exist fail to open as expected?
            ++num_tests_run;
            auto fd = open("/persistent/file_system_example/root/does_not_exist", O_RDONLY);
            if (fd != -1 || errno != ENOENT)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "open(\"/persistent/file_system_example/root/does_not_exist\",O_RDONLY) should have failed with errno = ENOENT, but did not.  It returned " + std::to_string(fd) + ", and errno: " + errno_string());
            }
            else
                inst->PostMessage(LINE_PFX + "open(\"/persistent/file_system_example/root/does_not_exist\", O_RDONLY)");
            
            // can we create a new file?
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/small_file",O_RDONLY | O_CREAT, 0666);
            if (fd == -1)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "open(\"/persistent/file_system_example/root/small_file\",O_RDONLY | O_CREAT, 0666) should have succeeded, but did not.  It returned -1, and errno: " + errno_string());
            }
            else
            {
                close(fd);
                inst->PostMessage(LINE_PFX + "open(\"/persistent/file_system_example/root/small_file\", O_RDONLY | O_CREAT, 0666)");
            }
            
            // can we create a file with write-only access, and does this access persist?
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/write_only_file",O_WRONLY | O_CREAT, S_IWUSR | S_IWGRP | S_IWOTH);
            if (fd == -1)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "open(\"/persistent/file_system_example/root/write_only_file\",O_WRONLY | O_CREAT, S_IWUSR | S_IWGRP | S_IWOTH) should have succeeded, but did not.  It returned -1, and errno: " + errno_string());
            }
            else
            {
                close(fd);
                inst->PostMessage(LINE_PFX + "open(\"/persistent/file_system_example/root/write_only_file\", O_WRONLY | O_CREAT, S_IWUSR | S_IWGRP | S_IWOTH)");
                
                // imediately after creation is the access mode S_IWUSR | S_IWGRP | S_IWOTH?
                ++num_tests_run;
                struct stat st;
                stat("/persistent/file_system_example/root/write_only_file",&st);
                if ((st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != (S_IWUSR | S_IWGRP | S_IWOTH))
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "stat(\"/persistent/file_system_example/root/write_only_file\",&st) incorrectly reported access as " + to_octal_string(st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) + " instead of " + to_octal_string(S_IWUSR | S_IWGRP | S_IWOTH));
                }
                else
                    inst->PostMessage(LINE_PFX + "stat(\"/persistent/file_system_example/root/write_only_file\",&st) correctly reported access as " + to_octal_string(S_IWUSR | S_IWGRP | S_IWOTH));
                
                ++num_tests_run;
                #if !defined(__native_client__) || PEPPER_VERSION >= 38
                // can we set the access time of this file and does that persist?
                struct utimbuf utb;
                utb.modtime = 17;
                utb.actime = 18;
                if (utime("/persistent/file_system_example/root/write_only_file", &utb) != 0)
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "utime(\"/persistent/file_system_example/root/write_only_file\", &utb) failed with errno: " + errno_string());
                }
                else
                {
                    inst->PostMessage(LINE_PFX + "utime(\"/persistent/file_system_example/root/write_only_file\", &utb)");
                    
                    // can we read them back right now?
                    ++num_tests_run;
                    struct stat st;
                    stat("/persistent/file_system_example/root/write_only_file", &st);
                    
                    // although the code above does set the atime field, following the
                    // "Criticism of atime" issues, we don't verify that it was set, or
                    // care whether it was set.
                    if (st.st_mtime != 17)
                    {
                        ++num_tests_failed;
                        inst->PostError(LINE_PFX + "immediately after setting with utime, file mod time should have been 17.  Instead is " + std::to_string(st.st_mtime));
                    }
                    else
                        inst->PostMessage(LINE_PFX + "immediately after setting with utime, file mod time was read as 17");
                }
                #elif defined(__native_client__)
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "Pepper_" + std::to_string(PEPPER_VERSION) + " is too old to run file modification time tests - it doesn't implement utime, please consider updating to at least Pepper_38");
                #endif
                
            }
            
            // can we open, read-only, an existing file (the one we just created above)
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/small_file",O_RDONLY);
            if (fd == 1)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "open(\"/persistent/file_system_example/root/small_file\",O_RDONLY) should have succeeded, but did not.  It returned -1, and errno: " + errno_string());
            }
            else
            {
                inst->PostMessage(LINE_PFX + "open(\"/persistent/file_system_example/root/small_file\", O_RDONLY)");
                
                // are we correctly blocked from writing to this file - we opened it O_RDONLY
                ++num_tests_run;
                const char* buf = "hello";
                auto written = write(fd, buf, strlen(buf));
                if (written != -1 || errno != EBADF)
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "write(read-only-fd, \"hello\", 5) should have failed with EBADF, but did not.  It returned " + std::to_string(written) + ", and errno: " + errno_string());
                }
                else
                     inst->PostMessage(LINE_PFX + "write(read-only-fd, \"hello\", 5)");
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
                inst->PostError(LINE_PFX + "open(\"/persistent/file_system_example/root/small_file\",O_WRONLY) should have succeeded, but did not.  It returned -1, and errno: " + errno_string());
            }
            else
            {
                inst->PostMessage(LINE_PFX + "open(\"/persistent/file_system_example/root/small_file\", O_WRONLY)");
                
                // are we correctly blocked from reading from this file?
                ++num_tests_run;
                char b;
                auto bytes_read = read(fd, &b, 1);
                if (bytes_read != -1 || errno != EBADF)
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "read(write-only-fd, &b, 1) should have failed with errno EBADF, but did not.  It returned " + std::to_string(bytes_read) + ", and errno: " + errno_string());
                }
                else
                    inst->PostMessage(LINE_PFX + "read(write-only-fd, &b, 1)");
                
                // can we write to this file?
                ++num_tests_run;
                auto written = write(fd, hello, strlen(hello));
                if (written != strlen(hello))
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "write(write-only-fd, \"hello\", 5) should have succeeded, but did not.  It returned " + std::to_string(written) + ", and errno: " + errno_string());
                }
                else
                    inst->PostMessage(LINE_PFX + "write(write-only-fd, \"hello\", 5)");
                close(fd);
            }
            
            // can we open, read-write, an existing file?
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/small_file",O_RDWR);
            if (fd == -1)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "open(\"/persistent/file_system_example/root/small_file\",O_RDWR) should have succeeded, but did not.  It returned -1, and errno: " + errno_string());
            }
            else
            {
                inst->PostMessage(LINE_PFX + "open(\"/persistent/file_system_example/root/small_file\", O_RDWR)");
                
                // can we read from this file, and does it contain "hello"?
                ++num_tests_run;
                char rd_buf[32] = {0};
                auto bytes_read = read(fd, &rd_buf[0], sizeof(rd_buf));
                if (bytes_read != strlen(hello))
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "read(read-write-fd, &buf, " + std::to_string(sizeof(rd_buf)) + ") should have returned 5, but did not.  It returned " + std::to_string(bytes_read) + (bytes_read == -1 ? (std::string(", and errno: ") + errno_string()) : std::string("")));
                }
                else if (memcmp(rd_buf, hello, strlen(hello)) != 0)
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "read(read-write-fd, &buf, sizeof(buf)) should have read \"hello\", but did not.  It read: " + &rd_buf[0]);
                }
                else
                    inst->PostMessage(LINE_PFX + "read(read-write-fd, &buf, sizeof(buf))");
                
                // can we also write (append) to this file?
                ++num_tests_run;
                const char* buf = " world";
                auto written = write(fd, buf, strlen(buf));
                if (written != strlen(buf))
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "write(read-write-fd, \" world\", 6) should have succeeded, but did not.  It returned " + std::to_string(written) + ", and errno: " + errno_string());
                }
                else
                    inst->PostMessage(LINE_PFX + "write(read-write-fd, \" world\", 6)");
                
                // can we lseek to the middle and read the right stuff?
                ++num_tests_run;
                auto reslt = lseek(fd, 3, SEEK_SET);
                if (reslt != 3)
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "lseek(fd, 3, SEEK_SET) should have returned 3, but did not.  It returned: " + std::to_string(reslt));
                }
                {
                    inst->PostMessage(LINE_PFX + "lseek(fd, 3, SEEK_SET)");

                    ++num_tests_run;
                    bytes_read = read(fd, &rd_buf[0], 3);
                    if ((bytes_read != 3) || (memcmp(&rd_buf[0], "lo ", 3) != 0))
                    {
                        ++num_tests_failed;
                        rd_buf[3] = 0;
                        inst->PostError(LINE_PFX + "lread(fd, &rd_buf[0], 3) should have read \"lo \", but did not.  It returned: " + std::to_string(bytes_read) + " and read: \"" + &rd_buf[0] + "\"");
                    }
                    else
                        inst->PostMessage(LINE_PFX + "read correctly returned \"lo \"");
                }
                
                // can we seek past the end of the file and write some new data and does that cause
                // the gap between the old and new data to be zero?
                ++num_tests_run;
                reslt = lseek(fd, 2, SEEK_END);
                if (reslt != 13)
                {
                    ++num_tests_failed;
                    inst->PostError(LINE_PFX + "lseek(fd, 2, SEEK_END) should have returned 13, but did not.  It returned: " + std::to_string(reslt));
                }
                else
                {
                    inst->PostMessage(LINE_PFX + "lseek(fd, 2, SEEK_END)");
                    
                    ++num_tests_run;
                    reslt = write(fd, "goodbye", 7);
                    if (reslt != 7)
                    {
                        ++num_tests_failed;
                        inst->PostError(LINE_PFX + "write(fd, \"goodbye\", 7), but did not.  It returned: " + std::to_string(reslt));
                    }
                    else
                    {
                        inst->PostMessage(LINE_PFX + "write(fd, \"goodbye\", 7)");
                        
                        ++num_tests_run;
                        lseek(fd, 0, SEEK_SET);
                        read(fd, &rd_buf[0], sizeof(rd_buf));
                        char b[32];
                        memcpy(b, "hello world  goodbye", 20);
                        b[11] = b[12] = 0;
                        if (memcmp(&rd_buf[0], &b[0], 20) != 0)
                        {
                            ++num_tests_failed;
                            rd_buf[20] = 0;
                            inst->PostError(LINE_PFX + "file should have contained \"hello worldZZgoodbye\", but did not, it contained: \"" + &rd_buf[0] + "\"");
                        }
                        else
                            inst->PostMessage(LINE_PFX + "file correctly contained \"hello worldZZgoodbye\" (with the Z's being NULL bytes)");
                    }
                    
                }
                
                
                close(fd);
            }
            
            // can we create and write to a really big file?
            ++num_tests_run;
            fd = open("/persistent/file_system_example/root/big_file",O_RDWR | O_CREAT, 0666);
            if (fd == -1)
            {
                ++num_tests_failed;
                inst->PostError(LINE_PFX + "open(\"/persistent/file_system_example/root/big_file\",O_RDWR | O_CREAT, 0666) should have succeeded, but did not.  It returned -1, and errno: " + errno_string());
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
                        inst->PostError(LINE_PFX + "write to really big file failed after writing " + std::to_string(i*len) + " bytes");
                        break;
                    }
                }
                inst->PostMessage(LINE_PFX + "wrote " + std::to_string(kReallyBigFileSize*len) + " bytes to \"/persistent/file_system_example/root/big_file\"");
                close(fd);
            }
            
        }
    }
    else
    {
        inst->PostError(LINE_PFX + "This browser does not support persistent storage. Persistent tests will not be tried.");
    }
    
    return std::make_pair(num_tests_run, num_tests_failed);
}
