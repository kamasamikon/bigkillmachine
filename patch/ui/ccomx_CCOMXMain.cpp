/**
 * @file     ccomx_XCOMXMain.cpp
 * @author   OS-PresEngine@nemotv.com
 * @date     Aug 22, 2011
 * @brief	 XCOMX Main JavaScript object, parent of all XCOMX objects
 * @defgroup XCOMX main object
 * @ingroup  XCOMX
 */

#include "ccomx_XCOMXMain.h"
#include "ccomx_utils.h"
#include "ccomx_NPPPlugin.h"

namespace ccomx {

XCOMXMain::XCOMXMain(NPP nppInst):
	NpObj(nppInst, "XCOMXMain")
{
    list<ObjectCreateInfo*> l = ((CNPPPlugin *)m_NPP->pdata)->m_singletonMgr->GetObjCreateInfoList();

    list<ObjectCreateInfo*>::iterator it;
    ObjectCreateInfo* info        = NULL;

    for ( it = l.begin() ; it != l.end(); it++ )
    {
        info = *it;
        if(info)
        {
            add_to_property_list(info->jsObjectName,NULL);
#ifndef NDEBUG
            O_LOG_DEBUG("Adding %s object to XCOMX property list\n", (const char*)NPIdentifierStr(info->jsObjectName));
#endif
        }
    }

#ifdef LOG_NTVNP_OBJECTS
    ((CNPPPlugin *)m_NPP->pdata)->m_singletonMgr->GetInstance(nppInst, NPN_GetStringIdentifier("RTEWindow"));
#endif
}

XCOMXMain::~XCOMXMain(void)
{
    O_LOG_TRACE("XCOMXMain::~XCOMXMain()\n ");
}

bool
XCOMXMain::HasMethod(NPObject *npobj, NPIdentifier name)
{
	UNUSED_PARAM(npobj);
	UNUSED_PARAM(name);

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

	O_LOG_ENTER("HasMethod: %s\n", (const char*)NPIdentifierStr(name));
	return false; /* does not have methods */
}

bool
XCOMXMain::Invoke(NPObject *npobj, NPIdentifier name,
					  const NPVariant *args, uint32_t argCount,
					  NPVariant *result)
{
	UNUSED_PARAM(npobj);
	UNUSED_PARAM(args);
	UNUSED_PARAM(result);

	O_LOG_ENTER("Calling %s with %d args, thread %d\n", (const char*)NPIdentifierStr(name), argCount, (int)pthread_self());

	if (!NPN_IdentifierIsString(name)) {
		O_LOG_WARNING("Invoke method name not string!/n");
	}

    return false;
}

bool
XCOMXMain::GetProperty(NPIdentifier name, NPVariant *result)
{
    if(!result)
        return false;

	VOID_TO_NPVARIANT(*result);

    NpObj *obj = ((CNPPPlugin *)m_NPP->pdata)->m_singletonMgr->GetInstance(m_NPP,name);
    if(obj)
    {
        /* The returned obj has already called NPN_RetainObject or is created in GetInstance, so it must not call NPN_RetainObject in GetProperty. */
        OBJECT_TO_NPVARIANT(obj->GetNPObject(), *result);
        return true;
    }
    else
        O_LOG_ERROR("Not able to get %s object from SingletonMgr!\n", (const char*)NPIdentifierStr(name));

    return false;
}

void
XCOMXMain::OnAddEventHandler(NPIdentifier name)
{
	UNUSED_PARAM(name);
}

}; /* end of ccomx namespace */
