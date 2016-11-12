#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "types.h"
#include "hash.h"
#include "symbol.h"
#include "operations.h"
#include "string.h"
#include "leaves.h"
#include "structure.h"

#include <sstream>
#include <vector>

template<typename T>
class AllOperationsImplementation :
    virtual public OperationsInterface,
    virtual public OperationsImplementation<T>,
    public ArithmeticOperationsImplementation<T>,
	public StructureOperationsImplementation<T> {
public:
    inline AllOperationsImplementation() {
    }
};

template<typename Slice>
class ExpressionImplementation :
	public Expression, public AllOperationsImplementation<ExpressionImplementation<Slice>> {
public:
	template<typename F>
	ExpressionRef apply(
		const BaseExpressionRef &head,
		size_t begin,
		size_t end,
		const F &f,
		TypeMask type_mask) const;

	virtual ExpressionRef slice(index_t begin, index_t end = INDEX_MAX) const;

	ExpressionRef evaluate_head_and_leaves(const Evaluation &evaluation) const {
		// Evaluate the head

		auto head = _head;

		while (true) {
			auto new_head = head->evaluate(head, evaluation);
			if (new_head) {
				head = new_head;
			} else {
				break;
			}
		}

		// Evaluate the leaves from left to right (according to Hold values).

		if (head->type() != SymbolType) {
			if (head != _head) {
				return Heap::Expression(head, _leaves);
			} else {
				return RefsExpressionRef();
			}
		}

		const auto head_symbol = static_cast<const Symbol *>(head.get());
		const auto attributes = head_symbol->attributes;

		// only one type of Hold attribute can be set at a time
		assert(count(
			attributes,
			Attributes::HoldFirst +
			Attributes::HoldRest +
			Attributes::HoldAll +
			Attributes::HoldAllComplete) <= 1);

		size_t eval_leaf_start, eval_leaf_stop;

		if (attributes & Attributes::HoldFirst) {
			eval_leaf_start = 1;
			eval_leaf_stop = _leaves.size();
		} else if (attributes & Attributes::HoldRest) {
			eval_leaf_start = 0;
			eval_leaf_stop = 1;
		} else if (attributes & Attributes::HoldAll) {
			// TODO flatten sequences
			eval_leaf_start = 0;
			eval_leaf_stop = 0;
		} else if (attributes & Attributes::HoldAllComplete) {
			// no more evaluation is applied
			if (head != _head) {
				return Heap::Expression(head, _leaves);
			} else {
				return RefsExpressionRef();
			}
		} else {
			eval_leaf_start = 0;
			eval_leaf_stop = _leaves.size();
		}

		assert(0 <= eval_leaf_start);
		assert(eval_leaf_start <= eval_leaf_stop);
		assert(eval_leaf_stop <= _leaves.size());

		return apply(
			head,
			eval_leaf_start,
			eval_leaf_stop,
			[&evaluation](const BaseExpressionRef &leaf) {
				return leaf->evaluate(leaf, evaluation);
			},
            MakeTypeMask(ExpressionType) | MakeTypeMask(SymbolType));
	}

	virtual BaseExpressionRef evaluate_values(const ExpressionRef &self, const Evaluation &evaluation) const {
		const auto head = _head;

		// Step 3
		// Apply UpValues for leaves
		// TODO

		// Step 4
		// Apply SubValues
		if (head->type() == ExpressionType) {
			auto head_head = boost::static_pointer_cast<const Expression>(head)->_head.get();
			if (head_head->type() == SymbolType) {
				auto head_symbol = static_cast<const Symbol *>(head_head);

				for (const Rule &rule : head_symbol->sub_rules) {
					auto child_result = rule(self, evaluation);
					if (child_result) {
						return child_result;
					}
				}
			}
		}

		// Step 5
		// Evaluate the head with leaves. (DownValue)
		if (head->type() == SymbolType) {
			auto head_symbol = static_cast<const Symbol *>(head.get());

			for (const Rule &rule : head_symbol->down_rules) {
				auto child_result = rule(self, evaluation);
				if (child_result) {
					return child_result;
				}
			}
		}

		return BaseExpressionRef();
	}

public:
	const Slice _leaves;  // other options: ropes, skip lists, ...

	inline ExpressionImplementation(const BaseExpressionRef &head, const Slice &slice) :
        Expression(head),
        _leaves(slice) {
		assert(head);
	}

	inline ExpressionImplementation(const BaseExpressionRef &head) :
		Expression(head) {
		assert(head);
	}

	inline ExpressionImplementation(ExpressionImplementation<Slice> &&expr) :
		Expression(expr._head), _leaves(expr.leaves) {
	}

	virtual BaseExpressionRef leaf(size_t i) const {
		return _leaves[i];
	}

	inline auto leaves() const {
		return _leaves.leaves();
	}

	template<typename T>
	inline auto primitives() const {
		return _leaves.template primitives<T>();
	}

	inline auto begin() const {
		return _leaves.begin();
	}

	inline auto end() const {
		return _leaves.begin();
	}

	virtual size_t size() const {
		return _leaves.size();
	}

	virtual TypeMask type_mask() const {
		return _leaves.type_mask();
	}

	virtual bool same(const BaseExpression &item) const {
		if (this == &item) {
			return true;
		}
		if (item.type() != ExpressionType) {
			return false;
		}
		const Expression *expr = static_cast<const Expression *>(&item);

		if (!_head->same(expr->head())) {
			return false;
		}

		const size_t size = _leaves.size();
		if (size != expr->size()) {
			return false;
		}
		for (size_t i = 0; i < size; i++) {
			if (!_leaves[i]->same(expr->leaf(i))) {
				return false;
			}
		}

		return true;
	}

	virtual hash_t hash() const {
		hash_t result = hash_combine(result, _head->hash());
		for (auto leaf : _leaves.leaves()) {
			result = hash_combine(result, leaf->hash());
		}
		return result;
	}

	virtual std::string fullform() const {
		std::stringstream result;

		// format head
		result << _head->fullform();
		result << "[";

		// format leaves
		const size_t argc = _leaves.size();

		for (size_t i = 0; i < argc; i++) {
			result << _leaves[i]->fullform();

			if (i < argc - 1) {
				result << ", ";
			}
		}
		result << "]";

		return result.str();
	}

	virtual BaseExpressionRef evaluated_form(const BaseExpressionRef &self, const Evaluation &evaluation) const {
		// returns empty BaseExpressionRef() if nothing changed

		// Step 1, 2
		const auto step2 = evaluate_head_and_leaves(evaluation);

		// Step 3, 4, ...
		BaseExpressionRef step3;
		if (step2) {
			step3 = step2->evaluate_values(step2, evaluation);
		} else {
			const auto expr = boost::static_pointer_cast<const Expression>(self);
			step3 = evaluate_values(expr, evaluation);
		}

		if (step3) {
			return step3;
		} else {
			return step2;
		}
	}

	virtual BaseExpressionRef replace_all(const Match &match) const {
		return apply(
			_head->replace_all(match),
            0,
            _leaves.size(),
            [&match](const BaseExpressionRef &leaf) {
                return leaf->replace_all(match);
            },
            MakeTypeMask(ExpressionType) | MakeTypeMask(SymbolType));
}

	virtual BaseExpressionRef clone() const {
		return Heap::Expression(_head, _leaves);
	}

	virtual BaseExpressionRef clone(const BaseExpressionRef &head) const {
		return Heap::Expression(head, _leaves);
	}

	virtual match_sizes_t match_num_args() const {
		return _head->match_num_args_with_head(this);
	}

	virtual RefsExpressionRef to_refs_expression(const BaseExpressionRef &self) const;

	virtual bool match_leaves(MatchContext &_context, const BaseExpressionRef &patt) const;

	virtual size_t unpack(BaseExpressionRef &unpacked, const BaseExpressionRef *&leaves) const;

	virtual SliceTypeId slice_type_id() const {
		return _leaves.type_id();
	}

	virtual const Symbol *lookup_name() const {
		return _head->lookup_name();
	}
};

