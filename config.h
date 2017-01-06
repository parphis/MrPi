#ifndef _CONFIGH_
#define _CONFIGH_

#include <string.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <syslog.h>

using namespace std;

class MRPIConfig {
private:
	bool m_btn_pressed;
	string m_port;
	int m_port_int;
	string m_direction;
	string m_command;
public:
	MRPIConfig(void);
	void setPort(string port);
	void setDirection(string direction);
	void setCommand(string command);
	void ButtonPressed(bool p);
	void getPort(string &port);
	int getPortInt(void);
	void getDirection(string &direction);
	void getCommand(string & command);
	bool IsButtonPressed(void);
};

class LoadConfig {
private:
	unsigned int m_maxGPIO_Ports;
	string m_configpath;
	string m_whitelistpath;
	bool m_hasconfigfile;
	bool m_haswhitelistfile;
	vector<string> m_whitelist;
	vector<MRPIConfig *> m_config;

	void addLog(string l);
	void discardComment(FILE *f);
	bool hasWhitelist(void);
	bool hasConfigFile(void);
	bool cmdAllowed(string c);
	void Tokenize(const string& str, vector<string>& tokens, const string& delimiters=" ");
	void tokenize(const string &str, vector<string> &tokens, const string &delimiters=" ");
	bool parseConfig(char* l, string &port, string &direction, string &command);
	void createConfigObjects(string port, string direction, string command);
	void readConfig(char* l);
public:
	LoadConfig(void);
	bool hasConfig(void);
	unsigned int getMaxGPIOPorts(void);
	unsigned int getConfigCount(void);
	void getPorts(int *ports);
	void getPortNumber(int key, string &direction);
	void getCommand(int key, string &command);
	void getDirection(int key, string &direction);
	void pressButton(int key);
	void releaseButton(int key);
	bool isButtonPressed(int key); 
};

#endif
