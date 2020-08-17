#include <argh.h>
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

int run_tests(int argc, char *argv[])
{
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    return context.run();
}

int main(int argc, char *argv[])
{
    argh::parser args(argv);
    if (args[{"-t", "--test"}])
    {
        return run_tests(argc, argv);
    }
}
