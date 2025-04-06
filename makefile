CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -Wextra -Werror -std=c2x -nostdlib -ffreestanding -O3 -pedantic -ffp-contract=off

# Unfortunately, I have found that make quite often selects the wrong shell
# (e.g. PowerShell), so commands like "find" won't work unless we explicitly
# specify bash.  You also can't use a variable for this (e.g. $(SHELL)) as make
# inexplicably tries to read something from the PATH and fails.  So hardcoding a
# reference to bash seems to be the only way to get a working build.
SRC_DIRECTORIES = src submodules/protocol/src
C_FILES = $(shell bash -c "find $(SRC_DIRECTORIES) -type f -iname ""*.c""")
H_FILES = $(shell bash -c "find $(SRC_DIRECTORIES) -type f -iname ""*.h""")
O_FILES = $(patsubst %.c,obj/c/%.o,$(C_FILES))
TOTAL_REBUILD_FILES = makefile $(H_FILES)

dist/joy-con-side-udp.exe: $(O_FILES)
	mkdir -p $(dir $@)
	$(CC) $(CLAGS) -flto $(O_FILES) -o $@ -lWs2_32 -lSetupapi

obj/c/%.o: %.c $(TOTAL_REBUILD_FILES)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj dist
