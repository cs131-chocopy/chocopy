#pragma once

#include <fmt/format.h>

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "Module.hpp"
#include "Type.hpp"
#include "User.hpp"
#include "Value.hpp"
#include "chocopy_logging.hpp"

namespace lightir {

string print_as_op(Value *v, bool print_ty, const string &method_ = "");
string print_cmp_type(lightir::CmpInst::CmpOp op);

}  // namespace lightir
