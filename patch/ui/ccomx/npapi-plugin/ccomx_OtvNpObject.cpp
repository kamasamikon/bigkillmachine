/**
 * @file     ccomx_NPObject.cpp
 * @author   OS-PresEngine@nemotv.com
 * @date     Aug 22, 2011
 * @brief	 XCOMX CNPNObject class - base class for all scriptable NP Objects.
 * @defgroup XCOMX CNPNObject
 * @ingroup  XCOMX
 */

#include "ccomx_NpObj.h"
#include "ccomx_npn_funcs.h"
#include "ccomx_utils.h"
#include "ccomx_NPPPlugin.h"

#define EVENT_LISTENER_ERROR_NONE                           0
#define EVENT_LISTENER_ERROR_UNSUPPORTED                    1
#define EVENT_LISTENER_ERROR_BAD_ARG                        2
#define REMOVE_EVENT_LISTENER_ERROR_HANDLER_NOT_REGISTERED  3
#define REMOVE_EVENT_LISTENER_ERROR_EVENT_NOT_REGISTERED    4
namespace ccomx {

int32_t NpObj::m_uniqueHandle = 0;
int32_t NpObj::m_uniqueEventHandlerID = 0;

#ifdef LOG_NTVNP_OBJECTS
map<string,NpObj::OtvNP_RTInfo> NpObj::m_all_objs;
unsigned int NpObj::m_totalNumObjs = 0;
#endif

NPClass NpObj::m_NPClass =
{
    NP_CLASS_STRUCT_VERSION,	/* uint32_t structVersion; */
    NpObj::NPAllocate,		/*NPAllocateFunctionPtr allocate; */
    NpObj::NPDeallocate,	/*NPDeallocateFunctionPtr deallocate;*/
    NpObj::NPInvalidate,	/*NPInvalidateFunctionPtr invalidate;*/
    NpObj::NPHasMethod,		/*NPHasMethodFunctionPtr hasMethod;*/
    NpObj::NPInvoke,		/*NPInvokeFunctionPtr invoke;*/
    NpObj::NPInvokeDefault,	/*NPInvokeDefaultFunctionPtr invokeDefault;*/
    NpObj::NPHasProperty,	/*NPHasPropertyFunctionPtr hasProperty;*/
    NpObj::NPGetProperty,	/*NPGetPropertyFunctionPtr getProperty;*/
    NpObj::NPSetProperty,	/*NPSetPropertyFunctionPtr setProperty;*/
    NpObj::NPRemoveProperty,/*NPRemovePropertyFunctionPtr removeProperty;*/
    NULL,						/*NPEnumerationFunctionPtr enumerate;*/
    NULL						/* NPConstructFunctionPtr construct;*/
};

NpObj::NpObj(NPP nppInst,const char* otvClassName):
    m_NPP(nppInst),
    m_OtvClassName(otvClassName)
{
    O_DEBUG_ASSERT(otvClassName);
    O_DEBUG_ASSERT(strlen(otvClassName) > 0);

    signal_list_head = NULL;
    signal_list_tail = NULL;
    signal_handler_list_head = NULL;
    signal_handler_list_tail = NULL;
    method_list_head = NULL;
    method_list_tail = NULL;
    property_list_head = NULL;
    property_list_tail = NULL;

    m_signal_handler_list_total = 0;

    /* Calling NPN_CreateObject() will effectively call NpObj::NPAllocate() */
    m_NPObject = (NPObjectExt *)NPN_CreateObject(m_NPP, &m_NPClass);
    m_NPObject->pNpObj = this;

#ifdef LOG_NTVNP_OBJECTS
    m_totalNumObjs++;
    map<string,OtvNP_RTInfo>::iterator it;
    it = m_all_objs.find(m_OtvClassName);

    if(it != m_all_objs.end())
    {
        it->second.aliveTotal++;
        it->second.createdTotal++;
    }
    else
    {
        OtvNP_RTInfo newObjInfo;
        newObjInfo.aliveTotal = 1;
        newObjInfo.createdTotal = 1;

        m_all_objs.insert(pair<string,OtvNP_RTInfo>(m_OtvClassName,newObjInfo));
    }
#endif
}

#ifdef LOG_NTVNP_OBJECTS
void
NpObj::logAllObjects()
{
    O_LOG_STATUS_AND_PRINT("-------------[ ccomx run-time object table ]--------------\n");
    O_LOG_STATUS_AND_PRINT("    class         active objects     total objects\n");
    O_LOG_STATUS_AND_PRINT("----------------------------------------------------------\n");

    map<string,OtvNP_RTInfo>::iterator it;

    for ( it=m_all_objs.begin(); it != m_all_objs.end(); it++ )
    {
        O_LOG_STATUS_AND_PRINT("%-25s%-d%20d\n", (*it).first.c_str(), (*it).second.aliveTotal, (*it).second.createdTotal);
    }
    O_LOG_STATUS_AND_PRINT("----------------------------------------------------------\n");
    PrintTrackedMemory();
}
#endif


void NpObj::CleanupSignalHanderList(void)
{
    while (signal_handler_list_head)
    {
    	_o_atom_event_t    *temp;
    	temp = signal_handler_list_head->next;

        if(signal_handler_list_head->pHandlerObject)
            NPN_ReleaseObject(signal_handler_list_head->pHandlerObject);

    	delete signal_handler_list_head;
    	signal_handler_list_head = temp;
    }
    signal_handler_list_tail = NULL;
    m_signal_handler_list_total = 0;
}

void NpObj::CleanupSignalList(void)
{
    while (signal_list_head)
    {
    	_o_atom_signal_t    *temp;
    	temp = signal_list_head->next;

    	if(signal_list_head->args)
            delete [] signal_list_head->args;

    	delete signal_list_head;
    	signal_list_head = temp;
    }
    signal_list_tail = NULL;

}

void NpObj::CleanupMethodList(void)
{
    while (method_list_head)
    {
    	_o_atom_method_t    *temp;
    	temp = method_list_head->next;

    	if(method_list_head->args)
            delete [] method_list_head->args;

    	delete method_list_head;
    	method_list_head = temp;
    }
    method_list_tail = NULL;

}

void NpObj::CleanupPropertyList(void)
{
    while (property_list_head)
    {
    	_o_atom_property_t    *temp;
    	temp = property_list_head->next;
    	delete property_list_head;
    	property_list_head = temp;
    }
    property_list_tail = NULL;
}

NpObj::~NpObj(void)
{
    /* The destructor must always be called on the Browser thread since it is accessing m_NPObject object
       which is also accessed by the Browser in NpObj::NPDeallocate() call */
    O_LOG_TRACE("NpObj::~NpObj(\"%s\")\n",NPIdentifierStr(getJSObjectName()).c_str());

    if(m_NPObject)
        m_NPObject->pNpObj = NULL; /* if we are deleting ourselves, we need to NULL the pointer to this object in
                                          the NPObject struct */

    /* NOTE: there is no NPN_ReleaseObject(m_NPObject) here since the ownership of refcount is handled by the users of the object*/

    CleanupSignalHanderList();
    CleanupSignalList();
    CleanupMethodList();
    CleanupPropertyList();

#ifdef LOG_NTVNP_OBJECTS
    map<string,OtvNP_RTInfo>::iterator it;
    it = m_all_objs.find(m_OtvClassName);

    if(it != m_all_objs.end())
    {
        it->second.aliveTotal--;
    }
    else
    {
        O_DEBUG_ASSERT(0);
    }
#endif
}

void
NpObj::Invalidate(NPObject *npobj)
{
    UNUSED_PARAM(npobj);
}

bool
NpObj::InvokeDefault(NPObject *npobj,
                           const NPVariant *args,
                           uint32_t argCount,
                           NPVariant *result)
{
    UNUSED_PARAM(npobj);
    UNUSED_PARAM(args);
    UNUSED_PARAM(argCount);
    UNUSED_PARAM(result);

    O_LOG_WARNING("NpObj::InvokeDefault is called for %s\n", NPIdentifierStr(getJSObjectName()).c_str());

    return false;
}

bool
NpObj::RemoveProperty(NPIdentifier name)
{
    UNUSED_PARAM(name);
    return false;
}

bool
NpObj::HasMethod(NPObject *npobj, NPIdentifier name)
{
    UNUSED_PARAM(npobj);

    _o_atom_method_t *node = method_list_head;

    if ((name == NPID(ADD_SIGNAL_LISTENER_ID))
     || (name == NPID(REMOVE_SIGNAL_LISTENER_ID)))
        return true;

    O_LOG_TRACE("%s::HasMethod is called for %s\n",(const char*)NPIdentifierStr(m_objectInfo.jsObjectName),
                                                   (const char*)NPIdentifierStr(name));

    while (node) {
        if (node->method == name) {
#ifndef NDEBUG
            O_LOG_TRACE("NpObj::HasMethod returning true\n");
#endif
            return true;
        }
        node = node->next;
    }

#ifdef __K_NHLOG_H__
    if ("nemo_use_klog") {
        char *utf8name = (char*)NPN_UTF8FromIdentifier(name);
        if (utf8name && !strcmp(utf8name, "klog")) {
            NPN_MemFree(utf8name);
            return true;
        }
        if (utf8name)
            NPN_MemFree(utf8name);
    }
#endif
    return false;
}

bool
NpObj::HasProperty(NPIdentifier name)
{
    _o_atom_property_t *node = property_list_head;

    while (node) {
        if (node->propertyName == name) {
#ifndef NDEBUG
    O_LOG_TRACE("%s::HasProperty is called for %s: returning true\n", (const char*)NPIdentifierStr(m_objectInfo.jsObjectName),
                                                                      (const char*)NPIdentifierStr(name));
#endif
            return true;
        }
        node = node->next;
    }

#ifndef NDEBUG
    O_LOG_TRACE("%s::HasProperty is called for %s returning false \n", (const char*)NPIdentifierStr(m_objectInfo.jsObjectName),
                                                                       (const char*)NPIdentifierStr(name));
#endif
    return false;
}


NPObject*
NpObj::NPAllocate(NPP npp, NPClass *aClass)
{
    UNUSED_PARAM(npp);
    UNUSED_PARAM(aClass);

    return (NPObject*)(new(std::nothrow) NPObjectExt);
}

/** This function is called by NPAPI framework when the NP Object
*   is being destroyed. This happens when all outstanding references
*   to the object are released.
* \param[in] npobj - NPObject being deleted
* \returns void
* \par Context
* Browser Main thread
* \par SideEffect
* Destroys unerlying OtvNPObject
*/
void
NpObj::NPDeallocate(NPObject *npobj)
{
    if( ((NPObjectExt*)npobj)->pNpObj )
    {
        O_LOG_TRACE("NpObj::NPDeallocate(%p) - deallocating NPObject before NpObj destructor\n", npobj);

        /* Since we are deleting the npobj structure attached to NpObj via m_NPObject data member,
           we need to set it to NULL to notify the  NpObj */
        ((NPObjectExt*)npobj)->pNpObj->m_NPObject = NULL;

        // delete the underlying OtvNPObject
        if(((NPObjectExt*)npobj)->pNpObj->ScheduleForDelete())
        {
            O_LOG_TRACE("NpObj::NPDeallocate() - the NpObj is scheduled to be destroyed\n");
        }
        else
            delete ((NPObjectExt*)npobj)->pNpObj; /* OtvNPObject does not require a delayed deletion so we simply delete it */
    }
    else
    {
        // underlying OtvNPObject has already been deleted so nothing to do
        O_LOG_TRACE("NpObj::NPDeallocate(%p) - deallocating an NPObject after NpObj destructor\n", npobj);
    }
    delete npobj;
}

void
NpObj::NPInvalidate(NPObject *npobj)
{
    if( ((NPObjectExt*)npobj)->pNpObj )
        ((NPObjectExt*)npobj)->pNpObj->Invalidate(npobj);
}

bool
NpObj::NPHasMethod(NPObject *npobj, NPIdentifier name)
{
    if( ((NPObjectExt*)npobj)->pNpObj )
        return ((NPObjectExt*)npobj)->pNpObj->HasMethod(npobj, name);
    else
    {
        O_LOG_WARNING("NpObj::NPHasMethod is called for deleted NpObj\n");
        return false;
    }
}

bool
NpObj::NPInvoke(NPObject *npobj, NPIdentifier name,
                      const NPVariant *args, uint32_t argCount,
                      NPVariant *result)
{
#ifdef __K_NHLOG_H__
    if ("nemo_use_klog") {
        char *utf8name = (char*)NPN_UTF8FromIdentifier(name);
        if (utf8name && !strcmp(utf8name, "klog")) {
            int stringlen = (int)NPVARIANT_TO_STRING(*args).UTF8Length;
            char *handlerName = new(std::nothrow)char[stringlen + 1];
            memcpy(handlerName,(char *)(NPVARIANT_TO_STRING(*args).UTF8Characters),stringlen);
            handlerName[stringlen] = 0;

            NHLOG_CHK_AND_CALL(NHLOG_INFO, 'L', "UI", __FILE__, __func__, __LINE__, "%s\n", handlerName);

            NPN_MemFree(utf8name);
            return true;
        }
        if (utf8name)
            NPN_MemFree(utf8name);
    }
#endif

    if( ((NPObjectExt*)npobj)->pNpObj )
    {
#ifdef LOG_XCOMX_API
        /* don't log 'XCOM' object methods since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
            O_LOG_XCOMX_API("%-25s%s.%s(%s)\n", "[XCOMX API Method Start]",
                    NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                    NPIdentifierStr(name).c_str(),
                    NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, args, argCount));
#endif
        if (!result)
        {
            O_LOG_ERROR("pointer of result NPVariant is null.!\n");
            O_DEBUG_ASSERT(0);
            return false;
        }
        VOID_TO_NPVARIANT(*result);
        ((NPObjectExt*)npobj)->pNpObj->Invoke(npobj,
            name,
            args,
            argCount,
            result);
#ifdef LOG_XCOMX_API
        /* don't log 'XCOM' object methods since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
            O_LOG_XCOMX_API("%-25s%s.%s(%s)\n", "[XCOMX API Method End]",
                    NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                    NPIdentifierStr(name).c_str(),
                    NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, args, argCount));
#endif

        return true;
    }
    else
    {
        O_LOG_WARNING("NpObj::NPInvoke is called for deleted NpObj\n");
        return false;
    }
}

    bool
NpObj::NPInvokeDefault(NPObject *npobj, const NPVariant *args,
        uint32_t argCount, NPVariant *result)
{
    if( ((NPObjectExt*)npobj)->pNpObj )
    {
        bool retval;
#ifdef LOG_XCOMX_API
        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
            O_LOG_XCOMX_API("%-25s%s.%s(%s)\n", "[XCOMX API Method Start]",
                    NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                    "InvokeDefault",
                    NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, args, argCount));
#endif
        retval = ((NPObjectExt*)npobj)->pNpObj->InvokeDefault(npobj,
                args,
                argCount,
                result);
#ifdef LOG_XCOMX_API
        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
            O_LOG_XCOMX_API("%-25s%s.%s(%s)\n", "[XCOMX API Method End]",
                    NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                    "InvokeDefault",
                    NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, args, argCount));
#endif
        return retval;
    }
    else
    {
        O_LOG_WARNING("NpObj::NPInvokeDefault is called for deleted NpObj\n");
        return false;
    }
}

    bool
NpObj::NPHasProperty(NPObject * npobj, NPIdentifier name)
{
    bool rc = false;

#ifdef __K_NHLOG_H__
    if ("nemo_use_klog") {
        char *utf8name = (char*)NPN_UTF8FromIdentifier(name);
        if (utf8name && !strcmp(utf8name, "klog")) {
            NPN_MemFree(utf8name);
            return false;
        }
        if (utf8name)
            NPN_MemFree(utf8name);
    }
#endif

    if( ((NPObjectExt*)npobj)->pNpObj )
    {
#ifdef LOG_XCOMX_API
        int32_t index;

        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
        {
            if((index = NPGetArrayIndexFromNPIdentifier(name)) == -1)
            {
                O_LOG_TRACE("%-25s%s.%s\n","[XCOMX API HasProperty Start] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        NPIdentifierStr(name).c_str());
            }
            else
            {
                O_LOG_TRACE("%-25s%s.[%d]\n","[XCOMX API HasProperty Start] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        index);
            }
        }
#endif
        if ((rc = ((NPObjectExt*)npobj)->pNpObj->HasProperty(name)) == false)
        {
            O_LOG_TRACE("[NO PROPERTY] Failed to has the property '%s' for %s with error=%s\n",
                    NPIdentifierStr(name).c_str(),
                    NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                    "UnknownError");
        }
#ifdef LOG_XCOMX_API
        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
        {
            if((index = NPGetArrayIndexFromNPIdentifier(name)) == -1)
            {
                O_LOG_TRACE("%-25s%s.%s\n","[XCOMX API HasProperty End] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        NPIdentifierStr(name).c_str());
            }
            else
            {
                O_LOG_TRACE("%-25s%s.[%d]\n","[XCOMX API HasProperty End] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        index);
            }
        }
#endif
    }
    else
    {
        O_LOG_WARNING("NpObj::NPHasProperty is called for deleted NpObj\n");
    }
    return rc;
}

    bool
NpObj::NPGetProperty(NPObject *npobj, NPIdentifier name,
        NPVariant *result)
{
    if( ((NPObjectExt*)npobj)->pNpObj )
    {
#ifdef LOG_XCOMX_API
        int32_t index;

        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
        {
            if((index = NPGetArrayIndexFromNPIdentifier(name)) == -1)
            {
                O_LOG_XCOMX_API("%-25s%s.%s\n","[XCOMX API GetProperty Start] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        NPIdentifierStr(name).c_str());
            }
            else
            {
                O_LOG_XCOMX_API("%-25s%s.[%d]\n","[XCOMX API GetProperty Start] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        index);
            }
        }
#endif
        if (result)
            VOID_TO_NPVARIANT(*result);
        if (((NPObjectExt*)npobj)->pNpObj->GetProperty(name,result) == false)
        {
            O_LOG_ERROR("Failed to get the property '%s' for %s with error=%s\n",
                    NPIdentifierStr(name).c_str(),
                    NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                    "UnknownError");
            ((NPObjectExt*)npobj)->pNpObj->SetException("UnknownError");
        }
#ifdef LOG_XCOMX_API
        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
        {
            if((index = NPGetArrayIndexFromNPIdentifier(name)) == -1)
            {
                O_LOG_XCOMX_API("%-25s%s.%s = %s\n","[XCOMX API GetProperty End] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        NPIdentifierStr(name).c_str(),
                        NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, result, 1));
            }
            else
            {
                O_LOG_XCOMX_API("%-25s%s.[%d] = %s\n","[XCOMX API GetProperty End] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        index,
                        NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, result, 1));
            }
        }
#endif
        return true;
    }
    else
    {
        O_LOG_WARNING("NpObj::NPGetProperty is called for deleted NpObj\n");
        return false;
    }
}

    bool
NpObj::NPSetProperty(NPObject *npobj, NPIdentifier name,
        const NPVariant *value)
{
    if( ((NPObjectExt*)npobj)->pNpObj )
    {
#ifdef LOG_XCOMX_API
        int32_t index;

        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
        {
            if((index = NPGetArrayIndexFromNPIdentifier(name)) == -1)
            {
                O_LOG_XCOMX_API("%-25s%s.%s = %s\n","[XCOMX API Property Set Start] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        NPIdentifierStr(name).c_str(),
                        NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, value, 1));
            }
            else
            {
                O_LOG_XCOMX_API("%-25s%s.[%d] = %s\n","[XCOMX API Property Set Start] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        index,
                        NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, value, 1));
            }
        }
#endif
        if (((NPObjectExt*)npobj)->pNpObj->SetProperty(name,value) == false)
        {
            O_LOG_ERROR("Failed to set the property '%s' for %s with error=%s\n",
                    NPIdentifierStr(name).c_str(),
                    NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                    "UnknownError");
            ((NPObjectExt*)npobj)->pNpObj->SetException("UnknownError");
        }
#ifdef LOG_XCOMX_API
        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
        {
            if((index = NPGetArrayIndexFromNPIdentifier(name)) == -1)
            {
                O_LOG_XCOMX_API("%-25s%s.%s = %s\n","[XCOMX API Property Set End] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        NPIdentifierStr(name).c_str(),
                        NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, value, 1));
            }
            else
            {
                O_LOG_XCOMX_API("%-25s%s.[%d] = %s\n","[XCOMX API Property Set End] ",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        index,
                        NPVariantSprintf(((NPObjectExt*)npobj)->pNpObj->m_NPP, value, 1));
            }
        }
#endif
        return true;
    }
    else
    {
        O_LOG_WARNING("NpObj::NPSetProperty is called for deleted NpObj\n");
        return false;
    }
}

    bool
NpObj::NPRemoveProperty(NPObject *npobj, NPIdentifier name)
{
    if( ((NPObjectExt*)npobj)->pNpObj )
    {
        bool retval;
#ifdef LOG_XCOMX_API
        int32_t index;

        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
        {
            if((index = NPGetArrayIndexFromNPIdentifier(name)) == -1)
            {
                O_LOG_XCOMX_API("%-25s%s.%s\n","[XCOMX API Property Remove Start]",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        NPIdentifierStr(name).c_str());

            }
            else
            {
                O_LOG_XCOMX_API("%-25s%s.[%d]\n","[XCOMX API Property Remove Start]",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        index);
            }
        }
#endif

        retval =  ((NPObjectExt*)npobj)->pNpObj->RemoveProperty(name);

#ifdef LOG_XCOMX_API
        /* don't log 'XCOM' object properties since they just expose singletons and do not present
           any useful info */
        if(((NPObjectExt*)npobj)->pNpObj->getJSObjectName() != NPID(MAIN_PLUGIN_OBJECT_NAME_ID))
        {
            if((index = NPGetArrayIndexFromNPIdentifier(name)) == -1)
            {
                O_LOG_XCOMX_API("%-25s%s.%s\n","[XCOMX API Property Remove End]",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        NPIdentifierStr(name).c_str());

            }
            else
            {
                O_LOG_XCOMX_API("%-25s%s.[%d]\n","[XCOMX API Property Remove End]",
                        NPIdentifierStr(((NPObjectExt*)npobj)->pNpObj->getJSObjectName()).c_str(),
                        index);
            }
        }
#endif
        return retval;
    }
    else
    {
        O_LOG_WARNING("NpObj::NPRemoveProperty is called for deleted NpObj\n");
        return false;
    }
}

