#include <windows.h>

LRESULT CALLBACK
WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) {
        // If we receive the destroy message, then quit the program.
        PostQuitMessage(0);
        return 0;
    } else {
        // We don't handle this message. Use the default handler.
        return DefWindowProc(window, message, wParam, lParam);
    }
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE previousInstance, 
                     LPSTR commandLine, int commandShow) {
    // Create the window class.
    WNDCLASS windowClass = {0};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "MyWindowClass";
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    RegisterClass(&windowClass);
    
    // Create and show the actual window.
    HWND window = CreateWindow("MyWindowClass", "Hello, GUI!", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 400, 200, NULL, NULL, instance, NULL);
    ShowWindow(window, commandShow);
    UpdateWindow(window);
    
    // Process window messages.
    MSG message;
    while (GetMessage(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    
    return message.wParam;
}