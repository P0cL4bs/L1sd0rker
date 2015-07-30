
/*
** Dorker - Coded by Constantine - 07/2015.
** My GitHub: https://github.com/jessesilva
** Team  GitHub: https://github.com/P0cL4bs
** Compile...
**  -> Windows: gcc.exe --std=c99 dorker.c -lws2_32 -o dorker.exe & dorker.exe
**  -> Linux: gcc -g -Wall -pthread -std=c99 dorker.c -lpthread -o dorker ; ./dorker
*/

#define WINUSER // Usuarios Linux comentar esta linha.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef WINUSER
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#define close closesocket
#define sleep Sleep
#else
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define MAXLIMIT 256
#define say printf
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0 
#endif

typedef struct {
	unsigned int number_of_threads;			/* Número de threads. */
	unsigned int scan_method;				/* Método de scaneamento. */
	unsigned char *top_level;				/* Top-level list. */
	unsigned char *dork_list;				/* Lista de dorks. */
	unsigned char *path_filter;				/* Path para buscar. */
	unsigned char *data_filter;				/* Dados para buscar no conteúdo da página. */
	unsigned char *search_engine;			/* Lista de buscadores utilizados. */
	unsigned char *results_domains;			/* Arquivo de resultados contendo apenas o domínio. */
	unsigned char *results_full_url;		/* Arquivo de resultados com URL completa. */
} instance_t;

typedef struct {
	unsigned int index;						/* Identificador. */
	unsigned char *line;					/* Linha pega da lista de dorks. */
} param_t;

typedef struct {
	unsigned int counter_a;					/* Controle das threads. */
	unsigned int counter_b ;
	unsigned int counter_c;
} thread_t;

typedef struct {
	unsigned int total_sites;				/* Total de hosts válidos encontrados. */
	unsigned int total_sites_all;			/* Total de hosts encontrados em todos os buscadores e com repetição. */
} statistics_t;

typedef struct {							/* Controle das requisições HTTP. */
	unsigned char *content;
	unsigned int length;
	unsigned int e_200_OK;
} http_request_t;

instance_t *instance;
thread_t *thread;
statistics_t *statistics;

static void core (const void *tparam);
static void bing (const unsigned char *dork);
static void google (const unsigned char *dork);
static void hotbot (const unsigned char *dork);
static void duckduckgo (const unsigned char *dork);
static void extract_urls_and_domains (unsigned char *content);
static void filter_url (const unsigned char *url, const unsigned int protocol);
static unsigned int path_and_data_filter (const unsigned char *domain, const unsigned int protocol);
static http_request_t *http_request_get (const unsigned char *domain, const unsigned char *path);
static unsigned int domain_exists (const unsigned char *domain);
static void save (const unsigned char *path, const unsigned char *content);
static unsigned int file_exists (const unsigned char *path);
void *xmalloc (const unsigned int size);
static void die (const unsigned char *content, const unsigned int exitCode);
static void banner (void);
static void help (void);

int main (int argc, char **argv) {
	
#ifdef WINUSER
	WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) 
      die("WSAStartup failed!", EXIT_FAILURE);
#endif
	
	if (!(instance = (instance_t *) xmalloc(sizeof(instance_t)))) {
		say("Error to alloc memory - instance_t struct.\n");
		exit(EXIT_FAILURE);
	}
	
	memset(instance, 0, sizeof(instance_t));
	instance->number_of_threads = 0;
	instance->scan_method = 3;
	instance->top_level = NULL;
	instance->dork_list = NULL;
	instance->path_filter = NULL;
	instance->data_filter = NULL;
	instance->search_engine = NULL;
	instance->results_domains = NULL;
	instance->results_full_url = NULL;
	
	if (!(statistics = (statistics_t *) xmalloc(sizeof(statistics_t)))) {
		say("Error to alloc memory - statistics_t struct.\n");
		exit(EXIT_FAILURE);
	}
	statistics->total_sites = 0;
	