    bool
NpObj::GetProperty(NPIdentifier name, NPVariant *result)
{
    _o_atom_property_t *node = property_list_head;

    while (node) {
        if (node->propertyName == name && node->propertyType == NP_PROPERTY_RO) {
            if(result && node->propertyVal)
            {
                if(NPVARIANT_IS_INT32_OR_DOUBLE(*node->propertyVal))
                {
                    memcpy(result,node->propertyVal, sizeof(NPVariant));
                    O_LOG_TRACE("%s::GetProperty is called for %s VALID\n", (const char*)NPIdentifierStr(m_objectInfo.jsObjectName),
                            (const char*)NPIdentifierStr(name));
                    return true;
                }
            }
        }
        node = node->next;
    }
    O_LOG_TRACE("%s::GetProperty is called for %s INVALID\n", (const char*)NPIdentifierStr(m_objectInfo.jsObjectName),
            (const char*)NPIdentifierStr(name));
    return false;
}

    bool
NpObj::SetProperty(NPIdentifier name, const NPVariant *value)
{
    UNUSED_PARAM(name);
    UNUSED_PARAM(value);
    return false;
}

    void
NpObj::Invoke(NPObject *npobj, NPIdentifier name,
        const NPVariant *args, uint32_t argCount,
        NPVariant *result)
{
    UNUSED_PARAM(npobj);

    O_LOG_ENTER("Calling %s with %d args\n",(const char*)NPIdentifierStr(name), argCount);

#ifdef __K_NHLOG_H__
    if ("nemo_use_klog") {
        char *utf8name = (char*)NPN_UTF8FromIdentifier(name);
        if (utf8name && !strcmp(utf8name, "klog")) {
            int stringlen = (int)NPVARIANT_TO_STRING(*args).UTF8Length;
            char *handlerName = new(std::nothrow)char[stringlen + 1];
            memcpy(handlerName,(char *)(NPVARIANT_TO_STRING(*args).UTF8Characters),stringlen);
            handlerName[stringlen] = 0;

            NHLOG_CHK_AND_CALL(NHLOG_INFO, 'L', "UI", __FILE__, __func__, __LINE__, "%s\n", handlerName);

            NPN_MemFree(utf8name);
            return;
        }
        if (utf8name)
            NPN_MemFree(utf8name);
    }
#endif

    if (!NPN_IdentifierIsString(name)) {
        O_LOG_ERROR("Invoke method name not string!\n");
        ASSERT(0);
    }
    else if ((name == NPID(ADD_SIGNAL_LISTENER_ID))
            || (name == NPID(REMOVE_SIGNAL_LISTENER_ID)))
    {
        if( argCount < 2 ||
                !NPVARIANT_IS_STRING(args[0]) ||
                !NPVARIANT_IS_OBJECT(args[1]))
        {
            O_LOG_ERROR("Wrong arguments passed for %s: %s\n", (const char*)NPIdentifierStr(name),
                    (argCount<2)?"Arg count less than 2":
                    (!NPVARIANT_IS_STRING(args[0]))?"Arg 1 is not a string":
                    "Arg 2 is not an object");
            INT32_TO_NPVARIANT(EVENT_LISTENER_ERROR_BAD_ARG, *result);
        }
        else
        {
            int stringlen = (int)NPVARIANT_TO_STRING(*args).UTF8Length;
            char *handlerName = new(std::nothrow)char[stringlen+1];
            memcpy(handlerName,(char *)(NPVARIANT_TO_STRING(*args).UTF8Characters),stringlen);
            handlerName[stringlen]=0; // end of string

            if(!has_signal(handlerName))
            {
                O_LOG_ERROR("Event name is not supported %s!\n", handlerName);
                INT32_TO_NPVARIANT(EVENT_LISTENER_ERROR_UNSUPPORTED, *result);
            }
            else
            {
                NPObject *eventObject;

                eventObject = NPVARIANT_TO_OBJECT(args[1]);

                if (name == NPID(ADD_SIGNAL_LISTENER_ID))
                {
                    O_LOG_DEBUG("Adding Event Listener %s\n", handlerName);
                    addEventHandler(handlerName,eventObject);
                    INT32_TO_NPVARIANT(EVENT_LISTENER_ERROR_NONE, *result);
                }
                else if (name == NPID(REMOVE_SIGNAL_LISTENER_ID))
                {
                    int retVal;
                    O_LOG_DEBUG("Removing Event Listener %s\n", handlerName);
                    retVal = removeEventHandler(handlerName,eventObject);
                    INT32_TO_NPVARIANT(retVal, *result);
                }
            }

            delete [] handlerName;
        }
    }
    else
    {
        _o_atom_method_t* pMethod = get_method_info(name);
        if(pMethod)
        {
            if(pMethod->sync)
                InvokeSync(name, args, argCount, result);
            else
            {
                int32_t invoke_handle = GetUniqueHandle();

                /* Add ref to the object to prevent JavaScript from deleting it while
                   we are waiting for the Async method callback from DBus */
                NPN_RetainObject(GetNPObject());

                InvokeAsync(name, args, argCount, invoke_handle);
                ASSERT(result);
                if(result)
                    INT32_TO_NPVARIANT(invoke_handle,*result);
            }
        }
    }

}

