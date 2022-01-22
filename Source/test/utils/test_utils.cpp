#include "purc.h"
#include "private/array_list.h"
#include "private/list.h"
#include "private/avl.h"
#include "private/hashtable.h"
#include "private/map.h"
#include "private/rbtree.h"
#include "private/atom-buckets.h"
#include "private/sorted-array.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#define ATOM_BUCKET     1

static struct atom_info {
    const char *string;
    int         bucket;
    purc_atom_t atom;
} my_atoms[] = {
    /* generic */
    { "HVML", 0, 0 },
    { "PurC", 0, 0 },

    /* HVML tags */
    { "hvml", 1, 0 },
    { "head", 1, 0 },
    { "body", 1, 0 },
    { "archetype", 1, 0 },
    { "archedata", 1, 0 },
    { "error", 1, 0 },
    { "except", 1, 0 },
    { "init", 1, 0 },
    { "update", 1, 0 },
    { "erase", 1, 0 },
    { "clear", 1, 0 },
    { "test", 1, 0 },
    { "match", 1, 0 },
    { "choose", 1, 0 },
    { "iterate", 1, 0 },
    { "reduce", 1, 0 },
    { "sort", 1, 0 },
    { "observe", 1, 0 },
    { "forget", 1, 0 },
    { "fire", 1, 0 },
    { "request", 1, 0 },
    { "connect", 1, 0 },
    { "send", 1, 0 },
    { "disconnect", 1, 0 },
    { "load", 1, 0 },
    { "back", 1, 0 },
    { "define", 1, 0 },
    { "include", 1, 0 },
    { "call", 1, 0 },
    { "return", 1, 0 },
    { "catch", 1, 0 },
    { "bind", 1, 0 },

    /* update actions */
    { "displace", 2, 0 },
    /* { "append", 2, 0 }, */
    /* { "prepend", 2, 0 }, */
    /* { "insertBefore", 2, 0 }, */
    /* { "insertAfter", 2, 0 }, */
};

// to test basic functions of atom
TEST(utils, atom_basic)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_atom_t atom;

    atom = purc_atom_from_static_string(NULL);
    ASSERT_EQ(atom, 0);

    atom = purc_atom_from_string(NULL);
    ASSERT_EQ(atom, 0);

    for (size_t i = 0; i < sizeof(my_atoms)/sizeof(my_atoms[0]); i++) {
        purc_atom_t atom = purc_atom_try_string(my_atoms[i].string);
        ASSERT_EQ(atom, 0);
    }

    for (size_t i = 0; i < sizeof(my_atoms)/sizeof(my_atoms[0]); i++) {
        my_atoms[i].atom = purc_atom_from_string(my_atoms[i].string);
    }

    for (size_t i = 0; i < sizeof(my_atoms)/sizeof(my_atoms[0]); i++) {
        const char *string = purc_atom_to_string(my_atoms[i].atom);
        int cmp = strcmp(string, my_atoms[i].string);
        ASSERT_EQ(cmp, 0);
    }

    purc_cleanup ();
}

// to test extended functions of atom
TEST(utils, atom_ex)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_atom_t atom;

    atom = purc_atom_from_static_string_ex(0, NULL);
    ASSERT_EQ(atom, 0);

    atom = purc_atom_from_string_ex(1, NULL);
    ASSERT_EQ(atom, 0);

    for (int bucket = 1; bucket < PURC_ATOM_BUCKETS_NR; bucket++) {
        for (size_t i = 0; i < sizeof(my_atoms)/sizeof(my_atoms[0]); i++) {
            purc_atom_t atom = purc_atom_try_string_ex(bucket,
                    my_atoms[i].string);
            ASSERT_EQ(atom, 0);
        }
    }

    for (size_t i = 0; i < sizeof(my_atoms)/sizeof(my_atoms[0]); i++) {
        my_atoms[i].atom = purc_atom_from_string_ex(my_atoms[i].bucket,
                my_atoms[i].string);
    }

    for (size_t i = 0; i < sizeof(my_atoms)/sizeof(my_atoms[0]); i++) {
        const char *string = purc_atom_to_string(my_atoms[i].atom);
        int cmp = strcmp(string, my_atoms[i].string);
        ASSERT_EQ(cmp, 0);

        purc_atom_t atom = purc_atom_try_string_ex(my_atoms[i].bucket,
                my_atoms[i].string);
        ASSERT_NE(atom, 0);
    }

    purc_cleanup ();
}

