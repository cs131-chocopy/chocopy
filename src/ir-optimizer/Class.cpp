#include "Class.hpp"

#include <utility>

#include "GlobalVariable.hpp"

namespace lightir {
void Class::add_method(Function *func) {
    auto func_exist = [&func](Function *func_) {
        auto a = func_->get_name();
        auto b = func->get_name();
        return a.substr(a.find('.')) == b.substr(b.find('.'));
    };
    auto idx = std::find_if(this->methods_->begin(), this->methods_->end(),
                            func_exist);
    if (idx != this->methods_->end()) {
        (*this->methods_)[std::distance(this->methods_->begin(), idx)] = func;
        return;
    }
    this->methods_->push_back(func);
}

Class::Class(Module *m, const string &name_, int type_tag,
             Class *super_class_info, bool with_dispatch_table_,
             bool is_dispatch_table_, bool is_append)
    : Type(static_cast<type>(type_tag), m),
      Value(this, name_),
      class_name_(name_),
      super_class_info_(super_class_info),
      type_tag_(type_tag),
      print_dispatch_table_(is_dispatch_table_) {
    prototype_label_ = fmt::format("${}${}", name_, "prototype");
    if (with_dispatch_table_) {
        dispatch_table_label_ = fmt::format("${}${}", name_, "dispatchTable");
    }
    attributes_ = new vector<AttrInfo *>();
    methods_ = new vector<Function *>();
    set_type(
        LabelType::get(prototype_label_ + "_type", this, this->get_module()));
    bool flag = false;
    for (auto &&c : m->get_class())
        if (c->type_tag_ == type_tag) flag = true;
    if (!flag && is_append) m->add_class(this);
}

Class::Class(Module *m, const string &name_, bool anon_)
    : Type(static_cast<type>(-7), m),
      Value(this, name_),
      class_name_(name_),
      super_class_info_(nullptr),
      anon_(anon_) {
    attributes_ = new vector<AttrInfo *>();
    methods_ = new vector<Function *>();
    m->add_class(this);
}

/** First print out the type name */
string Class::print_class() {
    string const_ir;

    // simple struct
    if (anon_) {
        /** %class.anon = type { i32* } */
        const_ir += fmt::format("%$class.anon_{} = type", class_name_) + " {";
        for (auto &&attr : *this->attributes_) {
            const_ir += fmt::format("  {}", attr->print());
            if (attr != this->attributes_->at(this->attributes_->size() - 1)) {
                const_ir += ",\n";
            } else {
                const_ir += "\n";
            }
        }
        const_ir += "}\n";

        return const_ir;
    }

    // print the type of the prototype
    const_ir += fmt::format(
        "%{}_type = type {{\n"
        "  i32,\n"
        "  i32,\n"
        "  ptr",
        this->prototype_label_);
    for (auto &attr : *this->attributes_)
        const_ir += fmt::format(",\n  {}", attr->print());
    const_ir += "\n}\n";

    // print the prototype
    const_ir += fmt::format(
        "@{} = global %{}_type {{\n"
        "  i32 {},\n"
        "  i32 {},\n"
        "  ptr @{}",
        prototype_label_, prototype_label_, type_tag_, 3 + attributes_->size(),
        dispatch_table_label_);
    for (auto &attr : *attributes_) {
        if (attr->get_type()->is_integer_type() ||
            attr->get_type()->is_bool_type()) {
            assert(attr->init_obj == 0);
            const_ir +=
                fmt::format(",\n  {} {}", attr->print(), attr->init_val);
        } else if (attr->init_obj == nullptr) {
            const_ir += fmt::format(",\n  {} null", attr->print());
        } else if (dynamic_cast<GlobalVariable *>(attr->init_obj)) {
            const_ir += fmt::format(",\n  {} @{}", attr->print(),
                                    attr->init_obj->get_name());
        } else {
            assert(0);
        }
    }
    const_ir += "\n}\n";

    // print the type of the dispatch table
    assert(methods_->size() > 0);
    const_ir += fmt::format(
        "%{}_type = type {{\n"
        "{}"
        "\n}}\n",
        dispatch_table_label_,
        fmt::join(vector<string>(methods_->size(), "  ptr"s), ",\n"));

    // print the dispatch table
    const_ir += fmt::format(
        "@{0} = global %{0}_type {{\n"
        "  ptr @{1}",
        dispatch_table_label_, methods_->front()->get_name());
    for (size_t i = 1; i < methods_->size(); i++)
        const_ir += fmt::format(",\n  ptr @{}", methods_->at(i)->get_name());
    const_ir += "\n}\n";
    return const_ir;
}

int Class::get_method_offset(string method) const {
    return std::distance(methods_->begin(),
                         std::find_if(methods_->begin(), methods_->end(),
                                      [&method](Function *tmp) {
                                          auto &a = tmp->name_;
                                          return a.substr(a.find('.') + 1) ==
                                                 method;
                                      }));
}

string AttrInfo::print() {
    string const_ir;
    const_ir += this->get_type()->print();
    return const_ir;
}

List::List(Class *list_class, vector<Value *> contained, const string &name)
    : Class(list_class->get_module(), name, type::LIST,
            list_class->super_class_info_, true, false),
      contained_(std::move(contained)) {}

List *List::get(Class *list_class, vector<Value *> contained,
                const string &name) {
    return new List(list_class, std::move(contained), name);
}

string List::print() {
    string list_ir;
    list_ir +=
        fmt::format("{} = global %$.list$prototype_type ", this->name_) + "{\n";
    list_ir += "  i32 -1,\n  i32 5,\n  %$unionl.type {";
    /** One prototype, one value */
    if (this->contained_.at(0) == nullptr) {
    }
    return list_ir;
}
}  // namespace lightir
