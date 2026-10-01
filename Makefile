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

EXE_CLIENT:=out/kioskcnc
all:$(EXE_CLIENT)
OFILES_CLIENT:=$(filter mid/client/%.o,$(OFILES))
$(EXE_CLIENT):$(OFILES_CLIENT);$(PRECMD) $(LD) -o$@ $^ $(LDPOST)

run:$(EXE_CLIENT);$(EXE_CLIENT)
clean:;rm -rf mid out
