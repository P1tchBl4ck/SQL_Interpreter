#ifndef __SOCKETTCP_HPP
#define __SOCKETCTP_HPP

#include <arpa/inet.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "Address.hpp"
#include "my_functions.h"

#define IP_LOOPBACK "127.0.0.1"
#define IP_MYSELF "0.0.0.0"
#define MAX_CONN 50
#define MAX_MSG 4096

using namespace std;
/**
 * Created by Alberto De Angelis
 * This library contains four classes to work easily with TCP
 * connections using sockets and the "Address.hpp" library I created.
 * define a constant named __SOCKETTCP_NO_DEBUG to hide step by step
 * info in the library.
 * USAGE:	#include "socket_tcp.hpp"
 *			#define __SOCKETTCP_NO_DEBUG
 */


//__________________________________________________________________
//START SocketTCP

/**
 * Superclass containing the procedures common
 * in ClientTCP and ServerTCP
 */
class SocketTCP{
	protected:	int sock_id;
	
	public:	SocketTCP();	//API: socket()
			~SocketTCP();	//API: close()

			bool set_broadcast(bool);
};

SocketTCP::SocketTCP(){
	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("socket() = %d\n", sock_id);
	#endif //__SOCKETTCP_NO_DEBUG
}

SocketTCP::~SocketTCP(){
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("Closing socket\n");
	#endif //__SOCKETTCP_NO_DEBUG
	close(sock_id);
}

bool SocketTCP::set_broadcast(bool broadcast){
//returns true if set_broadcast didn't work well.
//Usage: if(set_broadcast(true)) printf("Error");
	int optionvalue = broadcast? 1:0;
	int ret = setsockopt(sock_id,
			SOL_SOCKET,
			SO_BROADCAST,
			(void*)&optionvalue,
			(socklen_t)sizeof(int));
	return ret != 0;
}

//END SocketTCP
//__________________________________________________________________
//START Connection

/**
 * This class is used to manage TCP connections. Using connect() 
 * method on a ClientTCP returns a Connection object where you
 * can send or receive data.
 * The constructors methods aren't meant to be used directly in
 * your code, but to be called by the other classes.
 */

class Connection{
	private:	int 	conn_id;
				bool 	fuffa;
				//if fuffa is true, the connection is a fake connection
				//and the connection ID is a socket ID in disguise.

	public:		Connection(int id_conn, bool f); 
				~Connection();		

			bool 	send(char* msg);
			bool 	send_raw(void*, int); //API: send()
			char* 	receive();
			int 	get_id();
			void* 	receive_raw(int* len);//API: recv()
};

Connection::Connection(int id_conn, bool f){
	conn_id = id_conn;
	fuffa = f;
}

Connection::~Connection(){
	//if it is a connection and not a socket,
	//shutdown the connection
	if(!fuffa)	shutdown(conn_id, SHUT_RDWR);
}

//Send metod is used for text messages in simple
//programs and contains a call to send_raw().
bool Connection::send(char* msg){
	fflush(stdout);	
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("Entering Connection::send()\n");
	#endif //__SOCKETTCP_NO_DEBUG
	//*(msg + strlen(msg)) = '\0';	//This line lately gave some problems,
	//comment if not working. If the receiver is also using this library,
	//the end character will be added anyway on receiving.
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("Before send_raw()\n");
	#endif //__SOCKETTCP_NO_DEBUG
	return send_raw((void*) msg, strlen(msg)+1);
}

//The send_raw() method is used in more complex programs
//Where specifical data needs to be sent.
bool Connection::send_raw(void* msg, int len){
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("Entering Connection::send_raw\tlen: %d\n", len);
	printf("Before ::send()\n");
	#endif //__SOCKETTCP_NO_DEBUG
	int ret = (int) ::send(conn_id, msg, (size_t)len, 0);
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("After ::send()\n");
	printf("send() = %d\n", ret);
	#endif //__SOCKETTCP_NO_DEBUG
	return(ret != len);
}

//Receive method is used for text messages in simple
//programs and has a call to receive_raw() inside it.
char* Connection::receive(){
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("Entered in Connection::receive()\n");
	#endif //__SOCKETTCP_NO_DEBUG
	int len = 0;
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("Connection::receive()\tlen = %s\n", len);
	#endif //__SOCKETTCP_NO_DEBUG
	char* ret = (char*)receive_raw(&len);
	if(ret == NULL)	return NULL;
	ret[len] = '\0';
	return ret;
}

int Connection::get_id(){
	return conn_id;
}

