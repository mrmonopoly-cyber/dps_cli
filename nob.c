#include <stdio.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#include "nob.h"
#include "c_cli.h"

#define ArraySize(ARR) (sizeof(ARR)/sizeof(ARR[0]))

#define CC "cc"

#define BUILD_DIR "build"
#define O_FILE "main"

typedef enum{
    Debug =0,
    Release,
}BuildProfile;

typedef struct CCliUserArgs{
    bool verbose;
    bool help;
    BuildProfile profile;
}Args;

typedef struct GDef{
    const char* def;
    const char* val;
}GDef;

typedef struct{
    Procs *procs;
    BuildProfile profile;
}BuildData;

static const char* src_dirs[] = 
{
    "src",
    "./lib/DPS/src",
    //add here your sources directory like ThirdParty dependencies sources
};

static const char* src_files[] = 
{
    "main.c",
    "./lib/DPS/lib/c_vector/c_vector.c"
    //add here your sources directory like ThirdParty dependencies sources
};

static const char* compiler_opts[] = 
{
    "-Wall",
    "-Wextra",
    "-std=c99",
    "-xc",
    "-pedantic"
    //add here your compiler options: -c, -ggdb, -O2, ...
};

static const char* debug_compiler_opts[] = 
{
    "-O0",
    "-fsanitize=undefined,address",
    "-g",
    //add here your compiler options: -c, -ggdb, -O2, ...
};

static const char* release_compiler_opts[] = 
{
    "-O2",
    //add here your compiler options: -c, -ggdb, -O2, ...
};

static const char* linker_opts[] = 
{
    //add here your compiler options: -lm, -lgdb, ...
};

static const char* debug_linker_opts[] = 
{
    "-fsanitize=undefined,address",
    //add here your compiler options: -lm, -lgdb, ...
};

static const char* include_path[] =
{
    //add here your include path: -I...
    //consider the root of the project the starting source path
};

static const GDef global_defs[] =
{
    {"DEBUG", NULL},
    {"_DEFAULT_SOURCE", NULL},
    //add here your global definitions: -DVAR=VALUE == (GDef) {.def="VAR", .val="VALUE"}
};

static inline bool cli_parse(Args* const args, const int argc, char** argv);

static bool f_compile(Walk_Entry entry)
{
    BuildData* data = entry.data;
    bool res=true;
    const char* file_name = nob_temp_file_name(entry.path);

    if(entry.type == FILE_REGULAR && !strcmp(&file_name[strlen(file_name)-2], ".c"))
    {
        Cmd cmd = {0};

        cmd_append(&cmd, CC);

        //compiler options
        for(size_t i=0; i < ArraySize(compiler_opts); i++)
        {
            if(compiler_opts[i]) cmd_append(&cmd, compiler_opts[i]);
        }

        switch (data->profile)
        {
            case Debug:
                {
                    //DEBUG compiler options
                    for(size_t i=0; i < ArraySize(debug_compiler_opts); i++)
                    {
                        const char* opt = debug_compiler_opts[i];
                        if(opt) cmd_append(&cmd, opt);
                    }
                }
                break;
            case Release:
                {
                    //RELEASE compiler options
                    for(size_t i=0; i < ArraySize(release_compiler_opts); i++)
                    {
                        const char* opt = release_compiler_opts[i];
                        if(opt) cmd_append(&cmd, opt);
                    }
                }
                break;
        }

        //include path
        for(size_t i=0; i < ArraySize(include_path); i++)
        {
            if(include_path[i]) cmd_append(&cmd, temp_sprintf("-I%s", include_path[i]));
        }

        //global definitions
        for(size_t i=0; i < ArraySize(global_defs); i++)
        {
            const GDef* def = &global_defs[i];
            if(def && def->def)
            {
                if(def->val)
                {
                    cmd_append(&cmd, temp_sprintf("-D%s=%s", def->def, def->val));
                }
                else
                {
                    cmd_append(&cmd, temp_sprintf("-D%s", def->def));
                }
            }
        }

        cmd_append(&cmd, "-c");
        cmd_append(&cmd, "-o", temp_sprintf("%s/%.*s.o", BUILD_DIR, (int) strlen(file_name)-2, file_name));

        cmd_append(&cmd, entry.path);

        res = cmd_run(&cmd, .async = data->procs);

        cmd_free(cmd);
    }

    return res;
}

