#### D_WATCHER 1.0 : Makefile (Compile / Build Module) ####

###########################################################
## D_WATCHER 1.0 : Directory Structure for Project Build ##
##                                                       ##
## R_WATCHER_1.0 (root directory)                        ##
## +                                                     ##
## |--- bin         (for project binary)                 ##
## |--- include     (for header .h files)                ##
## |--- obj         (for object .o files)                ##
## |--- src         (for source .c files)                ##
## |--- tests       (for unit tests)                     ##
## +--- Makefile    (compile / build module file)        ##
##                                                       ##
###########################################################

########## Eye Candy for Makefile Module ###########

RED         := \033[1;31m
GREEN       := \033[1;32m
YELLOW      := \033[1;33m
BLUE        := \033[1;34m
RESET       := \033[0m

LINE        := $(RED)------$(RESET)

PRINT       := @echo -e
EXIT        := @exit 1

#####################################################

CC          := gcc
CSTANDARD   := -std=gnu99
FWARNINGS   := -Wall -Wextra

OPTIMIZATION:= -O0

CFLAGS      := $(CSTANDARD) $(FWARNINGS) $(OPTIMIZATION)
LDFLAGS     :=

DEBUGFLAG   := -g

DEBUG       := R_WATCHER_DEBUG
RELEASE     := R_WATCHER_RELEASE

DEBUGMACRO  := -D$(DEBUG)
RELEASEMACRO:= -D$(RELEASE)

DEBUGFLAGS  := $(CFLAGS) $(DEBUGMACRO) $(DEBUGFLAG)
RELEASEFLAGS:= $(CFLAGS) $(RELEASEMACRO) $(DEBUGFLAG)

EXECUTABLE  := dwatcher_1.0

EXEDIR	    := bin
INCDIR	    := include
EXTERNAL    := external
OBJDIR      := obj
SRCDIR      := src

SOURCES     := $(wildcard $(SRCDIR)/*.c)
OBJECTS     := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))
TARGET      := $(EXEDIR)/$(EXECUTABLE)

RUNCMD      := ./$(TARGET)

INCLUDES    := -I$(INCDIR)

LIBINCLUDES :=

LIBRARIES   :=


## Installation Options

INSTALLDIR  := /bin/
INSTALLCMD  := cp -v -f -u $(TARGET) -t

######################################################################

all: init build

init:
	@mkdir -p $(EXEDIR)
	@mkdir -p $(OBJDIR)

build: intro $(TARGET)

intro:
	$(PRINT) "$(RED)"
	$(PRINT) "+----------------------------------------------+"
	$(PRINT) "|  $(BLUE)D_WATCHER 1.0 : Makefile (Compile / Build Module)$(RED)  |"
	$(PRINT) "+----------------------------------------------+$(RESET)"
	$(PRINT)

$(TARGET): $(OBJECTS)
	$(PRINT)
	$(PRINT) ">> $(RED)Linking$(RESET):"
ifeq ($(BUILD), $(DEBUG))
	$(CC) $(INCLUDES) $(OBJECTS) $(LIBINCLUDES) $(LIBRARIES) -o $@
else
	$(CC) $(INCLUDES) $(OBJECTS) $(LIBINCLUDES) $(LIBRARIES) -o $@
endif
	$(PRINT)
	$(PRINT) "$(LINE) $(BLUE)Built successfully! $(LINE)"
	$(PRINT)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(PRINT)
	$(PRINT) ">> $(RED)Compiling$(RESET):$(BLUE)" $< "$(RESET)"
ifeq ($(BUILD), $(DEBUG))
	$(CC) $(DEBUGFLAGS)   $(INCLUDES) -c $^ -o $@
else ifeq ($(BUILD), $(RELEASE))
	$(CC) $(RELEASEFLAGS) $(INCLUDES) -c $^ -o $@
else
	$(CC) $(RELEASEFLAGS) $(INCLUDES) -c $^ -o $@
endif

run: intro validate_executable

validate_executable:
ifeq (,$(wildcard $(TARGET)))
	$(PRINT)
	$(PRINT) ">> $(YELLOW)FATAL ERROR$(RESET):"
	$(PRINT) "   $(BLUE)The executable \"$(TARGET)\" does NOT exist!$(RESET)"
	$(PRINT) "   $(BLUE)First 'make' the project, then 'run'.$(RESET)"
	$(PRINT)
	$(EXIT)
endif

install: intro validate_executable
	$(PRINT)
	$(PRINT) ">> $(RED)Installing D_WATCHER binaries$(RESET):"
ifneq (, $(wildcard $(DEST)))
	$(INSTALLCMD) $(DEST)
else
	$(INSTALLCMD) $(INSTALLDIR)
endif
	$(PRINT)
	$(PRINT) "$(LINE) $(BLUE)Installation completed! $(LINE)"
	$(PRINT)

clean: intro
	$(PRINT)
	$(PRINT) ">> $(RED)Cleaning$(RESET):"
	-$(RM) $(TARGET)
	-$(RM) -r $(EXEDIR)/$(COVDIR)
	-$(RM) $(EXEDIR)/*
	-$(RM) -r $(OBJDIR)
	$(PRINT)
	$(PRINT) "$(LINE) $(BLUE)Cleaned successfully! $(LINE)"
	$(PRINT)

.PHONY: all build install clean rpm

############## End of Makefile (Compile / Build Module) ##############