    void
NpObj::InvokeSync( NPIdentifier    name,
        const NPVariant*  args,
        uint32_t          argCount,
        NPVariant*        result)
{
    UNUSED_PARAM(name);
    UNUSED_PARAM(args);
    UNUSED_PARAM(argCount);
    UNUSED_PARAM(result);
}

    void
NpObj::InvokeAsync( NPIdentifier   name,
        const NPVariant* args,
        uint32_t         argCount,
        const int32_t    invoke_handle)
{
    UNUSED_PARAM(name);
    UNUSED_PARAM(args);
    UNUSED_PARAM(argCount);
    UNUSED_PARAM(invoke_handle);
}

bool
NpObj::get_handler(char *event, _o_atom_event_t **iter) {

    while (*iter != NULL) {
        //        O_LOG_TRACE("iter event %s, event %s, id = %d\n",(*iter)->event.c_str(),event, (*iter)->handlerId);
        if (!strcmp((*iter)->event.c_str(),event)) {
            //            O_LOG_TRACE("Found handler for %s\n",event);
            return true;
        }
        *iter = (*iter)->next;
    }
    return false;
}

/** This function is used to add a JS event listener object for the event.
 *   It does allow adding multiple listeners for the same event, as well as
 *   adding the same listener for muliptle events, but will not add the same
 *   listener twice for the same event.
 *   It adds and uses an event handler property id to each handler to keep track
 *   of multiple event handlers for the same event.
 * \param[in]  name name of the event
 * \param[in] handler JS listener Object
 * \returns none
 * \par Context
 * None.
 * \par SideEffect
 * None.
 */

void
NpObj::addEventHandler(char *name, NPObject *handler) {
    _o_atom_event_t *newHandler;
    NPVariant npVarEHId;
    _o_atom_event_t* iter = getEventHandlers();
    bool addNewHandler = true;
    int32_t existingHandlerID = 0;

    while (iter)
    {
        if (!get_handler(name,&iter))
        {
            // This is the case when JavaScript is registering a listener for an event which has no listeners registered
            // for it yet. This listener object might have been already registered for some other XCOM event, in which
            // case we need to use its existing  handler ID for associating it with the internal handler entry that will
            // be created for this event.
            if( NPN_HasProperty(m_NPP, handler, NPID(EVENT_HANDLER_PROP_ID)) &&
                    NPN_GetProperty(m_NPP, handler, NPID(EVENT_HANDLER_PROP_ID), &npVarEHId) )
            {
                if(NPVARIANT_IS_INT32_OR_DOUBLE(npVarEHId))
                {
                    O_LOG_TRACE("Registering the same handler for %s event\n", name);
                    existingHandlerID = NPVARIANT_INT32_OR_DOUBLE_TO_UINT32(npVarEHId);
                }
                NPN_ReleaseVariantValue(&npVarEHId);
            }
            break;
        }
        else {
            if (NPN_HasProperty(m_NPP, handler, NPID(EVENT_HANDLER_PROP_ID)) == false)
            {
                break;
            }
            else
            {
                if (NPN_GetProperty(m_NPP, handler, NPID(EVENT_HANDLER_PROP_ID), &npVarEHId))
                {
                    if (NPVARIANT_IS_DOUBLE(npVarEHId))
                    {
                        if (NPVARIANT_TO_DOUBLE(npVarEHId) == iter->handlerId)
                        {
                            addNewHandler = false;
                        }
                    }
                    else if (NPVARIANT_IS_INT32(npVarEHId))
                    {
                        if (NPVARIANT_TO_INT32(npVarEHId) == iter->handlerId)
                        {
                            addNewHandler = false;
                        }
                    }
                    else
                        O_LOG_ERROR("Unexpected event handler pointer passed when registering '%s' handler for object 'XCOM.%s\n",
                                name, NPIdentifierStr(getJSObjectName()).c_str());
                    NPN_ReleaseVariantValue(&npVarEHId);
                }

                if (addNewHandler == false)
                {
                    O_LOG_DEBUG("Ignoring request to add existing event handler.\n");
                    break;
                }
            }
        }
        iter = iter->next;
    }


    if (addNewHandler == true)
    {

        O_LOG_TRACE("Adding New Handler\n");
        newHandler = new(std::nothrow) _o_atom_event_t;
        if (newHandler == NULL) {
            O_LOG_ERROR("Can not create new signal handler!\n");
            return;
        }

        newHandler->event = name;
        newHandler->pHandlerObject = handler;
        NPN_RetainObject(newHandler->pHandlerObject);
        if(existingHandlerID != 0)
        {
            // this listener is already being used by some other event, so we simply use
            // its existing ID for association with newHandler entry.
            newHandler->handlerId = existingHandlerID;
        }
        else
        {
            // create unique XCOM handler ID property to allow registering multiple listeners
            // for the same event.
            newHandler->handlerId = GetUniqueEventHandlerID();
            INT32_TO_NPVARIANT(newHandler->handlerId, npVarEHId);
            NPN_SetProperty(m_NPP, handler, NPID(EVENT_HANDLER_PROP_ID), &npVarEHId);
        }

        newHandler->next = NULL;

        if (!signal_handler_list_head)
            signal_handler_list_head = newHandler;
        else
            signal_handler_list_tail->next = newHandler;
        signal_handler_list_tail = newHandler;

        m_signal_handler_list_total++;

        if (signal_handler_list_head && signal_handler_list_tail)
            O_LOG_TRACE("New handler %p, Id %d, list head %p, list tail %p. TOTAL: %d\n", handler, newHandler->handlerId, signal_handler_list_head, signal_handler_list_tail, m_signal_handler_list_total);

        if(m_signal_handler_list_total >= XCOMX_MAX_EVENTS_WARNING_THRESHOLD)
            O_LOG_WARNING("Registering '%s' handler for object '%s'. Total handlers for this this object = %d!! Application should not be adding unlimited amount of handlers without removing unused ones to avoid memory leak.\n",
                    name, NPIdentifierStr(getJSObjectName()).c_str(), m_signal_handler_list_total);

        OnAddEventHandler(NPN_GetStringIdentifier(name)); // notify derived classes

    }

}

/*
 * This function synchronously removes an event handler from the NpObj's signal handler list.
 * It first checks that the handler exists within the signal list. If so, it removes
 * it from the list and notifies its derived objects.
 */
int32_t
NpObj::removeEventHandler(char *name, NPObject *handler) {
    _o_atom_event_t *eventHandler;
    _o_atom_event_t *prevHandler = NULL;
    NPVariant npVarEHId;
    int32_t retVal = REMOVE_EVENT_LISTENER_ERROR_EVENT_NOT_REGISTERED;
    bool removeHandler = false;

    eventHandler = signal_handler_list_head;

    while (eventHandler) {
        if (!eventHandler->event.compare(name)) {
            retVal = REMOVE_EVENT_LISTENER_ERROR_HANDLER_NOT_REGISTERED;
            O_LOG_TRACE("found handler for %s, event handler Id = %d\n",eventHandler->event.c_str(),eventHandler->handlerId);

            if (NPN_GetProperty(m_NPP, handler, NPID(EVENT_HANDLER_PROP_ID), &npVarEHId))
            {
                if (NPVARIANT_IS_DOUBLE(npVarEHId))
                {
                    if ((int)NPVARIANT_TO_DOUBLE(npVarEHId)==eventHandler->handlerId)
                        removeHandler = true;
                }
                else if (NPVARIANT_IS_INT32(npVarEHId))
                {
                    if (NPVARIANT_TO_INT32(npVarEHId)==eventHandler->handlerId)
                        removeHandler = true;
                }
                else
                    O_LOG_ERROR("Unexpected event handler pointer passed when unregistering '%s' handler for object 'XCOM.%s'\n",
                            eventHandler->event.c_str(), NPIdentifierStr(getJSObjectName()).c_str());
                NPN_ReleaseVariantValue(&npVarEHId);
            }

            if (removeHandler == true) // found event handler in list
            {
                retVal = EVENT_LISTENER_ERROR_NONE;
                O_LOG_TRACE("removing handler Id %d from handler list\n",eventHandler->handlerId);

                if (eventHandler == signal_handler_list_head)
                {
                    signal_handler_list_head = eventHandler->next;
                    if (eventHandler == signal_handler_list_tail) {
                        signal_handler_list_tail = eventHandler->next;
                    }
                }
                else if(prevHandler)
                {
                    if (signal_handler_list_tail == eventHandler) {
                        signal_handler_list_tail = prevHandler;
                        signal_handler_list_tail->next = NULL;
                    }
                    else
                        prevHandler->next = eventHandler->next;
                }
                else
                {
                    O_LOG_ERROR("Error removing handler Id %d from handler list\n",eventHandler->handlerId);
                    return REMOVE_EVENT_LISTENER_ERROR_HANDLER_NOT_REGISTERED;
                }

                m_signal_handler_list_total--;

                OnRemoveEventHandler(NPN_GetStringIdentifier(name)); // notify derived classes

                if(eventHandler->pHandlerObject)
                    NPN_ReleaseObject(eventHandler->pHandlerObject);

                if (signal_handler_list_head && signal_handler_list_tail)
                    O_LOG_TRACE("Removed event handler - signal list head %p, list tail %p\n", signal_handler_list_head, signal_handler_list_tail);

                if (eventHandler)
                    delete eventHandler;

                break;
            }
        }

        prevHandler = eventHandler;
        eventHandler = eventHandler->next;
    }

    return retVal;
}

void
NpObj::add_to_event_list(NPIdentifier       event_name,
        _o_atom_signal_arg_t *args,
        int                  num_args) {
    _o_atom_signal_t *newSignal;

    if(args == NULL && num_args > 0 )
    {
        O_LOG_ERROR("Wrong arguments are passed into add_to_event_list. (args=NULL, but num_args=%d)!\n", num_args);
        return;
    }

    if ((newSignal = new(std::nothrow) _o_atom_signal_t) == NULL) {
        O_LOG_ERROR("Cant create new signal - out of memory!\n");
        return;
    }

    newSignal->signal = NPIdentifierStr(event_name).c_str();
    newSignal->next = NULL;
    newSignal->numArgs = num_args;
    newSignal->isMsgReturn = false;

    if(newSignal->numArgs > 0)
    {
        newSignal->args = new(std::nothrow) _o_atom_signal_arg_t[num_args];
        if(!newSignal->args)
        {
            delete newSignal;
            O_LOG_ERROR("Cant create new signal - out of memory!\n");
            return;
        }

        for(int i = 0; i < num_args; i++)
        {
            newSignal->args[i] = args[i];
        }
    }
    else
        newSignal->args = NULL;

    if (!signal_list_head)
        signal_list_head = newSignal;
    else
        signal_list_tail->next = newSignal;
    signal_list_tail = newSignal;

    O_LOG_TRACE("signal list head %p, signal list tail %p\n",signal_list_head, signal_list_tail);
}

void
NpObj::add_method_ret_event(NPIdentifier       event_name,
        _o_atom_signal_arg_t *args,
        int                  num_args) {
    _o_atom_signal_t *newSignal;

    if(args == NULL && num_args > 0 )
    {
        O_LOG_ERROR("Wrong arguments are passed into add_method_ret_event. (args=NULL, but num_args=%d)!\n", num_args);
        return;
    }

    if ((newSignal = new(std::nothrow) _o_atom_signal_t) == NULL) {
        O_LOG_ERROR("Cant create new signal - out of memory!\n");
        return;
    }

    newSignal->signal = NPIdentifierStr(event_name).c_str();
    newSignal->next = NULL;
    newSignal->numArgs = num_args;
    newSignal->isMsgReturn = true;
    if(newSignal->numArgs > 0)
    {
        newSignal->args = new(std::nothrow) _o_atom_signal_arg_t[num_args];
        if(!newSignal->args)
        {
            delete newSignal;
            O_LOG_ERROR("Cant create new signal - out of memory!\n");
            return;
        }

        for(int i = 0; i < num_args; i++)
        {
            newSignal->args[i] = args[i];
        }
    }
    else
        newSignal->args = NULL;

    if (!signal_list_head)
        signal_list_head = newSignal;
    else
        signal_list_tail->next = newSignal;
    signal_list_tail = newSignal;

    O_LOG_TRACE("signal list head %p, signal list tail %p\n",signal_list_head, signal_list_tail);
}

void
NpObj::add_to_property_list(NPIdentifier property_name,
        const char *type,
        NP_PROPERTY_TYPE property_type /* NP_PROPERTY_RW*/,
        NPVariant* property_val /*NULL*/) {
    _o_atom_property_t *newProperty;

    if ((newProperty = new(std::nothrow) _o_atom_property_t) == NULL) {
        O_LOG_ERROR("Cant create new property\n");
        return;
    }

    newProperty->propertyName = property_name;
    newProperty->propertyType = property_type;

    if (type)
        newProperty->type.assign(type);

    if(property_val)
    {
        newProperty->propertyVal = new(std::nothrow) NPVariant();
        if(newProperty->propertyVal)
        {
            if(NPVARIANT_IS_INT32_OR_DOUBLE(*property_val))
                memcpy(newProperty->propertyVal, property_val, sizeof(NPVariant));
            else
                O_LOG_ERROR("Property type %d is not supported when initializing property\n", property_val->type);
        }
    }
    else
        newProperty->propertyVal = NULL;

    newProperty->next = NULL;

    if (!property_list_head)
        property_list_head = newProperty;
    else
        property_list_tail->next = newProperty;
    property_list_tail = newProperty;

    O_LOG_TRACE("property list head %p, property list tail %p\n",property_list_head, property_list_tail);
}