#define COPY_ARGS_TO_STRUCT(PTR)\
	if (a+1 < argc)\
		if ((PTR = (unsigned char *) xmalloc(sizeof(unsigned char) * (strlen(argv[a+1])+1))) != NULL) {\
			memset(PTR, '\0', sizeof(unsigned char) * (strlen(argv[a+1])+1));\
			memcpy(PTR, argv[a+1], strlen(argv[a+1])); }
	
	for (int a=0; a<argc; a++) {
		if (strcmp(argv[a], "-l") == 0) { COPY_ARGS_TO_STRUCT(instance->dork_list) }
		if (strcmp(argv[a], "-t") == 0) { if (a+1 < argc) instance->number_of_threads = atoi(argv[a+1]); }
		if (strcmp(argv[a], "-v") == 0) { COPY_ARGS_TO_STRUCT(instance->top_level) }
		if (strcmp(argv[a], "-p") == 0) { COPY_ARGS_TO_STRUCT(instance->path_filter) }
		if (strcmp(argv[a], "-d") == 0) { COPY_ARGS_TO_STRUCT(instance->data_filter) }
		if (strcmp(argv[a], "-c") == 0) { COPY_ARGS_TO_STRUCT(instance->search_engine) }
		if (strcmp(argv[a], "-m") == 0) { if (a+1 < argc) instance->scan_method = atoi(argv[a+1]); }
	}
	
	if (instance->scan_method != 3)
		if (!(instance->scan_method == 1 || instance->scan_method == 2)) {
			banner();
			help();
			say("\n\n    Scan methods found: 1 or 2.\n");
			exit(EXIT_FAILURE);
		}
	
	if (((argc-1) % 2) != 0) {
		banner();
		help();
	} else if (instance->number_of_threads == 0 && instance->dork_list == NULL && instance->top_level == NULL && 
		instance->path_filter == NULL && instance->data_filter == NULL &&  instance->search_engine == NULL &&
		instance->scan_method == 3) {
		banner();
		help();
	} else {
		if (instance->number_of_threads == 0)
			instance->number_of_threads = 1;
		
		if (instance->dork_list == NULL) {
			say("It did not specify the list of dorks.\nRun %s --help to show help menu.\n", argv[0]);
			exit(EXIT_FAILURE);
		}
		
		if (file_exists(instance->dork_list) == FALSE) {
			say("File not exists: %s.\n", instance->dork_list);
			exit(EXIT_FAILURE);
		}
		
		instance->results_domains = (unsigned char *) xmalloc(sizeof(unsigned char) * MAXLIMIT);
		memset(instance->results_domains, '\0', sizeof(unsigned char) * MAXLIMIT);
		sprintf(instance->results_domains, "%s-domains.txt", instance->dork_list);
		
		instance->results_full_url = (unsigned char *) xmalloc(sizeof(unsigned char) * MAXLIMIT);
		memset(instance->results_full_url, '\0', sizeof(unsigned char) * MAXLIMIT);
		sprintf(instance->results_full_url, "%s-full_url.txt", instance->dork_list);
		
		if ((thread = (thread_t *) xmalloc(sizeof(thread_t))) != NULL) {
			thread->counter_a = instance->number_of_threads;
			thread->counter_b = 1;
			thread->counter_c = 0;
		} else {
			say("Error to alloc memory - thread_t struct.\n");
			exit(EXIT_FAILURE);
		}
		
		if (thread->counter_a == 0)
			thread->counter_a = 1;
		
		FILE *handle = NULL;
		unsigned char line [MAXLIMIT];
		memset(line, '\0', MAXLIMIT);

		if ((handle = fopen(instance->dork_list, "r")) != NULL) {
			banner();
			say("\n Starting...\n\n");
			while (fgets(line, MAXLIMIT-1, handle)) {
				for (unsigned int a=0; line[a] != '\0'; a++)
					if (line[a] == '\n') {
						line[a] = '\0';
						break;
					}
				
				if (thread->counter_a) {
					param_t *param = (param_t *) xmalloc(sizeof(param_t)); 
					param->line = (unsigned char *) xmalloc((strlen(line) * sizeof(unsigned char)) + MAXLIMIT);
					memset(param->line, '\0', (strlen(line) * sizeof(unsigned char)) + MAXLIMIT);
					memcpy(param->line, line, strlen(line));
					param->index = thread->counter_c;
					thread->counter_b++;
					thread->counter_a--;
#ifdef WINUSER
					CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) core, (void *) param, 0, 0);
#else 
					pthread_t td;
					pthread_create(&td, NULL, core, (void *) param);
#endif
				}

				while (1) {
					sleep(10);
					if (thread->counter_a) break;
				}
				
				thread->counter_c++;
				memset(line, '\0', MAXLIMIT);
			}
			fclose(handle);
		}

		while (1) {
			sleep(10);
			if (thread->counter_b == 1) break;
		}
	}
	
	instance->number_of_threads = 0;
	instance->scan_method = 0;
	if (instance->results_domains != NULL)
		free(instance->results_domains);
	if (instance->results_full_url != NULL)
		free(instance->results_full_url);
	if (instance->dork_list != NULL)
		free(instance->dork_list);
	if (instance->top_level != NULL)
		free(instance->top_level);
	if (instance->path_filter != NULL)
		free(instance->path_filter);
	if (instance->data_filter != NULL)
		free(instance->data_filter);
	if (instance->search_engine != NULL)
		free(instance->search_engine);
	if (instance)
		free(instance);
	
