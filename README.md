mutantspider
============

NaCl/asm.js Browser Framework

Tools for building web applications using C/C++, using Google's Native Client and Mozilla's Emscripten compilers.

The mutantspider project is intended to support the idea that C/C++ code can be a first-class citizen in the
development of web applications.  Projects using mutantspider will generally have a Makefile constructed so
that running 'make' compiles your C/C++ code into two different forms -- one, a PNaCl executable and two, an
asm.js file.  The mutantspider.mk file contains support logic to make this sort of Makefile fairly easy to
set up.  Mutantspider also contains a small amount javascript helper code letting you write web pages that
automatically load the PNaCl or asm.js version of your C/C++ code depending on the capabilities of the browser
viewing your page.

Mutantspider contains a C/C++ programming interface derived from the Native Client programming model Pepper.
In many cases classes in Pepper's "pp" namespace have identical names in the "mutantspider" namespace.  When
the code is compiled for PNaCl these mutantspider classes are directly mapped to the same-named classes in
Pepper.  When compiling for asm.js they are implemented, frequently with supporting code in javascript.

<b>Getting Started</b>

The best way to get started is to build and run some of the example projects in the "examples" directory.
For example, opening a console window and cd'ing to mutantspider/examples/hello_spider lets you then run
'make' to build the hello_spider web application.  Building any of these examples will require that your
machine have both the NaCl and Emscripten SDKs installed.  These are both freely available for download
on the web.

