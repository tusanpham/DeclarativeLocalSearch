# IDP solver with local search support
The repository consists of 3 folders. 
Folder _idpsourcecode_ includes the solver source code. 
Folder _idpbinary_ includes binary files.
Folder _TSPmodelling_ includes the problem's modelling, neighbourhood modelling and data instances for the Traveling Salesman Problem. 

Following are the instructions on how to build/install the software and run the example problems. 

## Building from source
Required software packages:
- C and C++ compiler, supporting most of the C++11 standard. Examples are GCC 4.4, Clang 3.1 and Visual Studio 11 or higher.
- Cmake build environment. 
- Bison and Flex parser generator software.
- Pdflatex for building the documentation.

Assume idp is unpacked in _idpdir_, you want to build in builddir (cannot be the same as _idpdir_) and install in _installdir_. Building and installing is then achieved by executing the following commands:

```
cd <builddir> cmake <idpdir> −DCMAKE_INSTALL_PREFIX=<installdir> −DCMAKE_BUILD_TYPE="Release"
make −j 4 
make check
make install
```

## Usage examples: 
For general usage, see [IDP manual](https://dtai.cs.kuleuven.be/krr/files/bib/manuals/idp3-manual.pdf). 

To execute the source file provided in this repository, the following command line can be used: 
```
Usage: 
	idp <source file> <instance file>

Examples: 
	idp tsp.idp instances/att48.tsp.struc
```
Currently, the above command line will invoke _first improvement_ using _2opt_ move. To run other local search algorithms, set the following parameter to the corresponding values:
```
stdoptions.localsearchtype = "firstimprove,2opt"
```
