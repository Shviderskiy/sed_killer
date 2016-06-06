#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>


// Ugly, but I have not other solution.
#ifndef _MSC_VER

#define sscanf_s(...) sscanf(__VA_ARGS__)

#define fopen_s(_Fife, _Filename, _Mode) \
    (NULL == ((*(_Fife)) = fopen((_Filename), (_Mode))))

#endif


#define _MAX_PATH_LEN 256
#define _MAX_LINE_LEN 2048


#define _OS_ERROR(_MES, ...) { \
    fprintf(stderr, "OS Error: " _MES "\n", ## __VA_ARGS__); \
    exit(-1); \
}


#define _USER_ERROR(_MES, ...) { \
    fprintf(stderr, "Error while using: " _MES "\n", ## __VA_ARGS__); \
    fprintf(stdout, "%s", help); \
    exit(-1); \
}



char help[] =
"Usage: sed_killer -f=FIND_STR -r=REPL_STR\n" \
"                  [-b=BEGIN_LINE] [-e=END_LINE] [-i]\n" \
"                  IN_FILE [OUT_FILE]\n" \
"-i (ignore case)\n" \
"OUT_FILE is stdout by default\n\n";



// Command line options.
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



void parse_clo(char const *const *argv, struct cl_options *pclo) {
    argv++;

    while (NULL != *argv) {

        if (0 == memcmp(*argv, "-f=", 3)) {

            if (NULL != pclo->find_str)
                _USER_ERROR("Wrong '-f' key. "
                            "The find string is already set '%s'.",
                            pclo->find_str);

            pclo->find_str = *argv + 3;

            if ('\0' == *pclo->find_str)
                _USER_ERROR("Wrong '-f' key. "
                            "The find string must be non-zero length.");

            argv++;
            continue;
        }

        if (0 == memcmp(*argv, "-r=", 3)) {

            if (NULL != pclo->repl_str)
                _USER_ERROR("Wrong '-r' key. "
                            "The find string is already set '%s'.",
                            pclo->repl_str);

            pclo->repl_str = *argv + 3;
            argv++;
            continue;
        }

        if (0 == strcmp(*argv, "-i")) {

            if (true == pclo->ignore_case)
                _USER_ERROR("The '-i' option is already set.");

            pclo->ignore_case = true;
            argv++;
            continue;
        }

        if (0 == memcmp(*argv, "-b=", 3)) {

            if (-1 != pclo->beg_line_num)
                _USER_ERROR("The '-b' option is already set '%d'.",
                            pclo->beg_line_num);

            if (sscanf_s(*argv + 3, "%d", &pclo->beg_line_num) <= 0 ||
                pclo->beg_line_num < 0)
                _USER_ERROR("Wrong '-b' option (Expected positive number).");

            argv++;
            continue;
        }

        if (0 == memcmp(*argv, "-e=", 3)) {

            if (-1 != pclo->end_line_num)
                _USER_ERROR("The '-e' option is already set '%d'.",
                            pclo->end_line_num);

            if (sscanf_s(*argv + 3, "%d", &pclo->end_line_num) <= 0 ||
                pclo->end_line_num < 0)
                _USER_ERROR("Wrong '-e' option (Expected positive number).");

            argv++;
            continue;
        }

        break;
    }


    if (-1 == pclo->beg_line_num)
        pclo->beg_line_num = 0;

    if (-1 == pclo->end_line_num)
        pclo->end_line_num = INT32_MAX;

    if (NULL == pclo->find_str)
        _USER_ERROR("The key '-f' must be specified.");

    if (NULL == pclo->repl_str)
        _USER_ERROR("The key '-r' must be specified.");

    if (pclo->beg_line_num > pclo->end_line_num)
        _USER_ERROR("The '-b' value must be <= '-e' value.");


    if (NULL == *argv) return;

    if (strlen(*argv) >= _MAX_PATH_LEN)
        _USER_ERROR("Too long input file name.");

    memcpy(pclo->in_file_name, *argv, _MAX_PATH_LEN);
    argv++;


    if (NULL == *argv) return;

    if (strlen(*argv) >= _MAX_PATH_LEN)
        _USER_ERROR("Too long output file name.");

    memcpy(pclo->out_file_name, *argv, _MAX_PATH_LEN);
    argv++;


    if (NULL != *argv)
        _USER_ERROR("Wrong arguments '%s ...'.\n"
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
    char const *str, char const *substr, bool ignore_case) {
        
    if (NULL == str) return NULL;
    if (NULL == substr) return NULL;

    if ('\0' == *substr) return str;

    if (false == ignore_case) return strstr(str, substr);

    char const *_substr = substr;

    for (; *str != '\0'; str++) {

        if (tolower(*str) != tolower(*_substr)) continue;

        char const *_str = str;
        while (true) {

            if ('\0' == *_substr) return str;
            if (tolower(*_str++) != tolower(*_substr++))
                break;
        }

        _substr = substr;
    }

    return NULL;
}



void start_replacement(
    FILE *in_file, FILE *out_file, struct cl_options const *pclo) {

    char line[_MAX_LINE_LEN];
    memset(line, '\0', _MAX_LINE_LEN);

    int32_t line_num = 0;

    while (line_num < pclo->beg_line_num) {

        if (NULL == fgets(line, _MAX_LINE_LEN, in_file)) return;
        fprintf(out_file, "%s", line);
        line_num++;
    }


    size_t find_str_len = strlen(pclo->find_str);
    size_t repl_str_len = strlen(pclo->repl_str);

    while (line_num <= pclo->end_line_num &&
           NULL != fgets(line, _MAX_LINE_LEN, in_file)) {

        char const *beg = line, *end = NULL;

        while (true) {

            end = find_substr(beg, pclo->find_str, pclo->ignore_case);

            if (NULL == end) {

                fwrite(beg, 1, strlen(beg), out_file);
                break;
            }

            fwrite(beg, 1, end - beg, out_file);
            fwrite(pclo->repl_str, 1, repl_str_len, out_file);

            beg = end + find_str_len;
        }

        line_num++;
    }


    while (NULL != fgets(line, _MAX_LINE_LEN, in_file))
        fprintf(out_file, "%s", line);

    if (stdout == out_file) fprintf(out_file, "\n");
}



int main(int32_t argc, char const *const *argv) {

    struct cl_options clo;
    init_clo(&clo);
    parse_clo(argv, &clo);

    if ('\0' == *clo.in_file_name) {

        printf("Warning: The name of the input file "
               "was not specified\nName: ");
        fgets(clo.in_file_name, _MAX_PATH_LEN, stdin);
        *(clo.in_file_name + strlen(clo.in_file_name) - 1) = '\0';
    }

    // dump_clo(&clo);

    FILE *in_file = NULL;
    fopen_s(&in_file, clo.in_file_name, "r");
    if (NULL == in_file)
        _OS_ERROR("Fail to open input file '%s'.", clo.in_file_name);

    FILE *out_file = stdout;
    if ('\0' != *clo.out_file_name) {

        fopen_s(&out_file, clo.out_file_name, "w");
        if (NULL == out_file)
            _OS_ERROR("Fail to open output file '%s'.", clo.out_file_name);
    }

    start_replacement(in_file, out_file, &clo);

    fclose(in_file);
    if (stdout != out_file) fclose(out_file);

    return 0;
}
