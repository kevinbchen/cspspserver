# CSPSP Server

This is the server application for [CSPSP](https://github.com/kevinbchen/cspsp), a homebrew game for the Sony PSP.
If you want to download and run the prebuilt server application, see the [Releases](https://github.com/kevinbchen/cspspserver/releases) page.
See [CSPSPServer/README.txt](CSPSPServer/README.txt) for instructions to run a server.

> **Warning**: I created this project back in high school and didn't expect to open source it, so the code is messy and, to put it mildy, not well written. I would not use this as any kind of reference :)

## Building
You can build the server application from source as either a GUI application (Windows only) or as a console application. The releases include the slightly more user-friendly GUI version, but the console application can be run on other platforms (Linux) and may be better for remote deployment.

**Important**: On startup, the server will register itself with the master server, making it visible to CSPSP clients. If you are making any incompatible changes to the networking (for example, adding or changing message types), please update the `NETVERSION` define in [CSPSPServer/GameServer.h](CSPSPServer/GameServer.h).

### GUI (Windows only)
Simply open `CSPSPServer.sln` with Microsoft Visual Studio 2019 with the "Desktop development with C++" component installed. Other recent Visual Studio versions will likely work too, but VS2019 was what I most recently tested on.

### CMake Console Application (Windows / Linux) 
Use CMake 3.8+. Example build instructions:
```
cd cspspserver
mkdir build && cd build
cmake ../
make
```
The CSPSPServer application will be output to `cspspserver/CSPSPServer/`. To run, assuming you're still in the `build/` directory:
```
(cd ../CSPSPServer; ./CSPSPServer)
```
> **Note**: The Linux filesystem is case-sensitive. This is relevant when specifying map names (e.g. in `mapcycle.txt` or via the `/map` command). In addition, map files `map.txt`, `tile.png`, `overview.png` must be all lowercase.
