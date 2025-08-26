#if !defined(_WIN32) || defined(_DEBUG)
#define USE_WIN_MAIN 0
#else
#define USE_WIN_MAIN 1
#endif

#if USE_WIN_MAIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

import sandbox.client;

#if USE_WIN_MAIN
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
)
#else
int main(int argc, char* argv[])
#endif
{
    auto& client = sandbox::Client::getInstance();
    if (!client.initialize()) {
        return 1;
    }
    client.run();
    client.dispose();
    return 0;
}
