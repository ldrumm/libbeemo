Unit Tests
==========
``libbeemo`` comes with a test-suite to test both the core C library, as well as the Lua binding that is exposed to the composer.
If you're doing any sort of development on ``libbeemo``, it's definitely worth familiarising yourself with building and running the tests.
If you've not read it already see :ref:`building`_ for how to build the main library and the tests.


Let's create two tests, ``tests/example_failure.c``::

    #include <assert.h>

    int main(void)
    {
        int night = 1;
        int day = 0;
        assert(night == day);
        return 0;
    }

``tests/example_error.c``::

    #include <stdlib.h>
    int main(void)
    {
        void *ptr = NULL;
        int(*func)(void) = ptr;
        return func();
    }

and now run the complete testsuite::

    scons tests

As you can see from the source, these tests are really not going to behave and will exhibit two distinct failure modes; **failure** and **error**.  The table below shows a matrix of a particular exit scenario and the error classification

    +-----------------------+----------------------+---------------------------------------------+
    |Failure                |Error                 |Pass                                         |
    +=======================+======================+=============================================+
    |- Assertion failures   |- Killed by signal    |- Successful return (``exit(EXIT_SUCCESS)``) |
    |- Nonzero return       |- Environment error   |                                             |
    +-----------------------+----------------------+---------------------------------------------+


The actual build output for our above examples should look something like this::

    ================================================================================
    FAILURE: '/tmp/libbeemo/tests/test_example_fail'

    test_example_fail: tests/example_fail.c:7: main: Assertion `night == day' failed.

    --------------------------------------------------------------------------------
    ================================================================================
    FAILURE: '/tmp/libbeemo/tests/test_example_error'


    --------------------------------------------------------------------------------
    scons: *** [tests/51538c42d20b4e29b2dc4e61596cde8f]
    2 test failures: ('test_example_fail', 'test_example_error')
    0 test errors

    scons: building terminated because of errors.


If you look closely at the above error block, you'll see that the build system has built ``example_fail.c`` as ``test_example_fail``.
This is a side-effect of how the build system works.  For now, just realise that all a test has to do is to ``return 0;`` to be considered a success.  To test your assumptions about data state, sprinkle your unit test with calls to ``assert(3)``.

C Test API
^^^^^^^^^^
The test API is designed to be as simple as possible.
Because of this, there are some basic requirements and limitations for the test-suite:

    - unit tests take no arguments. ``main()`` **must** be declared as ``int main(void);``
    - unit tests are single source. One ``.c`` file equates to a single unit test. The reasoning for this requirement is to keep the test-discovery and build stage as simple as possible. This behaviour will probably have to be modified as the library grows, but for now, it seems to serve its purpose just fine.
    - In your unit tests, the following environment variables are significant and affect the correspondingly named global variable in ``tests/lib/test_common.c``:

        - ``CHANNELS``: The number of audio channels to use for input/output
        - ``FRAMES``: The number of frames per buffer.  This affects (among other things) input/output latency.
        - ``RATE``: The audio engine sample rate as an integer (Hz)
        - ``LOGLEVEL``: How chatty the library will be.  This defaults to ``BMO_MESSAGE_DEBUG`` as defined in ``src/error.h``.  Valid values in decreasing level of verbosity: ``(DEBUG|INFO|ERROR|NONE)``
        - ``DRIVER``: The audio driver to use. ``(JACK|PORTAUDIO)``


        To get access to the above globals, simply ``#include "lib/test_common.c"`` and call ``bmo_test_setup()`` as the first line of your ``main()`` function.

Lua Test API
^^^^^^^^^^^^
The Lua test API is based on *TestingUnit* and all tests for the Lua bindings live in ``tests/lua``.

.. warning::
    TODO
