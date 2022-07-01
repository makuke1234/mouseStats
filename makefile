CC=gcc
MACROS=-D UNICODE -D _UNICODE
CDEFFLAGS=$(MACROS) -std=c2x -Wall -Wextra -Wpedantic -Wconversion -Wdouble-promotion -Wstrict-prototypes
DebFlags=-g -O0 -D _DEBUG
RelFlags=-O3 -Wl,--strip-all,--build-id=none,--gc-sections -D NDEBUG
LIB=-lgdi32 -ldwmapi -lcomctl32

BIN=bin
SRC=src
OBJ=obj
TARGET=mouseStats


SRCFILES=$(wildcard $(SRC)/*.c)
RSCFILES=$(wildcard $(SRC)/*.rc)

RELOBJFILES=$(SRCFILES:%.c=%.c.o)
RELOBJFILES+=$(RSCFILES:%.rc=%.rc.o)
RELOBJFILES:=$(RELOBJFILES:$(SRC)/%=$(OBJ)/%)

DEBOBJFILES=$(SRCFILES:%.cpp=%.c.d.o)
DEBOBJFILES+=$(RSCFILES:%.rc=%.rc.d.o)
DEBOBJFILES:=$(DEBOBJFILES:$(SRC)/%=$(OBJ)/%)


default: release

rel: release
deb: debug

release: $(RELOBJFILES)
	$(CC) $^ -o $(BIN)/$(TARGET).exe $(CDEFFLAGS) $(RelFlags) $(LIB)
debug: $(DEBOBJFILES)
	$(CC) $^ -o $(BIN)/deb$(TARGET).exe $(CDEFFLAGS) $(DebFlags) $(LIB)


$(OBJ)/%.rc.o: $(SRC)/%.rc $(OBJ)
	windres -i $< -o $@ $(MACROS) -D FILE_NAME='\"$(TARGET).exe\"'
$(OBJ)/%.rc.d.o: $(SRC)/%.rc $(OBJ)
	windres -i $< -o $@ $(MACROS) -D FILE_NAME='\"deb$(TARGET).exe\"'

$(OBJ)/%.o: $(SRC)/% $(OBJ)
	$(CC) -c $< -o $@ $(CDEFFLAGS) $(RelFlags)
$(OBJ)/%.d.o: $(SRC)/% $(OBJ)
	$(CC) -c $< -o $@ $(CDEFFLAGS) $(DebFlags)

$(OBJ):
	mkdir $(OBJ)

$(BIN):
	mkdir $(BIN)

clean:
	rm -r -f $(OBJ)
	rm -f $(BIN)/*.exe
