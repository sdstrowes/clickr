/**
 * This is my own simple upload tool for Flickr. It uploads one file,
 * as specified on the command line, to my photostream. It requires an
 * API key and a shared secret, which can be gotten from Flickr.
 * https://www.flickr.com/services/api/keys/
 *
 * Stephen D. Strowes, sdstrowes@gmail.com
 *
 * Released under a two-clause BSD license; see LICENSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include <getopt.h>
#include <openssl/md5.h>

#include <curl/curl.h>
#include <libconfig.h>
#include <pwd.h>

#define DIGEST_BUFFER_SIZE 129

const char* default_cacert_file = "/usr/lib/ssl/certs/ca-certificates.crt";

char* secret = NULL;
char* api_key = NULL;
char* auth_token = NULL;
char* cacert_file = NULL;

/* read_config:
 * Reads configuration file from ~/.clickr */
int read_config()
{
	config_t cfg;
	char* config_file = ".clickr";
	const char* api_key_temp, *secret_temp, *auth_token_temp, *cacert_file_temp;

	/* Determine home directory for current user, and construct the
	 * path to the configuration file name. */
	int uid = getuid();
	struct passwd* tmp = getpwuid(uid);
	/* The +1's here indicate the trailing slash on the path, and the
	 * null terminator. */
	int path_length = strlen(tmp->pw_dir) + 1 + strlen(config_file) + 1;
	char* config_path = (char*)malloc(path_length);
	memset(config_path, '\0', path_length);
	strcat(config_path, tmp->pw_dir);
	strcat(config_path, "/");
	strcat(config_path, config_file);

	config_init(&cfg);
 
	if (!config_read_file(&cfg, config_path)) {
		fprintf(stderr, "%d - %s\n", 
		        config_error_line(&cfg),
		        config_error_text(&cfg));
		config_destroy(&cfg);
		free(config_path);
		return 1;
	}
	free(config_path);

	/* api_key */
	if (config_lookup_string(&cfg, "api_key", &api_key_temp)) {
		api_key = (char*)malloc(strlen(api_key_temp) +1);
		strcpy(api_key, api_key_temp);
	}
	else {
		config_destroy(&cfg);
		return 2;
	}

	/* secret */
	if (config_lookup_string(&cfg, "secret", &secret_temp)) {
		secret = (char*)malloc(strlen(secret_temp) +1);
		strcpy(secret, secret_temp);
	}
	else {
		config_destroy(&cfg);
		return 2;
	}

	/* auth_token */
	if (config_lookup_string(&cfg, "auth_token", &auth_token_temp)) {
		auth_token = (char*)malloc(strlen(auth_token_temp) +1);
		strcpy(auth_token, auth_token_temp);
	}
	else {
		config_destroy(&cfg);
		return 3;
	}

	/* cacert_file */
	if (cacert_file == NULL) {
		if (config_lookup_string(&cfg, "cacert", &cacert_file_temp)) {
			cacert_file = (char*)malloc(strlen(cacert_file_temp)+1);
			strcpy(cacert_file, cacert_file_temp);
		}
		else {
			config_destroy(&cfg);
			return 4;
		}
	}
 
	config_destroy(&cfg);

	return 0;
}

/* write_config:
 * Writes the configuration file to ~/.clickr, using the
 * already-known API key and the shared secret, the final parameter
 * being 'auth_token' */
int write_config(char* auth_token)
{
	config_t cfg;
	config_setting_t* root;
	config_setting_t* setting;
	char* config_file = ".clickr";

	/* Determine home directory for current user, and construct the
	 * path to the configuration file name. */
	int uid = getuid();
	struct passwd* tmp = getpwuid(uid);
	/* The +1's here indicate the trailing slash on the path, and the
	 * null terminator. */
	int path_length = strlen(tmp->pw_dir) + 1 + strlen(config_file) + 1;
	char* config_path = (char*)malloc(path_length);
	memset(config_path, '\0', path_length);
	strcat(config_path, tmp->pw_dir);
	strcat(config_path, "/");
	strcat(config_path, config_file);

	config_init(&cfg);
	root = config_root_setting(&cfg);

	/* The three tokens we need to store. */
	setting = config_setting_add(root, "auth_token", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, auth_token);
	setting = config_setting_add(root, "api_key", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, api_key);
	setting = config_setting_add(root, "secret", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, secret);

	/* The location of the CA cert bundle */
	setting = config_setting_add(root, "cacert", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, cacert_file);

	/* Write the file! */
	if (!config_write_file(&cfg, config_path)) {
		fprintf(stderr, "%d - %s\n", 
		        config_error_line(&cfg),
		        config_error_text(&cfg));
	}

	/* Clean up */
	free(config_path);
	config_destroy(&cfg);
}

