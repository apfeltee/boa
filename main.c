
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "lit.h"

// Used for clean up on Ctrl+C / Ctrl+Z
static LitState* replstate;

void interupt_handler(int signalid)
{
    lit_state_destroy(replstate);
    printf("\nExiting.\n");

    exit(0);
}

static void run_repl(LitState* state)
{
    replstate = state;
    signal(SIGINT, interupt_handler);

#ifndef _WIN32
    signal(SIGTSTP, interupt_handler);
#endif

    printf("lit v%s, developed by @egordorichev\n", LIT_VERSION_STRING);

#ifdef LIT_USE_LIBREADLINE
    char* line;
#else
    char line[LIT_REPL_INPUT_MAX];
#endif

    while(true)
    {
        printf("%s>%s ", COLOR_BLUE, COLOR_RESET);

#ifdef LIT_USE_LIBREADLINE
        line = readline("");
        add_history(line);
#else
        if(!fgets(line, LIT_REPL_INPUT_MAX, stdin))
        {
            printf("\n");
            break;
        }
#endif

        if(strcmp(line, "exit"))
        {
            break;
        }

        LitResult result = lit_state_interpretsource(state, "repl", line);

        if(result.type == INTERPRET_OK && result.result != NULL_VALUE)
        {
            printf("%s%s%s\n", COLOR_GREEN, lit_tostring_value(state, result.result, 0)->chars, COLOR_RESET);
        }

        lit_eventsystem_loop(state);
    }
}

static void run_tests(LitState* state)
{
    DIR* dir = opendir(LIT_TESTS_DIRECTORY);

    if(dir == NULL)
    {
        fprintf(stderr, "Could not find '%s' directory\n", LIT_TESTS_DIRECTORY);
        return;
    }

    struct dirent* ep;
    struct dirent* node;

    size_t testsdirlength = strlen(LIT_TESTS_DIRECTORY);
    struct stat st;

    while((ep = readdir(dir)))
    {
        const char* dirname = ep->d_name;

        if(strcmp(dirname, "..") == 0 || strcmp(dirname, ".") == 0)
        {
            continue;
        }

        size_t dirnamelength = strlen(dirname);
        size_t totallength = dirnamelength + testsdirlength + 2;

        char subdirname[totallength];

        memcpy(subdirname, LIT_TESTS_DIRECTORY, testsdirlength);
        memcpy(subdirname + testsdirlength + 1, dirname, dirnamelength);

        subdirname[testsdirlength] = '/';
        subdirname[totallength - 1] = '\0';

        stat(subdirname, &st);

        if(S_ISDIR(st.st_mode))
        {
            DIR* subdir = opendir(subdirname);

            if(subdir == NULL)
            {
                fprintf(stderr, "Failed to open tests subdirectory '%s'\n", subdirname);
                continue;
            }

            while((node = readdir(subdir)))
            {
                const char* file_name = node->d_name;
                size_t namelength = strlen(file_name);

                if(namelength < 4 || memcmp(".lit", file_name + namelength - 4, 4) != 0)
                {
                    continue;
                }

                char filepath[totallength + namelength + 1];

                memcpy(filepath, subdirname, totallength - 1);
                memcpy(filepath + totallength, file_name, namelength);

                filepath[totallength - 1] = '/';
                filepath[totallength + namelength] = '\0';

                stat(filepath, &st);

                if(S_ISREG(st.st_mode))
                {
                    printf("Testing %s...\n", filepath);
                    lit_state_interpretfile(state, filepath);
                }
            }

            closedir(subdir);
        }
    }

    closedir(dir);
}

static void show_help()
{
    printf("lit [options] [files]\n");
    printf("\t-o --output [file]\tInstead of running the file the compiled bytecode will be saved.\n");
    printf("\t-e --eval [string]\tRuns the given code string.\n");
    printf("\t-p --pass [args]\tPasses the rest of the arguments to the script.\n");
    printf("\t-i --interactive\tStarts an interactive shell.\n");
    printf("\t-d --dump\t\tDumps all the bytecode chunks from the given file.\n");
    printf("\t-t --time\t\tMeasures and prints the compilation timings.\n");
    printf("\t-c --test\t\tRuns all tests (useful for code coverage testing).\n");
    printf("\t-h --help\t\tI wonder, what this option does.\n");
    printf("\tIf no code to run is provided, lit will try to run either main.lbc or main.lit and, if fails, default to an interactive shell will start.\n");
}



static bool match_arg(const char* arg, const char* a, const char* b)
{
    return strcmp(arg, a) == 0 || strcmp(arg, b) == 0;
}

