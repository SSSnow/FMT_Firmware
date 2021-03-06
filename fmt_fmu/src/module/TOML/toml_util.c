/******************************************************************************
 * Copyright 2020 The Firmament Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include <firmament.h>

#include "module/toml/toml.h"

static void print_table(toml_table_t* curtab);

static void print_escape_string(const char* s)
{
	for ( ; *s; s++) {
		int ch = *s;
		switch (ch) {
		case '\b': console_printf("\\b"); break;
		case '\t': console_printf("\\t"); break;
		case '\n': console_printf("\\n"); break;
		case '\f': console_printf("\\f"); break;
		case '\r': console_printf("\\r"); break;
		case '"': console_printf("\\\""); break;
		case '\\': console_printf("\\\\"); break;
		default: console_printf("%c", ch); break;
		}
	}
}

static void print_raw(const char* s)
{
	char* sval;
	int64_t ival;
	int bval;
	double dval;
	toml_timestamp_t ts;

	if (0 == toml_rtos(s, &sval)) {
		console_printf("\"");
		print_escape_string(sval);
		console_printf("\"");
		rt_free(sval);
	} else if (0 == toml_rtoi(s, &ival)) {
		console_printf("%lld", ival);
	} else if (0 == toml_rtob(s, &bval)) {
		console_printf("%s", bval ? "true" : "false");
	} else if (0 == toml_rtod(s, &dval)) {
		console_printf("%lf", dval);
	} else if (0 == toml_rtots(s, &ts)) {
		char millisec[10];
		if (ts.millisec)
			sprintf(millisec, ".%d", *ts.millisec);
		else
			millisec[0] = 0;
		if (ts.year && ts.hour) {
			console_printf("%04d-%02d-%02dT%02d:%02d:%02d%s%s",
				   *ts.year, *ts.month, *ts.day, *ts.hour, *ts.minute, *ts.second,
				   millisec,
				   (ts.z ? ts.z : ""));
		} else if (ts.year) {
			console_printf("%04d-%02d-%02d",
				   *ts.year, *ts.month, *ts.day);
		} else if (ts.hour) {
			console_printf("%02d:%02d:%02d%s",
				   *ts.hour, *ts.minute, *ts.second, millisec);
		}
	} else {
		console_printf("unknown type");
	}
}

static void print_table_array(toml_array_t* curarr)
{
	int i;
	toml_table_t* tab;

	for (i = 0; 0 != (tab = toml_table_at(curarr, i)); i++) {
		print_table(tab);
	}
}

static void print_array(toml_array_t* curarr)
{
	toml_array_t* arr;
	const char* raw;
	int i;

	if (toml_array_kind(curarr) == 't') {
        console_printf("[");
		print_table_array(curarr);
        console_printf("]");
		return;
	} 

	switch (toml_array_kind(curarr)) {

	case 'v': 
		for (i = 0; 0 != (raw = toml_raw_at(curarr, i)); i++) {
			print_raw(raw);
            console_printf(",");
		}
		break;

	case 'a': 
		for (i = 0; 0 != (arr = toml_array_at(curarr, i)); i++) {
            console_printf("[");
			print_array(arr);
            console_printf("],");
		}
		break;

	default:
		break;
	}
}

static void print_table(toml_table_t* curtab)
{
	int i;
	const char* key;
	const char* raw;
	toml_array_t* arr;
	toml_table_t* tab;

    console_printf("[%s]{\n", toml_table_key(curtab));

    /* traverse keys in table */
	for (i = 0; 0 != (key = toml_key_in(curtab, i)); i++) {
	
		if (0 != (raw = toml_raw_in(curtab, key))) {
            console_printf("%s = ", key);
			print_raw(raw);
            console_printf("\n");
		} else if (0 != (arr = toml_array_in(curtab, key))) {
            console_printf("%s = [", key);
			print_array(arr);
            console_printf("]\n");
		} else if (0 != (tab = toml_table_in(curtab, key))) {
			print_table(tab);
		} else {
			//some error here
            console_printf("ERROR: toml parse fail\n");
		}
	}
	console_printf("}\n");
}

int toml_print_parsed_data(const char* file)
{
    char  errbuf[200];
    FILE* fp = fopen(file, "r");

    if (!fp) {
        console_printf("ERROR: cannot open toml file: %s\n", file);
        return -1;
    }

    toml_table_t* root_tab = toml_parse_file(fp, errbuf, sizeof(errbuf));

    print_table(root_tab);
    toml_free(root_tab);
    fclose(fp);

    return 0;
}

toml_table_t* toml_parse_config_file(const char* file)
{
    char  errbuf[200];
    FILE* fp = fopen(file, "r");
    toml_table_t* root_tab;

    if (!fp) {
        console_printf("TOML: fail to open file: %s\n", file);
        return NULL;
    }

    root_tab = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if(root_tab == NULL){
        console_printf("TOML: %s\n", errbuf);
    }

    return root_tab;
}