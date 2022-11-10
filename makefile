CC=gcc
MACROS=-D UNICODE -D _UNICODE
CDEFFLAGS=$(MACROS) -std=c2x -Wall -Wextra -Wpedantic -Wconversion -Wdouble-promotion -Wstrict-prototypes
DebFlags=-g -O0 -D _DEBUG
RelFlags=-O3 -Wl,--strip-all,--build-id=none,--gc-sections -mwindows -D NDEBUG
LIB=-lgdi32 -ldwmapi -lcomctl32

BIN=bin
SRC=src
OBJ=obj
TARGET=mouseStats

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))


SRCFILES=$(call rwildcard,$(SRC),*.c)
RSCFILES=$(call rwildcard,$(SRC),*.rc)

RELOBJFILES=$(SRCFILES:%.c=%.c.o)
RELOBJFILES+=$(RSCFILES:%.rc=%.rc.o)
RELOBJFILES:=$(RELOBJFILES:$(SRC)/%=$(OBJ)/%)

DEBOBJFILES=$(SRCFILES:%.c=%.c.d.o)
DEBOBJFILES+=$(RSCFILES:%.rc=%.rc.d.o)
DEBOBJFILES:=$(DEBOBJFILES:$(SRC)/%=$(OBJ)/%)

objfolders = $(BIN) $(OBJ)
objfolders+= $(dir $(wildcard $(SRC)/*/))
objfolders:=$(objfolders:$(SRC)/%=$(OBJ)/%)

.PHONY: all

default: debug

rel: release
deb: debug

release: $(objfolders) release_inner

debug: $(objfolders) debug_inner

release_inner: $(RELOBJFILES)
	$(CC) $^ -o $(BIN)/$(TARGET).exe $(CDEFFLAGS) $(RelFlags) $(LIB)
debug_inner: $(DEBOBJFILES)
	$(CC) $^ -o $(BIN)/deb$(TARGET).exe $(CDEFFLAGS) $(DebFlags) $(LIB)


$(OBJ)/%.rc.o: $(SRC)/%.rc
	windres -i $< -o $@ $(MACROS) -D FILE_NAME='\"$(TARGET).exe\"'
$(OBJ)/%.rc.d.o: $(SRC)/%.rc
	windres -i $< -o $@ $(MACROS) -D FILE_NAME='\"deb$(TARGET).exe\"'

$(OBJ)/%.o: $(SRC)/%
	$(CC) -c $< -o $@ $(CDEFFLAGS) $(RelFlags)
$(OBJ)/%.d.o: $(SRC)/%
	$(CC) -c $< -o $@ $(CDEFFLAGS) $(DebFlags) -fstack-usage

$(OBJ):
	mkdir $(OBJ)
$(OBJ)/%:
	mkdir $@

$(BIN):
	mkdir $(BIN)

clean:
	rm -r -f $(OBJ)
	rm -f $(BIN)/*.exe
