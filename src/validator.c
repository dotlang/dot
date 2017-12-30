#include "validator.h"

bool findBinding(Module* module, Binding* target)
{
    Binding* current = module->first_binding;

    while ( current != target && current != NULL )
    {
        if ( !strcmp(current->lhs, target->lhs) ) return true;
        current = current->next;
    }

    return false;
}

void validateModule(Context* context, Module* module)
{
    Binding* current = module->first_binding;

    while ( current != NULL )
    {
        //TODO: extract this as a method
        if ( strcmp(current->lhs, "-") != 0 && findBinding(module, current) )
        {
            errorLog("Name %s is already used.", current->lhs);
        }

        //TODO: do this validation for function bindings' internal bindings
        //TODO: extract types for all bindings and expressions
        //TODO: validate type match for expressions (follow compiler steps but check types e.g. shl for float is invalid)

        current = current->next;
    }
}
