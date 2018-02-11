#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include <curl/curl.h>
#include "dstring/dstring.h"

#define static_strlen(field) (sizeof(field) / sizeof(field[0]))

int http_get(String *response, const char *url);
int http_post(String *response, const char *url, const char *buffer);
int http_download(const char *filename, const char *url);

static size_t curl_writecb(void *ptr, size_t size, size_t nmemb, String *str)
{
	size_t chunksize;

	chunksize = size * nmemb;
	if(string_addn(str, ptr, chunksize) == STRING_ERR) return(0);
	return(chunksize);
}

static size_t curl_writecb_file(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written;

	written = fwrite(ptr, size, nmemb, stream);
	return(written);
}

int http_get(String *response, const char *url)
{
	CURL *curl;
	CURLcode res;
	int status;

	curl = curl_easy_init();
	if(!curl) return(-1);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writecb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	/* Fake useragent */
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322");

	char *cptr;
	if(cptr = strstr(url, ".onion"))
	{
		cptr += static_strlen(".onion") - 1;
		if(*cptr == '\0' || *cptr == '/')
		{
			curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:9050");
			curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
		}
	}


	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{ status = -1; goto ret; }

	res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status);
	if(res != CURLE_OK) status = 0;


	ret:
		curl_easy_cleanup(curl);
		return(status);
}

int http_post(String *response, const char *url, const char *buffer)
{
	CURL *curl;
	CURLcode res;
	int status;

	curl = curl_easy_init();
	if(!curl) return(-1);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writecb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
	/* Fake useragent */
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322");

	char *cptr;
	if(cptr = strstr(url, ".onion"))
	{
		cptr += static_strlen(".onion") - 1;
		if(*cptr == '\0' || *cptr == '/')
		{
			curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:9050");
			curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
		}
	}


	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{ status = -1; goto ret; }

	res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status);
	if(res != CURLE_OK) status = 0;


	ret:
		curl_easy_cleanup(curl);
		return(status);
}

int http_download(const char *filename, const char *url)
{
	CURL *curl;
	CURLcode res;
	FILE *file;
	int status;

	curl = curl_easy_init();
	if(!curl) return(-1);

	file = fopen(filename, "wb");
	if(!file)
	{
		status = -1;
		goto ret;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writecb_file);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	/* Fake useragent */
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322");

	char *cptr;
	if(cptr = strstr(url, ".onion"))
	{
		cptr += static_strlen(".onion") - 1;
		if(*cptr == '\0' || *cptr == '/')
		{
			curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:9050");
			curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
		}
	}

	res = curl_easy_perform(curl);
	fclose(file);

	if(res != CURLE_OK)
	{ status = -1; goto ret; }

	res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status);
	if(res != CURLE_OK) status = 0;

	ret:
		curl_easy_cleanup(curl);
		return(status);

}

#ifdef HTTP_ASYNC

/* ASYNC FUNCTIONALITY */

#include <pthread.h>

int http_get_async(String *response, const char *url);
int http_post_async(String *response, const char *url, const char *buffer);
int http_download_async(const char *filename, const char *url);

typedef struct
{
	String *response;
	const char *url;
} http_get_thread_container;

void *http_async_get_helper(void *vptr)
{
	http_get_thread_container *container = (http_get_thread_container*)vptr;

	http_get(container->response, container->url);

	return(0);
}

void http_async_get(String *response, const char *url)
{
	pthread_t tid;
	http_get_thread_container container;

	container.response = response;
	container.url = url;

	pthread_create(&tid, 0, http_async_get_helper, (void*)&container);
}

typedef struct
{
	String *response;
	const char *url;
	const char *buffer;
} http_post_thread_container;

void *http_async_post_helper(void *vptr)
{
	http_post_thread_container *container = (http_post_thread_container*)vptr;

	http_post(container->response, container->url, container->buffer);
}

void http_async_post(String *response, const char *url, const char *buffer)
{
	pthread_t tid;
	http_post_thread_container container;

	container.response = response;
	container.url = url;
	container.buffer = buffer;

	pthread_create(&tid, 0, http_async_post_helper, (void*)&container);
}

typedef struct
{
	const char *url;
	const char *filename;
} http_download_thread_container;

void *http_async_download_helper(void *vptr)
{
	http_download_thread_container *container = (http_download_thread_container*)vptr;

	http_download(container->filename, container->url);
}

void http_async_download(const char *filename, const char *url)
{
	pthread_t tid;
	http_download_thread_container container;

	container.filename = filename;
	container.url = url;

	pthread_create(&tid, 0, http_async_download_helper, (void*)&container);
}

#endif /* HTTP_ASYNC */

#endif /* HTTP_H */