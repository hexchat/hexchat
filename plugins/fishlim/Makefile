
OURCFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c99 -pedantic `pkg-config --cflags glib-2.0 libcrypto` -DG_DISABLE_DEPRECATED=1 -fPIC $(CFLAGS)
#OURLINKFLAGS = `pkg-config --libs glib-2.0 libcrypto` -shared -fPIC -Wl,-z,defs $(CFLAGS) $(LDFLAGS)
OURLINKFLAGS = `pkg-config --libs glib-2.0 libcrypto` $(CFLAGS) $(LDFLAGS)

BASE_OBJECTS = irc.o fish.o keystore.o misc.o
PLUGIN_OBJECTS = $(BASE_OBJECTS) plugin_xchat.o
TEST_OBJECTS = $(BASE_OBJECTS) test.o

all: fishlim.so test

fish.o: fish.h keystore.h misc.h
irc.o: irc.h
keystore.o: keystore.h irc.h fish.h misc.h plugin_xchat.h
misc.o: misc.h
test.o: fish.h
plugin_xchat.o: fish.h irc.h keystore.h plugin_xchat.h

.c.o:
	$(CC) $(OURCFLAGS) -c $< -o $@

fishlim.so: $(PLUGIN_OBJECTS)
	$(CC) -shared $(OURLINKFLAGS) $(PLUGIN_OBJECTS) -o $@

test: $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) -o $@ $(OURLINKFLAGS)


.PHONY: all clean distclean install uninstall
clean:
	-$(RM) -f $(PLUGIN_OBJECTS) $(TEST_OBJECTS) fishlim.so test
distclean: clean

install: fishlim.so
	install -d $(DESTDIR)/usr/lib/xchat/plugins/
	install -m 644 fishlim.so $(DESTDIR)/usr/lib/xchat/plugins/
uninstall:
	rm $(DESTDIR)/usr/lib/xchat/plugins/fishlim.so