enum {
    ID_EXCEPT_BusError = 0,
    ID_EXCEPT_SegFault,
    ID_EXCEPT_Terminated,
    ID_EXCEPT_CPUTimeLimitExceeded,
    ID_EXCEPT_FileSizeLimitExceeded,
    ID_EXCEPT_BadEncoding,
    ID_EXCEPT_BadHVMLTag,
    ID_EXCEPT_BadHVMLAttrName,
    ID_EXCEPT_BadHVMLAttrValue,
    ID_EXCEPT_BadHVMLContent,
    ID_EXCEPT_BadTargetHTML,
    ID_EXCEPT_BadTargetXGML,
    ID_EXCEPT_BadTargetXML,
    ID_EXCEPT_BadExpression,
    ID_EXCEPT_BadExecutor,
    ID_EXCEPT_BadName,
    ID_EXCEPT_NoData,
    ID_EXCEPT_NotIterable,
    ID_EXCEPT_BadIndex,
    ID_EXCEPT_NoSuchKey,
    ID_EXCEPT_DuplicateKey,
    ID_EXCEPT_ArgumentMissed,
    ID_EXCEPT_WrongDataType,
    ID_EXCEPT_InvalidValue,
    ID_EXCEPT_MaxIterationCount,
    ID_EXCEPT_MaxRecursionDepth,
    ID_EXCEPT_Unauthorized,
    ID_EXCEPT_Timeout,
    ID_EXCEPT_eDOMFailure,
    ID_EXCEPT_LostRenderer,
    ID_EXCEPT_MemoryFailure,
    ID_EXCEPT_InternalFailure,
    ID_EXCEPT_ZeroDivision,
    ID_EXCEPT_Overflow,
    ID_EXCEPT_Underflow,
    ID_EXCEPT_InvalidFloat,
    ID_EXCEPT_AccessDenied,
    ID_EXCEPT_IOFailure,
    ID_EXCEPT_TooSmall,
    ID_EXCEPT_TooMany,
    ID_EXCEPT_TooLong,
    ID_EXCEPT_TooLarge,
    ID_EXCEPT_NotDesiredEntity,
    ID_EXCEPT_EntityNotFound,
    ID_EXCEPT_EntityExists,
    ID_EXCEPT_NoStorageSpace,
    ID_EXCEPT_BrokenPipe,
    ID_EXCEPT_ConnectionAborted,
    ID_EXCEPT_ConnectionRefused,
    ID_EXCEPT_ConnectionReset,
    ID_EXCEPT_NameResolutionFailed,
    ID_EXCEPT_RequestFailed,
    ID_EXCEPT_OSFailure,
    ID_EXCEPT_NotReady,
    ID_EXCEPT_NotImplemented,

    ID_EXCEPT_LAST = ID_EXCEPT_NotImplemented,
};

// to test extended functions of atom
static struct const_str_atom _except_names[] = {
    { "BusError", 0 },

    { "SegFault", 0 },
    { "Terminated", 0 },
    { "CPUTimeLimitExceeded", 0 },
    { "FileSizeLimitExceeded", 0 },

    { "BadEncoding", 0 },
    { "BadHVMLTag", 0 },
    { "BadHVMLAttrName", 0 },
    { "BadHVMLAttrValue", 0 },
    { "BadHVMLContent", 0 },
    { "BadTargetHTML", 0 },
    { "BadTargetXGML", 0 },
    { "BadTargetXML", 0 },
    { "BadExpression", 0 },
    { "BadExecutor", 0 },
    { "BadName", 0 },
    { "NoData", 0 },
    { "NotIterable", 0 },
    { "BadIndex", 0 },
    { "NoSuchKey", 0 },
    { "DuplicateKey", 0 },
    { "ArgumentMissed", 0 },
    { "WrongDataType", 0 },
    { "InvalidValue", 0 },
    { "MaxIterationCount", 0 },
    { "MaxRecursionDepth", 0 },
    { "Unauthorized", 0 },
    { "Timeout", 0 },
    { "eDOMFailure", 0 },
    { "LostRenderer", 0 },
    { "MemoryFailure", 0 },
    { "InternalFailure", 0 },
    { "ZeroDivision", 0 },
    { "Overflow", 0 },
    { "Underflow", 0 },
    { "InvalidFloat", 0 },
    { "AccessDenied", 0 },
    { "IOFailure", 0 },
    { "TooSmall", 0 },
    { "TooMany", 0 },
    { "TooLong", 0 },
    { "TooLarge", 0 },
    { "NotDesiredEntity", 0 },
    { "EntityNotFound", 0 },
    { "EntityExists", 0 },
    { "NoStorageSpace", 0 },
    { "BrokenPipe", 0 },
    { "ConnectionAborted", 0 },
    { "ConnectionRefused", 0 },
    { "ConnectionReset", 0 },
    { "NameResolutionFailed", 0 },
    { "RequestFailed", 0 },
    { "OSFailure", 0 },
    { "NotReady", 0 },
    { "NotImplemented", 0 },
};

