SRC = $(wildcard *.cc)
OBJ_DIR = obj/
OBJ = $(addprefix $(OBJ_DIR), $(SRC:.cc=.o))

# Libraries to resolve with pkgconfig
PKG_CFG_PATH=/usr/lib:/usr/local/lib
PKG_CFG_LIBS=protobuf-lite

CC		    =g++-6
CC_FLAGS	=-std=c++0x -g -static-libstdc++
OUT_DIR		=bin

LIB_DIRS	=-L"/usr/local/lib"
LIBS		=-lavahi-client -lavahi-common -pthread

PKG_CC_FLAGS	:=$(shell PKG_CONFIG_PATH=$(PKG_CFG_PATH) pkg-config $(PKG_CFG_LIBS) --cflags)
PKG_LIBS		:=$(shell PKG_CONFIG_PATH=$(PKG_CFG_PATH) pkg-config $(PKG_CFG_LIBS) --libs)

CC_FLAGS	:=$(CC_FLAGS) $(PKG_CC_FLAGS)
LIBS		:=$(LIB_DIRS) $(LIBS) $(PKG_LIBS)

OUT=$(OUT_DIR)/master_server

all: setup $(OUT)

setup:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)

$(OUT): $(OBJ)
	@echo "> Linking..."
	@$(CC) -o $@ $(OBJ) $(LIBS) $(CC_FLAGS)

$(OBJ_DIR)%.o: %.cc
	@echo "> $<"
	@$(CC) -o $@ $< -c $(CC_FLAGS)

rungdb : $(OUT)
	@env gdb ./$(OUT)

run : $(OUT)
	@env ./$(OUT)

debug : $(OUT)
	@env gdb ./$(OUT)

clean:
	@rm -f $(OUT) $(OBJ)