static bool f_link(BuildData* data)
{
    Dir_Entry dir = {0};
    Cmd cmd = {0};
    bool res = true;

    if(!dir_entry_open(BUILD_DIR, &dir)) return false;

    cmd_append(&cmd, CC);

    //linker options
    for(size_t i=0; i < ArraySize(linker_opts); i++)
    {
        if(linker_opts[i]) cmd_append(&cmd, linker_opts[i]);
    }

    switch (data->profile)
    {
        case Debug:
            {
                //DEBUG compiler options
                for(size_t i=0; i < ArraySize(debug_linker_opts); i++)
                {
                    const char* opt = debug_linker_opts[i];
                    if(opt) cmd_append(&cmd, opt);
                }
            }
            break;
        case Release:
            {
            }
            break;
    }

    cmd_append(&cmd, "-o", O_FILE);

    while(dir_entry_next(&dir))
    {
        const char* file_path = temp_sprintf("%s/%s", BUILD_DIR, dir.name);
        if (FILE_REGULAR == get_file_type(file_path))
        {
            printf("found %s\n", file_path);
            cmd_append(&cmd, file_path);
        }
    }

    res = cmd_run(&cmd);

    dir_entry_close(dir);
    cmd_free(cmd);
    return res;
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);
    Args args = {0};
    Procs procs = {0};
    BuildData data = {0};
    const char* pwd = get_current_dir_temp();

    if(!cli_parse(&args, argc, argv))
    {
        return 1;
    }

    data.profile = args.profile;
    data.procs = &procs;

    printf("build directory: %s\n", BUILD_DIR);
    printf("output file: %s\n", O_FILE);

    mkdir_if_not_exists(BUILD_DIR);

    //source directories
    for(size_t i=0; i < ArraySize(src_dirs); i++)
    {
        if(src_dirs[i])
        {
            printf("compiling sources in src: %s\n", src_dirs[i]);
            if(!walk_dir(src_dirs[i], f_compile, .data = &data))
            {
                fprintf(stderr, "failed compiling sources in %s\n", src_dirs[i]);
                return 1;
            }
        }
    }

    //source files
    for(size_t i=0; i<ArraySize(src_files); i++)
    {
        const char* file = src_files[i];
        printf("compiling source: %s\n", src_files[i]);
        Walk_Entry entry =
            (Walk_Entry) {
                .type = FILE_REGULAR,
                .path = temp_sprintf("%s/%s", pwd, file),
                .data = &data,
            };

        if(file && !f_compile(entry))
        {
            fprintf(stderr, "failed compiling source: %s\n", file);
            return 1;
        }
    }
    procs_flush(data.procs);

    if(!f_link(&data))
    {
        fprintf(stderr, "failed liking\n");
        return 1;
    }

  return 0;
}

#define CCLI_DEPLOY
#include "c_cli.h"

CCLI_PARSER_DECLARE(profile);

const CCliArgDef defs[] =
{
    {
        .f_long = CCLI_LONG_FLAG(profile),
        .f_short = CCLI_SHORT_FLAG(p),
        .f_args =
        {
            CCLI_NEW_ARG(profile_name, CCliArgStr),
        },
        .f_description = "profile build to use: [debug, release]",
        .f_parser = CCLI_PARSER_NAME(profile),
    },
};

static void default_values(struct CCliUserArgs* const restrict args)
{
    args->profile = Debug;
}

static inline bool cli_parse(Args* const args, const int argc, char** argv)
{
    return c_cli_parse(defs, CCLI_ARRAYSIZE(defs), args, argc, argv, default_values);
}

CCLI_PARSER_DECLARE_FULL(profile, args, ctx)
{
    const char* profile_name = NULL;
    CCliActionReturn res = c_cli_parse_nex_arg_str(ctx, &profile_name);

    if(res != CCliActionOK) return res;

    if(!strcmp(profile_name, "debug"))
    {
        args->profile = Debug;
    }
    else if(!strcmp(profile_name, "release"))
    {
        args->profile = Release;
    }
    else
    {
        res = CCliActionInvalidInput;
    }

    return res;
}