//Receive_raw is used in more complex programs
//where specifical data is received.
void* Connection::receive_raw(int* lenga){
	char buffo[MAX_MSG+1];
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("Entered in receive_raw\n");
	printf("Before recv\n");
	#endif //__SOCKETTCP_NO_DEBUG
	*lenga = recv(conn_id,
			buffo,
			MAX_MSG,
			0);
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("After recv\tlen = %d\n", *lenga);
	#endif //__SOCKETTCP_NO_DEBUG
	if(*lenga <= 0)	return NULL;
	return my_dup(buffo, lenga);
}

//END Connection
//________________________________________________________________
//START ServerTCP

/**
 * The ServerTCP class creates a working TCP server,
 * which can manage one or more TCP clients,
 * because it contains a list of connections.
 * You can work on a single connection taken by the result of the
 * accept() method and also send a message to all the clients
 * connected to the server with the send_everyone() method.
 */

class ServerTCP: public SocketTCP{
	//Inherits from SocketTCP
	private:	std::list<Connection> connections;
	public:		ServerTCP(int port, bool loopback);//API: bind() | listen()
				~ServerTCP();
			void 		disconnect(Connection c);
			void 		send_everyone(char* msg);
			Connection* accept(); //API: accept()
};

//The constructor wants to know the port  where you want to listen
//and if you are using this server in your net or in loopback.
ServerTCP::ServerTCP(int port, bool loopback):SocketTCP(){
	Address* myself;
	struct sockaddr_in myself_addr;
	if(loopback)
		myself = new Address(port, strdup(IP_LOOPBACK));
	else
		myself = new Address(port, strdup(IP_MYSELF));
	myself_addr = myself->get_address();
	bind(sock_id,
		(struct sockaddr*) &myself_addr,
		(socklen_t)sizeof(struct sockaddr));
	listen(sock_id, MAX_CONN);
}

ServerTCP::~ServerTCP(){
	connections.clear();
	//Automatic call to superclass destructor
}

//This method lately gave some problems, don't use it
//if you don't know how to fix it.
void ServerTCP::disconnect(Connection c){
	//connections.remove(c);
}

void ServerTCP::send_everyone(char* msg){
	//The function can generate warnings.
	//Usage: g++ (your_program.cpp) -o (output name) -w
	for(Connection c : connections)	c.send(msg);
}

Connection* ServerTCP::accept(){
	Connection* ret;
	Address* sender;
	struct sockaddr_in sender_addr;
	int addrlen = sizeof(struct sockaddr);
	int conn_id = ::accept(sock_id,
			(struct sockaddr*) &sender_addr,
			(socklen_t*)&addrlen);
	
	sender = new Address(sender_addr);
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("Accepted connection from %s\nconn_id: %d\n",
		sender->to_string(), conn_id);
	#endif //__SOCKETTCP_NO_DEBUG
	if(conn_id <= 0)	return NULL;
	ret = new Connection(conn_id, false);
	connections.push_back(*ret);
	return ret;
}

//END ServerTCP
//________________________________________________________________
//START ClientTCP

/**
 * Creates a working TCP client. Works differently from the server,
 * since connections in the client are treated as normal sockets,
 * so the connection is contained in it and the methods to send
 * and receive are called directly by it.
 */

class ClientTCP: public SocketTCP{
	//Inherits from SocketTCP
	private:	Connection* connection;

	public:		ClientTCP();//API: connect()
				~ClientTCP();

			bool 	connect(Address);
			bool 	send(char* message);
			bool 	send_raw(void* message, int len);
			char* 	receive();
			void* 	receive_raw(int* len);
};

ClientTCP::ClientTCP(){}

ClientTCP::~ClientTCP(){
	delete connection;
}

//To build an Address object, see the "Address.hpp" library.
bool ClientTCP::connect(Address server){
	struct sockaddr_in server_addr;
	server_addr = server.get_address();	
	int ret = ::connect(sock_id,
		(struct sockaddr*) &server_addr,
		(socklen_t) sizeof(struct sockaddr_in));
	connection = new Connection(sock_id, true);
	#ifndef __SOCKETTCP_NO_DEBUG
	printf("connect() = %d\n", ret);
	#endif //__SOCKETTCP_NO_DEBUG
	return ret;
}

bool ClientTCP::send(char* message){
	return connection->send(message);
}

bool ClientTCP::send_raw(void* message, int len){
	return connection->send_raw(message, len);
}

char* ClientTCP::receive(){
	return connection->receive();
}

void* ClientTCP::receive_raw(int* len){
	return connection->receive_raw(len);
}

//END ClientTCP____________________________________________________

#endif //__SOCKETTCP_HPP