/* sprint_hex:
 * print a long hex value into a string buffer. This function will
 * write 'len' bytes of data from 'char_buf' into 2*len bytes of space
 * in 'output'. (Each hex value is 4 bytes.) Space in output must be
 * pre-allocated. */
void sprint_hex(char* output, unsigned char* char_buf, int len)
{
	int i;
	for (i=0; i < len; i++) {
		sprintf(output + i*2, "%02x", char_buf[i]);
	}
}

/* md5sum:
 * Concatenates a series of strings together then generates an md5
 * message digest. Writes the result directly into 'output', which
 * MUST be large enough to accept the result. (Size ==
 * 128 + null terminator.)
 */
void md5sum(char* output, int counter, ...)
{
	va_list argp;
	unsigned char temp[MD5_DIGEST_LENGTH];
	char* md5_string;
	int full_len;
	int i, str_len;
	int buffer_size = 10;

	md5_string = (char*)malloc(buffer_size);
	md5_string[0] = '\0';

	full_len = 0;
	va_start(argp, secret);
	for (i = 0; i < counter; i++) {
		char* string = va_arg(argp, char*);
		int len = strlen(string);

		/* If the buffer is not large enough, expand until it is. */
		while (len + full_len > buffer_size - 1) {
			buffer_size += buffer_size;
			md5_string = realloc(md5_string, buffer_size);
		}

		strncat(md5_string, string, len);

		full_len += len;
		md5_string[full_len] = '\0';
	}
	va_end(argp);

	str_len = strlen(md5_string);
	MD5((const unsigned char*)md5_string, (unsigned int)str_len, temp);

	free(md5_string);

	sprint_hex(output, temp, MD5_DIGEST_LENGTH);
	output[129] = '\0';
}

/* print_usage:
 * Prints basic help for clickr. */
void print_usage(char* argv[])
{
	printf("%s is a command line tool to upload photos to Flickr\n", argv[0]);
	printf("Questions: Stephen D. Strowes <sdstrowes@gmail.com>\n");
	printf("\n");
	printf("There is a two-step process to getting going.\n");
	printf("\n");
	printf("%s needs to authorise with your flickr account to be able to work. To\n", argv[0]);
	printf("bootstrap, you need to an API key and a shared secret from\n");
	printf("https://www.flickr.com/services/api/keys/\n");
	printf("They should be put into ~/.clickr in the format:\n");
	printf("\n");
	printf(" api_key = \"your key\"\n");
	printf(" secret = \"your secret\"\n");
	printf("\n");
	printf("From there, you can run:\n");
	printf("\t%s -a -c /path/to/ca-certificates.crt\n", argv[0]);
	printf("If -c is not specified, %s will assume /usr/lib/ssl/certs/ca-certificates.crt\n", argv[0]);
	printf("If that's successful, %s will drop the additional creds into ~/.clickr, and\n", argv[0]);
	printf("then you're good to go.\n");
	printf("\n");
	printf("More generally, you'd run as:\n");
	printf("\t%s -f filename [-t title] [-d description]\n", argv[0]);

	printf("Options:");
	printf(" -a:\tThe auth step mentioned above.\n");
	printf(" -c:\tOverride the default location for the CA certs bundle.\n");
	printf(" -d:\tA description of the image being uploaded.\n");
	printf(" -f:\tLocation of the file to upload.\n");
	printf(" -h:\tThis help information.\n");
	printf(" -l:\tTags for the image.\n");
	printf(" -t:\tTitle for the image.\n");
	printf("\n");
}

/* handle_auth_token_response:
 * Response handler for the response to the flickr.auth.getToken API
 * call. Parses the token from the response, and writes the
 * information into the configuration file. */
size_t handle_auth_token_response(void *buffer, size_t size, size_t nmemb, void *userp)
{
	char* auth_token, *end;

	auth_token = strstr(buffer, "<token>");
	if (auth_token == NULL) {
		fprintf(stderr, "Error response!\n");
		fprintf(stderr, "%s\n", buffer);
		return -1;
	}
	auth_token += 7;

	end = strstr(buffer, "</token>");
	if (end == NULL) {
		fprintf(stderr, "Malformed response!\n");
		fprintf(stderr, "%s\n", buffer);
		return -1;
	}
	*end = '\0';

	printf("Token retrieved! %s\n", auth_token);
	printf("Writing to disk... ");
	fflush(stdout);
	write_config(auth_token);
	printf("Done!\n");
}


/* handle_getfrob_response:
 * Response handler for the response to the flickr.auth.getFrob API
 * call. Parses the frob token from the response, points the user to
 * the URL she should access, then dispatches the flickr.auth.getToken
 * API call. */
