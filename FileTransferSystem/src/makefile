CC=gcc
CFLAGS=-I$(IDIR)
IDIR =../include
ODIR=obj
LDIR =../lib
LIBS=-lpthread
BDIR=../bin

S_DEPS = serverfunc.h
SDEPS = $(patsubst %,$(IDIR)/%,$(S_DEPS))
S_LIBOBJ = serverfunc.o
SLIBOBJ = $(patsubst %,$(ODIR)/%,$(S_LIBOBJ))
SLIBSRC = serverfunc.c
S_APPOBJ = server.o
SAPPOBJ = $(patsubst %,$(ODIR)/%,$(S_APPOBJ))

C_DEPS = clientfunc.h
CDEPS = $(patsubst %,$(IDIR)/%,$(C_DEPS))
C_LIBOBJ = clientfunc.o
CLIBOBJ = $(patsubst %,$(ODIR)/%,$(C_LIBOBJ))
CLIBSRC = clientfunc.c
C_APPOBJ = client.o
CAPPOBJ = $(patsubst %,$(ODIR)/%,$(C_APPOBJ))


$(ODIR)/serverfunc.o: serverfunc.c 
	$(CC) -c -o $@ $< $(CFLAGS)

$(LDIR)/libserver.a:$(SLIBOBJ)
	ar rs $@ $^

$(SAPPOBJ): server.c $(SDEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)
	
$(BDIR)/server.out: $(SAPPOBJ) $(LDIR)/libserver.a						
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)


$(ODIR)/clientfunc.o: clientfunc.c 
	$(CC) -c -o $@ $< $(CFLAGS)

$(LDIR)/libclient.a:$(CLIBOBJ)
	ar rs $@ $^

$(CAPPOBJ): client.c $(CDEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)
	
$(BDIR)/client.out: $(CAPPOBJ) $(LDIR)/libclient.a						
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)


.PHONY: clean

clean: 
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
	rm -f $(BDIR)/*.out $(LDIR)/* 
	
.PHONY: all
all: $(BDIR)/server.out $(ODIR)/serverfunc.o $(BDIR)/client.out $(ODIR)/clientfunc.o