/*	$OpenBSD: rde.h,v 1.36 2011/03/24 08:36:00 claudio Exp $ */

/*
 * Copyright (c) 2004, 2005 Esben Norby <norby@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _RDE_H_
#define _RDE_H_

#include <sys/types.h>
#include <sys/time.h>
#include <sys/tree.h>
#include <sys/queue.h>
#include <event.h>
#include <limits.h>

struct v_nexthop {
	TAILQ_ENTRY(v_nexthop)	 entry;
	struct vertex		*prev;
	struct in_addr		 nexthop;
};

TAILQ_HEAD(v_nexthead, v_nexthop);

struct vertex {
	RB_ENTRY(vertex)	 entry;
	TAILQ_ENTRY(vertex)	 cand;
	struct v_nexthead	 nexthop;
	struct event		 ev;
	struct area		*area;
	struct lsa		*lsa;
	time_t			 changed;
	time_t			 stamp;
	u_int32_t		 cost;
	u_int32_t		 peerid;	/* neighbor unique imsg ID */
	u_int32_t		 ls_id;
	u_int32_t		 adv_rtr;
	u_int8_t		 type;
	u_int8_t		 flooded;
	u_int8_t		 deleted;
	u_int8_t		 self;
};

struct rde_req_entry {
	TAILQ_ENTRY(rde_req_entry)	entry;
	u_int32_t			ls_id;
	u_int32_t			adv_rtr;
	u_int8_t			type;
};

/* just the info RDE needs */
struct rde_nbr {
	LIST_ENTRY(rde_nbr)		 entry, hash;
	struct in_addr			 id;
	struct in_addr			 area_id;
	TAILQ_HEAD(, rde_req_entry)	 req_list;
	struct area			*area;
	u_int32_t			 peerid;	/* unique ID in DB */
	int				 state;
	int				 self;
};

struct rt_nexthop {
	TAILQ_ENTRY(rt_nexthop)	entry;
	struct in_addr		nexthop;
	struct in_addr		adv_rtr;
	time_t			uptime;
	u_int8_t		connected;
	u_int8_t		invalid;
};

struct rt_node {
	RB_ENTRY(rt_node)	entry;
	TAILQ_HEAD(,rt_nexthop)	nexthop;
	struct in_addr		prefix;
	struct in_addr		area;
	u_int32_t		cost;
	u_int32_t		cost2;
	u_int32_t		ext_tag;
	enum path_type		p_type;
	enum dst_type		d_type;
	u_int8_t		flags;
	u_int8_t		prefixlen;
	u_int8_t		invalid;
};

struct abr_rtr {
	struct in_addr		 addr;
	struct in_addr		 abr_id;
	struct in_addr		 dst_ip;
	struct in_addr		 area;
	u_int16_t		 metric;
};

extern struct lsa_tree	asext_tree;

/* rde.c */
pid_t		 rde(struct ospfd_conf *, int [2], int [2], int [2]);
int		 rde_imsg_compose_ospfe(int, u_int32_t, pid_t, void *,
		     u_int16_t);
u_int32_t	 rde_router_id(void);
struct area	*rde_backbone_area(void);
void		 rde_send_change_kroute(struct rt_node *);
void		 rde_send_delete_kroute(struct rt_node *);
void		 rde_nbr_del(struct rde_nbr *);
int		 rde_nbr_loading(struct area *);
struct rde_nbr	*rde_nbr_self(struct area *);
void		 rde_summary_update(struct rt_node *, struct area *);
struct lsa	*orig_sum_lsa(struct rt_node *, struct area *, u_int8_t, int);

/* rde_lsdb.c */
void		 lsa_init(struct lsa_tree *);
int		 lsa_compare(struct vertex *, struct vertex *);
void		 vertex_free(struct vertex *);
void		 vertex_nexthop_clear(struct vertex *);
void		 vertex_nexthop_add(struct vertex *, struct vertex *,
		    u_int32_t);
int		 lsa_newer(struct lsa_hdr *, struct lsa_hdr *);
int		 lsa_check(struct rde_nbr *, struct lsa *, u_int16_t);
int		 lsa_self(struct rde_nbr *, struct lsa *, struct vertex *);
int		 lsa_add(struct rde_nbr *, struct lsa *);
void		 lsa_del(struct rde_nbr *, struct lsa_hdr *);
void		 lsa_age(struct vertex *);
struct vertex	*lsa_find(struct area *, u_int8_t, u_int32_t, u_int32_t);
struct vertex	*lsa_find_net(struct area *area, u_int32_t);
u_int16_t	 lsa_num_links(struct vertex *);
void		 lsa_snap(struct area *, u_int32_t);
void		 lsa_dump(struct lsa_tree *, int, pid_t);
void		 lsa_merge(struct rde_nbr *, struct lsa *, struct vertex *);
void		 lsa_remove_invalid_sums(struct area *);
void		 lsa_generate_stub_sums(struct area *);

/* rde_spf.c */
void		 spf_calc(struct area *);
void		 rt_calc(struct vertex *, struct area *, struct ospfd_conf *);
void		 asext_calc(struct vertex *);
void		 spf_tree_clr(struct area *);

void		 cand_list_init(void);
void		 cand_list_add(struct vertex *);
struct vertex	*cand_list_pop(void);
int		 cand_list_present(struct vertex *);
void		 cand_list_clr(void);

void		 spf_timer(int, short, void *);
void		 start_spf_timer(void);
void		 stop_spf_timer(struct ospfd_conf *);
void		 start_spf_holdtimer(struct ospfd_conf *);

void		 rt_init(void);
int		 rt_compare(struct rt_node *, struct rt_node *);
struct rt_node	*rt_find(in_addr_t, u_int8_t, enum dst_type);
int		 rt_insert(struct rt_node *);
int		 rt_remove(struct rt_node *);
void		 rt_clear(void);
void		 rt_dump(struct in_addr, pid_t, u_int8_t);
struct rt_node	*rt_lookup(enum dst_type, in_addr_t);

struct lsa_rtr_link	*get_rtr_link(struct vertex *, int);
struct lsa_net_link	*get_net_link(struct vertex *, int);

RB_PROTOTYPE(lsa_tree, vertex, entry, lsa_compare)

#endif	/* _RDE_H_ */
