#include "purc.h"

#include "private/executor.h"
#include "private/utils.h"

#include <gtest/gtest.h>
#include <glob.h>
#include <libgen.h>
#include <limits.h>

#include "../helpers.h"

extern "C" {
#include "pcexe-helper.h"
#include "exe_key.h"
#include "exe_key.tab.h"
}

#include "utils.cpp.in"

TEST(exe_key, basic)
{
    purc_instance_extra_info info = {0, 0};
    bool cleanup = false;

    // initial purc
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    bool ok;

    struct purc_exec_ops ops;
    memset(&ops, 0, sizeof(ops));
    ok = purc_register_executor("KEY", &ops);
    EXPECT_FALSE(ok);
    EXPECT_EQ(purc_get_last_error(), PCEXECUTOR_ERROR_ALREAD_EXISTS);

    cleanup = purc_cleanup();
    ASSERT_EQ(cleanup, true);
}

static inline bool
parse(const char *rule, char **err_msg)
{
    struct exe_key_param param;
    memset(&param, 0, sizeof(param));
    param.debug_flex         = debug_flex;
    param.debug_bison        = debug_bison;

    bool r;
    r = exe_key_parse(rule, strlen(rule), &param) == 0;
    *err_msg = param.err_msg;

    logical_expression_destroy(param.rule.lexp);
    return r;
}

static inline bool
parse_ex(const char *rule, purc_variant_t input, char **err_msg)
{
    purc_exec_ops ops;
    bool ok = purc_get_executor("KEY", &ops);
    if (!ok) {
        if (err_msg) {
            *err_msg = strdup("failed to get executor of [KEY]");
        }
        return false;
    }

    pcexecutor_set_debug(debug_flex, debug_bison);

    purc_exec_inst_t inst = ops.create(PURC_EXEC_TYPE_CHOOSE,
            input, true);

    if (!inst) {
        if (err_msg) {
            *err_msg = strdup("failed to create [KEY] instance");
        }
        return false;
    }

    ok = true;

    purc_variant_t v = ops.choose(inst, rule);
    if (v == PURC_VARIANT_INVALID) {
        if (inst->err_msg) {
            if (err_msg) {
                *err_msg = strdup(inst->err_msg);
            }
            ok = false;
        }
    } else {
        purc_variant_unref(v);
    }

    ops.destroy(inst);

    return ok;
}

TEST(exe_key, files)
{
    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    purc_instance_extra_info info = {0, 0};
    r = purc_init("cn.fmsoft.hybridos.test",
        "vdom_gen", &info);
    EXPECT_EQ(r, PURC_ERROR_OK);
    if (r)
        return;

    const char *rel = "data/key.*.rule";
    get_option_from_env(rel, false);

    purc_variant_t key = purc_variant_make_string_static("hello", true);
    purc_variant_t val = purc_variant_make_string_static("world", true);
    purc_variant_t obj = purc_variant_make_object(1, key, val);
    purc_variant_unref(val);
    purc_variant_unref(key);

    process_sample_files(sample_files,
            [&](const char *rule, char **err_msg) -> bool {
        return parse_ex(rule, obj, err_msg);
    });

    purc_variant_unref(obj);

    bool ok = purc_cleanup ();

    std::cerr << std::endl;
    get_option_from_env(rel, true); // print
    print_statics();
    std::cerr << std::endl;

    ASSERT_TRUE(ok);
}