#define NR_CUSTOM_ATOMS (sizeof(_except_names)/sizeof(_except_names[0]))

static int is_custom_atom(purc_atom_t atom)
{
    if (atom < _except_names[0].atom ||
            atom > _except_names[NR_CUSTOM_ATOMS - 1].atom)
        return 0;

    return 1;
}

static purc_atom_t get_custom_atom_by_id(size_t id)
{
    if (id < NR_CUSTOM_ATOMS)
        return _except_names[id].atom;

    return 0;
}

TEST(utils, atom_buckets)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_atom_t atom;

    for (size_t n = 0; n < NR_CUSTOM_ATOMS; n++) {
        atom = purc_atom_try_string_ex(ATOM_BUCKET_CUSTOM,
                _except_names[n].str);
        ASSERT_EQ(atom, 0);

        _except_names[n].atom =
            purc_atom_from_static_string_ex(ATOM_BUCKET_CUSTOM,
                _except_names[n].str);
    }

    ret = is_custom_atom(1);
    ASSERT_EQ (ret, 0);

    for (size_t n = 0; n < NR_CUSTOM_ATOMS; n++) {
        ret = is_custom_atom(_except_names[n].atom);
        ASSERT_EQ (ret, 1);
    }

    for (size_t i = 0; i < ID_EXCEPT_LAST; i++) {
        atom = get_custom_atom_by_id(i);
        ASSERT_NE (atom, 0);
    }

    purc_cleanup ();
}

// to test sorted array
static int sortv[10] = { 1, 8, 7, 5, 4, 6, 9, 0, 2, 3 };

static int
intcmp(const void *sortv1, const void *sortv2)
{
    int i = (int)(intptr_t)sortv1;
    int j = (int)(intptr_t)sortv2;

    return i - j;
}

TEST(utils, pcutils_sorted_array_asc)
{
    struct sorted_array *sa;
    int n;

    sa = pcutils_sorted_array_create(SAFLAG_DEFAULT, 4, NULL,
            intcmp);

    n = (int)pcutils_sorted_array_count (sa);
    ASSERT_EQ(n, 0);

    for (int i = 0; i < 10; i++) {
        int ret = pcutils_sorted_array_add (sa, (void *)(intptr_t)sortv[i],
                (void *)(intptr_t)(sortv[i] + 100));
        ASSERT_EQ(ret, 0);
    }

    n = (int)pcutils_sorted_array_count (sa);
    ASSERT_EQ(n, 10);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)pcutils_sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        ASSERT_EQ(sortv, i);
    }

    pcutils_sorted_array_remove (sa, (void *)(intptr_t)0);
    pcutils_sorted_array_remove (sa, (void *)(intptr_t)9);
    pcutils_sorted_array_delete (sa, 0);

    n = (int)pcutils_sorted_array_count (sa);
    ASSERT_EQ(n, 7);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)pcutils_sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        ASSERT_EQ(sortv, i + 2);
    }

    pcutils_sorted_array_destroy(sa);
}

