LOGGER_CFLAGS=-framework ApplicationServices -framework Carbon
SOURCES=keylogger.c
EXECUTABLE=keylogger
INSTALLBIN=/usr/local/bin/keylogger
PLIST=keylogger.plist
PLISTFULL=/Library/LaunchAgents/keylogger.plist

all: keylogger keystat

config.h:
	./configure

keylogger: keylogger.c config.h
	$(CC) keylogger.c $(LOGGER_CFLAGS) -o $@

keystat: keystat.c config.h
	$(CC) keystat.c -o $@

install: all
	sudo mkdir -p /usr/local/bin
	sudo install -m755 keylogger /usr/local/bin/keylogger
	sudo install -m755 keystat /usr/local/bin/keystat

uninstall:
	launchctl unload $(PLISTFULL)
	sudo $(RM) /usr/local/bin/keylogger
	sudo $(RM) /usr/local/bin/keystat
	$(RM) $(PLISTFULL)

startup: install
	sudo cp -f -v $(PLIST) $(PLISTFULL)
	launchctl load -w $(PLISTFULL)

load:
	launchctl load $(PLISTFULL)

unload:
	launchctl unload $(PLISTFULL)

clean:
	$(RM) keylogger keystat

.PHONY: all clean config.h