size_t handle_getfrob_response(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CURL *curl;
	CURLcode res;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;

	char md5[DIGEST_BUFFER_SIZE];
	char* frob, *end, *frob_string;

	/* Locate the start of the frob token. Then locate the start of
	 * the closing tag, and insert a null terminator to end the
	 * string. If either of these fail, something is very wrong
	 * indeed. */
	frob = strstr(buffer, "<frob>");
	if (frob == NULL) {
		fprintf(stderr, "Error response!\n");
		fprintf(stderr, "%s\n", buffer);
		return -1;
	}
	frob += 6;

	end  = strstr(frob, "</frob>");
	if (end == NULL) {
		fprintf(stderr, "Malformed response!\n");
		fprintf(stderr, "%s\n", buffer);
		return -1;
	}
	*end = '\0';


	/* Generate the md5sum */
	md5sum(md5, 6, secret, "api_key", api_key, "frob", frob, "permswrite");

	/* Tell the user the URL to follow to allow the auth token to be
	 * generated. */
	printf("Follow auth URL:\n");
	printf("https://flickr.com/services/auth/?api_key=%s&frob=%s&perms=write&api_sig=%s\n",
	       api_key, frob, md5);

	printf("Have you authorised? [y] ");
	fflush(stdout);
	int rt;
	do {
		rt = getchar();
		if (rt == EOF) {
			exit(1);
		}
	} while (rt != 'Y' && rt != 'y');


	printf("Now generating auth_token.\n");

	/* Construct the auth.getToken request */
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(curl) {
		int frag1_len, frag2_len, frag3_len, api_key_len, frob_len, md5_len, len;
		char* frag1 = "method=flickr.auth.getToken&api_key=";
		char* frag2 = "&frob=";
		char* frag3 = "&api_sig=";
		char* options;

		frag1_len =   strlen(frag1);
		api_key_len = strlen(api_key);
		frag2_len =   strlen(frag2);
		frob_len =    strlen(frob);
		frag3_len =   strlen(frag3);
		md5_len =     DIGEST_BUFFER_SIZE;

		len = frag1_len + api_key_len + frag2_len + frob_len + frag3_len + md5_len;
		options = (char*)malloc(len + 1);
		memset(options, '\0', len + 1);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
		curl_easy_setopt(curl, CURLOPT_CAINFO, cacert_file);

		/* Set Host to target in HTTP header, and set response handler
		 * function */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_auth_token_response);
		curl_easy_setopt(curl, CURLOPT_URL, "https://api.flickr.com/services/rest/");

		/* Generate md5sum for this request */
		md5sum(md5, 6, secret, "api_key", api_key, "frob", frob, "methodflickr.auth.getToken");

		/* Build the query */
		strncat(options, frag1,   frag1_len);
		strncat(options, api_key, api_key_len);
		strncat(options, frag2,   frag2_len);
		strncat(options, frob,    frob_len);
		strncat(options, frag3,   frag3_len);
		strncat(options, md5,     md5_len);

		/* Set the POST fields, and fire. */
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, options);
		res = curl_easy_perform(curl);

		/* Done. Cleanup. */ 
		free(options);
		curl_easy_cleanup(curl);
		curl_formfree(formpost);
	}
}


/* authorise_client:
 * Commences the authorisation process by dispatching a
 * flickr.auth.getFrob API call. */
void authorise_client()
{
	CURL *curl;
	CURLcode rt;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	char md5[DIGEST_BUFFER_SIZE];
 
	curl_global_init(CURL_GLOBAL_ALL);
 
	curl = curl_easy_init();
	if(curl) {
		int frag1_len, frag2_len, api_key_len, md5_len, len;
		char* frag1 = "method=flickr.auth.getFrob&api_key=";
		char* frag2 = "&api_sig=";
		char* options;

		frag1_len =   strlen(frag1);
		api_key_len = strlen(api_key);
		frag2_len =   strlen(frag2);
		md5_len =     DIGEST_BUFFER_SIZE;

		len = frag1_len + api_key_len + frag2_len + md5_len;
		options = (char*)malloc(len + 1);
		memset(options, '\0', len + 1);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
		curl_easy_setopt(curl, CURLOPT_CAINFO, cacert_file);

		/* Set Host to target in HTTP header, and set response handler
		 * function */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_getfrob_response);
		curl_easy_setopt(curl, CURLOPT_URL, "https://api.flickr.com/services/rest/");

		/* Generate md5sum for this request */
		md5sum(md5, 4, secret, "api_key", api_key, "methodflickr.auth.getFrob");

		/* Build the query */
		strncat(options, frag1,   frag1_len);
		strncat(options, api_key, api_key_len);
		strncat(options, frag2,   frag2_len);
		strncat(options, md5,     md5_len);

		printf("%s \n", options);

		/* Set the POST fields, and fire. */
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, options);
		rt = curl_easy_perform(curl);

		if (!rt) {
			fprintf(stderr, "An error occurred when uploading!\n");
		}

		/* Done. Cleanup. */
		free(options);
		curl_easy_cleanup(curl);
		curl_formfree(formpost);
	}
}

