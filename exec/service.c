/*
 * Copyright (c) 2006 MontaVista Software, Inc.
 * Copyright (c) 2006-2008 Red Hat, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@redhat.com)
 *
 * This software licensed under BSD license, the text of which follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <corosync/lcr/lcr_ifact.h>
#include <corosync/swab.h>
#include <corosync/totem/totem.h>

#include "mainconfig.h"
#include "util.h"
#include <corosync/engine/logsys.h>

#include "timer.h"
#include <corosync/totem/totempg.h>
#include <corosync/totem/totemip.h>
#include "main.h"
#include "ipc.h"
#include <corosync/engine/coroapi.h>
#include "service.h"


LOGSYS_DECLARE_SUBSYS ("SERV", LOG_INFO);

struct default_service {
	char *name;
	int ver;
};

static struct default_service default_services[] = {
	{
		.name			 = "corosync_evs",
		.ver			 = 0,
	},
	{
		.name			 = "corosync_cfg",
		.ver			 = 0,
	},
	{
		.name			 = "corosync_cpg",
		.ver			 = 0,
	},
	{
		.name			 = "corosync_confdb",
		.ver			 = 0,
	},
	{
		.name			 = "corosync_pload",
		.ver			 = 0,
	},
};

struct corosync_service_engine *ais_service[SERVICE_HANDLER_MAXIMUM_COUNT];

static unsigned int object_internal_configuration_handle;


static unsigned int default_services_requested (struct corosync_api_v1 *corosync_api)
{
	unsigned int object_service_handle;
	unsigned int object_find_handle;
	char *value;

	/*
	 * Don't link default services if they have been disabled
	 */
	corosync_api->object_find_create (
		OBJECT_PARENT_HANDLE,
		"aisexec",
		strlen ("aisexec"),
		&object_find_handle);

	if (corosync_api->object_find_next (
		object_find_handle,
		&object_service_handle) == 0) {

		if ( ! corosync_api->object_key_get (object_service_handle,
			"defaultservices",
			strlen ("defaultservices"),
			(void *)&value,
			NULL)) {

			if (value && strcmp (value, "no") == 0) {
				return 0;
			}
		}
	}

	corosync_api->object_find_destroy (object_find_handle);

	return (-1);
}

unsigned int corosync_service_link_and_init (
	struct corosync_api_v1 *corosync_api,
	char *service_name,
	unsigned int service_ver)
{
	struct corosync_service_engine_iface_ver0 *iface_ver0;
	void *iface_ver0_p;
	unsigned int handle;
	struct corosync_service_engine *service;
	unsigned int res;
	unsigned int object_service_handle;

	/*
	 * reference the service interface
	 */
	iface_ver0_p = NULL;
	lcr_ifact_reference (
		&handle,
		service_name,
		service_ver,
		&iface_ver0_p,
		(void *)0);

	iface_ver0 = (struct corosync_service_engine_iface_ver0 *)iface_ver0_p;

	if (iface_ver0 == 0) {
		log_printf(LOG_LEVEL_ERROR, "Service failed to load '%s'.\n", service_name);
		return (-1);
	}


	/*
	 * Initialize service
	 */
	service = iface_ver0->corosync_get_service_engine_ver0();

	ais_service[service->id] = service;
	if (service->config_init_fn) {
		res = service->config_init_fn (corosync_api);
	}

	if (service->exec_init_fn) {
		res = service->exec_init_fn (corosync_api);
	}

	/*
	 * Store service in object database
	 */
	corosync_api->object_create (object_internal_configuration_handle,
		&object_service_handle,
		"service",
		strlen ("service"));

	corosync_api->object_key_create (object_service_handle,
		"name",
		strlen ("name"),
		service_name,
		strlen (service_name) + 1);

	corosync_api->object_key_create (object_service_handle,
		"ver",
		strlen ("ver"),
		&service_ver,
		sizeof (service_ver));

	res = corosync_api->object_key_create (object_service_handle,
		"handle",
		strlen ("handle"),
		&handle,
		sizeof (handle));

	corosync_api->object_key_create (object_service_handle,
		"service_id",
		strlen ("service_id"),
		&service->id,
		sizeof (service->id));

	log_printf (LOG_LEVEL_NOTICE, "Service initialized '%s'\n", service->name);
	return (res);
}

