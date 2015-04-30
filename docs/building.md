Build Instructions
==================

### Hard Build Dependencies
    - A C99 compiler (GCC, or clang is recommended)
    - [liblua5.2][0]
    - [SCons][1] (which in-turn requires Python2.7)

### Optional Build Dependencies
    - libsndfile (for reading audio other than PCM .au and .wav)
    - portaudio (for hearing your work)
    - libjack (for advanced audio routing outside the library)
    - libasan (clang/GCC 4.8+; testing only)
    - Valgrind (Linux only; testing only)
    - scan-build (clang; testing only)
    - LADSPA header (for LADSPA plugin support)

For Debian-based systems like Ubuntu, the following should get you everything you need to build:

    sudo apt-get install build-essential scons valgrind portaudio19-dev libjack-dev ladspa-sdk liblua5.2-dev libsndfile1-dev lua5.2

Other package managers may vary in the names/syntax of the above commands, but the spirit is similar.

### Building

TL;DR: `source tests/activate && scons tests`

Building `libbeemo` is easy, provided you've installed the hard dependencies from above.

Once you've installed the prequisites using either your OS's package manager, you can build both dynamic and static libraries simply with:

    scons

If you want to run the tests:

    scons tests

from the project's root directory should build the library and run the automated test suite.

A complete shell script that should fetch and build the latest development version from scratch on a fresh Debian system, and drop you straight into an interpreter:

    ```
    sudo apt-get install -y git build-essential scons valgrind portaudio19-dev libjack-dev ladspa-sdk liblua5.2-dev libsndfile1-dev lua5.2
    cd /tmp
    git clone https://github.com/ldrumm/libbeemo.git
    cd libbeemo
    source tests/activate
    scons tests
    ./test/interactive/repl

    ```

### Extra build options
    `--disable=feature1,feature2 `
        build without support for each item in the comma-separated list.
    `--enable=item1,item2`
        include the extra feature items.  Currently supported features are:
            - `asan` : build against the libasan Address Sanitizer. Any faulty memory accesses will cause the program to `abort()` with a detailed traceback on the evil memory access. Requires a recent version of Clang or GCC.
            - `anaylse`: run the scan-build static analyser. Invocation is `scan-build scons --enable=analyse`. **Requires the clang static analyser**
            - `release`: do an optimized build without debugging info, disabled asserts and with build warnings ignored.
            - `profile`: semi-above; useful for performance analysis as debugging symbols are included, but basic optimisations are made.

Given the above, the invocation `scons --enable=release --disable=ladspa,jack` will perform an optimised build with all features except LADSPA and jack support built in. See `SConstruct` for more info.

### Building on Windows
The C99 prerequisite rules out building with MSVC, as Microsoft appear to believe C no longer exists, and *everyone* is using C# .NET. There will probably never be a Microsoft c99 compiler.
Fortunately, it is becoming relatively more easy to aqcuire a decent C99 compiler on Windows.

[MinGW-w64][2] or [MinGW][3] is recommended. You'll also need Python and SCons installed.
1. First install MinGW using the default options.  Using `mingw-get`, make sure liblua5.2-dev, gcc, binutils, is installed.
2. Install Python with the [official python installer for windows][4]
3. Once Python is installed, run the SCons installer, selecting the path of your new Python2 installation in the process.
4. From the Control Panel->System->Advanced Settings, make sure that your MinGW *bin* directory, your Python installation's *bin* and *Scripts* directories are added to your system's path.
5. Download [Portaudio][5], and [libsndfile][6].
6. Open an msys shell from your MinGW directory.
7. For each of Portaudio and libsndfile, extract the archives and `cd` into the source root.
8. For each of Portaudio and libsndfile, run `./configure --prefix=`cd /mingw; pwd -W` && make && make install` from their source root.
9. From a Windows command prompt (not msys) run `scons` from the libbeemo source root.
10. To setup the test environment, run `call tests\activate`.

For all other platforms, you probably already have a sane toolchain. If not, use you package manager to install the needed dependencies.

### Build Environment Variables
In general the build does not inherit the environment to enable deterministic builds. However, in certain situations, the build will check for the existence of an environment variable.
The following environment variables are significant:
    `CC`: preferred compiler.
    `CI`, `TRAVIS`, `CONTINUOUS_INTEGRATION`, `JENKINS`, `MEMCHECK`: The existence of any of these variables causes some extra checks to be run during testing. Presently, this includes running Valgrind on all the testcases to check for memory errors.

### Notes on tested platforms

- All commits are tested at least against `gcc` and `clang` on x86_64 Linux machines.
- `icc` is known to have worked on 32bit Linux hosts in the past, but this is not regularly tested.
- PPC/arm32 are occasionally tested in QEMU emulated Linux machines. Due to the overhead, this doesn't happen often.
- Macintosh builds are done infrequently, but generally par with the Linux builds.
- The test suite and general functionality of the library is excercised on Win32 using MinGW on a fairly regular basis. Though not part of the automated test suite, Windows is considered a target platform.


[0][http://www.lua.org/versions.html#5.2]
[1][http://www.scons.org]
[2][http://mingw-w64.yaxm.org]
[3][http://mingw.org]
[4][https://www.python.org/downloads/windows/]
