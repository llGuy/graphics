CC := cl
CFLAGS := /std:c++latest -Zi /EHsc
LIBS := user32.lib opengl32.lib User32.lib Gdi32.lib Shell32.lib kernel32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib C:/dependencies/GLEW/lib/Release/Win32/glew32s.lib C:/dependencies/GLEW/lib/Release/Win32/glew32.lib C:/dependencies/GLFW/lib-vc2015/glfw3.lib msvcrt.lib msvcmrt.lib
DEF := /DGLM_ENABLE_EXPERIMENTAL /DSTB_IMAGE_IMPLEMENTATION /DGLEW_STATIC
INCS := /I C:/dependencies/GLEW/include /I C:/dependencies/GLFW/include /I C:/dependencies/json-loader/include /I C:/dependencies /I C:/dependencies/xml_parser /I C:/dependencies/c++
GCCDEF := -DGLM_ENABLE_EXPERIMENTAL -DSTB_IMAGE_IMPLEMENTATION -DGLEW_STATIC
GCCINC := -IC:/dependencies/GLEW/include -IC:/dependencies/GLFW/include -IC:/dependencies/json-loader/include -IC:/dependencies -IC:/dependencies/xml_parser -IC:/dependencies/c++

SRCDIR := ./
OBJDIR := ./
BINDIR := ./
CPPS := $(wildcard *.cpp)
OBJS := $(patsubst %,$(OBJDIR)%,$(CPPS:.cpp=.obj))
DEPFILES := $(patsubst %,$(OBJDIR)%,$(CPPS:.cpp=.dep))

run: prototype.exe
	prototype.exe

$(BINDIR)prototype.exe: $(OBJS)
	$(CC) $(CFLAGS) /DEBUG:FULL /Fe$@ $(DEF) $(INCS) $(OBJS) $(LIBS)

$(OBJDIR)%.obj: %.cpp
	$(CC) -Zi /EHsc /c /Fo$(OBJDIR) $(DEF) $(INCS) $< /std:c++latest

$(OBJDIR)%.dep: %.cpp
	gcc -MM $(GCCDEF) $(GCCINC) $< | sed 's/.o:/.obj:/' > $@

clean:
	rm $(BINDIR)*.exe $(OBJDIR)*.obj $(OBJDIR)*.ilk $(OBJDIR)*.pdb *.dep

-include $(DEPFILES)
