CFLAGS = -O2 -lz -I"./src/lib" -Wall -pedantic

ifeq ($(OS),Windows_NT)
	LDFLAGS = -lwinhttp
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS = -framework Foundation
		CFLAGS += -Wno-gnu-zero-variadic-macro-arguments
	else ifeq ($(UNAME_S),Linux)
		LDFLAGS = -lcurl -lpthread
	endif
endif

rpotool: src/*.c src/lib/*.c
	gcc $^ -o $@ $(CFLAGS) $(LDFLAGS)