TEST(utils, pcutils_sorted_array_desc)
{
    struct sorted_array *sa;
    int n;

    sa = pcutils_sorted_array_create(SAFLAG_ORDER_DESC, 4, NULL,
            intcmp);

    n = (int)pcutils_sorted_array_count (sa);
    ASSERT_EQ(n, 0);

    for (int i = 0; i < 10; i++) {
        int ret = pcutils_sorted_array_add (sa, (void *)(intptr_t)sortv[i],
                (void *)(intptr_t)(sortv[i] + 100));
        ASSERT_EQ(ret, 0);
    }

    n = (int)pcutils_sorted_array_count (sa);
    ASSERT_EQ(n, 10);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)pcutils_sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        sortv = 9 - sortv;
        ASSERT_EQ(sortv, i);
    }

    pcutils_sorted_array_remove (sa, (void *)(intptr_t)0);
    pcutils_sorted_array_remove (sa, (void *)(intptr_t)9);

    n = (int)pcutils_sorted_array_count (sa);
    ASSERT_EQ(n, 8);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)pcutils_sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        sortv = 8 - sortv;
        ASSERT_EQ(sortv, i);
    }

    bool found;
    found = pcutils_sorted_array_find (sa, (void *)(intptr_t)0, NULL);
    ASSERT_EQ(found, false);
    found = pcutils_sorted_array_find (sa, (void *)(intptr_t)9, NULL);
    ASSERT_EQ(found, false);

    for (int i = 1; i < 9; i++) {
        void *data;
        found = pcutils_sorted_array_find (sa, (void *)(intptr_t)i, &data);

        ASSERT_EQ(found, true);
        ASSERT_EQ((int)(intptr_t)data, i + 100);
    }

    pcutils_sorted_array_destroy(sa);
}

struct node
{
    struct list_head          node;
    int                       v;
};

TEST(utils, list_head)
{
    struct list_head list;
    INIT_LIST_HEAD(&list);

    struct node n1, n2, n3, n4;
    n1.v = 1;
    n2.v = 2;
    n3.v = 3;
    n4.v = 4;

    list_add_tail(&n1.node, &list);
    list_add_tail(&n2.node, &list);
    list_add_tail(&n3.node, &list);
    list_add_tail(&n4.node, &list);

    int v = 1;
    struct list_head *p;
    list_for_each(p, &list) {
        struct node *node;
        node = container_of(p, struct node, node);
        ASSERT_EQ(node->v, v++);
    }
}

TEST(utils, list_head_swap)
{
    struct list_head l,r;
    INIT_LIST_HEAD(&l);
    INIT_LIST_HEAD(&r);

    struct node n1, n2, n3, n4;
    n1.v = 1;
    n2.v = 2;
    n3.v = 3;
    n4.v = 4;

    if (1) {
        list_swap_list(&l, &r);
        ASSERT_TRUE(list_empty(&l));
        ASSERT_TRUE(list_empty(&r));
    }

    if (1) {
        list_add_tail(&n1.node, &l);
        ASSERT_FALSE(list_empty(&l));

        list_swap_list(&l, &r);
        ASSERT_TRUE(list_empty(&l));
        ASSERT_FALSE(list_empty(&r));

        struct list_head *p, *n;
        list_for_each_safe(p, n, &r) {
            struct node *node;
            node = container_of(p, struct node, node);
            list_del(p);
            ASSERT_EQ(node->v, 1);
        }
        ASSERT_TRUE(list_empty(&r));
    }

    if (1) {
        list_add_tail(&n1.node, &l);
        list_add_tail(&n2.node, &l);
        list_add_tail(&n3.node, &l);
        list_add_tail(&n4.node, &l);

        int v = 1;
        struct list_head *p;
        list_for_each(p, &l) {
            struct node *node;
            node = container_of(p, struct node, node);
            ASSERT_EQ(node->v, v++);
        }

        list_swap_list(&l, &r);
        ASSERT_TRUE(list_empty(&l));
        ASSERT_FALSE(list_empty(&r));

        v = 1;
        struct list_head *n;
        list_for_each_safe(p, n, &r) {
            struct node *node;
            node = container_of(p, struct node, node);
            list_del(p);
            ASSERT_EQ(node->v, v++);
        }
        ASSERT_TRUE(list_empty(&r));
    }
}

struct _avl_node {
    struct avl_node          node;
    size_t                   key;
    size_t                   val;
};
static struct _avl_node* _make_avl_node(int key, int val)
{
    struct _avl_node *p;
    p = (struct _avl_node*)calloc(1, sizeof(*p));
    if (!p) return nullptr;
    p->key = key;
    p->val = val;
    p->node.key = &p->key;
    return p;
}

static int _avl_tree_comp(const void *k1, const void *k2, void *ptr)
{
    (void)ptr;
    int delta = (*(size_t*)k1) - (*(size_t*)k2);
    return delta;
}

