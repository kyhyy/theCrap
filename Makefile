CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -O2
LDFLAGS =

SRCDIR  = src
BUILDDIR = build
TARGET   = thecrap
TESTS    = $(BUILDDIR)/test_shlex

SRCS = $(SRCDIR)/shlex.c

.PHONY: all clean test

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(TARGET): $(SRCS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(SRCS)
	@echo "  Built $(TARGET)"

test: $(TESTS)
	@echo ""
	./$(TESTS)

$(TESTS): tests/test_shlex.c $(SRCDIR)/shlex.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/test_shlex.c $(SRCDIR)/shlex.c
	@echo "  Built test_shlex"

clean:
	rm -rf $(BUILDDIR)
