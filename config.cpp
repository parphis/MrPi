/*
 * Read the config file and provide access to the port settings.
 *
 * Valid config file must be in the format of:
 * 
 * PORT_NUMBER=DIRECTION,COMMANDNAME<newline>
 * PORT_NUMBER=DIRECTION,COMMANDNAME<newline> etc...
 *
 * PORT_NUMBER is the phisycal place number of the pin on the rPI version 2 (B). (2014)
 *    Valid numbers are 03,05,07,08,10,11,12,13,15,16,18,19,21,22,23,24,26.
 * DIRECTION must be IN or OUT. IN is for switches, buttons, OUT is for LEDs e.g.
 * COMMANDNAME is an executable shell script file (.sh) which should be executed by 
 *  the system when the button was pressed. It can be empty in both IN and OUT directions.
 *  The file must be in the same folder where the mrpi is.
 *
 * @since 12th of Dec. 2014
 */

#include "config.h"

MRPIConfig::MRPIConfig() {
	;
}
void MRPIConfig::setPort(string port) {
	this->m_port = port;
	stringstream ss(this->m_port);
	ss >> this->m_port_int;
}
void MRPIConfig::setDirection(string direction) {
	this->m_direction = direction;
}
void MRPIConfig::setCommand(string command) {
	this->m_command = command;
}
void MRPIConfig::ButtonPressed(bool p) {
	this->m_btn_pressed = p;
}
void MRPIConfig::getPort(string &port) {
        port = this->m_port;
}
int MRPIConfig::getPortInt(void) {
	return this->m_port_int;
}
void MRPIConfig::getDirection(string &direction) {
        direction = this->m_direction;
}
void MRPIConfig::getCommand(string &command) {
        command = this->m_command;
}
bool MRPIConfig::IsButtonPressed(void) {
	return this->m_btn_pressed;
}

LoadConfig::LoadConfig(void) : m_maxGPIO_Ports(17), m_configpath("/etc/mrpi/conf"), m_whitelistpath("/etc/mrpi/whitelist"), m_hasconfigfile(false), m_haswhitelistfile(false) {
	this->m_haswhitelistfile = this->hasWhitelist();
	this->m_hasconfigfile = this->hasConfigFile();	
}
void LoadConfig::addLog(string l) {
	openlog("MRPI::LoadConfig", 0, LOG_USER);
	syslog(LOG_INFO, l.c_str());
	closelog();
}

void LoadConfig::discardComment(FILE *f) {
	char c;
	
	do {
		c = fgetc(f);
	} while ( (c!='\n') && (c!='\r') );
}

/**
 * Check if the whitelist file for the enabled commands
 * are available or not - m_whitelistpath.
 * If this file does not exist or it is empty then no any button will 
 * function! 
 *
 * @return false if the file does not exist or true otherwise
 */
bool LoadConfig::hasWhitelist(void) {
	FILE *fp;
	int linelen = 1024;
	int i = 0;
	int c;
	char line[linelen];

	fp = fopen(this->m_whitelistpath.c_str(), "r");

	memset(line, '\0', linelen);

	if(fp==NULL) {
		addLog("Error: Could not find the whitelist file! No button will work!");
		return false;
	}
	else {
		do {
			c = fgetc(fp);
			if( (c!='\n') && (c!='\r') && (i<linelen) ) {
				if (c=='#') {
					this->discardComment(fp);
					i = 0;
					memset(line, '\0', linelen);
				}
				else {
					line[i] = c;
					i++;
				}
			}
			else {
				this->m_whitelist.push_back(line);
				i = 0;
				memset(line, '\0', linelen);
			}
		} while (c!=EOF);
	}
	return true;
}

/**
 * Reads in the file given by the m_configpath.
 * It is now hard coded as /etc/mrpi/conf.
 * The config file is a plain text file in the format of 
 *  PORT=DIRECTION,COMMANDNAME\n
 * The function reads the file and passes the lines to the 
 * readConfig function line by line.
 *
 * @return false if the config file does not exist or true
 * otherwise
 */
