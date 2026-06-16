# Makefile multiplataforma

PROG = hdrvis

# Compilador
CC = gcc

# Flags
CFLAGS = -Iinclude -g -O3 # -Wall -g  # Todas as warnings, infos de debug
LDFLAGS  =
LIBS     =

# Detecta SO and e ajusta extensao se necessario
ifeq ($(OS),Windows_NT)
    # Windows (MinGW)
    EXEEXT  = .exe
    OSNAME  = Windows
    LIBS    += -lm
else
    UNAME_S := $(shell uname -s)
    EXEEXT  =
    ifeq ($(UNAME_S),Linux)
        OSNAME  = Linux
        LIBS    += -lm 
    endif
    ifeq ($(UNAME_S),Darwin)
        OSNAME  = macOS
        CFLAGS  += -Wno-deprecated
        LIBS    += -lm
    endif
endif

# Fontes
FONTES = main.c
OUTDIR = output

# Target (com extensao se necessario)
TARGET = $(OUTDIR)/$(PROG)$(EXEEXT)

# Regras
all: $(OUTDIR) $(TARGET)
	@echo "Build completado para $(OSNAME). Saida: $(TARGET)"

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(TARGET): $(FONTES) | $(OUTDIR)
	$(CC) $(CFLAGS) $(FONTES) -o $@ $(LDFLAGS) $(LIBS)

clean:
ifeq ($(OS),Windows_NT)
	del /Q $(subst /,\,$(OBJ)) $(TARGET) 2>nul || true
else
	rm -rf $(OUTDIR)
endif

# Mostra infos do SO detectado
info:
	@echo "SO detectado: $(OSNAME)"
	@echo "Compilador:   $(CC)"
	@echo "CFLAGS:       $(CFLAGS)"
	@echo "LIBS:        $(LIBS)"
	@echo "Target:       $(TARGET)"

.PHONY: all clean info

