struct str_object {
    int class_tag;
    int object_size;
    void* dispatch_table;
    int length;
    char* value;
};

struct str_object* strcat(struct str_object* s1, struct str_object* s2) {
    struct str_object* ret = malloc(s1->object_size * 4);
    ret->class_tag = s1->class_tag;
    ret->object_size = s1->object_size;
    ret->dispatch_table = s1->dispatch_table;
    ret->length = s1->length + s2->length;
    ret->value = malloc(ret->length + 1);
    for (int i = 0; i < s1->length; i++) {
        ret->value[i] = s1->value[i];
    }
    for (int i = 0; i < s2->length; i++) {
        ret->value[i + s1->length] = s2->value[i];
    }
    return ret;
}
