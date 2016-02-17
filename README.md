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

Mutantspider contains support to automatically construct the binding glue necessary to allow javascript and
C/C++ to call each other.

<b>Getting Started</b>

The best way to get started is to run, or better yet build, some of the example projects that use Mutantspider.
The source code for these can be found in the [mutantspider-examples repository](https://github.com/pkholland/mutantspider-examples).  A very simple, "hello world"
style Mutantspider project that is already built and running as a web page can be found [here]
(http://www.mutantspider.tech/code/hello-spider/index.html)


