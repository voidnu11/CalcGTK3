# need:
# brew install gtk+3
# brew install librsvg

TARGET=calcgtk3
CC ?= gcc
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-3.0) -std=c11
LIBS = $(shell $(PKGCONFIG) --libs gtk+-3.0) -lm
PKGCONFIG = $(shell which pkg-config)
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

SRC = main.c calculator.c double_stack.c
BUILT_SRC = resources.c

OBJS = $(BUILT_SRC:.c=.o) $(SRC:.c=.o)

all: clean $(TARGET)

install: $(TARGET)
	mkdir ~/prog
	cp -R assets ~/prog/assets
	mv $(TARGET) ~/prog

uninstall:
	rm -rf ~/prog/assets
	rm ~/prog/$(TARGET)

resources.c: assets/$(TARGET).gresource.xml assets/ui/main.ui assets/ui/plot.ui
	$(GLIB_COMPILE_RESOURCES) $< --target=$@ --sourcedir=assets/ --generate-source

%.o: %.c
	$(CC) -c -o $(@F) $(CFLAGS) -lm $<

$(TARGET): $(OBJS)
	$(CC) -o $(@F) $(OBJS) -rdynamic $(LIBS)

clean:
	rm -f $(BUILT_SRC) $(OBJS) $(TARGET) *.o *.gcda *.gcno *.info test gcov_test
	rm -rf report