static int _get_random(int max)
{
    static int seeded = 0;
    if (!seeded) {
        srand(time(0));
        seeded = 1;
    }

    if (max==0)
        return 0;

    return (max<0) ? rand() : rand() % max;
}

TEST(avl, init)
{
    struct avl_tree avl;
    pcutils_avl_init(&avl, _avl_tree_comp, false, NULL);
    int r;
    int count = 10240;
    for (int i=0; i<count; ++i) {
        size_t key = _get_random(-1);
        if (pcutils_avl_find(&avl, &key)) {
            --i;
            continue;
        }
        struct _avl_node *p = _make_avl_node(key, _get_random(0));
        r = pcutils_avl_insert(&avl, &p->node);
        ASSERT_EQ(r, 0);
    }
    struct _avl_node *p, *tmp;
    int i = 0;
    size_t prev;
    avl_remove_all_elements(&avl, p, node, tmp) {
        if (i>0) {
            ASSERT_GT(p->key, prev);
        }
        prev = p->key;
        free(p);
    }
}

/* test arrlist.double_free */
static size_t _arrlist_items_free = 0;
static void _arrlist_item_free(void *data)
{
    free(data);
    ++_arrlist_items_free;
}

TEST(arrlist, double_free)
{
    int t;
    // reset
    _arrlist_items_free = 0;

    struct pcutils_arrlist *al = pcutils_arrlist_new_ex(_arrlist_item_free, 3);

    char *s1 = strdup("hello");
    t = pcutils_arrlist_put_idx(al, 0, s1);
    ASSERT_EQ(t, 0);
    // double-free intentionally
    t = pcutils_arrlist_put_idx(al, 0, s1);
    ASSERT_EQ(t, 0);

    pcutils_arrlist_free(al);

    // test check
    ASSERT_EQ(_arrlist_items_free, 1);
}

/* test hashtable.double_free */
static size_t _hash_table_items_free = 0;
static void _hash_table_item_free(pchash_entry *e)
{
    free(pchash_entry_k(e));
    free(pchash_entry_v(e));
    ++_hash_table_items_free;
}

TEST(hashtable, double_free)
{
    int t;
    // reset
    _hash_table_items_free = 0;

    struct pchash_table *ht = pchash_kchar_table_new(3, _hash_table_item_free);

    const char *k1 = "hello";
    t = pchash_table_insert(ht, strdup(k1), strdup(k1));
    ASSERT_EQ(t, 0);

    struct pchash_entry *e = pchash_table_lookup_entry(ht, k1);
    EXPECT_NE(e, nullptr);
    const char *kk = (const char*)pchash_entry_k(e);
    ASSERT_NE(k1, kk);
    EXPECT_STREQ(k1, kk);
    ASSERT_EQ(0, strcmp(k1, kk));
    pchash_table_free(ht);

    // test check
    ASSERT_EQ(_hash_table_items_free, 1);
}

struct string_s {
    struct list_head      list;
    char                 *s;
};

struct strings_s {
    struct list_head      head;
};

TEST(utils, list)
{
    struct strings_s     strings;
    INIT_LIST_HEAD(&strings.head);
    for (int i=0; i<10; ++i) {
        struct string_s *str = (struct string_s*)malloc(sizeof(*str));
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", i+1);
        str->s = strdup(buf);
        list_add_tail(&str->list, &strings.head);
    }

    struct list_head *p, *n;
    int i = 0;
    list_for_each(p, &strings.head) {
        struct string_s *str = list_entry(p, struct string_s, list);
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", ++i);
        EXPECT_STREQ(buf, str->s);
    }

    i = 0;
    list_for_each_safe(p, n, &strings.head) {
        struct string_s *str = list_entry(p, struct string_s, list);
        n = str->list.next;
        list_del_init(&str->list);
        free(str->s);
        free(str);
    }
}

struct name_s {
    struct avl_node   node;
    char             *s;
};

struct names_s {
    struct avl_tree   root;
};

static int _avl_cmp (const void *k1, const void *k2, void *ptr) {
    const char *s1 = (const char*)k1;
    const char *s2 = (const char*)k2;
    (void)ptr;
    return strcmp(s1, s2);
}

