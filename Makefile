# Tested on MacOSX, GNU bash version 4.2.0(1)-release (x86_64--netbsd)
# x86_64--netbsd make (?) doesn't have all the tricks

PROJ  := MakeIndex
VA    := 0
VB    := 8
FILES := Recursor Parser Widget Files
BDIR  := bin
BACK  := backup
INST  := $(PROJ)-$(VA)_$(VB)
OBJS  := $(patsubst %,$(BDIR)/%.o,$(FILES))
SRCS  := $(patsubst %,%.c,$(FILES))
H     := $(patsubst %,%.h,$(FILES))
OBJS  := bin/Recursor.o bin/Parser.o bin/Widget.o bin/Files.o

CC   := gcc
CF   := -Wall -O3 -fasm -fomit-frame-pointer -ffast-math -funroll-loops -pedantic -ansi # not ansi! but compiles; POSIX?

default: $(BDIR)/$(PROJ)

$(BDIR)/$(PROJ): $(OBJS)
	$(CC) $(CF) -o $@ $(OBJS)
#	$(CC) $(CF) -o $@ $^

$(BDIR)/%.o: %.c
	@mkdir -p $(BDIR)
	$(CC) $(CF) -c $? -o $@

$(BDIR)/Recursor.o: Recursor.c
	@mkdir -p $(BDIR)
	$(CC) $(CF) -c $? -o $@

$(BDIR)/Parser.o: Parser.c
	@mkdir -p $(BDIR)
	$(CC) $(CF) -c $? -o $@

$(BDIR)/Widget.o: Widget.c
	@mkdir -p $(BDIR)
	$(CC) $(CF) -c $? -o $@

$(BDIR)/Files.o: Files.c
	@mkdir -p $(BDIR)
	$(CC) $(CF) -c $? -o $@

.PHONY: clean backup
clean:
	-rm $(OBJS)

backup:
	@mkdir -p $(BACK)
	zip $(BACK)/$(INST)-`date +%Y-%m-%dT%H%M%S`.zip readme.txt gpl.txt copying.txt Makefile Makefile.mingw $(SRCS) $(H) -r $(BDIR)/example/

setup: $(BDIR)/$(PROJ)
	@mkdir -p $(BDIR)/$(INST)
	cp $(BDIR)/$(PROJ) readme.txt gpl.txt copying.txt $(BDIR)/$(INST)
	cp -R $(BDIR)/example $(BDIR)/$(INST)
	rm -f $(BDIR)/$(INST).dmg
	hdiutil create $(BDIR)/$(INST).dmg -volname "MakeIndex $(VA).$(VB)" -srcfolder $(BDIR)/$(INST)
	rm -R $(BDIR)/$(INST)
