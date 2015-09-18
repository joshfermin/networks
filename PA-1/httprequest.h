#ifndef HTTPREQUEST_H_   /* Include guard */
#define HTTPREQUEST_H_
struct Http_Request {
	char 	method[100];
	char 	url[256];
	char 	http_version[16];
	char 	host[128];
	int 	keep_alive;
} http_request;
#endif