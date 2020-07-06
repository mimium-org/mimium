#include "ast_new.hpp"
namespace mimium{

void test(){
    newast::DebugInfo dbg;
    newast::Number num1 = {{dbg},1};
    auto numptr = makeExpr(newast::Number{dbg,1});
    auto strptr = makeExpr(newast::String{dbg,"test!"});
    newast::String str = {dbg,"test"};
    newast::Op opast = {{dbg},numptr,strptr};
}

}