template<typename U>
inline PackExpressionRef<U> expression(const BaseExpressionRef &head, const PackSlice<U> &slice) {
	return Heap::Expression(head, slice);
}

template<typename E, typename T>
std::vector<T> collect(const std::vector<BaseExpressionRef> &leaves) {
	std::vector<T> values;
	values.reserve(leaves.size());
	for (auto leaf : leaves) {
		values.push_back(boost::static_pointer_cast<const E>(leaf)->value);
	}
	return values;
}

template<typename T>
inline ExpressionRef tiny_expression(const BaseExpressionRef &head, const T &leaves) {
	const auto size = leaves.size();
	switch (size) {
		case 0: // e.g. {}
			return Heap::EmptyExpression0(head);
		case 1:
			return Heap::Expression(head, InPlaceRefsSlice<1>(leaves, OptionalTypeMask()));
		case 2:
			return Heap::Expression(head, InPlaceRefsSlice<2>(leaves, OptionalTypeMask()));
		case 3:
			return Heap::Expression(head, InPlaceRefsSlice<3>(leaves, OptionalTypeMask()));
		default:
			throw std::runtime_error("whoever called us should have known better.");
	}
}

inline ExpressionRef expression(
	const BaseExpressionRef &head,
	std::vector<BaseExpressionRef> &&leaves) {
	// we expect our callers to move their leaves vector to us. if you cannot move, you
	// should really recheck your design at the site of call.

	if (leaves.size() < 4) {
		return tiny_expression(head, leaves);
	} else {
		const auto type_mask = calc_type_mask(leaves);
		switch (type_mask) {
			case MakeTypeMask(MachineIntegerType):
				return expression(head, PackSlice<machine_integer_t>(
					collect<MachineInteger, machine_integer_t>(leaves)));
			case MakeTypeMask(MachineRealType):
				return expression(head, PackSlice<machine_real_t>(
					collect<MachineReal, machine_real_t>(leaves)));
			case MakeTypeMask(StringType):
				return expression(head, PackSlice<std::string>(
					collect<String, std::string>(leaves)));
			default:
				return Heap::Expression(head, RefsSlice(leaves, type_mask));
		}
	}
}

