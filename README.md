# NZBDrive
Mount NZB files as drives or folders in Linux or Windows (under construction!).

## Dependencies
* C++17 (Visual Studio 2017 on Windows)
* Tinyxml2
* Openssl
* Boost libraries:
  * thread
  * filesystem
  * system
* Libfuse (Linux)
* Dokany (Windows)
* Extended WPF Toolkit (Windows)

## How to compile on Linux
Install prerequisites and run `make` in the src-directory. This should generate an executable `nzbmounter`.

## How to compile on Windows

Download and install Visual Studio Community from <https://www.visualstudio.com/downloads/>
* enable .NET desktop development with .NET framework 4.8.
* enable Desktop development with C++ and Visual C++ MCF for x86 and x64 and C++/CLI Support.

Download and install SDK10 from <https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk>

Donwload and install WDK10 from <https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit>

Clone the Dokany library from <https://github.com/OleStauning/dokany.git> in the same directory as you cloned NZBDrive.

Enter the dokany directory and run bat-file `build.bat`. 

Read the wiki page <https://github.com/dokan-dev/dokany/wiki/Installation> about how to install dokany on your computer, or you 
can just download an installer at <https://github.com/dokan-dev/dokany/releases>. Notice that you have to enable "Test Mode" 
on Windows if you are installing the dokan driver on 64 bit Windows and your driver has been signed by the test certificate:
```
bcdedit.exe -set loadoptions DDISABLE_INTEGRITY_CHECKS
bcdedit.exe -set TESTSIGNING ON
```
Then reboot.

Download either 32 and/or 64 bit boost binaries from <https://sourceforge.net/projects/boost/files/boost-binaries/1.68.0/>

* boost_1_68_0-msvc-14.1-64.exe
* boost_1_68_0-msvc-14.1-32.exe

Install boost in directory "boost" in the same directory as you cloned NZBDrive.

Download and install Perl from <http://www.activestate.com/activeperl/downloads>.

Download and install nasm from <http://www.nasm.us/pub/nasm/releasebuilds/2.12.02>

Download openssl-1.1.0e.tar.gz from <https://www.openssl.org/source/> extract and rename to the directory "openssl" in the same directory as you cloned NZBDrive.

Enter the "openssl" directory.

For compiling 32 bit libraries:

Set environment variables by running `C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\vcvars32.bat`

Configure the build files by running `perl Configure VC-WIN32 --prefix=e:\openssl\x86`

Then build the openssl library by running `nmake clean` and `nmake install`

For compiling 64 bit libraries:

Set environment variables by running `C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\vcvars64.bat`

Configure the build files by running `perl Configure VC-WIN64A --prefix=e:\openssl\x64`

Then build the openssl library by running `nmake clean` and `nmake install`

Clone tinyxml2 from <https://github.com/leethomason/tinyxml2.git> in the same directory as you cloned NZBDrive.

Download the Community Edition of the Extended WPF Toolkit from <http://wpftoolkit.codeplex.com/>, and unpack to in the same directory as you cloned NZBDrive.

Download curl 7.62.0 for Windows https://curl.haxx.se/windows/. Install in same directory as you cloned NZBDrive and rename to curl32 and/or curl64.

Finally: open the solution file NZBDrive.sln in Visual Studio and Build the project "NZBDrive".

You will now have a directory structure like this:
```
/
├── boost/
├── curl32/
├── curl64/
├── dokany/
├── Extended WPF Tooklit Binaries/
├── NZBDrive/
│   ├── Source/
│   │   ├── Linux/
│   │   ├── NZBDriveLib/
│   │   ├── Windows/
│   │   ├── x64
│   │   ├── x86
│   │   ├── Makefile
│   │   └── NZBDrive.sln
│   ├── License
│   └── README.md
├── openssl/
└── tinyxml2/
```
The binaries are located in x86/x64.

## Usage in Linux
Create an empty directory that you want to use as mount point.

`mkdir mnt`

Then you can mount an nzb-file with the command:

`nzbmounter mnt filename.nzb`

## TODO'S:
* Fix warnings
* Proper configure scripts.
* 




