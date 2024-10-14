BIN := ./.bin/main
FLAGS := -O3 -o$(BIN) -I/mingw64/include
# GDB := gdb -tui -iex set disassembly-flavor intel
GDB := $(shell command -v gf2 2>/dev/null || command -v gf 2>/dev/null || echo "gdb -tui -iex 'set disassembly-flavor intel'")
GDB := gdb -tui -iex 'set disassembly-flavor intel'

ARGS := ~/media/videos/life_is_everything.mp4
ARGS = "/mnt/e/Torrents/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.COMPLETE.REMUX.HDR.ENG.LATINO.FRENCH.ITALIAN.POLISH.JAPANESE.TrueHD.Atmos.7.1.H265-BEN.THE.MEN/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.COMPLETE.REMUX.HDR.ENG.LATINO.FRENCH.ITALIAN.POLISH.JAPANESE.TrueHD.Atmos.7.1.H265-BEN.THE.MEN.mkv"

run: build
	 unset LIBGL_ALWAYS_INDIRECT; export DISPLAY=:0 && $(BIN) $(ARGS)


MINGW_CODE_DIR := /mnt/c/msys64/home/Administrator/code/unamed-video

sync:
	rsync --exclude=".*.bin|.*.out|./nob" -av . $(MINGW_CODE_DIR)



# -sanitize:address -sanitize:memory
release: FLAGS = -out:$(BIN) -define:DEV=false -o:speed -disable-assert -no-bounds-check -no-type-assert
release: build


BRDFS_DIR := ./test/brdfs

test:
	pdflatex $(BRDFS_DIR)/ward.tex
	pdflatex $(BRDFS_DIR)/ashikhmin-shirley.tex
	pdflatex $(BRDFS_DIR)/cook-torrance.tex
	pdflatex $(BRDFS_DIR)/blinn-phong.tex
	pdflatex $(BRDFS_DIR)/cook-torrance-disney.tex

	@$(BIN)  $(BRDFS_DIR)/ward.tex
	@$(BIN)  $(BRDFS_DIR)/ashikhmin-shirley.tex
	@$(BIN)  $(BRDFS_DIR)/cook-torrance.tex
	@$(BIN)  $(BRDFS_DIR)/blinn-phong.tex
	@$(BIN)  $(BRDFS_DIR)/cook-torrance-disney.tex

	find . -type f -name "**.brdf" -exec mv -f -t $(OUTPUT_DIR) {} +
	# find . -type f -name "**.pdf"  -exec mv -f -t $(OUTPUT_DIR) {} +



mingw-ssh: sync
	cmd.exe /C "cd C:\msys64\home\Administrator\code\unamed-video && C:\msys64\msys2_shell.cmd -defterm -here -no-start -mingw64"

SOURCES := sr.c

GLIB_FLAGS := -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/sysprof-6 -pthread -lglib-2.0 -lglfw3 -lopengl32


build:
	./nob &> /dev/null || gcc nob.c -o nob && ./nob
    

mingw-run: CC=gcc
mingw-run: BIN=./.bin/main.exe
# mingw-run: ARGS=/c/Users/Administrator/Downloads/rapidsave.com__-6vfpyw0jvlqd1.mp4
mingw-run: ARGS="/e/Torrents/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.COMPLETE.REMUX.HDR.ENG.LATINO.FRENCH.ITALIAN.POLISH.JAPANESE.TrueHD.Atmos.7.1.H265-BEN.THE.MEN/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.mkv"
mingw-run: build
	@echo "CC is set to: $(CC)"
	@echo "ARGS is set to: $(ARGS)"
	$(BIN) $(ARGS)

doc:
	pdflatex $(ARGS) && wsl-open $(notdir $(basename $(ARGS)).pdf)

svg: run
	wsl-open ast.svg

debug: FLAGS += -ggdb
debug: build
	$(GDB) --args $(BIN) $(ARGS)

clean:
	@echo "Cleaning up"
	find . -type f \( -name "*.brdf" -o -name "*.o" -o -name "*.a" -o -name "*.aux" -o -name "*.svg" \) -delete

# rsync -av --delete . "$(wslpath C:\msys64\home\Administrator\code)"


.PHONY: run debug doc build clean svg test sync mingw-run mingw-ssh
