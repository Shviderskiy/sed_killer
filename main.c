#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define _MAX_PATH_LEN 256
#define _MAX_LINE_LEN 2048



void handle_error(char const *message) {

	fprintf(stderr, "Error: %s\n", message);
	fprintf(stdout, "Usage: sed_killer.exe "
		"-f=FIND_STR -r=REPL_STR\n"
		"\t[-b=BEG_LENE, -e=END_LINE, -i] "
		"[INPUT_F_NAME, OUTPUT_F_NAME]\n");
	system("pause");
	exit(1);
}



// command line options
struct cl_options {

	char in_file_name[_MAX_PATH_LEN];
	char out_file_name[_MAX_PATH_LEN];

	int32_t beg_line;
	int32_t end_line;

	char const *find_str;
	char const *repl_str;

	bool ignore_case;

	bool is_beg_line_set;
	bool is_end_line_set;
};



void init_clo(struct cl_options *pclo) {

	memset(pclo->in_file_name, '\0', _MAX_PATH_LEN);
	memset(pclo->out_file_name, '\0', _MAX_PATH_LEN);
	pclo->beg_line = 0;
	pclo->end_line = INT32_MAX;
	pclo->find_str = NULL;
	pclo->repl_str = NULL;
	pclo->ignore_case = false;

	pclo->is_beg_line_set = false;
	pclo->is_end_line_set = false;
}



void parse_clo(
	char const *const *argv, struct cl_options *pclo)
{
	argv++;

	while (*argv != NULL) {

		if (memcmp(*argv, "-f=", 3) == 0) {

			if (pclo->find_str != NULL)
				handle_error(
				"The find string is already set");

			pclo->find_str = *argv + 3;

			if (*pclo->find_str == '\0')
				handle_error(
				"Wrong find string");

			argv++;
			continue;
		}

		if (memcmp(*argv, "-r=", 3) == 0) {

			if (pclo->repl_str != NULL)
				handle_error(
				"The string to be replaced is already set");

			pclo->repl_str = *argv + 3;

			argv++;
			continue;
		}

		if (memcmp(*argv, "-i", 2) == 0) {

			if (pclo->ignore_case)
				handle_error(
				"The '-i' option is already set");

			pclo->ignore_case = true;
			argv++;
			continue;
		}

		if (memcmp(*argv, "-b=", 3) == 0) {

			if (pclo->is_beg_line_set)
				handle_error(
				"The '-b' option is already set");

			if (sscanf_s(*argv + 3, "%d", &pclo->beg_line) <= 0
				|| pclo->beg_line < 0)
				handle_error(
				"Wrong '-b' option (Expected positive number)");

			pclo->is_beg_line_set = true;
			argv++;
			continue;
		}

		if (memcmp(*argv, "-e=", 3) == 0) {

			if (pclo->is_end_line_set)
				handle_error(
				"The '-e' option is already set");

			if (sscanf_s(*argv + 3, "%d", &pclo->end_line) <= 0
				|| pclo->end_line < 0)
				handle_error(
				"Wrong '-e' option (Expected positive number)");

			pclo->is_end_line_set = true;
			argv++;
			continue;
		}

		break;
	}


	if (pclo->find_str == NULL)
		handle_error(
		"The key '-f' must be specified");

	if (pclo->repl_str == NULL)
		handle_error(
		"The key '-r' must be specified");

	if (pclo->beg_line >= pclo->end_line)
		handle_error(
		"'-b' value must be < '-e' value");


	if (*argv == NULL) return;

	// memcpy
	if (strncpy_s( // безопасный аналог функции strcpy
		/*  */pclo->in_file_name, _MAX_PATH_LEN,
		/*  */*argv, _MAX_PATH_LEN
		) != 0)
		handle_error(
		"Wrong input file name");

	argv++;


	if (*argv == NULL) return;

	if (strncpy_s(
		/*  */pclo->out_file_name, _MAX_PATH_LEN,
		/*  */*argv, _MAX_PATH_LEN
		) != 0)
		handle_error(
		"Wrong output file name");

	argv++;


	if (*argv != NULL)
		handle_error(
		"The name of the output file must be last argument");

}



void dump_clo(struct cl_options const *pclo) {

	printf("in_f_name = %s\n", pclo->in_file_name);
	printf("out_f_name = %s\n",
		*pclo->out_file_name ? pclo->out_file_name : "stdout");
	printf("find_str = %s\n", pclo->find_str);
	printf("repl_str = %s\n", pclo->repl_str);
	printf("beg_line = %d\n", pclo->beg_line);
	printf("end_line = %d\n", pclo->end_line);
	printf("ignore_case = %d\n", pclo->ignore_case);
}



char const* find_substr(
	char const *str, char const *substr, bool ignore_case)
{
	if (str == NULL) return NULL;
	if (substr == NULL) return NULL;

	if (*substr == '\0') return str;

	if (!ignore_case) return strstr(str, substr);

	char const *_substr = substr;

	for (; *str != '\0'; str++) {

		if (tolower(*str) != tolower(*_substr)) continue;

		char const *_str = str;
		while (true) {

			if (*_substr == '\0') return str;
			if (tolower(*_str++) != tolower(*_substr++))
				break;
		}

		_substr = substr;
	}

	return NULL;
}



void start_replacement(
	FILE *in_file, FILE *out_file, struct cl_options const *pclo)
{
	char line[_MAX_LINE_LEN];

	int32_t line_num = 0;

	while (line_num < pclo->beg_line) {

		if (fgets(line, _MAX_LINE_LEN, in_file) == NULL) return;
		fprintf(out_file, "%s", line);
		line_num++;
	}


	size_t find_str_len = strlen(pclo->find_str);
	size_t repl_str_len = strlen(pclo->repl_str);

	while (line_num <= pclo->end_line &&
		fgets(line, _MAX_LINE_LEN, in_file) != NULL) {

		char const *beg = line, *end = NULL;

		while (true) {

			end = find_substr(
				beg, pclo->find_str, pclo->ignore_case);

			if (end == NULL) {

				fwrite(beg, 1, strlen(beg), out_file);
				break;
			}

			fwrite(beg, 1, end - beg, out_file);
			fwrite(pclo->repl_str, 1, repl_str_len, out_file);

			beg = end + find_str_len;
		}

		line_num++;
	}


	while (fgets(line, _MAX_LINE_LEN, in_file) != NULL)
		fprintf(out_file, "%s", line);

	fprintf(out_file, "\n");
}



int main(int32_t argc, char const *const *argv) {

	struct cl_options clo;
	init_clo(&clo);
	parse_clo(argv, &clo);

	if (*clo.in_file_name == '\0') {

		printf("Warning: The name of the input file "
			"was not specified\n");
		printf("Name: ");
		fgets(clo.in_file_name, _MAX_PATH_LEN, stdin);
		*(clo.in_file_name + strlen(clo.in_file_name) - 1) = '\0';
	}

	//dump_clo(&clo);


	FILE *in_file, *out_file;

	fopen_s(&in_file, clo.in_file_name, "r");
	if (in_file == NULL)
		handle_error("Fail to open input file");

	if (*clo.out_file_name == '\0')
		out_file = stdout;
	else {

		fopen_s(&out_file, clo.out_file_name, "w");
		if (out_file == NULL)
			handle_error("Fail to open output file");
	}

	start_replacement(in_file, out_file, &clo);

	fclose(in_file);
	if (out_file != stdout) fclose(out_file);

	system("pause");
	return 0;
}

