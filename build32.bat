rc /fofix_scrollbug.res fix_scrollbug.rc
cl /nologo /MT /W4 /WX /Od /Idetours32 /c fix_scrollbug.cpp
cl /nologo /MT /W4 /WX /Idetours32 /LD /Od /Fefix_scrollbug32.dll fix_scrollbug.obj fix_scrollbug.res /link /release /incremental:no /profile /nodefaultlib:oldnames.lib /subsystem:console /export:DetourFinishHelperProcess,@1,NONAME detours32\detours.lib kernel32.lib user32.lib

rc /fodll_inject.res dll_inject.rc
cl /nologo /MT /W4 /WX /Od /Idetours32 /c dll_inject.cpp
cl /nologo /MT /W4 /WX /Od /Idetours32 /Fedll_inject32.exe dll_inject.obj dll_inject.res /link /release /incremental:no /profile /nodefaultlib:oldnames.lib /subsystem:console detours32\detours.lib kernel32.lib