TEST(utils, avl)
{
    struct names_s names;
    pcutils_avl_init(&names.root, _avl_cmp, false, nullptr);
    for (int i=0; i<10; ++i) {
        struct name_s *name = (struct name_s*)malloc(sizeof(*name));
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", i+1);
        name->s = strdup(buf);
        name->node.key = name->s;
        int t = pcutils_avl_insert(&names.root, &name->node);
        ASSERT_EQ(t, 0);
    }

    struct name_s *name;
    avl_for_each_element(&names.root, name, node) {
        fprintf(stderr, "%s\n", name->s);
    }

    name = avl_find_element(&names.root, "9", name, node);
    ASSERT_NE(name, nullptr);
    ASSERT_STREQ((const char*)name->node.key, "9");

    struct name_s *ptr;
    avl_for_each_element_safe(&names.root, name, node, ptr) {
        ptr = avl_next_element(name, node);
        free(name->s);
        free(name);
    }
}

struct str_node {
    struct rb_node  node;
    const char     *str;
};

static int
cmp(struct rb_node *node, void *ud)
{
    struct str_node *p = container_of(node, struct str_node, node);
    const char *k = (const char*)ud;
    return strcmp(k, p->str);
}

static struct rb_node*
new_entry(void *ud)
{
    struct str_node *node;
    node = (struct str_node*)calloc(1, sizeof(*node));
    if (!node)
        return NULL;

    const char *str = (const char*)ud;
    node->str = str;

    return &node->node;
}

static inline void
do_insert(struct rb_root *root, const char *str, bool *ok)
{
    int r;
    r = pcutils_rbtree_insert(root, (void*)str, cmp, new_entry);
    *ok = r == 0 ? true : false;
}

TEST(utils, rbtree)
{
    const char *samples[] = {
        "hello",
        "world",
        "foo",
        "bar",
        "great",
        "wall",
    };
    const char *results[] = {
        "bar",
        "foo",
        "great",
        "hello",
        "wall",
        "world",
    };

    bool ok = true;

    struct rb_root root = RB_ROOT;
    struct rb_node *node;
    node = pcutils_rbtree_first(&root);
    ASSERT_EQ(node, nullptr);

    for (size_t i=0; i<PCA_TABLESIZE(samples); ++i) {
        const char *sample = samples[i];
        do_insert(&root, sample, &ok);
        if (!ok)
            break;
    }

    node = pcutils_rbtree_first(&root);
    for (; node; node = pcutils_rbtree_next(node)) {
        struct str_node *p = container_of(node, struct str_node, node);
        ASSERT_NE(p, nullptr);
    }

    node = pcutils_rbtree_first(&root);
    struct rb_node *next;
    const char **pp = results;
    for (; ({node && (next = pcutils_rbtree_next(node)); node;}); node=next) {
        struct str_node *p = container_of(node, struct str_node, node);
        pcutils_rbtree_erase(node, &root);
        ASSERT_STREQ(p->str, *pp++);
        free(p);
    }

    ASSERT_TRUE(ok);
}

static inline int
map_cmp(const void *key1, const void *key2)
{
    const char *s1 = (const char*)key1;
    const char *s2 = (const char*)key2;
    return strcmp(s1, s2);
}

static inline int
map_visit(void *key, void *val, void *ud)
{
    (void)ud;

    int r;
    r = strcmp((const char*)key, "name");
    if (r)
        return r;

    size_t v = (size_t)val;
    if (v < 12)
        return -1;
    if (v == 12)
        return 0;
    return 1;
}

TEST(utils, map)
{
    pcutils_map *map;
    map = pcutils_map_create(NULL, NULL, NULL, NULL,
            map_cmp, false);
    ASSERT_NE(map, nullptr);

    pcutils_map_entry *entry;
    int r;

    r = pcutils_map_insert(map, "name", (const void*)(size_t)1);
    ASSERT_EQ(r, 0);
    entry = pcutils_map_find(map, "name");
    ASSERT_NE(entry, nullptr);
    ASSERT_EQ((const char*)entry->key, "name");
    ASSERT_EQ((size_t)entry->val, 1);

    r = pcutils_map_insert(map, "name", (const void*)(size_t)12);
    ASSERT_EQ(r, 0);
    entry = pcutils_map_find(map, "name");
    ASSERT_NE(entry, nullptr);
    ASSERT_EQ((const char*)entry->key, "name");
    ASSERT_EQ((size_t)entry->val, 12);

    r = pcutils_map_traverse(map, NULL, map_visit);
    pcutils_map_destroy(map);
    ASSERT_EQ(r, 0);
}