int main(int argc, char* argv[])
{
    LitState* state = lit_state_make();
    lit_state_openlibraries(state);

    char* filestorun[argc - 1];
    LitUInt numfilestorun = 0;

    LitStatusCode result = INTERPRET_OK;
    bool dump = false;

    for(int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];

        if(arg[0] == '-')
        {
            if(match_arg(arg, "-e", "--eval") || match_arg(arg, "-o", "--output"))
            {
                // It takes an extra argument, count it or we will use it as the file name to run :P
                i++;
            }
            else if(match_arg(arg, "-p", "--pass"))
            {
                // The rest of the args go to the script, go home pls
                break;
            }
            else if(match_arg(arg, "-d", "--dump"))
            {
                dump = true;
            }


            continue;
        }

        filestorun[numfilestorun++] = (char*)arg;
    }

    LitArray* argarray = NULL;

    bool showrepl = false;
    bool evaled = false;
    bool showedhelp = false;
    bool performtests = false;

    char* bytecodefile = NULL;

    for(int i = 1; i < argc; i++)
    {
        int argsleft = argc - i - 1;
        const char* arg = argv[i];
        if(match_arg(arg, "-e", "--eval"))
        {
            evaled = true;

            if(argsleft == 0)
            {
                fprintf(stderr, "Expected code to run for the eval argument.\n");
                return LIT_EXIT_CODE_ARGUMENT_ERROR;
            }

            const char* string = argv[++i];
            size_t length = strlen(string) + 1;
            char source[length];

            memcpy(source, string, length);
            const char* modname = numfilestorun == 0 ? "repl" : filestorun[0];

            if(dump)
            {
                LitModule* module = lit_state_compilemodulesource(state, CONST_STRING(state, modname), source);

                if(module == NULL)
                {
                    break;
                }

                lit_debug_disasmodule(module, source);
            }
            else
            {
                result = lit_state_interpretsource(state, modname, source).type;

                if(result != INTERPRET_OK)
                {
                    break;
                }
            }
        }
        else if(match_arg(arg, "-h", "--help"))
        {
            show_help();
            showedhelp = true;
        }
        else if(match_arg(arg, "-t", "--trace"))
        {
            state->config.traceexecution = true;
        }
        else if(match_arg(arg, "--time", "--time"))
        {
            lit_state_enablecompilationtimemeasurement();
        }
        else if(match_arg(arg, "-i", "--interactive"))
        {
            showrepl = true;
        }
        else if(match_arg(arg, "-c", "--test"))
        {
            performtests = true;
        }
        else if(match_arg(arg, "-d", "--dump"))
        {
            dump = true;
        }
        else if(match_arg(arg, "-o", "--output"))
        {
            if(argsleft == 0)
            {
                fprintf(stderr, "Expected file name where to save the bytecode.\n");
                return LIT_EXIT_CODE_ARGUMENT_ERROR;
            }

            bytecodefile = (char*)argv[++i];
        }
        else if(match_arg(arg, "-p", "--pass"))
        {
            argarray = lit_object_makearray(state);

            for(int j = 0; j < argsleft; j++)
            {
                const char* argstring = argv[i + j + 1];
                lit_vallist_push(state, &argarray->values, OBJECT_CONST_STRING(state, argstring));
            }

            lit_state_setglobal(state, CONST_STRING(state, "args"), OBJECT_VALUE(argarray));
            break;
        }
        else if(arg[0] == '-')
        {
            fprintf(stderr, "Unknown argument '%s', run 'lit --help' for help.\n", arg);
            return LIT_EXIT_CODE_ARGUMENT_ERROR;
        }
    }

    if(numfilestorun > 0)
    {
        if(bytecodefile != NULL)
        {
            if(!lit_state_compileandsavefiles(state, filestorun, numfilestorun, bytecodefile))
            {
                result = INTERPRET_COMPILE_ERROR;
            }

        }
        else
        {
            if(argarray == NULL)
            {
                argarray = lit_object_makearray(state);
            }

            lit_state_setglobal(state, CONST_STRING(state, "args"), OBJECT_VALUE(argarray));

            for(LitUInt i = 0; i < numfilestorun; i++)
            {
                char* file = filestorun[i];
                result = (dump ? lit_state_dumpfile(state, file) : lit_state_interpretfile(state, file)).type;

                if(result != INTERPRET_OK)
                {
                    break;
                }
            }
        }
    }

    if(performtests)
    {
        run_tests(state);
        return 0;
    }

    if(showrepl)
    {
        run_repl(state);
    }
    else if(!showedhelp && !evaled && numfilestorun == 0)
    {
        if(lit_file_exists("main.lbc"))
        {
            result = lit_state_interpretfile(state, "main.lbc").type;
        }
        else if(lit_file_exists("main.lit"))
        {
            result = lit_state_interpretfile(state, "main.lit").type;
        }
        else
        {
            run_repl(state);
        }
    }

    lit_eventsystem_loop(state);
    int64_t amount = lit_state_destroy(state);

    if(result != INTERPRET_COMPILE_ERROR && amount != 0)
    {
        fprintf(stderr, "Error: memory leak of %i bytes!\n", (int)amount);
        return LIT_EXIT_CODE_MEM_LEAK;
    }

    if(result != INTERPRET_OK)
    {
        return result == INTERPRET_RUNTIME_ERROR ? LIT_EXIT_CODE_RUNTIME_ERROR : LIT_EXIT_CODE_COMPILE_ERROR;
    }

    return 0;
}