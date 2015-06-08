Building
========
Building ``libbeemo`` is pretty straightforward as far as it goes. However, it varies a bit depending on your platform.
For this tutorial, we'll be assuming a Linux based OS using the GCC C compiler.
While libbeemo works just-fine-thankyou on windows, building any C or C++ software is fraught with difficulty on that platform, and involves a lot more "Click on this"-type instructions, rather than "paste this into the console and be done with it" type instructions.

To ease the lexical burden, we're going to write from a Linux-centric perspective.
However, once the build-dependencies are installed, the steps are the same.


Build dependencies
^^^^^^^^^^^^^^^^^^
First off, the only hard dependencies are as follows

    - A C99 conformant compiler (e.g. ``gcc``, ``clang``, ``icc``)
    - ``pthreads`` support (shipped with the compiler)
    - `liblua <http://lua.org>`_ (version 5.2)
    - `SCons <http://scons.org>`_ for building (requires a Python2.7 installation)

For completeness you'll also want the following development headers and libraries installed.

    - `libportaudio19 <http://portaudio.com/>`_ (for hearing your work)
    - `kibjack <http://jackaudio.org>`_ (for advanced audio routing outside the library)
    - `libsndfile <http://mega-nerd.com/libsndfile/>`_ (for reading audio other than simple .au and .wav files)
    - `ladspa <http://www.ladspa.org/>`_
    - ``libasan`` (clang/GCC 4.8+; testing only)
    - ``valgrind`` (Linux only; testing only)
    - ``scan-build`` (clang; testing only)
    - ``LADSPA`` header file (for LADSPA plugin support)


For Debian-based systems like Ubuntu, the following should get you
everything you need to build:

::

    sudo apt-get install build-essential scons valgrind portaudio19-dev libjack-dev ladspa-sdk liblua5.2-dev libsndfile1-dev lua5.2

Other package managers may vary in the names/syntax of the above
commands, but the spirit is similar.

Building
~~~~~~~~
Building ``libbeemo`` is easy, provided you've installed the hard
dependencies from above.

Once you've installed the prequisites using either your OS's package
manager, or manually, and configured your ``PATH`` correctly (Windows, I'm looking at you)
you can build both dynamic and static libraries simply with:

::

    scons

If you want to run the tests:

::

    scons tests

from the project's root directory should build the library and run the
automated test suite.

A complete shell script that should fetch and build the latest
development version from scratch on a fresh Debian system, and drop you
straight into an interactive interpreter:

::

    ```
    sudo apt-get install -y git build-essential scons valgrind portaudio19-dev libjack-dev ladspa-sdk liblua5.2-dev libsndfile1-dev lua5.2
    cd /tmp
    git clone https://github.com/ldrumm/libbeemo.git
    cd libbeemo
    source tests/activate
    scons tests
    ./test/interactive/repl

    ```

Extra build options
~~~~~~~~~~~~~~~~~~~

::

    `--disable=feature1,feature2 `
        build without support for each item in the comma-separated list.
    `--enable=item1,item2`
        include the extra feature items.  Currently supported features are:
            - `asan` : build against the libasan Address Sanitizer. Any faulty memory accesses will cause the program to `abort()` with a detailed traceback on the evil memory access. Requires a recent version of Clang or GCC.
            - `anaylse`: run the scan-build static analyser. Invocation is `scan-build scons --enable=analyse`. **Requires the clang static analyser**
            - `release`: do an optimized build without debugging info, disabled asserts and with build warnings ignored.
            - `profile`: semi-above; useful for performance analysis as debugging symbols are included, but basic optimisations are made.

Given the above, the invocation
``scons --enable=release --disable=ladspa,jack`` will perform an optimised build with all features except LADSPA and jack support built-in. See ``SConstruct`` for more info.

Building on Windows
~~~~~~~~~~~~~~~~~~~

| The C99 prerequisite rules out building with MSVC, as Microsoft appear
  to believe C no longer exists, and *everyone* is using C# .NET. There
  will probably never be a Microsoft c99 compiler.
| Fortunately, it is becoming relatively more easy to aqcuire a decent
  C99 compiler on Windows.

[MinGW-w64][2] or [MinGW][3] is recommended. You'll also need Python and
SCons installed.

#. First install MinGW using the default options. Using ``mingw-get``,
   make sure liblua5.2-dev, gcc, binutils, is installed.
#. Install Python with the [official python installer for windows][4]
#. Once Python is installed, run the SCons installer, selecting the path
   of your new Python2 installation in the process.
#. From the Control Panel->System->Advanced Settings, make sure that
   your MinGW *bin* directory, your Python installation's *bin* and
   *Scripts* directories are added to your system's path.
#. Download [Portaudio][5], and [libsndfile][6].
#. Open an msys shell from your MinGW directory.
#. For each of Portaudio and libsndfile, extract the archives and ``cd``
   into the source root.
#. For each of Portaudio and libsndfile, run
   ``./configure --prefix="$(cd /mingw; pwd -W)" && make && make install``
   from their source root.
#. From a Windows command prompt (not msys) run ``scons`` from the
   libbeemo source root.
#. To setup the test environment, run ``call tests\activate``.

For all other platforms, you probably already have a sane toolchain. If
not, use you package manager to install the needed dependencies.

Build Environment Variables
~~~~~~~~~~~~~~~~~~~~~~~~~~~

| In general the build does not inherit the environment to enable
  deterministic builds. However, in certain situations, the build will
  check for the existence of an environment variable.
| The following environment variables are significant:
|  ``CC``: preferred compiler.
|  ``CI``, ``TRAVIS``, ``CONTINUOUS_INTEGRATION``, ``JENKINS``,
  ``MEMCHECK``: The existence of any of these variables causes some
  extra checks to be run during testing. Presently, this includes
  running Valgrind on all the testcases to check for memory errors.

Notes on tested platforms
~~~~~~~~~~~~~~~~~~~~~~~~~

-  All commits are tested at least against ``gcc`` and ``clang`` on
   x86\_64 Linux machines.
-  ``icc`` is known to have worked on 32bit Linux hosts in the past, but
   this is not regularly tested.
-  PPC/arm32 are occasionally tested in QEMU emulated Linux machines.
   Due to the overhead, this doesn't happen often.
-  Macintosh builds are done infrequently, but generally par with the
   Linux builds.
-  The test suite and general functionality of the library is exercised
   on Win32 using MinGW on a fairly regular basis. Though not part of
   the automated test suite, Windows is considered a target platform.


Assuming all went well, you should see ``scons: done building targets.`` as the last line output from the build command

Running the Automated Tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Unit tests for the main library - along with some simple interactive examples - can be built by running ``scons tests`` from the project root.
However, as the tests will need to be linked against the main library at runtime, the environment will have to be configured so that your operating system's dynamic linker knows where to find the ``libbeemo.(so|dll|dylib)`` library.

If you're using a POSIX compatible shell::

    . tests/activate
    scons tests

...should do it. On Win32::

    call tests\activate
    scons tests

If everything goes as planned, you should again see the friendly ``scons: done building targets.`` echoed onto your terminal indicating the tests passed.
For a more thorough treatment of the tests, see :ref:`tests`

..doxygenindex::