struct array_list_sample_node {
    struct pcutils_array_list_node          node;
    int                                     val;
};

TEST(utils, array_list_append)
{
    const int samples[] = {
        1,2,3,4,5,6,7,8,9
    };
    int r;
    struct pcutils_array_list arrlist;

    r = pcutils_array_list_init(&arrlist);
    ASSERT_EQ(r, 0);

    for (size_t i=0; i<PCA_TABLESIZE(samples); ++i) {
        int sample = samples[i];
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)calloc(1, sizeof(*node));
        ASSERT_NE(node, nullptr);
        node->val = sample;
        r = pcutils_array_list_append(&arrlist, &node->node);
        ASSERT_EQ(r, 0);
    }

    const int checks[] = {1,2,3,4,5,6,7,8,9};
    r = 0;
    struct pcutils_array_list_node *p;
    array_list_for_each(&arrlist, p) {
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)container_of(p,
                struct array_list_sample_node, node);

        ASSERT_EQ(node->val, checks[r++]);
    }

    struct pcutils_array_list_node *n;
    array_list_for_each_safe(&arrlist, p, n) {
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)container_of(p,
                struct array_list_sample_node, node);

        struct pcutils_array_list_node *old;
        r = pcutils_array_list_remove(&arrlist, node->node.idx, &old);
        ASSERT_EQ(r, 0);
        ASSERT_EQ(old, &node->node);

        free(node);
    }

    ASSERT_EQ(pcutils_array_list_length(&arrlist), 0);

    pcutils_array_list_reset(&arrlist);
}

TEST(utils, array_list_prepend)
{
    const int samples[] = {
        1,2,3,4,5,6,7,8,9
    };
    int r;
    struct pcutils_array_list arrlist;

    r = pcutils_array_list_init(&arrlist);
    ASSERT_EQ(r, 0);

    for (size_t i=0; i<PCA_TABLESIZE(samples); ++i) {
        int sample = samples[i];
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)calloc(1, sizeof(*node));
        ASSERT_NE(node, nullptr);
        node->val = sample;
        r = pcutils_array_list_prepend(&arrlist, &node->node);
        ASSERT_EQ(r, 0);
    }

    const int checks[] = {9,8,7,6,5,4,3,2,1};
    r = 0;
    struct pcutils_array_list_node *p;
    array_list_for_each(&arrlist, p) {
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)container_of(p,
                struct array_list_sample_node, node);

        ASSERT_EQ(node->val, checks[r++]);
    }

    struct pcutils_array_list_node *n;
    array_list_for_each_safe(&arrlist, p, n) {
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)container_of(p,
                struct array_list_sample_node, node);

        struct pcutils_array_list_node *old;
        r = pcutils_array_list_remove(&arrlist, node->node.idx, &old);
        ASSERT_EQ(r, 0);
        ASSERT_EQ(old, &node->node);

        free(node);
    }

    ASSERT_EQ(pcutils_array_list_length(&arrlist), 0);

    pcutils_array_list_reset(&arrlist);
}

TEST(utils, array_list_set)
{
    const int samples[] = {
        1,2,3,4,5,6,7,8,9
    };
    int r;
    struct pcutils_array_list arrlist;

    r = pcutils_array_list_init(&arrlist);
    ASSERT_EQ(r, 0);

    for (size_t i=0; i<PCA_TABLESIZE(samples); ++i) {
        int sample = samples[i];
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)calloc(1, sizeof(*node));
        ASSERT_NE(node, nullptr);
        node->val = sample;
        r = pcutils_array_list_insert_before(&arrlist, 5, &node->node);
        ASSERT_EQ(r, 0);
    }

    const int checks[] = {1,2,3,4,5,9,8,7,6};
    r = 0;
    struct pcutils_array_list_node *p;
    array_list_for_each(&arrlist, p) {
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)container_of(p,
                struct array_list_sample_node, node);

        ASSERT_EQ(node->val, checks[r++]);
    }

    struct pcutils_array_list_node *n;
    array_list_for_each_safe(&arrlist, p, n) {
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)container_of(p,
                struct array_list_sample_node, node);

        struct pcutils_array_list_node *old;
        r = pcutils_array_list_remove(&arrlist, node->node.idx, &old);
        ASSERT_EQ(r, 0);
        ASSERT_EQ(old, &node->node);

        free(node);
    }

    ASSERT_EQ(pcutils_array_list_length(&arrlist), 0);

    pcutils_array_list_reset(&arrlist);
}