#ifdef WINUSER
	WSACleanup();
#endif
	
	return 0;
}

static void core (const void *tparam) {
	param_t *param = (param_t *) tparam;
	int one = 0, two = 0, three = 0, four = 0;
	
	if (instance->search_engine != NULL) {
		if (strstr(instance->search_engine, "1")) one = 1;
		if (strstr(instance->search_engine, "2")) two = 1;
		if (strstr(instance->search_engine, "3")) three = 1;
		if (strstr(instance->search_engine, "4")) four = 1;
	}
	
	if (one == 0 && two == 0 && three == 0 && four == 0) {
		one = 1;
		two = 1;
		three = 1;
		four = 1;
	}
	
	if (one == 1) {
		say("  [%d] -> Dorking: %s - Search engine: Bing\n", param->index, param->line);
		bing(param->line);
	}
	
	if (two == 1) {
		say("[%d] -> Dorking: %s - Search engine: Google\n", param->index, param->line);
		google(param->line);
	}
	
	if (three == 1) {
		say("[%d] -> Dorking: %s - Search engine: Hotbot\n", param->index, param->line);
		hotbot(param->line);
	}
	
	if (four == 1) {
		say("[%d] -> Dorking: %s - Search engine: DuckDuckGo\n", param->index, param->line);
		duckduckgo(param->line);
	}
	
	thread->counter_a++;
	thread->counter_b--;
}

static void google (const unsigned char *dork) {
	;
}

static void hotbot (const unsigned char *dork) {
	;
}

static void duckduckgo (const unsigned char *dork) {
	;
}

static void bing (const unsigned char *dork) {
	if (!dork) return;
	
	for (unsigned int page=0; page<1000; page++) {
		int sock = (int) (-1), result = (int) (-1);
		struct sockaddr_in st_addr;
		struct hostent *st_hostent;
		
		if ((st_hostent = gethostbyname("www.bing.com")) == NULL) {
			say("Bing - gethostbyname failed.\n");
			return;
		}
		
		st_addr.sin_family		= AF_INET;
		st_addr.sin_port		= htons(80);
		st_addr.sin_addr.s_addr	= *(u_long *) st_hostent->h_addr_list[0];
		
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			say("Bing - socket failed.\n");
			return;
		}
		
		if ((result = connect(sock, (struct sockaddr *)&st_addr, sizeof(st_addr))) < 0) {
			say("Bing - connect failed.\n");
			close(sock);
			return;
		}
		
		unsigned char header [] = 
			"Host: www.bing.com\r\n"
			"User-Agent: Mozilla/5.0 (Windows NT 6.1; Trident/7.0; SLCC2; "
			".NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; "
			"Media Center PC 6.0; .NET4.0C; .NET4.0E; rv:11.0) like Gecko\r\n"
			"Accept-Encoding: identity\r\n"
			"Connection: Close\r\n\r\n";

		unsigned char *final_dork = (unsigned char *) xmalloc(sizeof(unsigned char) * (MAXLIMIT + strlen(dork)));
		memset(final_dork, '\0', sizeof(unsigned char) * (MAXLIMIT + strlen(dork)));
		memcpy(final_dork, dork, strlen(dork));
		
		for (int a=0; final_dork[a]!='\0'; a++)
			if (final_dork[a] == ' ')
				final_dork[a] = '+';
		
		unsigned char *header_final = (unsigned char *) xmalloc(sizeof(unsigned char) * (strlen(header) + MAXLIMIT));
		memset(header_final, '\0', sizeof(unsigned char) * (strlen(header) + MAXLIMIT));
		sprintf(header_final, "GET /search?q=%s&first=%d1&FORM=PERE HTTP/1.1\r\n%s", final_dork, page, header);
		
		if (send(sock, header_final, strlen(header_final), 0) == -1) {
			say("Bing - send failed.\n");
			free(header_final);
			free(final_dork);
			close(sock);
			return;
		}
		
		result = 0;
		unsigned int is_going = 1;
		unsigned int total_length = 0;
		unsigned char *response = (unsigned char *) xmalloc(sizeof(unsigned char) * (MAXLIMIT*2));
		memset(response, '\0', sizeof(unsigned char) * (MAXLIMIT*2));
		unsigned char *response_final = (unsigned char *) xmalloc(sizeof(unsigned char) * (MAXLIMIT*2));
		memset(response_final, '\0', sizeof(unsigned char) * (MAXLIMIT*2));
		
		while (is_going) {
			result = recv(sock, response, (sizeof(unsigned char) * (MAXLIMIT*2)) - 1, 0);
			if (result == 0 || result < 0)
				is_going = 0;
			else {
				if ((response_final = (unsigned char *) realloc(response_final, total_length + 
					(sizeof(unsigned char) * (MAXLIMIT*2)))) != NULL) {
					memset(&response_final[total_length], '\0', sizeof(unsigned char) * (MAXLIMIT*2));
					memcpy(&response_final[total_length], response, result);
					total_length += result;
				}
			}
		}
		
		close(sock);
		
		unsigned int status_end = FALSE;
		if (total_length > 0 && response_final != NULL)
			if (strlen(response_final) > 0 && strstr(response_final, "class=\"sb_pagN\"")) {
				extract_urls_and_domains(response_final);
			} else status_end = TRUE;
		
		free(header_final);
		free(final_dork);
		free(response);
		free(response_final);
		
		if (status_end == TRUE)
			break;
	}
}

