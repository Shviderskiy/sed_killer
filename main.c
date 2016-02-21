#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define _MAX_PATH_LEN 256
#define _MAX_LINE_LEN 2048

#define _ASSERT(_COND, _MES, ...) \
if (!(_COND)) \
{ \
	fprintf(stderr, "Error: " _MES "\n", __VA_ARGS__); \
	fprintf(stdout, "%s", help); \
	exit(1); \
}


char help[] =
"Usage: sed_killer.exe -f -r [-b -e -i] [IN_FILE, OUT_FILE]\n" \
"\toptions:\n" \
"\t\t-f=FIND_STR\n" \
"\t\t-r=REPL_STR\n"
"\t\t-b=BEGIN_LINE optional\n" \
"\t\t-e=END_LINE optional\n" \
"\t\t-i (ignore case) optional\n\n"; 



// command line options
struct cl_options {

	char in_file_name[_MAX_PATH_LEN];
	char out_file_name[_MAX_PATH_LEN];

	int32_t beg_line_num;
	int32_t end_line_num;

	char const *find_str;
	char const *repl_str;

	bool ignore_case;
};



void init_clo(struct cl_options *pclo) {

	memset(pclo->in_file_name, '\0', _MAX_PATH_LEN);
	memset(pclo->out_file_name, '\0', _MAX_PATH_LEN);
	pclo->beg_line_num = -1;
	pclo->end_line_num = -1;
	pclo->find_str = NULL;
	pclo->repl_str = NULL;
	pclo->ignore_case = false;
}



void parse_clo(
	char const *const *argv, struct cl_options *pclo)
{
	argv++;

	while (*argv != NULL) {

		if (memcmp(*argv, "-f=", 3) == 0) {

			_ASSERT(pclo->find_str == NULL,
				"Wrong '-f' key. "
				"The find string is already set '%s'.",
				pclo->find_str);

			pclo->find_str = *argv + 3;

			_ASSERT(*pclo->find_str != '\0',
				"Wrong '-f' key. "
				"The find string must be non-zero length.");

			argv++;
			continue;
		}

		if (memcmp(*argv, "-r=", 3) == 0) {

			_ASSERT(pclo->repl_str == NULL,
				"Wrong '-r' key. "
				"The find string is already set '%s'.",
				pclo->repl_str);

			pclo->repl_str = *argv + 3;
			argv++;
			continue;
		}

		if (memcmp(*argv, "-i", 2) == 0) {

			_ASSERT(pclo->ignore_case == false,
				"The '-i' option is already set.");

			pclo->ignore_case = true;
			argv++;
			continue;
		}

		if (memcmp(*argv, "-b=", 3) == 0) {

			_ASSERT(pclo->beg_line_num == -1,
				"The '-b' option is already set '%d'.",
				pclo->beg_line_num);

			_ASSERT(sscanf_s(*argv + 3, "%d", &pclo->beg_line_num) > 0
				&& pclo->beg_line_num >= 0,
				"Wrong '-b' option (Expected positive number).");

			argv++;
			continue;
		}

		if (memcmp(*argv, "-e=", 3) == 0) {

			_ASSERT(pclo->end_line_num == -1,
				"The '-e' option is already set '%d'.",
				pclo->end_line_num);

			_ASSERT(sscanf_s(*argv + 3, "%d", &pclo->end_line_num) > 0
				&& pclo->end_line_num >= 0,
				"Wrong '-e' option (Expected positive number).");

			argv++;
			continue;
		}

		break;
	}


	if (pclo->beg_line_num == -1)
		pclo->beg_line_num = 0;

	if (pclo->end_line_num == -1)
		pclo->end_line_num = INT32_MAX;

	_ASSERT(pclo->find_str != NULL,
		"The key '-f' must be specified.");

	_ASSERT(pclo->repl_str != NULL,
		"The key '-r' must be specified.");

	_ASSERT(pclo->beg_line_num <= pclo->end_line_num,
		"The '-b' value must be <= '-e' value.");


	if (*argv == NULL) return;

	_ASSERT(strlen(*argv) < _MAX_PATH_LEN,
		"Too long input file name.");

	memcpy(pclo->in_file_name, *argv, _MAX_PATH_LEN);
	argv++;


	if (*argv == NULL) return;

	_ASSERT(strlen(*argv) < _MAX_PATH_LEN,
		"Too long output file name.");

	memcpy(pclo->out_file_name, *argv, _MAX_PATH_LEN);
	argv++;


	_ASSERT(*argv == NULL,
		"Wrong arguments '%s ...'.\n"
		"The name of the output file must be last argument.",
		*argv);

}



void dump_clo(struct cl_options const *pclo) {

	printf("in_f_name = %s\n", pclo->in_file_name);
	printf("out_f_name = %s\n",
		*pclo->out_file_name ? pclo->out_file_name : "stdout");
	printf("find_str = %s\n", pclo->find_str);
	printf("repl_str = %s\n", pclo->repl_str);
	printf("beg_line_num = %d\n", pclo->beg_line_num);
	printf("end_line_num = %d\n", pclo->end_line_num);
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
	memset(line, '\0', _MAX_LINE_LEN);

	int32_t line_num = 0;

	while (line_num < pclo->beg_line_num) {

		if (fgets(line, _MAX_LINE_LEN, in_file) == NULL) return;
		fprintf(out_file, "%s", line);
		line_num++;
	}


	size_t find_str_len = strlen(pclo->find_str);
	size_t repl_str_len = strlen(pclo->repl_str);

	while (line_num <= pclo->end_line_num &&
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

	if (out_file == stdout) fprintf(out_file, "\n");
}



int main(int32_t argc, char const *const *argv) {

	struct cl_options clo;
	init_clo(&clo);
	parse_clo(argv, &clo);

	if (*clo.in_file_name == '\0') {

		printf("Warning: The name of the input file "
			"was not specified\nName: ");
		fgets(clo.in_file_name, _MAX_PATH_LEN, stdin);
		*(clo.in_file_name + strlen(clo.in_file_name) - 1) = '\0';
	}

	//dump_clo(&clo);


	FILE *in_file = NULL, *out_file = NULL;

	fopen_s(&in_file, clo.in_file_name, "r");
	_ASSERT(in_file != NULL,
		"Fail to open input file '%s'.",
		clo.in_file_name);

	if (*clo.out_file_name == '\0') out_file = stdout;
	else {

		fopen_s(&out_file, clo.out_file_name, "w");
		_ASSERT(out_file != NULL,
			"Fail to open output file '%s'.",
			clo.out_file_name);
	}

	start_replacement(in_file, out_file, &clo);

	fclose(in_file);
	if (out_file != stdout) fclose(out_file);

	return 0;
}

