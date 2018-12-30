#include "validator.h"

bool _findBinding(Binding* start, Binding* target)
{
    Binding* current = start;

    while ( current != target && current != NULL )
    {
        if ( !strcmp(current->lhs, target->lhs) ) return true;
        current = current->next;
    }

    return false;
}

void _validateUniqueBindingNames(Binding* start)
{
    Binding* current = start;

    while ( current != NULL )
    {
        if ( strcmp(current->lhs, "_") != 0 && _findBinding(start, current) )
        {
            errorLog("Name %s is already used.", current->lhs);
        }

        current = current->next;
    }
}

void _validateUniqueBindingNamesInFunctions(Binding* module_start)
{
    Binding* current = module_start;

    while ( current != NULL )
    {
        if ( current->function_decl != NULL )
        {
            _validateUniqueBindingNames(current->function_decl->first_binding);
        }

        current = current->next;
    }
}

void _extractExpressionType(Binding* binding)
{
    /* Expression* expression = binding->expression; */
    binding->ex_type = 1;
}

void _extractBindingType(Binding* binding)
{
    if ( binding->function_decl == NULL )
    {
        _extractExpressionType(binding);
    }
    else
    {
        //TODO: set binding->ex_type to appropriate type
        Binding* current = binding->function_decl->first_binding;

        while ( current != NULL )
        {
            _extractBindingType(current);
            current = current->next;
        }
    }
}

void _extractBindingTypes(Module* module)
{
    Binding* current = module->first_binding;

    while ( current != NULL )
    {
        _extractBindingType(current);

        current = current->next;
    }
}

void validateModule(Context* context, Module* module)
{
    _validateUniqueBindingNames(module->first_binding);
    _validateUniqueBindingNamesInFunctions(module->first_binding);
    _extractBindingTypes(module);


    /*     //TODO: extract types for all bindings and expressions */
    /*     //TODO: validate type match for expressions (follow compiler steps but check types e.g. shl for float is invalid) */
}
