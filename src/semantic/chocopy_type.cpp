#include <FunctionDefType.hpp>
#include <ValueType.hpp>

namespace semantic {
bool FunctionDefType::operator==(const FunctionDefType &f2) const {
    std::vector<SymbolType *> *a = this->params;
    std::vector<SymbolType *> *b = f2.params;
    if (a->size() == b->size()) {
        for (int i = 1; i < a->size(); i++) {
            if (a->at(i)->get_name() != b->at(i)->get_name()) return false;
        }
        if (this->return_type->get_name() != f2.return_type->get_name())
            return false;
        return true;
    } else
        return false;
}
}  // namespace semantic
