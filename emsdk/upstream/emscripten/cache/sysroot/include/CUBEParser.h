#ifndef CUBEPARSER_H   /* Include guard */
#define CUBEPARSER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/* Call CUBEParser passing as arguments:
   - const char pointer to the path or buffer of the CUBE file
   - bool mode, 1 if the previous argument is a path to the file, 0 if is buffer
   - char array of size 100 for the title or NULL if isn't needed
   - int32_t value for size
   - double pointer to a NULL pointer for the data table

   It returns:
	0 if everything is OK
   -1 if there isn't the data table
   -2 if isn't a CUBE file
   -3 if the input file hasn't been found

	In the source code remember to free memory of data table after usage, if
	buffer is passed as argument it will be modified and remember that
	it has to be free.

	Written by Michele Renzullo
*/

int32_t CUBEParser(char* const, int32_t, char*, int32_t*, double**);
char* get_file_contents_(const char*);

char* get_file_contents_(const char* filename) {
	FILE* fp = fopen(filename, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		size_t bytes = ftell(fp);
		rewind(fp);
		char* contents = (char*)malloc(bytes + 1);
		if (contents) {
			fread(contents, 1, bytes, fp);
			contents[bytes] = '\0';
		}
		fclose(fp);
		return contents;
	}
	return NULL;
}

//C implementation of substr
char* substr(const char* string, size_t length) {
	char* substring = (char*)malloc(length + 1);
	if (substring) {
		memcpy(substring, string, length);
		substring[length] = '\0';
		return substring;
	}
	return NULL;
};

//C implementation of find_last_of
size_t find_last_of(const char* path, const char* symbols) {

	const char* last = NULL;
	size_t result;
	for (const char* symbol = NULL; *symbols;) {
		symbol = strrchr(path, *symbols++);
		if (symbol > last) last = symbol;
	}
	if (!last) result = -1; //not found
	else result = (last - path); //return index 
	return result;
}


int32_t CUBEParser(char* const file, int32_t mode, char* title, int32_t* size, double** data) {
	int32_t result;
	char* const text_ = (mode) ? get_file_contents_(file) : file;

	//if file has been read correctly
	if (text_) {
		char* found_ = NULL;
		if ((found_ = strstr(text_, "_SIZE")) != NULL) {
			int32_t input_size = atoi(found_ + 5);
			*size = input_size;

			char* search_start = text_;
			while (*search_start) if (*search_start++ == '\n' && (*search_start == '-' || (*search_start <= '9' && *search_start >= '0'))) break;
			if (*search_start) {

				double* samples = (double*)malloc(sizeof(double) * (size_t)pow(input_size, 3) * 3);
				for (uint32_t idx = 0; idx < pow(input_size, 3) * 3;) samples[idx++] = strtod(search_start++, &search_start);

				*data = samples;

				//if title is requested
				if (title) {
					char* title_ = NULL;
					if ((found_ = strstr(text_, "TITLE")) != NULL) {
						if (*(found_ + 6) == '"')   title_ = substr(found_ + 7, (size_t)strstr(found_ + 7, "\"") - ((size_t)found_ + 7));
						else if (*(found_ + 6) == '\'') title_ = substr(found_ + 7, (size_t)strstr(found_ + 7, "'") - ((size_t)found_ + 7));

					}
					else if (mode) title_ = substr(file + find_last_of(file, "/\\") + 1, find_last_of(file, ".") - (find_last_of(file, "/\\") + 1));

					//if title has been found and parsed correctly
					if (title_) {
						//trim title if >99 chars
						if (strlen(title_) > 99) {
							memcpy(title, title_, 99);
							title[99] = '\0';
						}
						else strcpy(title, title_);

						free(title_);
					}
				}

				result = 0;
			}
			else result = -1;
		}
		else result = -2;

		if (mode) free(text_); //free mem only if it has been allocated internally
	}

	else result = -3;

	return result;

}

#endif