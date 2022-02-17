/*
 * @file var-mgr.c
 * @author XueShuming
 * @date 2021/12/06
 * @brief The impl of PurC variable manager.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "purc.h"

#include "config.h"

#include "internal.h"

#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"

#include <stdlib.h>
#include <string.h>

#define TO_DEBUG 1

#define TYPE_STR_CHANGE    "change"
#define TYPE_STR_ATTACHED  "attached"
#define TYPE_STR_DETACHED  "detached"
#define TYPE_STR_DISPLACED  "displaced"

enum var_event_type {
    VAR_EVENT_TYPE_ATTACHED,
    VAR_EVENT_TYPE_DETACHED,
    VAR_EVENT_TYPE_DISPLACED,
};

struct var_observe {
    char* name;
    enum var_event_type type;
    pcintr_stack_t stack;
};

struct pcvarmgr_list {
    purc_variant_t object;
    struct pcvar_listener* grow_listener;
    struct pcvar_listener* shrink_listener;
    struct pcvar_listener* change_listener;
    pcutils_array_t* var_observers;
};

int find_var_observe_idx(struct pcvarmgr_list* mgr, const char* name,
        enum var_event_type type, pcintr_stack_t stack)
{
    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == type && obs->stack == stack) {
            return i;
        }
    }
    return -1;
}

struct var_observe* find_var_observe(struct pcvarmgr_list* mgr,
        const char* name, enum var_event_type type, pcintr_stack_t stack)
{
    int idx = find_var_observe_idx(mgr, name, type, stack);
    if (idx == -1) {
        return NULL;
    }
    return pcutils_array_get(mgr->var_observers, idx);
}

bool mgr_grow_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(nr_args);

    if (ctxt == NULL) {
        return true;
    }

    pcvarmgr_list_t mgr = (pcvarmgr_list_t)ctxt;

    purc_variant_t type = purc_variant_make_string(TYPE_STR_CHANGE, false);
    if (type == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t sub_type = purc_variant_make_string(TYPE_STR_ATTACHED,
            false);
    if (sub_type == PURC_VARIANT_INVALID) {
        purc_variant_unref(type);
        return false;
    }

    const char* name = purc_variant_get_string_const(argv[0]);

    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == VAR_EVENT_TYPE_ATTACHED) {
            pcintr_dispatch_message(obs->stack, source, type, sub_type,
                    PURC_VARIANT_INVALID);
        }
    }

    purc_variant_unref(sub_type);
    purc_variant_unref(type);
    return true;
}

bool mgr_shrink_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(nr_args);

    if (ctxt == NULL) {
        return true;
    }

    pcvarmgr_list_t mgr = (pcvarmgr_list_t)ctxt;

    purc_variant_t type = purc_variant_make_string(TYPE_STR_CHANGE, false);
    if (type == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t sub_type = purc_variant_make_string(TYPE_STR_DETACHED,
            false);
    if (sub_type == PURC_VARIANT_INVALID) {
        purc_variant_unref(type);
        return false;
    }

    const char* name = purc_variant_get_string_const(argv[0]);

    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == VAR_EVENT_TYPE_DETACHED) {
            pcintr_dispatch_message(obs->stack, source, type, sub_type,
                    PURC_VARIANT_INVALID);
        }
    }

    purc_variant_unref(sub_type);
    purc_variant_unref(type);
    return true;
}

bool mgr_change_handler(purc_variant_t source, purc_atom_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(nr_args);

    if (ctxt == NULL) {
        return true;
    }

    pcvarmgr_list_t mgr = (pcvarmgr_list_t)ctxt;

    purc_variant_t type = purc_variant_make_string(TYPE_STR_CHANGE, false);
    if (type == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t sub_type = purc_variant_make_string(TYPE_STR_DISPLACED,
            false);
    if (sub_type == PURC_VARIANT_INVALID) {
        purc_variant_unref(type);
        return false;
    }

    const char* name = purc_variant_get_string_const(argv[0]);

    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == VAR_EVENT_TYPE_DISPLACED) {
            pcintr_dispatch_message(obs->stack, source, type, sub_type,
                    PURC_VARIANT_INVALID);
        }
    }

    purc_variant_unref(sub_type);
    purc_variant_unref(type);
    return true;
}

#define DEF_ARRAY_SIZE 10
pcvarmgr_list_t pcvarmgr_list_create(void)
{
    pcvarmgr_list_t mgr = (pcvarmgr_list_t)calloc(1,
            sizeof(struct pcvarmgr_list));
    if (!mgr) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto err_ret;
    }

    mgr->object = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (mgr->object == PURC_VARIANT_INVALID) {
        goto err_free_mgr;
    }

    mgr->grow_listener = purc_variant_register_post_listener(mgr->object,
        pcvariant_atom_grow, mgr_grow_handler, mgr);
    if (!mgr->grow_listener) {
        goto err_clear_object;
    }

    mgr->shrink_listener = purc_variant_register_post_listener(mgr->object,
        pcvariant_atom_shrink, mgr_shrink_handler, mgr);
    if (!mgr->shrink_listener) {
        goto err_revoke_grow_listener;
    }

    mgr->change_listener = purc_variant_register_post_listener(mgr->object,
        pcvariant_atom_change, mgr_change_handler, mgr);
    if (!mgr->change_listener) {
        goto err_revoke_shrink_listener;
    }

    mgr->var_observers = pcutils_array_create();
    unsigned int ret = pcutils_array_init(mgr->var_observers, DEF_ARRAY_SIZE);
    if (PURC_ERROR_OK != ret) {
        purc_set_error(ret);
        goto err_revoke_change_listener;
    }

    return mgr;

err_revoke_change_listener:
    purc_variant_revoke_listener(mgr->object, mgr->change_listener);

err_revoke_shrink_listener:
    purc_variant_revoke_listener(mgr->object, mgr->shrink_listener);

err_revoke_grow_listener:
    purc_variant_revoke_listener(mgr->object, mgr->grow_listener);

err_clear_object:
    purc_variant_unref(mgr->object);

err_free_mgr:
    free(mgr);

err_ret:
    return NULL;
}

int pcvarmgr_list_destroy(pcvarmgr_list_t mgr)
{
    if (mgr) {
        size_t sz = pcutils_array_length(mgr->var_observers);
        for (size_t i = 0; i < sz; i++) {
            struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                    mgr->var_observers, i);
            free(obs->name);
            free(obs);
        }
        pcutils_array_destroy(mgr->var_observers, true);
        purc_variant_revoke_listener(mgr->object, mgr->grow_listener);
        purc_variant_revoke_listener(mgr->object, mgr->shrink_listener);
        purc_variant_revoke_listener(mgr->object, mgr->change_listener);
        purc_variant_unref(mgr->object);
        free(mgr);
    }
    return 0;
}

bool pcvarmgr_list_add(pcvarmgr_list_t mgr, const char* name,
        purc_variant_t variant)
{
    if (mgr == NULL || mgr->object == PURC_VARIANT_INVALID
            || name == NULL || !variant) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    purc_variant_t k = purc_variant_make_string(name, true);
    if (k == PURC_VARIANT_INVALID) {
        return false;
    }
    bool b = purc_variant_object_set(mgr->object, k, variant);
    purc_variant_unref(k);
    return b;
}

purc_variant_t pcvarmgr_list_get(pcvarmgr_list_t mgr, const char* name)
{
    if (mgr == NULL || name == NULL) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;
    v = purc_variant_object_get_by_ckey(mgr->object, name, false);
    if (v) {
        return v;
    }

    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

bool pcvarmgr_list_remove(pcvarmgr_list_t mgr, const char* name)
{
    if (name) {
        return purc_variant_object_remove_by_static_ckey(mgr->object,
                name, false);
    }
    return false;
}

static purc_variant_t
_find_named_scope_var(pcvdom_element_t elem, const char* name)
{
    if (!elem || !name) {
        PC_ASSERT(name); // FIXME: still recoverable???
        purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcintr_get_scope_variable(elem, name);
    if (v) {
        return v;
    }

    pcvdom_element_t parent = pcvdom_element_parent(elem);
    if (parent) {
        return _find_named_scope_var(parent, name);
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
_find_doc_buildin_var(purc_vdom_t vdom, const char* name)
{
    PC_ASSERT(name);
    if (!vdom) {
        purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcvdom_document_get_variable(vdom, name);
    if (v) {
        return v;
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t _find_inst_var(const char* name)
{
    if (!name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_list_t varmgr = pcinst_get_variables();
    if (varmgr == NULL) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcvarmgr_list_get(varmgr, name);
    if (v) {
        return v;
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

purc_variant_t
pcintr_find_named_var(pcintr_stack_t stack, const char* name)
{
    if (!stack || !name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    purc_variant_t v = _find_named_scope_var(frame->pos, name);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = _find_doc_buildin_var(stack->vdom, name);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = _find_inst_var(name);
    if (v) {
        purc_clr_error();
        return v;
    }

    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

enum purc_symbol_var _to_symbol(char symbol)
{
    switch (symbol) {
    case '?':
        return PURC_SYMBOL_VAR_QUESTION_MARK;
    case '^':
        return PURC_SYMBOL_VAR_CARET;
    case '@':
        return PURC_SYMBOL_VAR_AT_SIGN;
    case '!':
        return PURC_SYMBOL_VAR_EXCLAMATION;
    case ':':
        return PURC_SYMBOL_VAR_COLON;
    case '=':
        return PURC_SYMBOL_VAR_EQUAL;
    case '%':
        return PURC_SYMBOL_VAR_PERCENT_SIGN;
    default:
        // FIXME: NotFound???
        purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "symbol:%c", symbol);
        return PURC_SYMBOL_VAR_MAX;
    }
}

purc_variant_t
pcintr_get_symbolized_var (pcintr_stack_t stack, unsigned int number,
        char symbol)
{
    enum purc_symbol_var symbol_var = _to_symbol(symbol);
    if (symbol_var == PURC_SYMBOL_VAR_MAX) {
        PC_ASSERT(0);
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    for (unsigned int i = 0; i < number; i++) {
        frame = pcintr_stack_frame_get_parent(frame);
    }

    if (!frame) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = frame->symbol_vars[symbol_var];
    PC_ASSERT(v != PURC_VARIANT_INVALID);
    if (v != PURC_VARIANT_INVALID) {
        purc_clr_error();
        return v;
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "symbol:%c", symbol);
    return PURC_VARIANT_INVALID;
}

purc_variant_t
pcintr_get_numbered_var (pcintr_stack_t stack, unsigned int number)
{
    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    for (unsigned int i = 0; i < number; i++) {
        frame = pcintr_stack_frame_get_parent(frame);
    }

    if (!frame) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK];
    if (v != PURC_VARIANT_INVALID) {
        purc_clr_error();
        return v;
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "number:%d", number);
    return PURC_VARIANT_INVALID;
}

bool pcvarmgr_list_add_observer(pcvarmgr_list_t list, const char* name,
        const char* event)
{
    purc_variant_t var = pcvarmgr_list_get(list, name);
    if (var == PURC_VARIANT_INVALID) {
        return false;
    }

    enum var_event_type type = VAR_EVENT_TYPE_ATTACHED;
    if (strcmp(event, TYPE_STR_ATTACHED) == 0) {
        type = VAR_EVENT_TYPE_ATTACHED;
    }
    else if (strcmp(event, TYPE_STR_DETACHED) == 0) {
        type = VAR_EVENT_TYPE_DETACHED;
    }

    pcintr_stack_t stack = purc_get_stack();
    struct var_observe* obs = find_var_observe(list, name, type, stack);
    if (obs != NULL) {
        return true;
    }

    obs = (struct var_observe*) malloc(sizeof(struct var_observe));
    if (obs == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    obs->name = strdup(name);
    obs->type = type;
    obs->stack = stack;
    unsigned int ret = pcutils_array_push(list->var_observers, obs);
    if (ret == PURC_ERROR_OK) {
        return true;
    }

    free(obs->name);
    free(obs);
    return false;
}

bool pcvarmgr_list_remove_observer(pcvarmgr_list_t mgr, const char* name,
        const char* event)
{
    purc_variant_t var = pcvarmgr_list_get(mgr, name);
    if (var == PURC_VARIANT_INVALID) {
        return false;
    }

    enum var_event_type type = VAR_EVENT_TYPE_ATTACHED;
    if (strcmp(event, TYPE_STR_ATTACHED) == 0) {
        type = VAR_EVENT_TYPE_ATTACHED;
    }
    else if (strcmp(event, TYPE_STR_DETACHED) == 0) {
        type = VAR_EVENT_TYPE_DETACHED;
    }
    else if (strcmp(event, TYPE_STR_DISPLACED) == 0) {
        type = VAR_EVENT_TYPE_DISPLACED;
    }

    pcintr_stack_t stack = purc_get_stack();
    int idx = find_var_observe_idx(mgr, name, type, stack);

    if (idx != -1) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, idx);
        free(obs->name);
        free(obs);
        pcutils_array_delete(mgr->var_observers, idx, 1);
    }

    return true;
}
