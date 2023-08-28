CXX   := g++
MKDIR := mkdir
RM    := rm
BINDIR   := bin
BUILDDIR := build

SBDIR    := soundbag-http-server
SBBINDIR := $(SBDIR)/bin

CFLAGS  += -Isrc/include -I$(SBDIR)/src/include -std=c++2a -g
LDFLAGS += -lsqlite3 -lpthread

all: $(BUILDDIR) $(BINDIR) $(SBBINDIR)/libsoundbag-httpserver.a $(BINDIR)/server$(EXT)

$(BINDIR)/server$(EXT): $(addprefix $(BUILDDIR)/,main.o ImageServer.o PNG.o) $(SBBINDIR)/libsoundbag-httpserver.a
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SBBINDIR)/libsoundbag-httpserver.a:
	$(MAKE) -C $(SBDIR)

$(BUILDDIR):
	$(MKDIR) $@

$(BINDIR):
	$(MKDIR) $@

$(BUILDDIR)/%.o: src/%.cpp
	$(CXX) $(CFLAGS) -c -o $@ $^

clean:
	$(RM) -r $(BUILDDIR) $(BINDIR)

.PHONY: clean