/* upload_photo:
 * Quite simply, uploads a photo with the given parameters. */
void upload_photo(char* auth_token, char* filename, char* title, char* description, char* tags)
{
	CURL *curl;
	CURLcode rt;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	char md5[DIGEST_BUFFER_SIZE];

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
		curl_easy_setopt(curl, CURLOPT_CAINFO, cacert_file);

		/* Set Host to target in HTTP header, and set response handler
		 * function */
		curl_easy_setopt(curl, CURLOPT_URL, "https://up.flickr.com/services/upload/");
		/* curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); */

		md5sum(md5, 11, secret, "api_key", api_key, "auth_token", auth_token, "description", description, "tags", tags, "title", title);

		/* Build the form post */
		curl_formadd(&formpost, &lastptr,
		             CURLFORM_COPYNAME, "api_key",
		             CURLFORM_COPYCONTENTS, api_key, CURLFORM_END);

		curl_formadd(&formpost, &lastptr,
		             CURLFORM_COPYNAME, "auth_token",
		             CURLFORM_COPYCONTENTS, auth_token, CURLFORM_END);

		curl_formadd(&formpost, &lastptr,
		             CURLFORM_COPYNAME, "api_sig",
		             CURLFORM_COPYCONTENTS, md5, CURLFORM_END);

		curl_formadd(&formpost, &lastptr,
		             CURLFORM_COPYNAME, "description",
		             CURLFORM_COPYCONTENTS, description, CURLFORM_END);

		curl_formadd(&formpost, &lastptr,
		             CURLFORM_COPYNAME, "title",
		             CURLFORM_COPYCONTENTS, title, CURLFORM_END);

		curl_formadd(&formpost, &lastptr,
		             CURLFORM_COPYNAME, "tags",
		             CURLFORM_COPYCONTENTS, tags, CURLFORM_END);

		curl_formadd(&formpost, &lastptr,
		             CURLFORM_COPYNAME, "photo",
		             CURLFORM_FILE, filename, CURLFORM_END);

		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

		printf("Uploading...\n");
		rt = curl_easy_perform(curl);
		if (rt) {
			fprintf(stderr, "An error occurred during upload: %s\n", curl_easy_strerror(rt));
		}

		/* Done. Cleanup. */ 
		curl_easy_cleanup(curl);
		curl_formfree(formpost);
	}
}


int main(int argc, char* argv[])
{
	char opt;
	int rt;
	int do_auth = 0;
	char* filename    = NULL;
	char* title       = NULL;
	char* description = NULL;
	char* tags        = NULL;

	/* Pull in configuration data */
	rt = read_config();
	switch (rt) {
	case 0:
		break;
	case 1:
		print_usage(argv);
		exit(1);
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	}

	/* Parse command-line options. */
	while ((opt = getopt(argc, argv, "af:d:t:l:c:h")) != -1) {
		switch (opt) {
		case 'a':
			do_auth = 1;
			break;
		case 'f':
			filename = (char*)malloc(strlen(optarg)+1);
			strcpy(filename, optarg);
			break;
		case 't':
			title = (char*)malloc(strlen(optarg)+1);
			strcpy(title, optarg);
			break;
		case 'd':
			description = (char*)malloc(strlen(optarg)+1);
			strcpy(description, optarg);
			break;
		case 'l':
			tags = (char*)malloc(strlen(optarg)+1);
			strcpy(tags, optarg);
			break;
		case 'c':
			cacert_file = (char*)malloc(strlen(optarg)+1);
			strcpy(cacert_file, optarg);
			break;
		case 'h':
			print_usage(argv);
			exit(0);
		}
	}

	if (do_auth) {
		if (cacert_file == NULL) {
			printf("I'm going to assume %s for your CA certs bundle.\n", default_cacert_file);
			printf("This will be saved into your config.\n");
			printf("If this is incorrect, you can specify an alternative with the -c flag.\n");
		}
		authorise_client();
		exit(0);
	}


	if (auth_token == NULL) {
		printf("To use clickr, you need an api_key and a shared secret from Flickr.\n");
		printf("See: https://www.flickr.com/services/apps/create/apply\n");
		exit(1);
	}

	if (filename == NULL) {
		print_usage(argv);
		exit(0);
	}

	upload_photo(auth_token, filename,
	             (title == NULL)? filename : title,
	             (description == NULL)? filename : description,
	             (tags == NULL)? "" : tags);

	/*sleep(10);*/
	/*exit(1);*/
}
