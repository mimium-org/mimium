#include <memory>
#include <vector>
#include <string>
#include <variant>
namespace mimium{
    //https://medium.com/@dennis.luxen/breaking-circular-dependencies-in-recursive-union-types-with-c-17-the-curious-case-of-4ab00cfda10d
    template <typename T> struct recursive_wrapper {
    // construct from an existing object
    recursive_wrapper(T t_) { t.emplace_back(std::move(t_)); }
    // cast back to wrapped type
    operator const T &() const { return t.front(); }
    // store the value
    std::vector<T> t;
    };

    namespace types{
        struct Float{
            std::string toString(){
                return "Float";
            };
        };
        struct String{
            std::string toString(){
                return "String";
            };
        };
        struct Function;
        struct Array;

        using Value = std::variant<std::monostate,Float,String,recursive_wrapper<Function>,recursive_wrapper<Array>>
        struct Function {
            std::vector<Value> arg_types;
            Value ret_type;
            Value getReturnType(){
                return ret_type;
            }
            std::vector<Value>& getArgTypes(){
                return arg_types;
            }
            std::string toString(){
                std::string s = "(";
                for(const auto& v: arg_types){
                    s+= v.toString();
                    if(v != arg_types.back()) s+= " , "
                }
                s+=")->" + ret_type.toString();
            };
        };
        struct Array{
            Value elem_type;
            Array(Value elem):elem_type(elem){

            }
            std::string toString() override{
                return "array["+ elem_type.toString()+ "]";
            }
            Value getElemType(){
                return elem_type;
            }
        };
    }//namespace types
    class TypeEnv{
        std::unoredered_map<std::string,Values> env;

    };
}//namespace mimium