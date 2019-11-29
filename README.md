# Age of Empires II scrollbug fix

## TL;DR

- Download `age2_x1_fixed.tar.gz` from the [Releases page](https://github.com/SFTtech/sftscrollbugfixer/releases)
- Unpack it into your `age2_x1` folder
- From now on launch the game with `age2_x1_fixed.exe` instead of `age2_x1.exe`
- No patching of the game binary is required
- Black magic now removes all of the suffering
- ?????
- PROFIT
- If you want to use the fix with a different version of the game, just modify the `.ini` file accordingly.

## What is the scrollbug?

Sometimes when you tab out of the game, after tabbing back in the game will be stuck in a state where it is constantly scrolling in some direction.

This makes the game unplayable and requires a restart of the game. This is especially annoying in multiplayer games.

The scrollbug happens mostly when running the game in Wine, but has supposedly also been observed on Windows.

## How do I reliably reproduce the scrollbug?

- Use Wine. I don't know if these steps work on Windows.
- Start a game, e.g. a standard single-player random map game
- Hold the left-arrow key for 3 seconds
- Release the left-arrow key
- Wait for one second
- Tab out of the game
- Wait for one second
- Tab back into the game
- Congratulations; you now have a scroll bug

## Why is there a scrollbug?

Age of Empires II uses the `GetKeyboardState()` and `GetKeyState()` Win32 API functions to determine whether one of the scrolling keys is pressed.

In the result of these functions, the key stats are reported as a single byte.

According to the MSDN documentation,

- Bit 7 of the byte says whether the key is currently pressed
- Bit 0 of the byte toggles each time the key is pressed
- All other bits are not documented

Age of Empires II wants to check whether the key is currently pressed. To do this it checks whether the byte value is `>= 1` instead of checking if Bit 7 is set (`& 0x80`).

This works as long as bits 1..6 are zero.

Unfortunately, wine seems to use bit 2 to provide some other metadata, obviously related to tabbing out of the window, so it doesn't guarantee that bits 1..6 are always 0.

Windows also seems to sometimes use some of the other bits.

## Which versions of Age of Empires II have the scrollbug?

Every single one, i.e.:

- AoK
- AoC
- Forgotten Empires
- UserPatch
- AoE2:HD
- AoE2:DE

## How does this fix work?

This uses the Microsoft Research [Detours](https://github.com/microsoft/Detours) library to inject replacements of `GetKeyState()` and `GetKeyboardState()` into the running Age of Empires II binary.

The replacement functions mask out bits 1..6 in the results.

The fix consists of two binaries:

- `age2_x1_fixed.exe` reads the configuration from `age2_x1_fixed.ini` and calls the `exe` that is specified in the `ini` file with the injector `dll` file that is specified in the `ini` file.
- `fix_scrollbug32.dll` injects fixed versions of `GetKeyState()` and `GetKeyboardState()` into any 32-bit Windows executable it is used with.

## How do I build this fix?

- Use Windows (please, somebody port this to winegcc or mingw or _something_ that works on Linux)
- Install Visual C++ Build Tools 2015
- Get this source code (duh.)
- Get `detours.lib` and `detours.h`, e.g. by building the Detours library from source by invoking `nmake`, or by downloading the binaries from this Github repo. Place these two files in a subfolder `detours/` of this source code
- Call `build.bat`
- You will now have a whole bunch of garbage files, plus
  - `fix_scrollbug32.dll`
  - `dll_inject.exe`
  - `dll_inject.ini`

## How do I use this fix?

`dll_inject.exe` can be renamed to whatever name. It is recommended to rename it to `{name_of_the_exe_you_want_to_fix}_fixed.exe`.

When launched, `dll_inject.exe` will

- open the `.ini` file with the same name
- in that `.ini` file in the section `[dll_inject]`, read the keys `exe_file` and `dll_file`
- launch the specified `exe` file and inject the specified `dll` file
- wait until the exe file has finished

If you use the pre-compiled and pre-configured `age2_x1_fixed.exe`, `age2_x1_fixed.ini`, `fix_scrollbug32.dll` you can just place them in the `age2_x1` folder and launch `age2_x1_fixed.exe` instead of `age2_x1.exe`.

## Credits

Thanks to **Sulix** who posted an analysis of the bug on the [Steam Forum](https://steamcommunity.com/app/221380/discussions/2/622954302095447538/#c154645539343670235).

Thanks to Microsoft Research for the Detours library and the excellent accompanying samples. `dll_inject.cpp` was derived directly from the `withdll` example, and `fix_scrollbug.cpp` was derived by modifying one of the other examples.
