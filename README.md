# NZBDrive
Mount NZB files as drives or folders in Linux or Windows (under construction!).
## Dependencies
* C++11
* Tinyxml
* Libfuse on Linux
* Dokanx on Windows
* Openssl
* Boost libraries:
  * thread
  * filesystem
  * system
## How to compile on Linux
Install prerequisites and run _make_ in the src-directory. This should generate an executable _nzbmounter_.
## Usage in Linux
Create an empty directory that you want to use as mount point.
```mkdir mnt```
Then you can mount an nzb-file with the command:
```nzbmounter mnt filename.nzb```
## TODO'S:
* Proper configure scripts.
* 