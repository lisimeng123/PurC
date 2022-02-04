/**
 * @file match.c
 * @author Xue Shuming
 * @date 2022/01/13
 * @brief The ops for <match>
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
 *
 */

#include "purc.h"

#include "internal.h"

#include "private/debug.h"
#include "private/runloop.h"

#include "ops.h"

#include "purc-executor.h"

// FIXME:
#include "../executors/match_for.h"

#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define TO_DEBUG 1

struct ctxt_for_match {
    struct pcvdom_node *curr;
    purc_variant_t for_var;
    purc_variant_t exclusively;

    struct match_for_param param;
    bool is_exclusively;
    bool matched;
};

static void
ctxt_for_match_destroy(struct ctxt_for_match *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->for_var);
        PURC_VARIANT_SAFE_CLEAR(ctxt->exclusively);

        match_for_param_reset(&ctxt->param);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_match_destroy((struct ctxt_for_match*)ctxt);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    int r;

    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;
    PC_ASSERT(ctxt);

    purc_variant_t for_var = ctxt->for_var;
    bool matched = false;
    if (for_var == PURC_VARIANT_INVALID) {
        matched = true;
    }
    else {
        const char *for_value = purc_variant_get_string_const(for_var);
        r = match_for_parse(for_value, strlen(for_value), &ctxt->param);
        PC_ASSERT(r == 0);

        purc_variant_t parent_result;
        parent_result = frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK];
        PC_ASSERT(parent_result != PURC_VARIANT_INVALID);

        r = match_for_rule_eval(&ctxt->param.rule, parent_result, &matched);
        PC_ASSERT(r == 0);
    }

    ctxt->matched = matched;

    purc_variant_t exclusively = ctxt->exclusively;
    if (exclusively != PURC_VARIANT_INVALID) {
        ctxt->is_exclusively = true;
    }

    if (matched) {
        purc_variant_t parent_result;
        parent_result = frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK];
        PC_ASSERT(parent_result != PURC_VARIANT_INVALID);
        PURC_VARIANT_SAFE_CLEAR(frame->result_var);
        frame->result_var = parent_result;
        purc_variant_ref(parent_result);
    }

    return 0;
}

static int
process_attr_for(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;
    if (ctxt->for_var != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->for_var = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_exclusively(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;
    if (ctxt->exclusively != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->exclusively = val;
    purc_variant_ref(val);

    return 0;
}

static int
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_ASSIGNMENT);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FOR)) == name) {
        return process_attr_for(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, EXCLUSIVELY)) == name) {
        return process_attr_exclusively(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, EXCL)) == name) {
        return process_attr_exclusively(frame, element, name, val);
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "vdom attribute '%s' for element <%s>",
            purc_atom_to_string(name), element->tag_name);

    return -1;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    frame->pos = pos; // ATTENTION!!

    if (pcintr_set_symbol_var_at_sign())
        return NULL;

    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return NULL;

    purc_clr_error();

    r = post_process(&stack->co, frame);
    if (r)
        PC_ASSERT(0);
    if (r)
        return NULL;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;
    if (ctxt) {
        if (ctxt->is_exclusively && ctxt->matched) {
            // FIXME: what if target element in between test/match???
            struct pcintr_stack_frame *parent;
            parent = pcintr_stack_frame_get_parent(frame);
            PC_ASSERT(parent);
            PURC_VARIANT_SAFE_CLEAR(parent->result_from_child);
            parent->result_from_child = purc_variant_make_boolean(true);
            PC_ASSERT(parent->result_from_child != PURC_VARIANT_INVALID);
        }
        ctxt_for_match_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static void
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
}

static void
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(content);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(comment);
}


static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    pcintr_coroutine_t co = &stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    struct ctxt_for_match *ctxt;
    ctxt = (struct ctxt_for_match*)frame->ctxt;

    struct pcvdom_node *curr;

    if (!ctxt->matched)
        return NULL;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        purc_clr_error();
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                on_element(co, frame, element);
                PC_ASSERT(stack->except == 0);
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr));
            goto again;
        case PCVDOM_NODE_COMMENT:
            on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr));
            goto again;
        default:
            PC_ASSERT(0); // Not implemented yet
    }

    PC_ASSERT(0);
    return NULL; // NOTE: never reached here!!!
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_match_ops(void)
{
    return &ops;
}

