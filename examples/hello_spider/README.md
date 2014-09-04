<h2>Hello Spider</h2>

Simple example of a project using mutantspider tools to build a web page that contains an
element that whose behavior is implemented by some C++ code.

<b>Getting Started:</b>

First thing, in a console window cd to this directory (the one with this README.md file in it)
and then execute:

    make run_server
    
If this succeeds it will have compiled the C++ code and built a simple web site that includes
the compiled code.  It will also have launched a simple web server on that site.  You can see
this web page by opening a browser of your choice and going to:

    http://localhost:5103

This should present you with a page titled "Hello Spider" with a big red box that you can
click on and drag in.  The code behind this red box is implemented in C++ in the file
hello_spider.cpp

<b>Basics of how it works:</b>

The index.html file contains a couple of <code>&#60script&#62</code> elements, one for
hello_spider_boot.js and the other for mutantspider.js.  mutantspider.js contains the basic
logic for checking whether your browser supports pnacl or not, and then if it does adding a
DOM element to trigger the download of the pnacl version of hello_spider.  If it does not
support pnacl then it adds a DOM element to trigger the download of the javascript version of
hello_spider.  hello_spider_boot.js just contains a small piece of code that is called when the
DOM finishes being parsed, and at that point calls the mutantspider support code.

The primary targets of the makefile build both the pnacl and javascript versions of
hello_spider.cpp (plus some support code) and copy these, along with the other needed files
into a directory named "deploy".  The "run_server" target of the makefile builds all of the
primary targets, and then runs a simple web server on port 5103.

