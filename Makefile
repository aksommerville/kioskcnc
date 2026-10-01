all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit
LD:=gcc
LDPOST:=

CFILES:=$(shell find src -name '*.c')
OFILES:=$(patsubst src/%.c,mid/%.o,$(CFILES))
-include $(OFILES:.o=.d)
mid/%.o:src/%.c;$(PRECMD) $(CC) -o$@ $<

EXE:=out/kioskcnc
all:$(EXE)
$(EXE):$(OFILES);$(PRECMD) $(LD) -o$@ $^ $(LDPOST)

run:$(EXE);$(EXE)
clean:;rm -rf mid out