bool LoadConfig::hasConfigFile(void) {
	// test
	//return false;

	FILE *fp;
	int linelen = 1024;
	int i = 0;
	int c;
	char line[linelen];

	memset(line, '\0', linelen);
	
	fp = fopen(this->m_configpath.c_str(), "r");

	if(fp==NULL) {
		this->addLog("Error: could not find the config file!");
		return false;
	}
	else {
		do {
			c = fgetc(fp);
			if ( (c!='\0') && (c!='\n') && (c!='\r') && (i<linelen) ) {
				// discard comment
				if (c=='#') {
					this->discardComment(fp);
					i = 0;
					memset(line, '\0', linelen);
				}
				else {
					line[i] = c;
					i++;
				}
			}
			else {
				this->readConfig(line);
				i = 0;
				memset(line, '\0', linelen);
			}
		} while (c!=EOF);

		fclose(fp);
		return true;
	}
}

/**
 * Given a command as a string the function checks if the command itself 
 * can be executed by the program or not.
 * Command itself means the command name without any parameters. So this 
 * is not the only one perfect solution to sanitize the command string 
 * before executing it with the system function. It is recommended to 
 * restrict the rights for the configuration file and for the other bash 
 * shell files! You can modify the rights of the configuration file 
 * for the root as 700 because the program msut be executed with sudo.
 *
 * @param string c, the command with the parameters
 * @return false if the command can not be executed by pressing a button or 
 * true otherwise
 */
bool LoadConfig::cmdAllowed(string c) {
	if (c.empty())	return false;

	vector<string> tokens;
	string pure_cmd;
	
	this->Tokenize(c, tokens);
	if (tokens.size()>=1)
		pure_cmd = tokens[0];
	else	return false;
	
	for (unsigned int i=0; i<this->m_whitelist.size(); i++) {
		if (this->m_whitelist[i].compare(pure_cmd)==0) {
			return true;
		}
	}

	return false;
}

/**
 * Split a string into tokens.
 * Credit: http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
 *
 * @param string & str, the string to be tokenized
 * @param vector<string>& tokens, the vector which will contain the tokens
 * @param string &delimiters, the delimiter on which the string will be splitted, default is space
 */
void LoadConfig::Tokenize(const string& str, vector<string>& tokens, const string& delimiters)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

/**
 * Other kind of string tokenization function. It can split a string 
 * using multiple characters as delimiter as well.
 * Credit: http://stackoverflow.com/a/14266139/1829303
 */
void LoadConfig::tokenize(const string &str, vector<string> &tokens, const string &delimiters) {
	string s = str;
	size_t pos = 0;
	string token;

	while ( (pos=s.find(delimiters) )!= string::npos) {
		token = s.substr(0, pos);
		tokens.push_back(token);
		s.erase(0, pos + delimiters.length());
	}
	tokens.push_back(s);
}

/**
 * Given a characher array named l. This contains one configuration line.
 * The line must be in the format of 
 *  PORT=DIRECTION,COMMANDNAME
 * The function splits the string to tokens using the separators = and , .
 *
 * @param char* l, the configuration line.
 * @param string& port, direction, command; the exact configuration for GPIO port
 * @return true if the configuration line is correct or false otherwise
 */
bool LoadConfig::parseConfig(char *l, string &port, string & direction, string &command) {
	vector<string> parts;
	vector<string> params;
	string conf(l);

	this->tokenize(conf, parts, "=");

	if (parts.size()!=2) {
		this->addLog("Invalid configuration line (splitting by the = sign):");
		this->addLog(conf.c_str());
		return false;
	}
	else {
		port = parts.at(0);
	}

	this->tokenize(parts.at(1), params, "<cmd:>");

	if (params.size()<2) {
		this->addLog("Invalid configuration line (splitting by the <cmd:> tag):");
		this->addLog(conf.c_str());
		return false;
	}
	else {
		direction = params.at(0);
		command = params.at(1);
	}
	return true;
}