TEST(utils, array_list_append_reverse)
{
    const int samples[] = {
        1,2,3,4,5,6,7,8,9
    };
    int r;
    struct pcutils_array_list arrlist;

    r = pcutils_array_list_init(&arrlist);
    ASSERT_EQ(r, 0);

    for (size_t i=0; i<PCA_TABLESIZE(samples); ++i) {
        int sample = samples[i];
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)calloc(1, sizeof(*node));
        ASSERT_NE(node, nullptr);
        node->val = sample;
        r = pcutils_array_list_append(&arrlist, &node->node);
        ASSERT_EQ(r, 0);
    }

    const int checks[] = {9,8,7,6,5,4,3,2,1};
    r = 0;
    struct pcutils_array_list_node *p;
    array_list_for_each_reverse(&arrlist, p) {
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)container_of(p,
                struct array_list_sample_node, node);

        ASSERT_EQ(node->val, checks[r++]);
    }

    struct pcutils_array_list_node *n;
    array_list_for_each_reverse_safe(&arrlist, p, n) {
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)container_of(p,
                struct array_list_sample_node, node);

        struct pcutils_array_list_node *old;
        r = pcutils_array_list_remove(&arrlist, node->node.idx, &old);
        ASSERT_EQ(r, 0);
        ASSERT_EQ(old, &node->node);

        free(node);
    }

    ASSERT_EQ(pcutils_array_list_length(&arrlist), 0);

    pcutils_array_list_reset(&arrlist);
}

TEST(utils, array_list_append_entry)
{
    const int samples[] = {
        1,2,3,4,5,6,7,8,9
    };
    int r;
    struct pcutils_array_list arrlist;

    r = pcutils_array_list_init(&arrlist);
    ASSERT_EQ(r, 0);

    for (size_t i=0; i<PCA_TABLESIZE(samples); ++i) {
        int sample = samples[i];
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)calloc(1, sizeof(*node));
        ASSERT_NE(node, nullptr);
        node->val = sample;
        r = pcutils_array_list_append(&arrlist, &node->node);
        ASSERT_EQ(r, 0);
    }

    const int checks[] = {1,2,3,4,5,6,7,8,9};
    r = 0;
    struct array_list_sample_node *p;
    array_list_for_each_entry(&arrlist, p, node) {
        ASSERT_EQ(p->val, checks[r++]);
    }

    struct array_list_sample_node *n;
    array_list_for_each_entry_safe(&arrlist, p, n, node) {
        struct pcutils_array_list_node *old;
        r = pcutils_array_list_remove(&arrlist, p->node.idx, &old);
        ASSERT_EQ(r, 0);
        ASSERT_EQ(old, &p->node);

        free(p);
    }

    ASSERT_EQ(pcutils_array_list_length(&arrlist), 0);

    pcutils_array_list_reset(&arrlist);
}

TEST(utils, array_list_append_entry_reverse)
{
    const int samples[] = {
        1,2,3,4,5,6,7,8,9
    };
    int r;
    struct pcutils_array_list arrlist;

    r = pcutils_array_list_init(&arrlist);
    ASSERT_EQ(r, 0);

    for (size_t i=0; i<PCA_TABLESIZE(samples); ++i) {
        int sample = samples[i];
        struct array_list_sample_node *node;
        node = (struct array_list_sample_node*)calloc(1, sizeof(*node));
        ASSERT_NE(node, nullptr);
        node->val = sample;
        r = pcutils_array_list_append(&arrlist, &node->node);
        ASSERT_EQ(r, 0);
    }

    const int checks[] = {9,8,7,6,5,4,3,2,1};
    r = 0;
    struct array_list_sample_node *p;
    array_list_for_each_entry_reverse(&arrlist, p, node) {
        ASSERT_EQ(p->val, checks[r++]);
    }

    struct array_list_sample_node *n;
    array_list_for_each_entry_reverse_safe(&arrlist, p, n, node) {
        struct pcutils_array_list_node *old;
        r = pcutils_array_list_remove(&arrlist, p->node.idx, &old);
        ASSERT_EQ(r, 0);
        ASSERT_EQ(old, &p->node);

        free(p);
    }

    ASSERT_EQ(pcutils_array_list_length(&arrlist), 0);

    pcutils_array_list_reset(&arrlist);
}