static void extract_urls_and_domains (unsigned char *content) {
	if (!content)
		return;
	
	unsigned char *pointer = content;
	while (1) {
		pointer = strstr(pointer += 7, "://");
		if (!pointer) break;
		
		unsigned int a=0;
		for (a=0; pointer[a]!='\0'; a++)
			if (pointer[a] == 0x22 || pointer[a] == '>' || pointer[a] == ' ') 
				break;
		
		unsigned int protocol = 2;
		if (a>0) {
			pointer -= 1;
			if (pointer[0] == 's' || pointer[0] == 'p') {
				pointer -= 1;
				if (pointer[1] == 'p') {
					pointer -= 2;
					if (pointer[2] == 't' && pointer[1] == 't' && pointer[0] == 'h')
						protocol = 0;
				} else if (pointer[1] == 's') {
					pointer -= 3;
					if (pointer[3] == 'p' && pointer[2] == 't' && pointer[1] == 't' && pointer[0] == 'h')
						protocol = 1;
				}
			}
			
			if (protocol == 0 || protocol == 1) {
				if (protocol == 0)
					a += 4;
				else
					a += 5;
				
				unsigned char *url = (unsigned char *) xmalloc(sizeof(unsigned char) * (a + 1));
				memset(url, '\0', sizeof(unsigned char) * (a + 1));
				memcpy(url, pointer, a);
				
				if (url)
					if (!(strlen(url) == 7 || strlen(url) == 8))
						filter_url(url, protocol);
					
				free(url);
			}
		}
	}
}