inline ExpressionRef expression(
	const BaseExpressionRef &head,
	const std::initializer_list<BaseExpressionRef> &leaves) {

	if (leaves.size() < 4) {
		return tiny_expression(head, leaves);
	} else {
		return Heap::Expression(head, RefsSlice(leaves, OptionalTypeMask()));
	}
}

inline ExpressionRef expression(const BaseExpressionRef &head, const RefsSlice &slice) {
	return Heap::Expression(head, slice);
}

template<size_t N>
inline ExpressionRef expression(const BaseExpressionRef &head, const InPlaceRefsSlice<N> &slice) {
    return Heap::Expression(head, slice);
}

class heap_storage {
private:
	std::vector<BaseExpressionRef> _leaves;

public:
	inline heap_storage(size_t size) {
		_leaves.reserve(size);
	}

	inline heap_storage &operator<<(const BaseExpressionRef &expr) {
		_leaves.push_back(expr);
		return *this;
	}

	inline heap_storage &operator<<(BaseExpressionRef &&expr) {
		_leaves.push_back(expr);
		return *this;
	}

	inline ExpressionRef to_expression(const BaseExpressionRef &head) {
		return expression(head, std::move(_leaves));
	}
};

template<size_t N>
class direct_storage {
protected:
	InPlaceExpressionRef<N> _expr;
	BaseExpressionRef *_addr;
	BaseExpressionRef *_end;

public:
	inline direct_storage(const BaseExpressionRef &head) :
		_expr(Heap::Expression(head, InPlaceRefsSlice<N>())),
		_addr(_expr->_leaves.late_init()), _end(_addr + N) {
	}

	inline direct_storage &operator<<(const BaseExpressionRef &expr) {
		assert(_addr < _end);
		*_addr++ = expr;
		return *this;
	}

