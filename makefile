!include "../global.mak"

ALL : "$(OUTDIR)\MQ2Vendors.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2Vendors.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2Vendors.dll"
	-@erase "$(OUTDIR)\MQ2Vendors.exp"
	-@erase "$(OUTDIR)\MQ2Vendors.lib"
	-@erase "$(OUTDIR)\MQ2Vendors.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2Vendors.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2Vendors.dll" /implib:"$(OUTDIR)\MQ2Vendors.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2Vendors.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2Vendors.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2Vendors.dep")
!INCLUDE "MQ2Vendors.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2Vendors.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2Vendors.cpp

"$(INTDIR)\MQ2Vendors.obj" : $(SOURCE) "$(INTDIR)"

