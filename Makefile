all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

UNAMEN:=$(shell uname -n)

CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-stringop-overflow
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

# Server is Javascript and its client app is static, ready to go. Nothing to build.
serve:;node src/server/main.js --port=8080 --htdocs=src/www

run-with-server:$(EXE_CLIENT);$(EXE_CLIENT) --host=$(UNAMEN) --remote=localhost:8080
run-local:$(EXE_CLIENT);$(EXE_CLIENT) --host=$(UNAMEN)
clean:;rm -rf mid out
