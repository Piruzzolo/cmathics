#ifndef EVALUATION_H
#define EVALUATION_H

#include <stdbool.h>

typedef enum {
    PrintType, MessageType
} OutType;


typedef struct Out{
    OutType type;
    struct Out* next;
} Out;


typedef struct {
    Out base;
    char* text;
} PrintOut;


typedef struct {
    Out base;
    Symbol* symbol;
    char* tag;
    char* text;
} MessageOut;


typedef struct Message {
    Symbol* symbol;
    char* tag;
    struct Message* next;
} Message;


typedef enum {
    NoInterrupt, AbortInterrupt, TimeoutInterrupt, ReturnInterrupt, BreakInterrupt, ContinueInterrupt
} EvaluationInterrupt;


class Definitions;

class Evaluation {
public:
    Definitions &definitions;
    int64_t recursion_depth;
    bool timeout;
    bool stopped;
    // Output* output;
    bool catch_interrupts;
    EvaluationInterrupt interrupt;
    Out* out;

    Evaluation(Definitions &definitions, bool new_catch_interrupts);

    BaseExpressionRef evaluate(BaseExpressionRef expression);
};


void send_message(Evaluation* evaluation, Symbol* symbol, char* tag);
#endif
