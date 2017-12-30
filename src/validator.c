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
        if ( strcmp(current->lhs, "-") != 0 && findBinding(module, current) )
        {
            errorLog("Name %s is already used.", current->lhs);
        }

        current = current->next;
    }
}
