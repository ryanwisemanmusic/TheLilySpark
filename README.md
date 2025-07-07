# TheLilySpark

## Framebuffer Export Options

- Press 'd' to save framebuffer as PPM (output.ppm)
- Press 'x' to export framebuffer as text (output.txt, choose RGB or ASCII-art)
- Press 'c' to export framebuffer as CSV (output.csv)

## Human-Readable Exports for Other Components
- System probe and ARM opcode functions can log to text files for inspection.
- Extend or call their logging functions with a file pointer to capture output.

## Extending Exports
- To add more exports, use or extend the pattern in `framebuffer.c` and call from `main.c` as needed.

## Menu Options

- **d**: Save framebuffer as PPM (output.ppm)
- **x**: Export framebuffer as text (output.txt, choose RGB or ASCII-art)
- **c**: Export framebuffer as CSV (output.csv)
- **w**: Bruteforce window probing. Automatically tries a set of common device paths (e.g., `/dev/fb0`, `/dev/graphics/fb0`, `/dev/dri/card0`, etc.) and several default resolutions (640x480, 800x600, 1280x720, 1920x1080). Logs all actions, errors, and explanations to `window_probe_log_*.txt` files. No user input required for this command.
- **a**: Show advanced options for ARM/Mach-O/Opcode tools (see below)

### Advanced Options (after pressing 'a')
- **a**: Disassemble a test ARM instruction
- **b**: Print floating point addition for test code
- **c**: Print ARM load compute for test code
- **d**: Print ARM source info for test code
- **e**: Print ARM floating point source info
- **f**: Print instruction byte length
- **z**: Disassemble test code
- **h**: Print Mach-O header info for a file
- **p**: Patch first byte of a Mach-O binary (dangerous, research only!)
- **l**: List load commands for a Mach-O file
- **s**: Show code signature presence for a Mach-O file
- **b**: List linked libraries for a Mach-O file
- **a**: List architectures in a Mach-O file

## Window Probing
- The 'w' option attempts to open a window by probing/register searching on several device files (e.g., /dev/fb0, /dev/graphics/fb0, /dev/dri/card0, etc.).
- All attempts, errors, and explanations are logged in detail to `window_probe_log.txt` for human inspection.
- This is a bruteforce-style approach and is useful for debugging or exploring hardware/OS support.

## Hacker's Guide: Under-the-Hood Window Probing

When you press 'w', the program:
- Tries to open each device path with multiple flag combinations (O_RDWR, O_WRONLY, O_RDONLY).
- For each open, tries to mmap with different protections and flags (PROT_READ, PROT_WRITE, MAP_SHARED, MAP_PRIVATE).
- Attempts ioctl calls (like FBIOGET_VSCREENINFO) if available.
- Writes a test pattern to the mapped memory if possible.
- Logs every syscall, error code, symbolic error name, and a human-readable explanation to a numbered log file in `window_probing_results/`.
- Summarizes all attempts and failures for each device/resolution.

This approach is designed to maximize the diversity of error codes and system responses, helping you understand exactly why access is denied or what happens at each step. The more error codes and logs you see, the closer you are to finding a viable path to direct hardware access for windowing.

You can expand the device list, add more open/mmap/ioctl strategies, or try additional syscalls for even deeper probing.

## Deep Device Probing & Privilege Escalation

- The program now enumerates all likely framebuffer, graphics, tty, and block devices in `/dev` and attempts to probe every one.
- For each device, it tries multiple open flags, mmap protections, and ioctl calls, logging every syscall and error.
- Before probing, it attempts privilege escalation (setuid(0), seteuid(0), prctl if available) and logs the results.
- All results are written to detailed log files in `window_probing_results/`.
- This approach maximizes the chance of finding a viable path to hardware access and provides a full audit trail of every attempt and error.

**Note:** This is a research/hacker tool for exploring hardware access, not for malicious use. Always run with appropriate permissions and on systems you own/control.

