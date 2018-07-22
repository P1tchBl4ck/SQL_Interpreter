#ifndef ADDRESS_HPP
#define ADDRESS_HPP
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Written by Alberto De Angelis
 * This C++ library contains a class to wrap the structure sockaddr_in
 * contained in the inet.h library, to work with IPs and ports in
 * easier way when working on C++.
 * USAGE: #include "Address.hpp"
 */

class Address{
	private:
		char* 	ip;
		int 	port;

	public:
		//Constructor
		Address();						//Empty constructor
		Address(int, char*);			//Constructor overload
		Address(struct sockaddr_in);	//Overload costruttore
		Address(const Address&); 		//Cloner
		~Address();						//Destructor
		//Altri metodi
		char* 	to_string();
		void 	init(int, char*);	//Called on constructing
		void 	show(); 			//Prints address IP and port
		//Accessors
		char* 	get_ip();
		int 	get_port();
		void 	set_address(struct sockaddr_in);
		void 	set_ip(char*);
		void 	set_port(int);
		struct sockaddr_in get_address();
};

//When no parameters are defined in the constructor, everything is set to 0
Address::Address(){init(0, strdup("0.0.0.0"));}

//Constructor called with port and IP
Address::Address(int p, char* i){init(p, i);}

//If you already have a struct sockaddr, you can use it to build an Address
Address::Address(struct sockaddr_in a){
	free(ip);
	port = ntohs(a.sin_port);
	ip = strdup(inet_ntoa(a.sin_addr));
}

Address::Address(const Address& dolly){
	ip = strdup(dolly.ip);
	port = dolly.port;
}

Address::~Address(){}

char*	Address::to_string(){
		char s[22]; //IP max length(dots included) + ':'
					// + max port length + end character.
		sprintf(s, "%s:%d", ip, port);
		return strdup(s);
}

//Called everytime the constructor is called
void	Address::init(int p, char* i){
		port = p;
		ip = strdup(i);
}

void	Address::show(){printf("%s:%d\n", ip, port);}

char*	Address::get_ip(){return ip;}

int 	Address::get_port(){return port;}

void	Address::set_ip(char* i){
		free(ip);
		ip = i;
}

void 	Address::set_address(struct sockaddr_in address){
	free(ip);
	port = ntohs(address.sin_port);
	ip = strdup(inet_ntoa(address.sin_addr));
}

void	Address::set_port(int p){
		port = p;
}

struct sockaddr_in Address::get_address(){
	struct sockaddr_in ret;
	inet_aton(ip, &ret.sin_addr);
	ret.sin_port = htons(port);
	ret.sin_family = AF_INET;
	for(int i = 0; i < 8; i++)	ret.sin_zero[i] = 0;
	return ret;
}

#endif