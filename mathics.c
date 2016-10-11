#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/misc.h"
#include "core/expression.h"
#include "core/definitions.h"
#include "core/formatter.h"
#include "core/evaluation.h"
#include "core/pattern.h"


int main() {
    char* buf;
    Definitions* definitions = Definitions_new(64);
    Definitions_init(definitions, NULL);  // System Definitions

    Expression* expr = Expression_new(2);

    BaseExpression* head = (BaseExpression*) Definitions_lookup(definitions, "Plus");
    BaseExpression* la = (BaseExpression*) Definitions_lookup(definitions, "a");
    BaseExpression* lb = (BaseExpression*) Definitions_lookup(definitions, "b");
    BaseExpression* leaves[2];
    leaves[0] = la;
    leaves[1] = lb;
    Expression_init(expr, head, leaves);

    Evaluation* evaluation = Evaluation_new(definitions, true);
    BaseExpression* result = Evaluate(evaluation, (BaseExpression*) expr);

    buf = FullForm((BaseExpression*) result);
    printf("%s\n", buf);

    if (MatchQ((BaseExpression*) expr, (BaseExpression*) expr)) {
        printf("matches!\n");
    }

    printf("height = %i\n", Expression_height((BaseExpression*) expr));
    printf("hash = %lu\n", expr->hash);

    free(buf);
    free(expr);
    Definitions_free(definitions);
    Evaluation_free(evaluation);
    return 0;
}
