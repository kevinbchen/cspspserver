# CSPSP Server

This is the server application for [CSPSP](https://github.com/kevinbchen/cspsp), a homebrew game for the Sony PSP.
If you want to download and run the prebuilt server application, see the [Releases](https://github.com/kevinbchen/cspspserver/releases) page.

> **Warning**: I created this project back in high school and didn't expect to open source it, so the code is messy and, to put it mildy, not well written. I would not use this as any kind of reference :)

## Building

To build from source, simply open CSPSPServer.sln with Microsoft Visual Studio 2019. It will likely also work with older versions, but VS2019 was what I most recently tested on. Currently only works on Windows.

**Important**: On startup, the server will register itself with the master server, making it visible to CSPSP clients. If you are making any incompatible changes to the networking (for example, adding or changing message types), please update the `NETVERSION` define in [CSPSPServer/GameServer.h](CSPSPServer/GameServer.h).