static void filter_url (const unsigned char *url, const unsigned int protocol) {
	if (!url)
		return;
	
	if (!strstr(url, "microsofttranslator.com") && 
		!strstr(url, "live.com") &&
		!strstr(url, "microsoft.com") &&
		!strstr(url, "bingj.com") &&
		!strstr(url, "w3.org") &&
		!strstr(url, "wikipedia.org") &&
		!strstr(url, "google.com") &&
		!strstr(url, "yahoo.com") &&
		!strstr(url, "msn.com") &&
		!strstr(url, "facebook.com") &&
		!strstr(url, "bing.net") &&
		!strstr(url, "\n") &&
		!strstr(url, "<") &&
		strstr(url, ".")) 
	{	
		unsigned char *domain = (unsigned char *) xmalloc(sizeof(unsigned char) * strlen(url));
		memset(domain, '\0', sizeof(unsigned char) * strlen(url));
		unsigned char *pointer = strstr(url, "://");
		unsigned int a = 0;
		
		pointer += strlen("://");
		for (a=0; pointer[a]!='\0'; a++)
			if (pointer[a] == '/' || pointer[a] == ' ') {
				a++;
				break;
			}
		
		unsigned int top_level_enabled = FALSE;
		unsigned int tdl_is_valid = FALSE;
		memcpy(domain, pointer, a);
		domain[a-1] = '/';
		
		/* TDL, save-mode, path and data filter. */
#define FILTER_CODE_BLOCK_A\
		if (strstr(domain, temporary)) {\
			domain[strlen(domain) - 1] = '\0';\
			if (domain_exists(domain) == FALSE) {\
				if (instance->scan_method == 1) {\
					if (path_and_data_filter(domain, protocol) == TRUE) {\
						save(instance->results_domains, domain);\
						say("   + %d -> %s\n", statistics->total_sites, domain);\
						statistics->total_sites++;\
					}\
				} else if (instance->scan_method == 2) {\
					if (path_and_data_filter(domain, protocol) == TRUE) {\
						save(instance->results_full_url, url);\
						say("   + %d -> %s\n", statistics->total_sites, domain);\
						statistics->total_sites++;\
					}\
				} else {\
					if (path_and_data_filter(domain, protocol) == TRUE) {\
						save(instance->results_domains, domain);\
						save(instance->results_full_url, url);\
						say("   + %d -> %s\n", statistics->total_sites, domain);\
						statistics->total_sites++;\
					}\
				}\
				statistics->total_sites_all++;\
			}\
		}
		
		if (instance->top_level != NULL) {
			unsigned char temporary [MAXLIMIT];
			if (strstr(instance->top_level, ",")) {
				for (int a=0,b=0; ; a++,b++) {
					if (instance->top_level[a] == ',' || instance->top_level[a] == '\0') {
						temporary[b] = '/';
						temporary[b+1] = '\0';
						b = 0;
						a++;
						FILTER_CODE_BLOCK_A
						if (instance->top_level[a] == '\0')
							break;
					}
					temporary[b] = instance->top_level[a];
				}
				top_level_enabled = TRUE;
			}
		}
		
		if (top_level_enabled == FALSE) {
			unsigned char temporary [MAXLIMIT];
			unsigned int flag_control = FALSE;
			
			if (instance->top_level != NULL) {
				if (!strstr(instance->top_level, ",")) 
					sprintf(temporary, "%s/", instance->top_level);
				else flag_control = TRUE;
			} else flag_control = TRUE;
			if (flag_control == TRUE) 
				sprintf(temporary, "/");
			FILTER_CODE_BLOCK_A
		}
		
		free(domain);
	}
}

static unsigned int path_and_data_filter (const unsigned char *domain, const unsigned int protocol) {
	if (!domain)
		return FALSE;
	
	unsigned int result = FALSE;
	unsigned char path [MAXLIMIT*2];
	unsigned char save_buffer [MAXLIMIT*2];
	
	if (instance->path_filter != NULL) {
		unsigned char temporary [MAXLIMIT];
		if (strstr(instance->path_filter, ",")) {
			for (int a=0,b=0; ; a++,b++) {
				if (instance->path_filter[a] == ',' || instance->path_filter[a] == '\0') {
					
					temporary[b] = '\0';
					b = 0; a++;
					http_request_t *request = http_request_get(domain, temporary);
					
					if (request != NULL) {
						if (request->e_200_OK == TRUE && instance->data_filter == NULL) {
							sprintf(save_buffer, "%s%s%s", (protocol) ? "https://" : "http://",  domain, temporary);
							sprintf(path, "%s-paths.txt", instance->dork_list);
							save(path, save_buffer);
							result = TRUE;
						} 
						else if (request->e_200_OK == TRUE && instance->data_filter != NULL) {
							if (strstr(request->content, instance->data_filter)) {
								sprintf(save_buffer, "%s%s%s", (protocol) ? "https://" : "http://", domain, temporary);
								sprintf(path, "%s-paths.txt", instance->dork_list);
								save(path, save_buffer);
								result = TRUE;
							}
							if (request->content)
								free(request->content);	
						}
						
						free(request);
					}
					
					if (instance->path_filter[a] == '\0')
						break;
				}
				temporary[b] = instance->path_filter[a];
			}
		}
	}
	
	return result;
}

