# Snow tests

This is the test suite for Snow. It's probably not a very great test suite, and
I prefer writing test libraries over writing test suites (Snow started out as a
way to procrastinate instead of writing tests for another project...), but I
feel like there should be at least some test coverage of Snow.

Testing a test suite is a bit strange, especially when it tries to be somewat
"magic" like Snow does. My solution is to have programs in `cases/*.c` which
should produce some expected output, and then the test suite (`test.c`) runs
those programs and analyze their output, and verify that it works as expected.

The `test.c` file uses an old version of the `snow.h` header in `snow/snow.h`,
while the programs in `cases/` use the current version of Snow from
`../snow/snow.h`. The `snow/snow.h` file should be updated to reflect
`../snow/snow.h` for every release.

On Windows, this test suite assumes you're using MinGW to compile, and Git Bash
or another Linux-style shell to run the test suite.
