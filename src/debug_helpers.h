#ifndef __DEBUG_HELPERS_H__
#define __DEBUG_HELPERS_H__

void debugMathExpression(CompilationContext* context, MathExpression* math_expression);
void debugMathFactor(CompilationContext* context, MathFactor* factor);

void debugLog(CompilationContext* context, const char* format, ...)
{
    if ( context->debug_mode == 0 ) return;
	char result[1024];

    /* Declare a va_list type variable */
    va_list myargs;

    /* Initialise the va_list variable with the ... after fmt */
    va_start(myargs, format);

    /* Forward the '...' to vprintf */
    vsprintf(result, format, myargs);

    /* Clean up the va_list */
    va_end(myargs);

	printf("%s\n", result);
}

void debugLogNoNewLine(CompilationContext* context, const char* format, ...)
{
    if ( context->debug_mode == 0 ) return;
	char result[1024];

    /* Declare a va_list type variable */
    va_list myargs;

    /* Initialise the va_list variable with the ... after fmt */
    va_start(myargs, format);

    /* Forward the '...' to vprintf */
    vsprintf(result, format, myargs);

    /* Clean up the va_list */
    va_end(myargs);

	printf("%s", result);
}

void debugLogAddNewLine(CompilationContext* context)
{
    if ( context->debug_mode == 0 ) return;
	printf("\n");
}

void debugExpression(CompilationContext* context, Expression* expression)
{
    if ( context->debug_mode == 0 ) return;

    debugMathExpression(context, expression->math_expression);
}

void debugMathExpression(CompilationContext* context, MathExpression* math_expression)
{
    if ( context->debug_mode == 0 ) return;

    debugLogNoNewLine(context, "(");
    debugMathFactor(context, math_expression->factor);

    if ( math_expression->expression != NULL )
    {
        debugLogNoNewLine(context, "%c", math_expression->op);
        debugMathExpression(context, math_expression->expression);
    }

    debugLogNoNewLine(context, ")");
}

void debugMathFactor(CompilationContext* context, MathFactor* factor)
{
    if ( factor->expression != NULL )
    {
        debugExpression(context, factor->expression);
    }
    else
    {
        debugLogNoNewLine(context, "%d", factor->number);
    }
}


#endif