static http_request_t *http_request_get (const unsigned char *domain, const unsigned char *path) {
	if (!domain)
		return (http_request_t *) NULL;
	
	int sock = (int) (-1), result = (int) (-1);
	struct sockaddr_in st_addr;
	struct hostent *st_hostent;
	
	if ((st_hostent = gethostbyname(domain)) == NULL) {
		say("HTTP GET - gethostbyname failed.\n");
		return (http_request_t *) NULL;
	}
	
	st_addr.sin_family		= AF_INET;
	st_addr.sin_port		= htons(80);
	st_addr.sin_addr.s_addr	= *(u_long *) st_hostent->h_addr_list[0];
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		say("HTTP GET - socket failed.\n");
		return (http_request_t *) NULL;
	}
	
	if ((result = connect(sock, (struct sockaddr *)&st_addr, sizeof(st_addr))) < 0) {
		say("HTTP GET - connect failed.\n");
		close(sock);
		return (http_request_t *) NULL;
	}
	
	unsigned char header [] = 
		"User-Agent: Mozilla/5.0 (Windows NT 6.1; Trident/7.0; SLCC2; "
		".NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; "
		"Media Center PC 6.0; .NET4.0C; .NET4.0E; rv:11.0) like Gecko\r\n"
		"Accept-Encoding: identity\r\n"
		"Connection: Close\r\n\r\n";

	unsigned char *header_final = (unsigned char *) xmalloc(sizeof(unsigned char) * (strlen(header) + MAXLIMIT));
	memset(header_final, '\0', sizeof(unsigned char) * (strlen(header) + MAXLIMIT));
	sprintf(header_final, "GET %s HTTP/1.1\r\nHost: %s\r\n%s", path, domain, header);
	
	if (send(sock, header_final, strlen(header_final), 0) == -1) {
		say("HTTP GET - send failed.\n");
		free(header_final);
		close(sock);
		return (http_request_t *) NULL;
	}
	
	result = 0;
	unsigned int is_going = 1;
	unsigned int total_length = 0;
	unsigned char *response = (unsigned char *) xmalloc(sizeof(unsigned char) * (MAXLIMIT*2));
	memset(response, '\0', sizeof(unsigned char) * (MAXLIMIT*2));
	unsigned char *response_final = (unsigned char *) xmalloc(sizeof(unsigned char) * (MAXLIMIT*2));
	memset(response_final, '\0', sizeof(unsigned char) * (MAXLIMIT*2));
	
	while (is_going) {
		result = recv(sock, response, (sizeof(unsigned char) * MAXLIMIT) - 1, 0);
		if (result == 0 || result < 0)
			is_going = 0;
		else {
			if (!strstr(response, "\r\n\r\n") && instance->data_filter == NULL) {
				if ((response_final = (unsigned char *) realloc(response_final, total_length + 
					(sizeof(unsigned char) * MAXLIMIT))) != NULL) {
					memset(&response_final[total_length], '\0', sizeof(unsigned char) * MAXLIMIT);
					memcpy(&response_final[total_length], response, result);
					total_length += result;
				}
			} else if (instance->data_filter != NULL) {
				if ((response_final = (unsigned char *) realloc(response_final, total_length + 
					(sizeof(unsigned char) * MAXLIMIT))) != NULL) {
					memset(&response_final[total_length], '\0', sizeof(unsigned char) * MAXLIMIT);
					memcpy(&response_final[total_length], response, result);
					total_length += result;
				}
			} else if (use_method == 1) {
				if ((response_final = (unsigned char *) realloc(response_final, total_length + 
					(sizeof(unsigned char) * MAXLIMIT))) != NULL) {
					memset(&response_final[total_length], '\0', sizeof(unsigned char) * MAXLIMIT);
					memcpy(&response_final[total_length], response, result);
					total_length += result;
				}
			}
			else 
				break;
		}
	}
	
	close(sock);
	http_request_t *request = (http_request_t *) xmalloc(sizeof(http_request_t));
	if (total_length > 0 && response_final != NULL) {
		if (instance->data_filter != NULL) {
			request->length = total_length;
			request->content = (unsigned char *) xmalloc(sizeof(unsigned char) * (total_length + 1));
			memset(request->content, '\0', sizeof(unsigned char) * (total_length + 1));
			memcpy(request->content, response_final, total_length);
		}
		
		if (strlen(response_final) > 0 && strstr(response_final, "200 OK")) {
			request->e_200_OK = TRUE;
		}
	}
		
	free(header_final);
	free(response);
	free(response_final);
	
	return request;
}

