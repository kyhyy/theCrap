CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -O2 -I.
LDFLAGS =

SRCDIR  = src
BUILDDIR = build
TARGET   = thecrap

SRCS    = $(SRCDIR)/main.c \
          $(SRCDIR)/shlex.c \
          $(SRCDIR)/cli.c \
          $(SRCDIR)/match.c \
          $(SRCDIR)/init.c \
          $(SRCDIR)/ui.c \
          $(SRCDIR)/rules/rules.c \
          $(SRCDIR)/rules/rules_registry.c \
          $(SRCDIR)/rules/rules_sudo.c \
          $(SRCDIR)/rules/rules_git_wrong.c \
          $(SRCDIR)/rules/rules_misc.c \
          $(SRCDIR)/rules/command_not_found.c

TEST_SRCS = $(BUILDDIR)/test_shlex \
            $(BUILDDIR)/test_match \
            $(BUILDDIR)/test_cli

.PHONY: all clean test

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(TARGET): $(SRCS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)
	@echo "  Built $(TARGET)"

test: $(TEST_SRCS)
	@echo ""
	@echo "=== shlex ===" && ./$(BUILDDIR)/test_shlex
	@echo ""
	@echo "=== match ===" && ./$(BUILDDIR)/test_match
	@echo ""
	@echo "=== cli ===" && ./$(BUILDDIR)/test_cli

$(BUILDDIR)/test_shlex: tests/test_shlex.c $(SRCDIR)/shlex.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/test_shlex.c $(SRCDIR)/shlex.c
	@echo "  Built test_shlex"

$(BUILDDIR)/test_match: tests/test_match.c $(SRCDIR)/match.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/test_match.c $(SRCDIR)/match.c
	@echo "  Built test_match"

$(BUILDDIR)/test_cli: tests/test_cli.c $(SRCDIR)/cli.c $(SRCDIR)/shlex.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ tests/test_cli.c $(SRCDIR)/cli.c $(SRCDIR)/shlex.c
	@echo "  Built test_cli"

clean:
	rm -rf $(BUILDDIR)
