rc /DDETOURS_BITS=32 /fofix_scrollbug.res /idetours fix_scrollbug.rc
cl /nologo /MT /W4 /WX /Od /Idetours /c fix_scrollbug.cpp
cl /nologo /MT /W4 /WX /Idetours /LD /Od /Fefix_scrollbug32.dll fix_scrollbug.obj fix_scrollbug.res /link /release /incremental:no /profile /nodefaultlib:oldnames.lib /subsystem:console /export:DetourFinishHelperProcess,@1,NONAME detours\detours.lib kernel32.lib user32.lib

rc /DDETOURS_BITS=32 /fodll_inject.res /idetours dll_inject.rc
cl /nologo /MT /W4 /WX /Od /Idetours /c dll_inject.cpp
cl /nologo /MT /W4 /WX /Od /Idetours /Fedll_inject.exe dll_inject.obj dll_inject.res /link /release /incremental:no /profile /nodefaultlib:oldnames.lib /subsystem:console detours\detours.lib kernel32.lib