	inline direct_storage &operator<<(BaseExpressionRef &&expr) {
		assert(_addr < _end);
		*_addr++ = expr;
		return *this;
	}

	inline ExpressionRef to_expression() {
		return _expr;
	}
};

template<size_t N, typename F>
inline ExpressionRef tiny_expression(const BaseExpressionRef &head, const F &generate) {
	direct_storage<N> storage(head);
	generate(storage);
	return storage.to_expression();
}

template<typename F>
inline ExpressionRef expression(const BaseExpressionRef &head, const F &generate, size_t size) {
	heap_storage storage(size);
	generate(storage);
	return storage.to_expression(head);
}

template<typename Slice>
template<typename F>
ExpressionRef ExpressionImplementation<Slice>::apply(
	const BaseExpressionRef &head,
	size_t begin,
	size_t end,
	const F &f,
	TypeMask type_mask) const {

	const auto &slice = _leaves;

	if ((type_mask & _leaves.type_mask()) != 0) {
		/*if (_extent.use_count() == 1) {
			// FIXME. optimize this case. we do not need to copy here.
		}*/

		for (size_t i0 = begin; i0 < end; i0++) {
			const auto &leaf = slice[i0];

			if ((leaf->type_mask() & type_mask) == 0) {
				continue;
			}

			const auto leaf0 = f(leaf);

			if (leaf0) { // copy is needed now
				const BaseExpressionRef &new_head = head ? head : _head;
				const size_t size = slice.size();

				auto generate_leaves = [i0, end, size, type_mask, &slice, &f, &leaf0] (auto storage) {
					for (size_t j = 0; j < i0; j++) {
						storage << slice[j];
					}

					storage << leaf0; // leaves[i0]

					for (size_t j = i0 + 1; j < end; j++) {
						const auto &old_leaf = slice[j];

						if ((old_leaf->type_mask() & type_mask) == 0) {
							storage << old_leaf;
						} else {
							auto new_leaf = f(old_leaf);
							if (new_leaf) {
								storage << new_leaf;
							} else {
								storage << old_leaf;
							}
						}
					}

					for (size_t j = end; j < size; j++) {
						storage << slice[j];
					}

				};

				switch (size) {
					case 1:
						return tiny_expression<1>(new_head, generate_leaves);
					case 2:
						return tiny_expression<2>(new_head, generate_leaves);
					case 3:
						return tiny_expression<3>(new_head, generate_leaves);
					default:
						return expression(new_head, generate_leaves, size);
				}
			}
		}
	}

	if (head == _head) {
		return RefsExpressionRef(); // no change
	} else {
		return expression(head ? head : _head, slice);
	}
}

template<typename Slice>
ExpressionRef ExpressionImplementation<Slice>::slice(index_t begin, index_t end) const {
	return expression(_head, _leaves.slice(begin, end));
}

template<typename Slice>
size_t ExpressionImplementation<Slice>::unpack(
	BaseExpressionRef &unpacked, const BaseExpressionRef *&leaves) const {

	const auto &slice = _leaves;
	if (slice.is_packed()) {
		auto expr = Heap::Expression(_head, slice.unpack());
		unpacked = expr;
		leaves = expr->_leaves.refs();
		return expr->_leaves.size();
	} else {
		leaves = slice.refs();
		return slice.size();
	}
}

template<typename Slice>
RefsExpressionRef ExpressionImplementation<Slice>::to_refs_expression(const BaseExpressionRef &self) const {
	if (std::is_same<Slice, RefsSlice>()) {
		return boost::static_pointer_cast<RefsExpression>(self);
	} else {
		std::vector<BaseExpressionRef> leaves;
		leaves.reserve(_leaves.size());
		for (auto leaf : _leaves.leaves()) {
			leaves.push_back(leaf);
		}
		return Heap::Expression(_head, RefsSlice(std::move(leaves), _leaves.type_mask()));

	}
}

#include "arithmetic_implementation.h"
#include "structure_implementation.h"

#endif
