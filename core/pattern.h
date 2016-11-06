#ifndef PATTERN_H
#define PATTERN_H

#include <assert.h>

#include "types.h"
#include "definitions.h"
#include "evaluation.h"

#include <iostream>

class Matcher {
private:
    MatchContext &_context;
    const Symbol *_variable;
    const BaseExpressionRef &_this_pattern;
    const Slice &_next_pattern;
    const Slice &_sequence;

    bool heads(size_t n, const Symbol *head) const;
    bool consume(size_t n) const;
    bool match_variable(size_t match_size, const BaseExpressionRef &item) const;

public:
    inline Matcher(MatchContext &context, const BaseExpressionRef &this_pattern, const Slice &next_pattern, const Slice &sequence) :
        _context(context), _variable(nullptr), _this_pattern(this_pattern), _next_pattern(next_pattern), _sequence(sequence) {
    }

    inline Matcher(MatchContext &context, const Symbol *variable, const BaseExpressionRef &this_pattern, const Slice &next_pattern, const Slice &sequence) :
        _context(context), _variable(variable), _this_pattern(this_pattern), _next_pattern(next_pattern), _sequence(sequence) {
    }

    inline const BaseExpressionRef &this_pattern() const {
        return _this_pattern;
    }

    inline const Slice &sequence() const {
        return _sequence;
    }

    bool operator()(size_t match_size, const Symbol *head) const;

    bool operator()(const Symbol *var, const BaseExpressionRef &pattern) const;

    bool descend() const;

    bool blank_sequence(match_size_t k, const Symbol *head) const;
};

inline Match match(const BaseExpressionRef &patt, const BaseExpressionRef &item, Definitions &definitions) {
    MatchContext context(patt, item, definitions);
    {
        Matcher matcher(context, patt, empty_slice, Slice(item));

        if (patt->match_sequence(matcher)) {
            return Match(true, context);
        } else {
            return Match(); // no match
        }
    }
}

const Symbol *blank_head(ExpressionPtr patt);

class Blank : public Symbol {
public:
    Blank(Definitions *definitions) :
        Symbol(definitions, "System`Blank") {
    }

    virtual match_sizes_t match_num_args_with_head(const Expression *patt) const {
        return std::make_tuple(1, 1);
    }

    virtual bool match_sequence_with_head(const Expression *patt, const Matcher &matcher) const {
        return matcher(1, blank_head(patt));
    }
};

template<int MINIMUM>
class BlankSequenceBase : public Symbol {
public:
    BlankSequenceBase(Definitions *definitions, const char *name) :
        Symbol(definitions, name) {
    }

    virtual match_sizes_t match_num_args_with_head(const Expression *patt) const {
        return std::make_tuple(MINIMUM, MATCH_MAX);
    }

    virtual bool match_sequence_with_head(const Expression *patt, const Matcher &matcher) const {
        return matcher.blank_sequence(MINIMUM, blank_head(patt));
    }
};

class BlankSequence : public BlankSequenceBase<1> {
public:
    BlankSequence(Definitions *definitions) :
        BlankSequenceBase(definitions, "System`BlankSequence") {
    }
};

class BlankNullSequence : public BlankSequenceBase<0> {
public:
    BlankNullSequence(Definitions *definitions) :
        BlankSequenceBase(definitions, "System`BlankNullSequence") {
    }
};

class Pattern : public Symbol {
public:
    Pattern(Definitions *definitions) :
        Symbol(definitions, "System`Pattern") {
    }

    virtual bool match_sequence_with_head(ExpressionPtr patt, const Matcher &matcher) const {
        auto var_expr = patt->_leaves[0].get();
        assert(var_expr->type() == SymbolType);
        // if nullptr -> throw self.error('patvar', expr)
        const Symbol *var = static_cast<const Symbol*>(var_expr);
        return matcher(var, patt->_leaves[1]);
    }

    virtual match_sizes_t match_num_args_with_head(ExpressionPtr patt) const {
        if (patt->_leaves.size() == 2) {
            // Pattern is only valid with two arguments
            return patt->_leaves[1]->match_num_args();
        } else {
            return std::make_tuple(1, 1);
        }
    }
};

class Alternatives : public Symbol {
public:
    Alternatives(Definitions *definitions) :
        Symbol(definitions, "System`Alternatives") {
    }

    virtual match_sizes_t match_num_args_with_head(ExpressionPtr patt) const {
        auto leaves = patt->_leaves;

        if (leaves.size() == 0) {
            return std::make_tuple(1, 1);
        }

        size_t min_p, max_p;
        std::tie(min_p, max_p) = leaves[0]->match_num_args();

        for (auto i = std::begin(leaves) + 1; i != std::end(leaves); i++) {
            size_t min_leaf, max_leaf;
            std::tie(min_leaf, max_leaf) = (*i)->match_num_args();
            max_p = std::max(max_p, max_leaf);
            min_p = std::min(min_p, min_leaf);
        }

        return std::make_tuple(min_p, max_p);
    }
};

class Repeated : public Symbol {
public:
    Repeated(Definitions *definitions) :
        Symbol(definitions, "System`Repeated") {
    }

    virtual match_sizes_t match_num_args_with_head(ExpressionPtr patt) const {
        switch(patt->_leaves.size()) {
            case 1:
                return std::make_tuple(1, MATCH_MAX);
            case 2:
                // TODO inspect second arg
                return std::make_tuple(1, MATCH_MAX);
            default:
                return std::make_tuple(1, 1);
        }
    }
};

#endif
