#include <stdio.h>
#include "cJSON.h"

static void test_parse_cjson() {
	char *my_json_string =
		"{"
		"\"name\": \"Jack Bee Nimble\","
		"\"format\" : {"
		"\"type\": \"rect\","
		"\"width\" : 1920,"
		"\"height\" : 1080,"
		"\"interlace\" : false,"
		"\"frame rate\" : 24"
		"}"
		"}";

	cJSON *root = cJSON_Parse(my_json_string);
	cJSON *format = cJSON_GetObjectItem(root, "format");
	int framerate = cJSON_GetObjectItem(format, "frame rate")->valueint;
	printf("framerate: %d\n", framerate);

	/* You need to set BOTH valueint and valuedouble. -- Dave Gamble, the author*/
	cJSON_GetObjectItem(format, "frame rate")->valueint = 25;
	cJSON_GetObjectItem(format, "frame rate")->valuedouble = 25;
	framerate = cJSON_GetObjectItem(format, "frame rate")->valueint;
	printf("framerate: %d\n", framerate);

	char *rendered = cJSON_Print(root);
	printf("rendered: %s\n", rendered);

	cJSON_Delete(root);
}

static void test_build_json() {
	cJSON *root, *fmt;
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
	cJSON_AddItemToObject(root, "format", fmt = cJSON_CreateObject());
	cJSON_AddStringToObject(fmt, "type", "rect");
	cJSON_AddNumberToObject(fmt, "width", 1920);
	cJSON_AddNumberToObject(fmt, "height", 1080);
	cJSON_AddFalseToObject(fmt, "interlace");
	cJSON_AddNumberToObject(fmt, "frame rate", 26);

	char *rendered = cJSON_Print(root);
	printf("rendered: %s\n", rendered);

	cJSON_Delete(root);
}