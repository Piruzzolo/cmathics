#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "types.h"
#include "hash.h"
#include <vector>

class Expression : public BaseExpression {
private:
    BaseExpressionRef evaluate2(const BaseExpressionRef &head, const Slice &leaves, Evaluation &evaluation) const;
    BaseExpressionRef evaluate3(Evaluation &evaluation) const;

public:
    mutable hash_t _computed_hash;

    BaseExpressionRef _head;
    Slice _leaves;  // other options: ropes, skip lists, ...

    inline Expression(BaseExpressionRef head, const Slice &leaves) :
        _head(head),
        _leaves(leaves),
        _computed_hash(0) {
    }

    inline Expression(BaseExpressionRef head, std::initializer_list<BaseExpressionRef> leaves) :
        _head(head),
        _leaves(leaves.begin(), leaves.size()),
        _computed_hash(0) {
    }

    inline Expression(BaseExpressionRef head, const std::vector<BaseExpressionRef> &leaves) :
        _head(head),
        _leaves(&leaves[0], leaves.size()),
        _computed_hash(0) {
    }

    inline const BaseExpressionRef &leaf(size_t i) const {
        return _leaves.leaf(i);
    }

    inline BaseExpressionPtr leaf_ptr(size_t i) const {
        return _leaves.leaf_ptr(i);
    }

    virtual Type type() const {
        return ExpressionType;
    }

    virtual bool same(const BaseExpression &item) const;

    virtual hash_t hash() const;

    virtual std::string fullform() const;

    virtual BaseExpressionRef evaluate(Evaluation &evaluation) const;

    virtual BaseExpressionRef head() const {
        return _head;
    }

    const Slice &leaves() const {
        return _leaves;
    }

    virtual BaseExpressionPtr head_ptr() const {
        return _head.get();
    }

    virtual bool is_sequence() const {
        return _head->is_symbol_sequence();
    }

    virtual match_sizes_t match_num_args() const {
        return _head->match_num_args_with_head(this);
    }

    virtual Match match_sequence(
        const Matcher &matcher) const;
};

inline BaseExpressionRef expression(BaseExpressionRef head, const std::vector<BaseExpressionRef> &leaves) {
    return std::make_shared<Expression>(head, leaves);
}

inline BaseExpressionRef expression(BaseExpressionRef head, const Slice &slice) {
    return std::make_shared<Expression>(head, slice);
}

template<typename... T>
inline BaseExpressionRef expression(BaseExpressionRef head, T... leaves) {
    return std::make_shared<Expression>(head, {leaves...});
}

/*inline operator+(const BaseExpressionRef &a, BaseExpressionRef &b) {
    return expression('Plus', {a, b});
}*/

/*inline Slice get_sequence(BaseExpressionPtr expr) {
    // IMPORTANT: the caller guarantees that the returned Slice lives as
    // long as the given expression pointer "expr"!

    if (expr->is_sequence()) {
        return Slice(static_cast<const Expression*>(expr)->_leaves);
    } else {
        return Slice(expr);
    }
}*/

#endif