void LoadConfig::createConfigObjects(string port, string direction, string command) {
	if (this->cmdAllowed(command)) {
		MRPIConfig *c = new MRPIConfig;

		c->setPort(port);
		c->setDirection(direction);
		c->setCommand(command);

		this->m_config.push_back(c);
	}
	else {
		this->addLog("The command in configuration is not allowed to run:");
		this->addLog(command.c_str());
	}
}

/**
 * Determine wether the file given by the path m_configpath
 * exists or not.
 *
 * @return true if the file exists or false otherwise
 */
bool LoadConfig::hasConfig(void) {
	return this->m_hasconfigfile;
}

/**
 * Given a character array as one config line
 * this function parses the line and if it is well 
 * formed and does not contain any disallowed commands 
 * it calls the function createConfigObjects in order to 
 * create the configurations in the memory.
 * The command part of the config must be in a whitelist 
 * in order to allow executing it. If either there is no any 
 * whitelist nor the command part is the member of the 
 * whitelist the function WILL NOT CREATE the config 
 * object, that is the connected button will not work.
 * Log entry will be created in the /var/log/messages file.
 *
 * @param char* l, on line read from the config file.
 */
void LoadConfig::readConfig(char* l) {
	string port, direction, command;

	if (this->parseConfig(l, port, direction, command))
		this->createConfigObjects(port, direction, command);
}

/**
 * Return how many GPIO ports are enabled to use on the RPi.
 * This is herd coded now to 17 -> m_maxGPIO_Ports.
 *
 * @return unsigned int
 */
unsigned int LoadConfig::getMaxGPIOPorts(void) {
	return this->m_maxGPIO_Ports;
}

/**
 * Return the number of the config lines in the memory.
 * Actually the size of the m_config vector.
 *
 * @return unsigned int
 */
unsigned int LoadConfig::getConfigCount(void) {
	return this->m_config.size();
}

/**
 * Pass the port numbers into the ports integer array.
 * This array will be used by the main function to 
 * set up the GPIO ports and used as keys to get the directions
 * and the commands to be executed.
 * 
 * @param int *ports;
 */
void LoadConfig::getPorts(int *ports) {
	MRPIConfig *c;
	
	for(unsigned int i=0; (i<this->m_config.size()) && (i<this->m_maxGPIO_Ports); i++) {
		c = this->m_config.at(i);
		int p = c->getPortInt();
		if (p>0)	ports[i] = p;
	}
}

/**
 * Return the direction of a configuration line where the key is the port number.
 *
 * @param int port number
 * @param string &direction
 */
void LoadConfig::getDirection(int key, string &direction) {
	MRPIConfig *c;
	for (unsigned int i=0; i<this->m_config.size(); i++) {
		c = this->m_config.at(i);
		if (c->getPortInt()==key) {
			c->getDirection(direction);
			return;
		}
	}
}

/**
 * Return the command of a configuration line where the key is the port number.
 *
 * @param int port number
 * @param string &direction
 */
void LoadConfig::getCommand(int key, string &command) {
	MRPIConfig *c;
	for (unsigned int i=0; i<this->m_config.size(); i++) {
		c = this->m_config.at(i);
		if (c->getPortInt()==key) {
			c->getCommand(command);
			return;
		}
	}
}

void LoadConfig::pressButton(int key) {
	MRPIConfig *c;
	for (unsigned int i=0; i<this->m_config.size(); i++) {
		c = this->m_config.at(i);
		if (c->getPortInt()==key)	c->ButtonPressed(true);
//cout << "port press button " << key << c->IsButtonPressed() << endl;
	}
}

void LoadConfig::releaseButton(int key) {
	MRPIConfig *c;
	for (unsigned int i=0; i<this->m_config.size(); i++) {
		c = this->m_config.at(i);
		if (c->getPortInt()==key)	c->ButtonPressed(false);
//cout << "port release button" << key << c->IsButtonPressed() << endl;
	}
}

bool LoadConfig::isButtonPressed(int key) {
	MRPIConfig *c;
	for (unsigned int i=0; i<this->m_config.size(); i++) {
		c = this->m_config.at(i);
//cout << "ispressed " <<  key << c->IsButtonPressed() << endl;
		if (c->getPortInt()==key)	return c->IsButtonPressed();
	}
	return false;
}
