INCLUDES = -I/usr/include/raptor2 -I/usr/include/rasqal -I/usr/include/libxml2 -Iliburi
CPPFLAGS = -W -Wall 
LIBS = -lrdf -L/usr/lib/x86_64-linux-gnu -lcurl -L/usr/lib -lxml2 liburi/.libs/liburi.a
CFLAGS = $(CPPFLAGS) $(DEFS) $(INCLUDES) -O0 -g

OUT = lod-util
OBJ = lod-util.o

LIBOUT = liblod.a
LIBOBJ = context.o subject.o resolve.o fetch.o html.o

all: $(OUT) $(LIBOUT)

$(OUT): $(OBJ) $(LIBOUT)
	$(CC) $(LDFLAGS) -o $@ $+ $(LIBS)

$(LIBOUT): $(LIBOBJ)
	$(AR) rcs $@ $+

clean:
	rm -f $(LIBOUT) $(LIBOBJ) $(OUT) $(OBJ)
