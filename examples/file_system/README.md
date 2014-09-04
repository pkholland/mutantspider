<h2>File System</h2>

Project containing usage examples and test code for the two file systems implemented
by mutantspider.

<b>Some Background</b>

While much of the "normal" behavior of an operating system can be implmeneted in
a web-based application, normal POSIX -like file io frequently can't.  There are
several reasons for this including general lack of access to the host OS file system,
lack of "blocking" API calls like fread, and differences in the way file -like data
persists from one view of a web page to the next.

While not all of these problems can be overcome completely, both Emscripten and
Native Client contain components to partially address the difference between what
a C, POSIX-like codebase might expect and what can realistially be implemented inside
of a web page.

Mutantspider contains two additional file system services of its own, also aimed at
reducing this difference.  The basic idea for all of these is that code that can live
with certain restrictions and/or compromises can otherwise get POSIX-based file io
operations to work.

<b>Mutantspider's two file systems</b>

From the perspective of the C code, both of mutantspider's file systems are accessed
as particular locations within the normal filesystem.  These two locations are:

    /persistent

File's within the /persistent subdirectory persist from one page view to the next.
So, for example, the file /persistent/my_credentials can be used to store information
that a user might have entered previously when visiting your web app.

    /resources
    
"Resources" are mutantspider's term for files like icons, menu strings, and other
data that you might have in a file when developing your application, and your code
expects to be able to open and read that file when executing.  The mutantspider
build systems has a mechanism allowing you to list all of the files of this type
in a way that 'make' can understand, and then make available within the /resources
subdirectory when your application runs.

<b>This example project</b>

This example project builds the test and verification code both of the file systems
that mutantspider implements.  You build and run this project the same way you do
for the hello_spider example.  The visual result of the web page is mostly just telling
you whether all tests passed, or if not, which ones failed.  But you can use the
source code in the is example to see how these services are intended to work.
