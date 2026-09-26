#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    (void)argc;
    if (setenv("LD_LIBRARY_PATH",
               "/vendor/lib64/gps_legacy:/system/lib64:/vendor/lib64", 1) != 0)
        return 1;

    argv[0] = "/vendor/bin/lowi-server";
    execv(argv[0], argv);
    return 1;
}
