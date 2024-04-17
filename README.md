A TCP/IP protocol stack written in C++, implementing transmission control, IP routing, ARP protocol, and communicating with external networks through Linux virtual device TAP.
![](https://obsidian-1314737433.cos.ap-beijing.myqcloud.com/202404162128670.png)
![](https://obsidian-1314737433.cos.ap-beijing.myqcloud.com/202404171357696.png)
![](https://obsidian-1314737433.cos.ap-beijing.myqcloud.com/202404171302061.png)

Stanford CS 144 Networking Lab
==============================

These labs are open to the public under the (friendly) request that to
preserve their value as a teaching tool, solutions not be posted
publicly by anybody.

Website: https://cs144.stanford.edu

To set up the build system: `cmake -S . -B build`

To compile: `cmake --build build`

To run tests: `cmake --build build --target test`

To run speed benchmarks: `cmake --build build --target speed`

To run clang-tidy (which suggests improvements): `cmake --build build --target tidy`

To format code: `cmake --build build --target format`
