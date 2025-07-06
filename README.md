# TheLilySpark
 
TheLilySpark is a low-levels system tool that attempts to solve the 
problem of inability to access the M1/M2/M3 GPU directly and draw a
window.

Mac's graphical stack often involves a lot more abstraction compared
to the windows.h header that Windows uses. Because there are less
calls to Win32 visible, it often appears as if these calls are never
used, since they run discretely in the background.

Coding with the GPU is a very difficult task given you have a lot of
barriers to working with hardware directly. This project aims to solve
the problem of accessing the GPU directly and drawing to a window.