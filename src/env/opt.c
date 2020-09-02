//
// Created by Flo Schmidt on 27.07.20.
//

#include <stddef.h>
#include <getopt.h>
#include <util/log.h>
#include <common.h>
#include "opt.h"

t_opt_parse_result parse_cmd_arguments(int argc, char **argv) {
    int opt_index;
    char *file_path = NULL;
    int fileIndex = 0;

    t_opt_parse_result parse_result;

    //handle no arguments passed
    if (argc <= 1) goto HELP;

    //read command line options (ex. -f for executable file, -v for verbose logging, etc.)
    while((opt_index = getopt(argc, argv, ":f:mavgiorcshdb")) != -1) {
        switch(opt_index) {
            case 'a':
                flag_do_analyze = true;
                break;
            case 'v':
                flag_log_general = true;
                flag_log_asm_in = true;
                flag_log_asm_out = true;
                flag_log_reg_dump = false; //don't do register dump with the verbose option by default
                flag_log_cache = true;
                flag_translate_opt = false;
                break;
            case 'g':
                flag_log_general = true;
                break;
            case 'i':
                flag_log_asm_in = true;
                break;
            case 'o':
                flag_log_asm_out = true;
                break;
            case 'r':
                flag_log_reg_dump = true;
                break;
            case 'c':
                flag_log_cache = true;
                break;
            case 'f':
                file_path = optarg;
                fileIndex = optind - 1;
                break;
            case 's':
                flag_fail_silently = true;
                break;
            case 'd':
                flag_single_step = true;
                break;
            case 'm':
                flag_translate_opt = true;
                break;
            case 'b':
                flag_do_benchmark = true;
                break;
            case ':':
            case 'h':
            default:
            HELP:
                dprintf(1,
                        "Usage: dynamic-translate -f <filename> <option(s)>\n\t-v\tBe more verbose. Does not dump "
                        "register file. (equivalent to -gioc)\n"
                        "\t-g\tDisplay general verbose info\n\t-i\tDisplay parsed RISC-V input assembly\n"
                        "\t-o\tDisplay translated output x86 assembly\n"
                        "\t-r\tDump registers on basic block boundaries\n"
                        "\t-c\tDisplay cache info\n"
                        "\t-s\tFail silently for some  error conditions. Allows continued execution, but the client "
                        "program may enter undefined states.\n"
                        "\t-d\tEnable Single stepping mode. Each instruction will be its own block.\n"
                        "\t-m\tOptimize block translation.\n"
                        "\t-a\tAnalyze binary. Inspects passed program binary and shows instruction mnemonics.\n"
                        "\t-b\tBenchmark execution. Times the execution of the program, excluding mapping the binary into memory.\n"
                        "\t-h\tShow this help.\n"
                );
                parse_result.status = 1;
                return parse_result;
        }
    }

    log_general("Translator version %s\n", translator_version);
    log_general("Command line options:\n");
    log_general("General verbose: %d\n", flag_log_general);
    log_general("Input assembly: %d\n", flag_log_asm_in);
    log_general("Output assembly: %d\n", flag_log_asm_out);
    log_general("Register dump: %d\n", flag_log_reg_dump);
    log_general("Cache info: %d\n", flag_log_cache);
    log_general("Fail silently: %d\n", flag_fail_silently);
    log_general("Single stepping: %d\n", flag_single_step);
    log_general("Translate opt: %d\n", flag_translate_opt);
    log_general("Do analyze: %d\n", flag_do_analyze);
    log_general("Do benchmarking: %d\n", flag_do_benchmark);
    log_general("File path: %s\n", file_path);

    if (file_path == NULL) {
        dprintf(2, "Bad. Invalid file path.\n");
        parse_result.status = 2;
        return parse_result;
    }

    //we're fine, fill struct and return
    parse_result.status = 0;
    parse_result.file_index = fileIndex;
    parse_result.file_path = file_path;
    parse_result.last_optind = optind;
    return parse_result;
}
