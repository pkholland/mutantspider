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

Files within the /persistent subdirectory persist from one page view to the next.
So, for example, the file <code>/persistent/my_credentials.txt</code> could be used to store
information that a user might have entered during a previous visit to your web app.
If they had done so, and your application had written that information to this file
then you would be able to read it during this visit to your page.  This data is stored
locally on the user's machine - not on your server.

It is implemented using IndexedDB on Emscripten, and html5fs on NaCl, but adds some
behavior on top of those tools.  Emscripten contains a file system named IDBFS which
is similar to the /persistent file system.  But IDBFS requires an explicit "synchronize"
call to transfer file data from the file system to the persistent storage.  Mutantspider's
implementation automatically transfers all changed data to the persistent storage,
making it behave much more like a normal POSIX file system.

Google's html5fs does not require IDBFS's "synchronize" call, but can only be called
off of the main thread.  Mutantspider's /persistent file system can be accessed from
any thread.  But this comes with the cost of requiring that all data in this file system
be replicated in RAM.  So this is essentially a RAM-based file system that automatically
persists changes to the underlying html5fs file system (using a background thread).
All data in this file system from previous visits to your web page will be loaded into
RAM when the new visit starts, and will stay in RAM until the user exits or navigates
away from your page.

    /resources
    
"Resources" are mutantspider's term for files like icons, menu strings, and other
data that you might have in a file when developing your application, and your code
expects to be able to open and read that file when executing.  The mutantspider
build systems has a mechanism allowing you to list all of the files of this type
in a way that 'make' can understand, and then make available within this /resources
subdirectory when your application runs.

mutantspider/src/README.makefile contains information on how to use this RESOURCES
feature in your makefile.

<b>This example project</b>

This example project builds the test and verification code for both of the file systems
that mutantspider implements.  You build and run this project the same way you do
for the hello_spider example.  The visual result of the web page is mostly just telling
you whether all tests passed, or if not, which ones failed.  But you can use the
source code in this example to see how these services are intended to be used.