    uint32_t
NpObj::getNumInputArgs(const _o_atom_method_arg_t* args, const uint32_t& num_args)
{
    if(args && num_args > 0)
    {
        uint32_t count = 0;
        for(uint32_t i =0; i < num_args; i++)
        {
            if(!strcmp("in", args[i].dir.c_str()))
                count++;
        }
        return count;
    }
    else
        return 0;
}

    void
NpObj::add_to_method_list( NPIdentifier        method_name,
        _o_atom_method_arg_t* args,
        int                   num_args,
        bool                  sync, /*true*/
        int                   timeoutMS /* XCOMX_TIMEOUT_INFINITE */)
{
    _o_atom_method_t *newMethod;

    if(method_name == NULL)
    {
        O_LOG_ERROR("Can't create new method with enpty method_name!\n");
        return;
    }

    if (args == NULL && num_args > 0)
    {
        O_LOG_ERROR("Wrong arguments are passed into add_to_method_list (args=NULL, but num_args=%d)\n", num_args);
        return;
    }

    if ((newMethod = new(std::nothrow) _o_atom_method_t) == NULL) {
        O_LOG_ERROR("Cant create new method!\n");
        return;
    }

    newMethod->method = method_name;
    newMethod->methodStr = NPIdentifierStr(method_name).c_str();
    newMethod->numArgs = num_args;
    newMethod->numInputArgs = getNumInputArgs(args, num_args);
    if(num_args > 0 && args)
    {
        newMethod->args = new(std::nothrow) _o_atom_method_arg_t[num_args];
        if(!newMethod->args)
        {
            delete newMethod;
            O_LOG_ERROR("Cant create new method!\n");
            return;
        }

        for(int i = 0; i < num_args; i++)
        {
            newMethod->args[i] = args[i];

            O_LOG_TRACE("Argument name- %s, type- %s, dir- %s\n",newMethod->args[i].name.c_str(),
                    newMethod->args[i].type.c_str(),
                    newMethod->args[i].dir.c_str());
        }
    }
    else
        newMethod->args = NULL;
    newMethod->sync = sync;
    newMethod->timeoutMS = timeoutMS;
    newMethod->next = NULL;
    newMethod->reserved = NULL;

    if (!method_list_head)
        method_list_head = newMethod;
    else
        method_list_tail->next = newMethod;
    method_list_tail = newMethod;

    O_LOG_TRACE("method list head %p, method list tail %p\n",method_list_head, method_list_tail);

    if(!sync)
    {
        std::string tempSignalName;
        int numSignalArgs                = 0;

        numSignalArgs = newMethod->numArgs - newMethod->numInputArgs;

        /*Add [Method]OK event */
        NPIdentifierStr methodNameStr(method_name);
        tempSignalName = methodNameStr.c_str();
        tempSignalName += "OK";
        O_LOG_DEBUG("Adding event %s\n", tempSignalName.c_str());

        if (numSignalArgs) {
            _o_atom_signal_arg_t signalArgs[numSignalArgs];
            for (int i=0; i<numSignalArgs; i++) {
                signalArgs[i].name = args[newMethod->numInputArgs+i].name;
            }

            add_method_ret_event(NPN_GetStringIdentifier(tempSignalName.c_str()),
                    signalArgs,
                    numSignalArgs);
            O_LOG_TRACE("Event %s has %d arguments \n",tempSignalName.c_str(), numSignalArgs);
        }
        else {
            add_method_ret_event(NPN_GetStringIdentifier(tempSignalName.c_str()),
                    NULL,
                    0);
            O_LOG_TRACE("Event %s has %d arguments \n",tempSignalName.c_str(), 0);
        }

        /*Add [Method]Failed event */
        tempSignalName = methodNameStr.c_str();
        tempSignalName += "Failed";
        O_LOG_TRACE("Adding event %s\n", tempSignalName.c_str());

        add_method_ret_event(NPN_GetStringIdentifier(tempSignalName.c_str()),
                NULL,
                0);
        O_LOG_TRACE("Signal %s has no arguments\n",tempSignalName.c_str());
    }
}

bool
NpObj::has_signal(const char* name) {
    _o_atom_signal_t *node = signal_list_head;

    while (node) {
        if (node->signal == name) {
            return true;
        }

        node = node->next;
    }

    O_LOG_TRACE("object does not have %s signal\n", name);
    return false;
}

_o_atom_method_t*
NpObj::get_method_info(NPIdentifier method_name) {
    _o_atom_method_t *node = method_list_head;

    while (node) {
        if (node->method == method_name) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

_o_atom_signal_t*
NpObj::get_signal_info(char *signal_name) {
    _o_atom_signal_t *node = signal_list_head;

    while (node) {
        if (node->signal == signal_name) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

const char*
NpObj::get_property_type(const char *prop_name) {
    _o_atom_property_t *node = property_list_head;

    NPIdentifier name = NPN_GetStringIdentifier(prop_name);
    while (node) {
        if (node->propertyName == name) {
            return node->type.c_str();
        }
        node = node->next;
    }
    return NULL;

}

    void
NpObj::DispatchEvent(NPIdentifier eventName,
        NPVariant*   argsArr,
        uint32_t     argsArrSize)
{
    _o_atom_signal_data_t *eventData = NULL;

    if(eventName)
    {
        NPObject *resultObj = NPCreateNewWindowObject(m_NPP);
        if(resultObj)
        {
            _o_atom_signal_t* signalInfo = get_signal_info((char*)(NPIdentifierStr(eventName).c_str()));
            if(signalInfo)
            {
                /*
                   Set all arguments as properties of the return object
                   */
                for (uint32_t i=0;i < argsArrSize; i++)
                {
                    if(argsArr && signalInfo && signalInfo->args[i].name != "")
                    {
                        O_LOG_TRACE("arg(%d): adding property %s to return object with NPVariant type = %s\n",
                                i,
                                signalInfo->args[i].name.c_str(),
                                NPVARIANT_TYPESTRING(argsArr[i]));

                        NPN_SetProperty(m_NPP,
                                resultObj,
                                NPN_GetStringIdentifier(signalInfo->args[i].name.c_str()),
                                &argsArr[i]);
                    }
                }

                eventData = new(std::nothrow) _o_atom_signal_data_t;

                memset(eventData, 0x0, sizeof(_o_atom_signal_data_t));
                eventData->msgName = eventName;
                eventData->isErrorMsg = false;
                eventData->argsArr = new(std::nothrow) NPVariant[1];
                OBJECT_TO_NPVARIANT(resultObj,*eventData->argsArr);
                eventData->argsArrSize = 1;
                eventData->remoteObjInst = this;

                // Referencing the object to prevent the Browser deleting it before we
                // handle the event
                NPN_RetainObject(GetNPObject());

                // handleEvent will take care of freeing eventData
                handleEvent((void *)eventData);
            }
            else
            {
                O_LOG_ERROR("Could not get signalInfo JSObjectName=%s\n", NPIdentifierStr(getJSObjectName()).c_str());
                NPN_ReleaseObject(resultObj);
            }
        }
        else
            O_LOG_ERROR("Could not create result window\n");
    }
    else
        O_LOG_ERROR("DispatchEvent was passed NULL for eventName\n");
}

    void
NpObj::DispatchMethodOK(NPIdentifier methodName,
        NPVariant*   argsArr,
        uint32_t     argsArrSize,
        uint32_t     invoke_handle)
{
    _o_atom_signal_data_t *eventData = NULL;
    bool eventSent                   = false;

    if(methodName)
    {
        NPObject *resultObj = NPCreateNewWindowObject(m_NPP);
        if(resultObj)
        {
            NPIdentifierStr methodNameStr(methodName);
            std::string signalName = methodNameStr.c_str();
            signalName += "OK";
            _o_atom_signal_t* signalInfo = get_signal_info((char*)signalName.c_str());
            if(signalInfo)
            {
                /*
                   Set all arguments as properties of the return object
                   */
                for (uint32_t i=0;i < argsArrSize; i++)
                {
                    if(argsArr && signalInfo && signalInfo->args[i].name != "")
                    {
                        O_LOG_DEBUG("arg(%d): adding property %s to return object with NPVariant type = %s\n",
                                i,
                                signalInfo->args[i].name.c_str(),
                                NPVARIANT_TYPESTRING(argsArr[i]));

                        NPN_SetProperty(m_NPP,
                                resultObj,
                                NPN_GetStringIdentifier(signalInfo->args[i].name.c_str()),
                                &argsArr[i]);
                    }
                }

                /*
                   Set "handle" property
                   */
                NPVariant npVarHandle;
                INT32_TO_NPVARIANT(invoke_handle, npVarHandle);

                NPN_SetProperty(m_NPP,
                        resultObj,
                        NPN_GetStringIdentifier("handle"),
                        &npVarHandle);

                /* Prepare _o_atom_signal_data_t */
                eventData = new(std::nothrow) _o_atom_signal_data_t;

                memset(eventData, 0x0, sizeof(_o_atom_signal_data_t));
                eventData->msgName = methodName;
                eventData->isErrorMsg = false;
                eventData->argsArr = new(std::nothrow) NPVariant[1];
                OBJECT_TO_NPVARIANT(resultObj,*eventData->argsArr);
                eventData->argsArrSize = 1;
                eventData->remoteObjInst = this;

                eventSent = true;
                /* handleEvent will take care of freeing the eventData*/
                handleEvent((void*)eventData);
            }
            else
            {
                O_LOG_ERROR("Could not get signalInfo JSObjectName=%s\n", NPIdentifierStr(getJSObjectName()).c_str());
                NPN_ReleaseObject(resultObj);
            }
        }
        else
            O_LOG_ERROR("Could not create result window\n");
    }
    else
        O_LOG_ERROR("DispatchMethodOK was passed NULL for methodName\n");

    if(!eventSent)
    {
        /* Release object reference created by InvokeAsync */
        if(GetNPObject())
            NPN_ReleaseObject(GetNPObject());
    }
}

    void
NpObj::DispatchMethodFailed(NPIdentifier methodName,
        const std::string name,
        const std::string message,
        const std::string domain,
        uint32_t invoke_handle)
{
    _o_atom_signal_data_t *eventData = NULL;
    bool eventSent                    = false;

    if(methodName)
    {
        NPObject *errorObj = NPCreateNewWindowObject(m_NPP);
        if (errorObj)
        {
            NPObject *resultObj = NPCreateNewWindowObject(m_NPP);
            if(resultObj)
            {
                NPVariant temp;

                /*
                   Set all error args as properties of the error object
                   */
                STRINGZ_TO_NPVARIANT(message.c_str(), temp);
                NPN_SetProperty(m_NPP, errorObj, NPN_GetStringIdentifier("message"), &temp );

                STRINGZ_TO_NPVARIANT(name.c_str(), temp);
                NPN_SetProperty(m_NPP, errorObj, NPN_GetStringIdentifier("name"), &temp );

                STRINGZ_TO_NPVARIANT(domain.c_str(), temp);
                NPN_SetProperty(m_NPP, errorObj, NPN_GetStringIdentifier("domain"), &temp );

                /*
                   Set error object as a property of the return object
                   */
                OBJECT_TO_NPVARIANT(errorObj,temp);
                NPN_SetProperty(m_NPP, resultObj, NPN_GetStringIdentifier("error"),&temp );

                /*
                   Set "handle" property
                   */
                NPVariant npVarHandle;
                INT32_TO_NPVARIANT(invoke_handle, npVarHandle);

                NPN_SetProperty(m_NPP,
                        resultObj,
                        NPN_GetStringIdentifier("handle"),
                        &npVarHandle);

                /* Prepare _o_atom_signal_data_t */
                eventData = new(std::nothrow) _o_atom_signal_data_t;

                memset(eventData, 0x0, sizeof(_o_atom_signal_data_t));
                eventData->msgName = methodName;
                eventData->isErrorMsg = true;
                eventData->argsArr = new(std::nothrow) NPVariant[1];
                OBJECT_TO_NPVARIANT(resultObj,*eventData->argsArr);
                eventData->argsArrSize = 1;
                eventData->remoteObjInst = this;

                eventSent = true;
                /* handleEvent will take care of freeing eventData*/
                handleEvent((void *)eventData);
            }
            else
                O_LOG_ERROR("Could not create result window. JSObjectName=%s\n", NPIdentifierStr(getJSObjectName()).c_str());
            NPN_ReleaseObject(errorObj);
        }
        else
            O_LOG_ERROR("Could not create error window. \n");
    }
    else
        O_LOG_ERROR("DispatchMethodFailed was passed NULL for methodName\n");

    if(!eventSent)
    {
        /* Release object reference created by InvokeAsync */
        if(GetNPObject())
            NPN_ReleaseObject(GetNPObject());
    }
}

void NpObj::setObjectInfo(ObjectCreateInfo *info) {
    if (info)
    {
        m_objectInfo.connAddr = info->connAddr;
        m_objectInfo.intfName = info->intfName;
        m_objectInfo.objectPath = info->objectPath;
        m_objectInfo.jsObjectName = info->jsObjectName;
        m_objectInfo.intfXMLFile = info->intfXMLFile;
    }
}

/*
 * This function invokes the handler that it is called with.
 * It first verifies the handler is valid (in case the handler was removed between calling
 * this function and its execution), and then subsequent to invoking the handler, it cleans up the event data.
 */
void NpObj::executeHandler(void* userData) {
    NPVariant retval;
    _o_event_handler_data_t *eventCtx = (_o_event_handler_data_t *)userData;
    bool isValidHandler = false;
    _o_atom_event_t *iter;

    if (eventCtx == NULL)
    {
        O_LOG_ERROR("Error executing handler\n");
        return;
    }

    //	O_LOG_TRACE("executing handler %d\n",eventCtx->pHandler->handlerId);

    if(!((ccomx::NpObj *)eventCtx->pSignal->remoteObjInst)->GetNPObject())
    {
        // The NPObject has already been deleted by JS, so we are discarding the event
        isValidHandler = false;
    }
    else
    {
        // is this a valid handler - it may have been removed
        iter = ((ccomx::NpObj *)eventCtx->pSignal->remoteObjInst)->getEventHandlers();
        while (iter)
        {
            if (iter == eventCtx->pHandler) {
                // we found a matching pointer, let's make sure the handler name is also matching
                // (this is needed to prevent rare cases, where the handler was removed and a new handler was inserted
                // and the same pointer was re-used)
                if(eventCtx->handlerName == iter->event)
                    isValidHandler = true;

                break;
            }
            iter = iter->next;
        }
    }

    if (isValidHandler) {
        // We are about to call into the handler defined in JavaScript. Since the handler might call into us and remove itself
        // by calling removeEventListener(), we need to increment a reference count before the call to prevent premature destroying
        // of the handler object. Note that in this case, the memory pointed to by "iter" variable will be deleted, so we can't use
        // this variable after NPN_InvokeDefault() returns. Intead, we use a local NPObject* variable:
        NPObject* objRef = iter->pHandlerObject;
        O_DEBUG_ASSERT(objRef);
        if(objRef)
            NPN_RetainObject(objRef);

#ifdef LOG_XCOMX_API
        string eventHandler = iter->event.c_str();
        O_LOG_XCOMX_API("%-25s%s.%s\n", "[XCOMX API Event Handler Start] ",
                NPIdentifierStr(((ccomx::NpObj *)eventCtx->pSignal->remoteObjInst)->getJSObjectName()).c_str(),
                eventHandler.c_str());
#endif
        // NOTE: after the NPN_InvokeDefault() call below, eventCtx->pHandler may not be valid and should not be referenced
        if (!NPN_InvokeDefault(eventCtx->pSignal->remoteObjInst->m_NPP, iter->pHandlerObject, eventCtx->pSignal->argsArr, eventCtx->pSignal->argsArrSize, &retval))
        {
            O_LOG_ERROR("Failed to handle %s event\n", eventCtx->handlerName.c_str());
        }
        else
        {
            //    O_LOG_TRACE("Event succeeded\n");
            NPN_ReleaseVariantValue(&retval);
        }

#ifdef LOG_XCOMX_API
        O_LOG_XCOMX_API("%-25s%s.%s\n", "[XCOMX API Event Handler End] ",
                NPIdentifierStr(((ccomx::NpObj *)eventCtx->pSignal->remoteObjInst)->getJSObjectName()).c_str(),
                eventHandler.c_str());
#endif

        if(objRef)
            NPN_ReleaseObject(objRef);
    }

    if (eventCtx->freeSignal == true)
    {
        /* Release object reference created by DispatchEvent and InvokeAsync */
        if(eventCtx->pSignal->remoteObjInst)
            NPN_ReleaseObject(((NpObj*)eventCtx->pSignal->remoteObjInst)->GetNPObject());

        DelSignalData(eventCtx->pSignal);
    }
    delete eventCtx;

    //	O_LOG_TRACE("done with handler %d\n",eventCtx->pHandler->handlerId);
}

/* This function is called when a XCOMX NpObj class fires an event.
 * It is called with signal data (_o_atom_signal_data_t).
 * It sets the target property of the signal data to the NpObj that it pertains to
 * and then asynchronously calls each registered event handler pertaining to this event.
 * If registered handlers exist for this event, the executehandler method
 * will execute each handler asynchronously and clean up all signal data.
 * If no registered handlers exist for this event, this function cleans up the signal data.
 */
void NpObj::handleEvent(void* userData) {
    NpObj *remoteObjInst;
    std::string eventHandlerName;
    _o_atom_event_t *iter;
    bool registeredHandlerExists = false;
    NPObject *targetObject = NULL;

    _o_atom_signal_data_t *signalCtx = (_o_atom_signal_data_t *)userData;

    if(!signalCtx)
    {
        O_LOG_ERROR("Event was fired with no event data!\n");
        return;
    }

    NPIdentifierStr signalMsgNameStr(signalCtx->msgName);
    O_LOG_DEBUG("Received XCOM Event %s on thread %d\n",signalMsgNameStr.c_str(), (int)pthread_self());

    remoteObjInst=(NpObj *)(signalCtx->remoteObjInst);
    if(!remoteObjInst)
    {
        O_LOG_ERROR("Event was fired with no remoteObjInst!\n");
        DelSignalData(signalCtx);
        return;
    }

    targetObject = remoteObjInst->GetNPObject();
    if(!targetObject)
    {
        O_LOG_WARNING("The NPObject for %s has already been deleted, discarding the %s event\n",
                ((remoteObjInst)->getObjectInfo())->objectPath.c_str(),
                signalMsgNameStr.c_str());
        DelSignalData(signalCtx);
        return;
    }

    eventHandlerName = signalMsgNameStr.c_str();

    if (remoteObjInst->get_method_info(signalCtx->msgName) != NULL)
    {
        if (signalCtx->isErrorMsg == false)
            eventHandlerName += "OK";
        else
            eventHandlerName += "Failed";
    }

    iter = remoteObjInst->getEventHandlers();

    while (iter)
    {
        if (remoteObjInst->get_handler((char *)eventHandlerName.c_str(), &iter ) == false) {
            break;
        }

        if (registeredHandlerExists == false)
        {
            NPVariant target;
            NPObject *resultObject;

            OBJECT_TO_NPVARIANT(targetObject, target);
            resultObject = NPVARIANT_TO_OBJECT(signalCtx->argsArr[0]);
            NPN_SetProperty(remoteObjInst->m_NPP, resultObject, NPN_GetStringIdentifier("target"), &target);
            registeredHandlerExists = true;
        }
        O_LOG_DEBUG("Found event handler for %s! and got %d arg(s), the args=%p\n",
                eventHandlerName.c_str(),
                signalCtx->argsArrSize,
                signalCtx->argsArr);

        _o_event_handler_data_t *eventData = new(std::nothrow) _o_event_handler_data_t;
        eventData->pSignal = signalCtx;
        eventData->pHandler = iter;
        // storing a copy of eventData->pHandler->event since eventData->pHandler may get removed before executeHandler() is executed
        eventData->handlerName = eventData->pHandler->event;

        iter = iter->next;
        // if there are multiple handlers for this event, only free the signal data on last execution
        if (iter && (remoteObjInst->get_handler((char *)eventHandlerName.c_str(), &iter) == true))
            eventData->freeSignal = false;
        else
            eventData->freeSignal = true;

        if(!NPN_PluginThreadAsyncCallX(remoteObjInst->m_NPP, executeHandler, (void *)eventData))
        {
            registeredHandlerExists = false; // this is needed to execute clean-up below
            delete eventData;
        }
    }

    // if there were no handlers for this event, delete the signal data here (otherwise, this is done in executeHandler)
    if (registeredHandlerExists == false)
    {
#ifdef LOG_XCOMX_API
        O_LOG_XCOMX_API("%-25s%s.%s\n", "[XCOMX API Event Handler Unregistered] ",
                NPIdentifierStr(((ccomx::NpObj *)signalCtx->remoteObjInst)->getJSObjectName()).c_str(),
                eventHandlerName.c_str());
#endif

        /* Release object reference created by DispatchEvent and InvokeAsync */
        NPN_ReleaseObject(targetObject);
        DelSignalData(signalCtx);
    }
}

}; /* end of ccomx namespace */
