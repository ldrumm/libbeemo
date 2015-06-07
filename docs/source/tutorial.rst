Tutorial
========
To get up to speed we'll create a simple project that creates a heartbeat sound, whose pulse-rate and intensity varies proportional to the keyboard rate. (Think of this like the amount of frenzied activity in your game). That is, when you're in mild peril, your heartbeat will be mildy perilous.  When you're near death and frantically trying to outwit your enemies, your heartbeat will be crazy fast, and loud.

Because th

Building
^^^^^^^^
First-off, we'll need to build libbeemo. This varies a bit depending on your platform.
For this tutorial, we'll be assuming a Linux based OS using the GCC C compiler.
While libbeemo works just-fine on windows, building any C or C++ software is frought with difficulty, and involves a lot more "Click on this"-type instructions, rather than "paste this into the console and be done with it.


Build dependencies
^^^^^^^^^^^^^^^^^^
First off, the only hard dependencies are as follows

    - A C99 conformant compiler (e.g. ``gcc``, ``clang``, ``icc``)
    - ``pthreads`` support (shipped with the
    - `liblua <http://lua.org>`_ (5.1 and 5.2 are supported)
    - `SCons <http://scons.org>`_for building (requires a Python2.7 installation)

For completeness you'll also want the following development headers installed.

    - `Portaudio <http://portaudio.com/>`_
    - `JACK <http://jackaudio.org>`_
    - `libsndfile <http://mega-nerd.com/libsndfile/>`_
    - `ladspa <http://www.ladspa.org/>`_

To get all of the above (assuming you're on a Debian-based machine) .::

    sudo apt-get install build-essential liblua5.2dev SCons libportaudio19-dev libjack-dev ladspa-sdk libsndfile-dev

Running the Build
^^^^^^^^^^^^^^^^^
To build libbeemo simply ``cd`` to the ``libbeemo`` directory and run the following::

    scons

Assuming all went well, you should see ``scons: done building targets.``
as the last line output from the build command



Running the Automated Tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Unit tests of the main library, along with some simple interactive examples can be built by running ``scons tests``.
However, as the tests will need to be linked against the main library at runtime, the environment will have to be configured so that your operating system's dynamic linker knows where to find the ``libbeemo.(so|dll|dylib)``

If you're using a POSIX compatible shell::

    . tests/activate
on Win32::

    call tests\activate
