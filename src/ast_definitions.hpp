#include <map>

enum AST_ID{
    NUMBER,
    FCALL,
    ASSIGN,
    FDEF,
    ARRAYINIT,
    LAMBDA
};


namespace mimium{
    const std::map<std::string,AST_ID> ast_id = {
        {"fcall",FCALL},
        {"define",ASSIGN},
        {"fdef",FDEF},
        {"array",ARRAYUNIT},
        {"lambda",LAMBDA}
    }
}