#include "platform.h"

int main(int argc, char **argv) {
    PlatformContext platform_context{};
    platform_init(&platform_context);
    platform_main_loop(&platform_context);
    platform_terminate(&platform_context);
    return 0;
}
