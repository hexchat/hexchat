CONV = gdk-pixbuf-csource

LIST =	bookpng book.png \
			hoppng hop.png \
			oppng op.png \
			purplepng purple.png \
			redpng red.png \
			trayfilepng fileoffer.png \
			trayhilightpng highlight.png \
			traymsgpng message.png \
			voicepng voice.png \
			xchatpng ..\..\xchat.png

all: 
	@$(CONV) --build-list $(LIST) > inline_pngs.h

clean:
	@del *.h
