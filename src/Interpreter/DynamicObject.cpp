#include "DynamicObject.h"
#include "BlockObject.h"
#include "EvalContext.h"

namespace Finch
{
    void DynamicObject::Trace(std::ostream & stream) const
    {
        stream << mName;
    }
    
    Ref<Object> DynamicObject::Receive(Ref<Object> thisRef, EvalContext & context, 
                                       String message, vector<Ref<Object> > args)
    {
        if (message == "copy")
        {
            return Object::New(thisRef);
        }
        
        if (message == "add-field:value:")
        {
            String      name  = args[0]->AsString();
            Ref<Object> value = args[1];
            
            if (name.size() == 0)
            {
                //### bob: need better error handling
                std::cout << "oops. need a name to add a field." << std::endl;
                return Ref<Object>();
            }
            
            //### bob: should this fail if the field already exists?
            
            // add the field
            mFields[name] = value;
            
            return value;
        }
        
        if (message == "add-method:body:")
        {
            String      name  = args[0]->AsString();
            Ref<Object> value = args[1];
            
            //### bob: need better error handling
            if (name.size() == 0)
            {
                std::cout << "oops. need a name to add a method." << std::endl;
                return Ref<Object>();
            }
            
            if (value->AsBlock() == NULL)
            {
                std::cout << "oops. 'body:' argument must be a block." << std::endl;
                return Ref<Object>();
            }
            
            //### bob: should this fail if the method already exists?
            
            // add the method
            mMethods[name] = value;
            
            return Ref<Object>();
        }
        
        // see if it's a field access
        map<String, Ref<Object> >::iterator found = mFields.find(message);
        if (found != mFields.end())
        {
            return found->second;
        }
        
        // see if it's a field assignment
        if ((args.size() == 1) && (message[message.size() - 1] == ':'))
        {
            String fieldAssign = message.substr(0, message.size() - 1);

            //### bob: copy/pasted from Scope.cpp. need Dict class
            found = mFields.lower_bound(fieldAssign);
            
            Ref<Object> oldValue;
            
            if ((found != mFields.end()) &&
                !(mFields.key_comp()(fieldAssign, found->first)))
            {
                // found it at this scope, so get the old value then replace it
                oldValue = found->second;
                found->second = args[0];
                
                return oldValue;
            }
        }
        
        // see if it's a method call
        found = mMethods.find(message);
        if (found != mMethods.end())
        {
            BlockObject* block = found->second->AsBlock();
            
            ASSERT_NOT_NULL(block);
            
            Ref<Object> result = context.EvaluateMethod(thisRef, block->Body());
            
            return result;
        }
        
        // walk up the prototype chain
        if (!mPrototype.IsNull())
        {
            // we're using thisRef and not the prototype's own reference here
            // on purpose. this way, if you send a "copy" message to some
            // object a few links down the prototype chain from Object, you'll
            // get a copy of *that* object, and not Object itself where "copy"
            // is implemented.
            return mPrototype->Receive(thisRef, context, message, args);
        }
        
        //### bob: should do some sort of message not handled thing here
        return Ref<Object>();
    }
}