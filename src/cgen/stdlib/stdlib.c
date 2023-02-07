#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct object {
    int class_tag;
    int object_size;
    void* dispatch_table;
};

void object_init() asm("$object.__init__");
void object_init() {}

struct int_object {
    int class_tag;
    int object_size;
    void* dispatch_table;
    int value;
};

struct str_object {
    int class_tag;
    int object_size;
    void* dispatch_table;
    int length;
    char* value;
};

struct list_object {
    int class_tag;
    int object_size;
    void* dispatch_table;
    int length;
    struct object** value;
};

// alloc a new object with the same as obj_prototype
struct object* alloc_object(struct object* obj_prototype) {
    struct object* obj = (struct object*)malloc(obj_prototype->object_size * 4);
    memcpy(obj, obj_prototype, obj_prototype->object_size * 4);
    return obj;
}

struct int_object* makeint(int v) {
    struct int_object* int_prototype;
    __asm__ __volatile__("la %0, $int$prototype" : "=r"(int_prototype));
    struct int_object* int_obj =
        (struct int_object*)alloc_object((struct object*)int_prototype);
    int_obj->value = v;
    return int_obj;
};

struct int_object* makebool(bool v) {
    struct int_object* bool_prototype;
    __asm__ __volatile__("la %0, $bool$prototype" : "=r"(bool_prototype));
    struct int_object* bool_obj =
        (struct int_object*)alloc_object((struct object*)bool_prototype);
    bool_obj->value = v;
    return bool_obj;
}

struct str_object* makestr(char c) {
    struct str_object* str_prototype;
    __asm__ __volatile__("la %0, $str$prototype" : "=r"(str_prototype));
    struct str_object* str_obj =
        (struct str_object*)alloc_object((struct object*)str_prototype);
    str_obj->length = 1;
    str_obj->value = (char*)malloc(2);
    str_obj->value[0] = c;
    str_obj->value[1] = '\0';
    return str_obj;
}

// return true if s1 == s2
bool str_object_eq(struct str_object* s1, struct str_object* s2) {
    return strcmp(s1->value, s2->value) == 0;
}

// return true if s1 != s2
bool str_object_neq(struct str_object* s1, struct str_object* s2) {
    return strcmp(s1->value, s2->value) != 0;
}

// return s1 + s2
struct str_object* str_object_concat(struct str_object* s1,
                                     struct str_object* s2) {
    struct str_object* str_obj =
        (struct str_object*)alloc_object((struct object*)s1);
    str_obj->length = s1->length + s2->length;
    str_obj->value = (char*)malloc(str_obj->length + 1);
    strcpy(str_obj->value, s1->value);
    strcat(str_obj->value, s2->value);
    return str_obj;
}

int len(struct object*) asm("$len");
int len(struct object* obj) {
    if (obj == NULL) goto invalid;
    if (obj->class_tag == 3)  // str
        return ((struct str_object*)obj)->length;
    if (obj->class_tag == -1)  // list
        return ((struct list_object*)obj)->length;
invalid:
    printf("Invalid argument\n");
    exit(1);
}

struct str_object* input() asm("$input");
struct str_object* input() {
    struct str_object* str_prototype;
    __asm__ __volatile__("la %0, $str$prototype" : "=r"(str_prototype));
    struct str_object* str_obj =
        (struct str_object*)alloc_object((struct object*)str_prototype);
    str_obj->value = (char*)malloc(128);
    if (fgets(str_obj->value, 128, stdin) == NULL) {
        str_obj->value[0] = '\0';
        str_obj->length = 0;
    } else {
        str_obj->length = strlen(str_obj->value);
        if (str_obj->length > 0 && str_obj->value[str_obj->length - 1] == '\n')
            str_obj->value[--str_obj->length] = '\0';
    }
    return str_obj;
}

void print(struct object* obj) {
    if (obj == NULL) goto invalid;
    if (obj->class_tag == 1) {  // int
        printf("%d\n", ((struct int_object*)obj)->value);
        return;
    }
    if (obj->class_tag == 3) {  // str
        char* s = ((struct str_object*)obj)->value;
        if (s)
            printf("%s\n", ((struct str_object*)obj)->value);
        else
            printf("\n");
        return;
    }
    if (obj->class_tag == 2) {  // bool
        if (((struct int_object*)obj)->value)
            printf("True\n");
        else
            printf("False\n");
        return;
    }
invalid:
    printf("Invalid argument\n");
    exit(1);
}

struct list_object* construct_list(int n, ...) {
    struct list_object* list_prototype;
    __asm__ __volatile__("la %0, $.list$prototype" : "=r"(list_prototype));
    struct list_object* list_obj =
        (struct list_object*)alloc_object((struct object*)list_prototype);
    list_obj->length = n;
    list_obj->value = (struct object**)malloc(n * sizeof(struct object*));

    va_list ptr;
    va_start(ptr, n);
    for (int i = 0; i < n; i++)
        list_obj->value[i] = va_arg(ptr, struct object*);
    va_end(ptr);

    return list_obj;
}

struct list_object* concat_list(struct list_object* l1,
                                struct list_object* l2) {
    struct list_object* list_prototype;
    __asm__ __volatile__("la %0, $.list$prototype" : "=r"(list_prototype));
    struct list_object* list_obj =
        (struct list_object*)alloc_object((struct object*)list_prototype);
    if (l1 == NULL || l2 == NULL) {
        printf("Operation on None\n");
        exit(4);
    }
    list_obj->length = l1->length + l2->length;
    list_obj->value =
        (struct object**)malloc(list_obj->length * sizeof(struct object*));
    memcpy(list_obj->value, l1->value, l1->length * sizeof(struct object*));
    memcpy(list_obj->value + l1->length, l2->value,
           l2->length * sizeof(struct object*));
    return list_obj;
}

void error_Div() asm("error.Div");
void error_Div() {
    printf("Division by zero\n");
    exit(2);
}

void error_OOB() asm("error.OOB");
void error_OOB() {
    printf("Index out of bounds\n");
    exit(3);
}

void error_None() asm("error.None");
void error_None() {
    printf("Operation on None\n");
    exit(4);
}