static int corosync_service_unlink_common (
	struct corosync_api_v1 *corosync_api,
	unsigned int object_service_handle,
	const char *service_name,
	unsigned int service_version) 
{
	unsigned int res;
	unsigned short *service_id;
	unsigned int *found_service_handle;

	res = corosync_api->object_key_get (object_service_handle,
		"handle",
		strlen ("handle"),
		(void *)&found_service_handle,
		NULL);
	
	res = corosync_api->object_key_get (object_service_handle,
		"service_id",
		strlen ("service_id"),
		(void *)&service_id,
		NULL);
	
	log_printf(LOG_LEVEL_NOTICE, "Unloading corosync component: %s v%u\n",
		service_name, service_version);

	if (ais_service[*service_id]->exec_exit_fn) {
		ais_service[*service_id]->exec_exit_fn ();
	}
	ais_service[*service_id] = NULL;
    
	return lcr_ifact_release (*found_service_handle);	
}

extern unsigned int corosync_service_unlink_and_exit (
	struct corosync_api_v1 *corosync_api,
	char *service_name,
	unsigned int service_ver)
{
	unsigned int res;
	unsigned int object_service_handle;
	char *found_service_name;
	unsigned int *found_service_ver;
	unsigned int object_find_handle;

	corosync_api->object_find_create (
		object_internal_configuration_handle,
		"service",
		strlen ("service"),
		&object_find_handle);

	while (corosync_api->object_find_next (
		object_find_handle,
		&object_service_handle) == 0) {

		corosync_api->object_key_get (object_service_handle,
			"name",
			strlen ("name"),
			(void *)&found_service_name,
			NULL);

		corosync_api->object_key_get (object_service_handle,
			"ver",
			strlen ("ver"),
			(void *)&found_service_ver,
			NULL);

		/*
		 * If service found and linked exit it
		 */
		if ((strcmp (service_name, found_service_name) == 0) &&
			(service_ver == *found_service_ver)) {

			res = corosync_service_unlink_common (
				corosync_api, object_service_handle,
				service_name, service_ver);

			corosync_api->object_destroy (object_service_handle);
			return res;
		}
	}

	corosync_api->object_find_destroy (object_find_handle);

	return (-1);
}

extern unsigned int corosync_service_unlink_all (
	struct corosync_api_v1 *corosync_api)
{
	char *service_name;
	unsigned int *service_ver;
	unsigned int object_service_handle;
	unsigned int object_find_handle;
	int found; 

	log_printf(LOG_LEVEL_NOTICE, "Unloading all corosync components\n");

	/*
	 * TODO
	 * Deleting of keys not supported during iteration at this time
	 * hence this ugly hack
	 */
	while(corosync_api->object_find_create (
			object_internal_configuration_handle,
			"service",
			strlen ("service"),
			&object_find_handle) == 0)
	{

		found = 0;

		while(corosync_api->object_find_next (
			object_find_handle,
			&object_service_handle) == 0)
			found = 1;

		if(!found)
			break;

		corosync_api->object_key_get (
			object_service_handle,
			"name",
			strlen ("name"),
			(void *)&service_name,
			NULL);

		corosync_api->object_key_get (
			object_service_handle,
			"ver",
			strlen ("ver"),
			(void *)&service_ver,
			NULL);

		corosync_service_unlink_common (
			corosync_api, object_service_handle,
			service_name, *service_ver);

		corosync_api->object_destroy (object_service_handle);

		corosync_api->object_find_destroy (object_find_handle);

	}

	return (0);
}

/*
 * Links default services into the executive
 */
unsigned int corosync_service_defaults_link_and_init (struct corosync_api_v1 *corosync_api)
{
	unsigned int i;

	unsigned int object_service_handle;
	char *found_service_name;
	char *found_service_ver;
	unsigned int found_service_ver_atoi;
	unsigned int object_find_handle;
 
	corosync_api->object_create (OBJECT_PARENT_HANDLE,
		&object_internal_configuration_handle,
		"internal_configuration",
		strlen ("internal_configuration"));

	corosync_api->object_find_create (
		OBJECT_PARENT_HANDLE,
		"service",
		strlen ("service"),
		&object_find_handle);

	while (corosync_api->object_find_next (
		object_find_handle,
		&object_service_handle) == 0) {

		corosync_api->object_key_get (object_service_handle,
			"name",
			strlen ("name"),
			(void *)&found_service_name,
			NULL);

		corosync_api->object_key_get (object_service_handle,
			"ver",
			strlen ("ver"),
			(void *)&found_service_ver,
			NULL);

		found_service_ver_atoi = atoi (found_service_ver);

		corosync_service_link_and_init (
			corosync_api,
			found_service_name,
			found_service_ver_atoi);
	}

	corosync_api->object_find_destroy (object_find_handle);

 	if (default_services_requested (corosync_api) == 0) {
 		return (0);
 	}

	for (i = 0;
		i < sizeof (default_services) / sizeof (struct default_service); i++) {

		corosync_service_link_and_init (
			corosync_api,
			default_services[i].name,
			default_services[i].ver);
	}
			
	return (0);
}