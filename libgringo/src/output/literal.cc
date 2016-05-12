#include <gringo/output/literal.hh>
#include <gringo/output/statements.hh>

namespace Gringo { namespace Output {

ULit Literal::negateLit(LparseTranslator &x) const {
    ULit aux = x.makeAux();
    ULit lit(clone());
    LRC().addHead(aux).addBody(std::move(lit)).toLparse(x);
    return aux->negateLit(x); // aux literals can negate themselves
}

} } // namespace Output Gringo
