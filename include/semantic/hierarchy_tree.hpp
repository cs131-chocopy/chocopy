#pragma once
#include <cassert>
#include <map>
#include <string>
#include <vector>
using std::map;
using std::string;
using std::vector;
namespace semantic {
class HierachyTree {
   public:
    map<string, string> superclass_map;
    map<string, int> depth_map;
    map<string, vector<string>> subclasses_map;
    HierachyTree() {
        superclass_map["object"] = "";
        depth_map["object"] = 0;
        add_class("str", "object");
        add_class("int", "object");
        add_class("bool", "object");
    }
    void add_class(const string& class_, const string& super_class) {
        assert(superclass_map.count(class_) == 0);
        assert(superclass_map.contains(super_class));
        superclass_map[class_] = super_class;
        depth_map[class_] = depth_map.at(super_class) + 1;
        subclasses_map[super_class].emplace_back(class_);
    }
    string common_ancestor(string class1, string class2) {
        assert(superclass_map.contains(class1));
        assert(superclass_map.contains(class2));
        auto depth1 = depth_map.at(class1);
        auto depth2 = depth_map.at(class2);
        if (depth1 > depth2) {
            std::swap(class1, class2);
            std::swap(depth1, depth2);
        }
        while (depth1 < depth2) {
            class2 = superclass_map[class2];
            depth2--;
        }
        while (class1 != class2) {
            class1 = superclass_map[class1];
            class2 = superclass_map[class2];
        }
        return class1;
    }
    bool is_superclass(const string& subclass, const string& superclass) {
        if (!superclass_map.contains(subclass) ||
            !superclass_map.contains(superclass))
            return false;
        return common_ancestor(subclass, superclass) == superclass;
    }
};
}  // namespace semantic