static unsigned int domain_exists (const unsigned char *domain) {
	if (!domain)
		return FALSE;
	FILE *handle = NULL;
	unsigned char line [MAXLIMIT];
	unsigned int flag_exists = FALSE;
	memset(line, '\0', MAXLIMIT);
	if ((handle = fopen(instance->results_domains, "r")) != NULL) {
		while (fgets(line, MAXLIMIT-1, handle)) {
			if (strstr(line, domain)) {
				flag_exists = TRUE;
				break;
			}
			memset(line, '\0', MAXLIMIT);
		}
		fclose(handle);
	}
	if (flag_exists == TRUE)
		return TRUE;
	return FALSE;
}

static void save (const unsigned char *path, const unsigned char *content) {
	if (!path || !content)
		return;
	FILE *handle = NULL;
	if ((handle = fopen(path, "a+")) != NULL) {
		fprintf(handle, "%s\n", content);
		fclose(handle);
	}
}

static unsigned int file_exists (const unsigned char *path) {
	if (!path)
		return FALSE;
	FILE *handle = NULL;
	if ((handle = fopen(path, "r")) != NULL) {
		fclose(handle);
		return TRUE;
	}
	return FALSE;
}

void *xmalloc (const unsigned int size) {
	if (!size)
		return NULL;
	void *pointer = NULL;
	if ((pointer = malloc(size)) != NULL) 
		return pointer;
	return NULL;
}

static void die (const unsigned char *content, const unsigned int exitCode) {
	say("%s", content);
	exit(exitCode);
}

static void banner (void) {
	say("\n\
     __                       __                 __                      \n\
    /\\ \\       __            /\\ \\               /\\ \\                     \n\
    \\ \\ \\     /\\_\\    ____   \\_\\ \\    ___   _ __\\ \\ \\/'\\      __   _ __  \n\
     \\ \\ \\  __\\/\\ \\  /',__\\  /'_` \\  / __`\\/\\\\`'__\\ \\ , <   /'__`\\/\\`'__\\\n\
      \\ \\ \\L\\ \\\\ \\ \\/\\__, `\\/\\ \\L\\ \\/\\ \\L\\ \\ \\ \\/ \\ \\ \\\\`\\ /\\  __/\\ \\ \\/ \n\
       \\ \\____/ \\ \\_\\/\\____/\\ \\___,_\\ \\____/\\ \\_\\  \\ \\_\\ \\_\\ \\____\\\\ \\_\\ \n\
        \\/___/   \\/_/\\/___/  \\/__,_ /\\/___/  \\/_/   \\/_/\\/_/\\/____/ \\/_/ \n\
                                                                     \n\
                    Coded by Constantine - Greatz for L1sbeth\n\
                        My GitHub: github.com/jessesilva\n\
                        P0cL4bs Team: github.com/P0cL4bs\n\n");
}

static void help (void) {
	say("\n  -l  -  List of dorks.\n\
  -t  -  Number of threads (Default: 1).\n\
  -v  -  Top-level filter.\n\
  -p  -  Path to search (Default: 200 OK HTTP error).\n\
      '-> Output path file: DORK_FILE_NAME-paths.txt\n\
  -d  -  Data to search.\n\
  -c  -  Search engine (Default: all).\n\
      |-> 1: Bing\n\
      |-> 2: Google - NOT IMPLEMENTED\n\
      |-> 3: Hotbot - NOT IMPLEMENTED\n\
      '-> 4: DuckDuckGo - NOT IMPLEMENTED\n\
  -m  -  Save method (Default: all).\n\
      |-> 1: Extract domains (DORK_FILE_NAME-domains.txt).\n\
      '-> 2: Extract full URL (DORK_FILE_NAME-full_url.txt).\n\n\
  Examples...\n\
    dorker -l dorks.txt -t 30 -m 1 -v .com.br\n\
    dorker -l dorks.txt -t 30 -m 2 -v .gov\n\
    dorker -l dorks.txt  -t 30 -v .gov,.com,.com.br\n\
    dorker -l dorks-bing.txt -t 30 -m 1 -c 1 -v .br\n\
              -p /wp-login.php,/wp-admin.php,/wordpress/wp-login.php -d \"wp-submit\"\n\
    dorker -l dorks-all.txt -t 100 -c 12\n");
}
