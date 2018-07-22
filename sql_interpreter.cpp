#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "socket_tcp.hpp"
#include "my_functions.h"

#define HUGE_VECTOR 16000
#define DB_PARAM "db=\""
#define QUERY_PARAM "query=\""
#define SQL_TAG "<sql"
#define HTML_OPENING "<html><head><title>query</title><body>"
#define HTML_CLOSING "</body></html>"

/**
 * This program opens a server listening to HTTP headrs including
 * HTML pages with an invented tag to make sqlite queries.
 * Tag usage: <sql db="(db_name)" query="(sqlite query)">
 * The program will send back the same page, replacing the <sql>
 * tags with a <table> containing the result of the query.
 * To compile: g++ sql_interpreter.cpp -o (output name.out) -lsqlite3
 * (You must have sqlite3 installed)
 * To execute: ./(output name.out) PORT
 * The PORT parameter is the port on which you want to listen.
 */

int main(int argc, char** argv){
	int 			i;
	int 			port;
	int 			ret;
	char 			temp;
	char* 			dbname;
	char* 			dbstart;
	char* 			error;
	char* 			pointer;
	char 			response[HUGE_VECTOR];
	char* 			query;
	char* 			querystart;
	char* 			request;
	char* 			root;
	char* 			table;
	sqlite3* 		db;
	sqlite3_stmt* 	res;
	Connection* 	connection;
	ServerTCP* 		myself;

	if(argc != 2){
		printf("USAGE: %s PORT\n", argv[0]);
		return -1;
	}
	sprintf(response, "http response");
//	sprintf(response, "%s", HTTP_RESPONSE);
	printf("Before ATOI\n");
	port = atoi(argv[1]);
	printf("After ATOI\n");
	myself = new ServerTCP(port, true);
	printf("Before ACCEPT\n");
	connection = myself->accept();
	printf("After ACCEPT\n");
	request = strdup(connection->receive());
//	Request contains the HTML file which could contain an SQL tag.
	printf("Before strstr\n");
	pointer = strstr(request, SQL_TAG);
	printf("after strstr\n");
	if(pointer == NULL){
		printf("pointer is null\n");
//		Didn't receive any sql tag, so closing connection.
		sprintf(response, "Error message for missing SQL tag");
		printf("After sprintf\n");
//		sprintf(response, "%s%s", HTTP_RESPONSE, NO_SQL);
		connection -> send(response);
		printf("No operation done.\n");
		delete connection;
		delete myself;
		return 1;
	}
	dbstart = strstr(pointer, DB_PARAM);
	if(dbstart == NULL){
		printf("dbstart is null\n");
//		There is an sql tag, but the db parameter doesn't exist.
		sprintf(response, "Error message for missing DB param");
//		sprintf(response, "%s%s", HTTP_RESPONSE, NO_DB_PARAM);
		connection -> send(response);
		printf("Missing DB parameter.\n No operation done.\n");
		delete connection;
		delete myself;
		return 2;	
	}
//	Taking value in param and putting it into dbname
	temp = *(dbstart + strlen(DB_PARAM));
	i = 0;
	printf("before while\n");
	while(temp != '"'){
		*(dbname + i) = temp;
		i++;
		temp = *(dbstart + strlen(DB_PARAM) + i);
	}
	printf("after while\n");
	*(dbname + i) = '\0';
	querystart = strstr(pointer, QUERY_PARAM);
	if(querystart == NULL){
//		There is an sql tag, but the query parameter doesn't exist.
		sprintf(response, "Error message for missing query param");
//		sprintf(response, "%s%s", HTTP_RESPONSE, NO_QUERY_PARAM);
		connection -> send(response);
		printf("Missing DB parameter.\nNo operation done.\n");
		delete connection;
		delete myself;
		return 3;	
	}
//	Taking value in param and putting it into query
	temp = *(querystart + strlen(QUERY_PARAM));
	i = 0;
	while(temp != '"'){
		*(query + i) = temp;
		i++;
		temp = *(querystart + strlen(QUERY_PARAM) + i);
	}	
	*(query + i) = '\0';

//	Opening DB
	ret = sqlite3_open(dbname, &db);
	if(ret != SQLITE_OK){
		printf("Error: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		sprintf(response, "SQLite error message: %s", sqlite3_errmsg(db));
/*		sprintf(	response,	"%s%s%s%s",
				HTTP_RESPONSE,	SQLITE_ERROR,
				sqlite3_errmsg(db), HTML_CLOSING);*/
		connection -> send(response);
		delete connection;
		delete myself;
		return -2;
	}
//	Preparing query
	ret = sqlite3_prepare_v2(db, query, -1, &res, 0);
	if(ret != SQLITE_OK){
		printf("Error: %s\n", sqlite3_errmsg(db));
		sprintf(response, "SQLite error message: %s", sqlite3_errmsg(db));
/*		sprintf(	response,	"%s%s%s%s",
				HTTP_RESPONSE,	SQLITE_ERROR,
				sqlite3_errmsg(db), HTML_CLOSING);*/
		sqlite3_close(db);
		connection -> send(response);
		delete connection;
		delete myself;
		return -3;
	}
//	Commiting query
	table = "<table>";
	while(sqlite3_step(res) == SQLITE_ROW){
		//Printing query result
		sprintf(table, "<tr>");
		for(int j = 0; j < sqlite3_column_count(res); j++){
			sprintf(table, "<td>%s</td>",
				sqlite3_column_text(res, j));
		}
		sprintf(table, "</tr>");
	}
	sprintf(table, "</table>");
	sprintf(response, "");
	sprintf(response, "Successive response: %s", table);
/*	sprintf(response, "%s%s%s%s", HTTP_RESPONSE,
					HTML_OPENING,
					table,
					HTML_CLOSING);*/
//Sending response.
	if(connection->send(response))my_error("send()",-3);
	delete connection;
	delete myself;
	free(root);
	free(request);
	return 0;
}
