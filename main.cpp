// main.cpp
//
// Entry point for the software.
#include <bcm2835.h>
#include <stdio.h>
#include <iostream>
#include <syslog.h>
#include <unistd.h>
#include "config.h"

// is debug needed?
bool d;
// set the delay when waiting for the button press
int delay;

void usage(void) {
	cout << "Usage: mrpi [-g] [-d NUMBER]" << endl;
	cout << "g turns on debug mode -> verbose messages in the /var/log/messages" << endl;
	cout << "d NUMBER - delays the GPIO port check with NUMBER milliseconds. Default value is 500." << endl;
}

void ReadOptions(int argv, char *argc[]) {
	int c;

	opterr = 0;

	while ((c = getopt (argv, argc, "gd:")) != -1) {
		switch (c)
		{
			case 'g':
				d = true;
				break;
			case 'd':
				delay = atoi(optarg);
				break;
			default:
				usage();
		}
	}
}

int main(int argc, char **argv)
{
	// two options are available g to use verbose debug in the /var/log/messages
	// and d NUMBER to change the delay of the GPIO port status check from 500ms
	d = false;
	delay = 500;

	ReadOptions(argc, argv);

	if (d)	openlog("MRPI", 0, LOG_USER);

	// If you call this, it will not actually access the GPIO
	// Use for testing
	//    bcm2835_set_debug(1);

	if (!bcm2835_init())
		return 1;

	string cmd, dir;
	LoadConfig *c = new LoadConfig();

	if (d)	syslog(LOG_INFO, "Found config file: %s", (c->hasConfig()==false) ? "no" : "yes");

	int configcnt = c->getConfigCount();
	int ports[configcnt];

	memset(ports, 0, configcnt);

	c->getPorts(ports);
	
	// initialize the ports
	for(int i=0; i<configcnt; i++) {
		c->getDirection(ports[i], dir);
		c->getCommand(ports[i], cmd);

		if (d)	syslog(LOG_INFO, "Direction/command for port %d is %s/%s", ports[i], dir.c_str(), cmd.c_str());

		if (dir.compare("IN")==0)
			bcm2835_gpio_fsel(ports[i], BCM2835_GPIO_FSEL_INPT);
		if (dir.compare("OUT")==0)
			bcm2835_gpio_fsel(ports[i], BCM2835_GPIO_FSEL_OUTP);

		bcm2835_gpio_set_pud(ports[i], BCM2835_GPIO_PUD_UP);

		bcm2835_gpio_clr_ren(ports[i]);
		bcm2835_gpio_clr_fen(ports[i]);
		bcm2835_gpio_clr_hen(ports[i]);
		bcm2835_gpio_clr_len(ports[i]);
		bcm2835_gpio_clr_aren(ports[i]);
		bcm2835_gpio_clr_afen(ports[i]);
	}

	while (1)
	{
		for(int i=0; i<configcnt; i++) {
			uint8_t v = bcm2835_gpio_lev(ports[i]);
			c->getCommand(ports[i], cmd);
			if (v==0) {
				if (d)	syslog(LOG_INFO, "Button on GPIO port %d was pressed , executing command %s", ports[i], cmd.c_str());
				if (!c->isButtonPressed(ports[i])) system(cmd.c_str());
				c->pressButton(ports[i]);
			}
			else	c->releaseButton(ports[i]);
		}
		delay(delay);
	}
	if (d)	closelog();
	bcm2835_close();
	return 0;
}

