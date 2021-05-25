Changes of this repo
--------------------

- added info about Tunnel ID of EoIP header
- fixed some issue on description of EoIPv6 header
- added dev option to command line for Linux, for VRF binding
- code reformatted (also added `.clang-format`)
- shifted to use CMake

### CMake build & installation guide

(Make sure both [Make](https://en.wikipedia.org/wiki/Make_(software)) and [CMake](https://cmake.org/) are installed to the system)

```
# cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
# make -C build install
```

Also you can choose your favorite build system (e.g. [Ninja](https://ninja-build.org/)) by setting `-G` option to CMake.


### Issues

In my test environment, nothing received with EoIP tunnel (tcpdump found the imcoming packet but the socket got nothing), while EoIPv6 worked fine. 
