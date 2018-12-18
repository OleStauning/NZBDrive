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

If you do not already have Visual Studio 2017 or later then download and start a developer virtual machine from:

https://developer.microsoft.com/en-us/windows/downloads/virtual-machines

Start "Visual Studio Installer" and add the following components:

- Visual C++ MFC for x86 and x64

- C++/CLI support

Install Dokany from:

https://github.com/dokan-dev/dokany/releases

Install git:

https://git-scm.com/download/win

Launch git-bash and clone NZBDrive repository:

$ git clone https://github.com/OleStauning/NZBDrive.git

Clone tinyxml2 repository:

$ git clone https://github.com/leethomason/tinyxml2.git

Follow the clone- and build instructions for boost:

https://github.com/boostorg/boost/wiki/Getting-Started

Clone the OpenSSL repository:

$ git clone https://github.com/openssl/openssl.git

Follow the notes that comes with the OpenSSL source to build the libraries (requires installation of Perl and NASM) and install with prefix ./openssl/x64 and ./openssl/x86.

Start Visual Studio and batch-build all in NZBDrive.sln. The binaries are now located in x86/x64.

Download and install InnoSetup if you want to make an installer:

http://www.jrsoftware.org/isdl.php

Download the files:

- "dotNetFx45_Full_setup.exe" from https://www.microsoft.com/en-hk/download/confirmation.aspx?id=30653

- "DokanSetup.exe" from https://github.com/dokan-dev/dokany/releases

To the folder "NZBDrive\Source\Windows\NZBDriveInnoSetup". Start Inno Setup Compiler and compile the file "NZBDriveSetupScript.iss".

## Usage in Linux
Create an empty directory that you want to use as mount point.

`mkdir mnt`

Then you can mount an nzb-file with the command:

`nzbmounter mnt filename.nzb`

## TODO'S:
* Fix warnings
* Proper configure scripts.
* 




