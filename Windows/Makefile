CFLAGS = /nologo /W3 /EHsc
COMPILER = cl
LINK = link

all: build

.PHONY: clean

build: libscheduler.dll

libscheduler.dll: so_scheduler.obj priority_queue.obj linkedlist.obj hashtable.obj
	$(LINK) /nologo /dll /out:$@ /implib:libscheduler.lib $**

so_scheduler.obj: so_scheduler.c
	$(COMPILER) $(CFLAGS) /Fo$@ /c $**

hashtable.obj: hashtable.c
	$(COMPILER) $(CFLAGS) /Fo$@ /c $**

linkedlist.obj: linkedlist.c
	$(COMPILER) $(CFLAGS) /Fo$@ /c $**

priority_queue.obj: priority_queue.c
	$(COMPILER) $(CFLAGS) /Fo$@ /c $**	

clean:
	del /Q so_scheduler.obj 2>NUL
	del /Q libscheduler.dll libscheduler.lib libscheduler.exp 2>NUL
