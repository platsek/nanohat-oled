#include <openssl/ssl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "yt.h"

#define MAX_KEY_LEN	31
#define MAX_VAL_LEN	63

/*
 * Slightly modified Jonathan Leffler's code from:
 * https://stackoverflow.com/questions/24490410/extracting-key-value-with-scanf-in-c
 */
int find_any_key_value(char *str, char *key, char *value)
{
	char junk[256];
	const char *search = str;

	while (*search != '\0') {
		int offset;
		if (sscanf
		    (search, " \"%31[a-zA-Z]\": \"%63[0-9]\"%n", key, value,
		     &offset) == 2)
			return (search + offset - str);
		int rc;
		if ((rc = sscanf(search, "%255s%n", junk, &offset)) != 1)
			return EOF;
		search += offset;
	}

	return EOF;
}

int find_key_value(char *str, char *key, char *value)
{
	char found[MAX_KEY_LEN + 1];
	int offset;
	char *search = str;

	while ((offset = find_any_key_value(search, found, value)) > 0) {
		if (strcmp(found, key) == 0)
			return (search + offset - str);
		search += offset;
	}

	return offset;
}

/*
 * Minimalist HTTPS client in C on Linux: https://gist.github.com/nir9
 */
int get_channel_statistics(char **views, char **subs, char **videos)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	const struct sockaddr_in addr = {
		AF_INET,
		htons(443),
		htonl(0xACD90C6A)	/* 172.217.12.106 = www.googleapis.com */
	};

	connect(sockfd, (void *)&addr, sizeof(addr));

	const struct timeval tv = {
		5,		/* 5 seconds */
		0		/* 0 useconds */
	};

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, sizeof(tv));

	SSL_CTX *ctx = SSL_CTX_new(TLS_method());
	SSL *ssl = SSL_new(ctx);
	SSL_set_fd(ssl, sockfd);
	SSL_connect(ssl);
	char *request =
	    "GET https://youtube.googleapis.com/youtube/v3/channels?part=statistics&id=&key= HTTP/1.1\r\n\r\nAccept: application/json\r\n";
	SSL_write(ssl, request, strlen(request));
	char response[1024] = { 0 };
	/* blocking method will return -1 if nothing is received after 5 seconds */
	int rc = SSL_read(ssl, response, 1023);
	/* connection error or timeout */
	if (rc == -1)
		return EXIT_FAILURE;
	/* check http status code */
	if (response[9] != '2' && response[10] != '0' && response[11] != '0')
		return EXIT_FAILURE;

	static char view_count[MAX_VAL_LEN + 1];
	static char subscriber_count[MAX_VAL_LEN + 1];
	static char video_count[MAX_VAL_LEN + 1];

	find_key_value(response, "viewCount", view_count);
	*views = view_count;

	find_key_value(response, "subscriberCount", subscriber_count);
	*subs = subscriber_count;

	find_key_value(response, "videoCount", video_count);
	*videos = video_count;

	return EXIT_SUCCESS;
}
