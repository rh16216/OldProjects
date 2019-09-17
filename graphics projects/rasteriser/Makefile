FILE=skeleton

########
#   Directories
S_DIR=Source
B_DIR=Build
GLMDIR=../glm
########
#   Output
EXEC=$(B_DIR)/$(FILE)

# default build settings
CC_OPTS=-c -pipe -Wall -Wno-switch -ggdb -g3 
LN_OPTS=
CC=g++ --std=c++11

########
#       SDL options
SDL_CFLAGS := $(shell sdl2-config --cflags)
GLM_CFLAGS := -I$(GLMDIR)
SDL_LDFLAGS := $(shell sdl2-config --libs)

########
#   This is the default action
all:Build


########
#   Object list
#
OBJ = $(B_DIR)/$(FILE).o

# Haskell stuff
HS_LIBS := $(shell ghc --print-libdir)
$(B_DIR)/Fireball.o: $(S_DIR)/Fireball.hs
	ghc -hidir Build -o $(B_DIR)/Fireball.o -XForeignFunctionInterface -c $(S_DIR)/Fireball.hs

########
#   Objects
$(B_DIR)/$(FILE).o : $(S_DIR)/$(FILE).cpp $(S_DIR)/SDLauxiliary.h $(S_DIR)/TestModelH.h
	$(CC) $(CC_OPTS) -o $(B_DIR)/$(FILE).o $(S_DIR)/$(FILE).cpp $(SDL_CFLAGS) $(GLM_CFLAGS) -I"$(HS_LIBS)/include"

########
#   Make C thing     
main : $(CC) $(LN_OPTS) -o $(EXEC) $(OBJ) $(SDL_LDFLAGS)

########
#   Main build rule     
Build : $(B_DIR)/Fireball.o $(OBJ) Makefile
	ghc -o $(B_DIR)/main -no-hs-main $(B_DIR)/Fireball.o $(B_DIR)/$(FILE).o -lstdc++ -I$(SDL_LDFLAGS)

clean:
	rm -f $(B_DIR)/* && rm -rf $(S_DIR)/*_stub.h