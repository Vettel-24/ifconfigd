/*
 * Copyright (c) 2005, PADL Software Pty Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of PADL Software nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PADL SOFTWARE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL PADL SOFTWARE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "kcm_locl.h"

RCSID("$KTH: acl.c,v 1.4 2005/02/06 01:22:48 lukeh Exp $");

krb5_error_code
kcm_access(krb5_context context,
	   kcm_client *client,
	   kcm_operation opcode,
	   kcm_ccache ccache)
{
    int read_p = 0;
    int write_p = 0;
    u_int16_t mask;
    krb5_error_code ret;

    KCM_ASSERT_VALID(ccache);

    switch (opcode) {
    case KCM_OP_INITIALIZE:
    case KCM_OP_DESTROY:
    case KCM_OP_STORE:
    case KCM_OP_REMOVE_CRED:
    case KCM_OP_SET_FLAGS:
    case KCM_OP_CHOWN:
    case KCM_OP_CHMOD:
    case KCM_OP_GET_INITIAL_TICKET:
    case KCM_OP_GET_TICKET:
	write_p = 1;
	read_p = 0;
	break;
    case KCM_OP_NOOP:
    case KCM_OP_GET_NAME:
    case KCM_OP_RESOLVE:
    case KCM_OP_GEN_NEW:
    case KCM_OP_RETRIEVE:
    case KCM_OP_GET_PRINCIPAL:
    case KCM_OP_GET_FIRST:
    case KCM_OP_GET_NEXT:
    case KCM_OP_END_GET:
    case KCM_OP_MAX:
	write_p = 0;
	read_p = 1;
	break;
    }

    if (ccache->flags & KCM_FLAGS_OWNER_IS_SYSTEM) {
	/* System caches cannot be reinitialized or destroyed by users */
	if (opcode == KCM_OP_INITIALIZE ||
	    opcode == KCM_OP_DESTROY ||
	    opcode == KCM_OP_REMOVE_CRED) {
	    ret = KRB5_FCC_PERM;
	    goto out;
	}

	/* Let root always read system caches */
	if (client->uid == 0) {
	    ret = 0;
	    goto out;
	}
    }

    mask = 0;

    if (client->uid == ccache->uid) {
	if (read_p)
	    mask |= S_IRUSR;
	if (write_p)
	    mask |= S_IWUSR;
    } else if (client->gid == ccache->gid) {
	if (read_p)
	    mask |= S_IRGRP;
	if (write_p)
	    mask |= S_IWGRP;
    } else {
	if (read_p)
	    mask |= S_IROTH;
	if (write_p)
	    mask |= S_IWOTH;
    }

    ret = ((ccache->mode & mask) == mask) ? 0 : KRB5_FCC_PERM;

out:
    if (ret) {
	kcm_log(2, "Process %d is not permitted to call %s on cache %s",
		client->pid, kcm_op2string(opcode), ccache->name);
    }

    return ret;
}

krb5_error_code
kcm_chmod(krb5_context context,
	  kcm_client *client,
	  kcm_ccache ccache,
	  u_int16_t mode)
{
    KCM_ASSERT_VALID(ccache);

    /* System cache mode can only be set at startup */
    if (ccache->flags & KCM_FLAGS_OWNER_IS_SYSTEM)
	return KRB5_FCC_PERM;

    if (ccache->uid != client->uid)
	return KRB5_FCC_PERM;

    if (ccache->gid != client->gid)
	return KRB5_FCC_PERM;

    HEIMDAL_MUTEX_lock(&ccache->mutex);

    ccache->mode = mode;

    HEIMDAL_MUTEX_unlock(&ccache->mutex);

    return 0;
}

krb5_error_code
kcm_chown(krb5_context context,
	  kcm_client *client,
	  kcm_ccache ccache,
	  uid_t uid,
	  gid_t gid)
{
    KCM_ASSERT_VALID(ccache);

    /* System cache owner can only be set at startup */
    if (ccache->flags & KCM_FLAGS_OWNER_IS_SYSTEM)
	return KRB5_FCC_PERM;

    if (ccache->uid != client->uid)
	return KRB5_FCC_PERM;

    if (ccache->gid != client->gid)
	return KRB5_FCC_PERM;

    HEIMDAL_MUTEX_lock(&ccache->mutex);

    ccache->uid = uid;
    ccache->gid = gid;

    HEIMDAL_MUTEX_unlock(&ccache->mutex);

    return 0;
}

