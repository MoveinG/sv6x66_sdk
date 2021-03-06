/*
 * Copyright 2011-2012 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <ayla/assert.h>
#include <ayla/utypes.h>
#include <ada/err.h>
#include <al/al_clock.h>
#include <ayla/crc.h>
#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <ayla/conf.h>
#include <ayla/notify_proto.h>
#include <ayla/timer.h>
#include <al/al_ada_thread.h>
#include <al/al_net_if.h>
#include <al/al_net_udp.h>
#include <ada/notify.h>
#include <ada/ada_conf.h>

#include <al/al_aes.h>
#include <al/al_hash_sha1.h>
#include <al/al_net_dns.h>
#include <al/al_random.h>

#include "lwip/def.h"

#include "client_lock.h"
#include "notify_int.h"

#define NP_DEBUG	1
#ifdef NP_DEBUG_LOCAL
#define NP_DEBUG_KEY	"0123456789012345"
#define NP_DEBUG_IP	ip_addr(172, 17, 2, 100)
#endif /* NP_DEBUG_LOCAL */

#define NP_KA_MARGIN	3	/* secs before probed value to do keep-alive */
#define NP_MIN_CHANGE_WAIT 1	/* secs, min wait to re-request ANS server */
#define NP_CHANGE_WAIT_MULT 2	/* exponential backoff multiplier */
#define NP_MAX_CHANGE_WAIT 600	/* secs, max change wait */
#define NP_ERR_LIMIT	4	/* consecutive errors before going down */
#define NP_RETRY_INTERVAL 60

/* default # of secs between ans reachability check */
#define NOTIFY_POLL_DEFAULT	300

enum notify_client_state {
	NCS_DOWN = 0,		/* not started or fatal error */
	NCS_DNS_PASS,		/* not started, DNS lookup succeeded */
	NCS_REGISTER,		/* register sent, waiting for response */
	NCS_UP,			/* up, sending keep-alives */
	NCS_CHANGE_WAIT,	/* wait before asking for a new ANS server */
	NCS_DOWN_RETRY,		/* ANS down, in retry loop (until GET dsns) */
};

enum notify_probe_state {
	NCS_PROBE_DOWN = 0,
	NCS_PROBE_SEND,
	NCS_PROBE_WAIT,
	NCS_PROBE_IDLE,
};

u32 np_poll_default = NOTIFY_POLL_DEFAULT;

struct notify_state {
	enum notify_client_state client_state;
	char	notify_host[40];		/* notify server name */
	struct al_net_addr notify_addr;	/* IP address of server */
	u16	sequence;
	u16	keep_alive;			/* period for keep-alives */
	be32	reg_key;

	/*
	 * Probe-related state.
	 */
	enum notify_probe_state probe_state;
	u16	probe_seq;
	u16	probe;				/* trial period for probes */
	u16	min_probe;
	u16	max_probe;
	u16	change_wait;
	u8	retries_left;
	u8	probe_retries_left;
	u8	probes_needed;
	u8	err_count;
	struct al_net_udp *probe_pcb;

	u8	probe_send:1;	/* send a probe instead of a keep-alive */
	u8	restart_probing:1;	/* restart probing when conn resumes */
	u64	probe_time;	/* time for the next probe */
	u32	poll_interval;	/* time between probes */
	u16	probe_sequence;	/* sequence number for probes */

	struct al_net_udp *pcb;
	void	(*notify)(void);
	struct al_net_dns_req dns_req;

	u8 *aes_key;
	size_t aes_key_len;
	struct al_aes_ctxt *aes_rx_ctx;
	struct al_aes_ctxt *aes_tx_ctx;

	struct timer probe_timer;
	struct timer timer;
};

static inline const char *np_op_name(enum np_op op)
{
#ifdef NP_DEBUG
	static const char *np_ops[] = NP_OPS;
#define ARRAY_LEN(x) (sizeof(x) / sizeof(*(x)))

	if (op < ARRAY_LEN(np_ops) && np_ops[op]) {
		return np_ops[op];
	}
#endif /* NP_DEBUG */
	return "unknown";
}

static struct notify_state notify_state;

static void np_down(struct notify_state *);
static void np_reset_keep_alive(struct notify_state *);
static enum ada_err np_send(struct notify_state *, struct al_net_udp *,
	void *am, size_t am_len);

static void np_log(const char *fmt, ...)
	ADA_ATTRIB_FORMAT(1, 2);

#define notify_timer_set(t, n)	al_ada_timer_set(t, n)
#define notify_timer_cancel(t)	al_ada_timer_cancel(t)
#define notify_udp_close(p)	al_net_udp_free(p)
static enum ada_err notify_udp_open(struct al_net_udp **pudp,
	struct al_net_addr *addr, u16 port, void *cb_arg);
static enum ada_err notify_udp_send(struct al_net_udp *udp,
	void *buf, size_t len);

static void np_log(const char *fmt, ...)
{
	ADA_VA_LIST args;

	ADA_VA_START(args, fmt);
	log_put_va(MOD_LOG_NOTIFY, fmt, args);
	ADA_VA_END(args);
}

/*
 * notify command.
 */
void np_cli(int argc, char **argv)
{
	if (argc == 1) {
		np_status();
		printcli("notify: poll_interval: %lu",
		    np_poll_default);
	}
	printcli("usage: notify");
	return;
}

static void np_close(struct al_net_udp **pcbp)
{
	struct al_net_udp *pcb = *pcbp;

	if (pcb) {
		*pcbp = NULL;
		notify_udp_close(pcb);
	}
}

/*
 * Allocate and set up header for new packet (not for DSN use).
 * Size includes the encapsulation header.
 */
static void np_init_head(struct np_head *head, enum np_op op, u16 seq)
{
#if NP_VERS != 0
	head->ver = NP_VERS;
#endif
	head->op = op;
	head->seq = htons(seq);
	head->time = (u32)al_clock_get_total_ms();
	head->time = htonl(head->time);
}

/*
 * Allocate new packet.
 */
static void *np_new_packet(size_t size)
{
	struct notify_state *state = &notify_state;
	void *am;

	am = al_net_udp_buf_alloc(size);
	if (!am) {
		np_log(LOG_WARN "new_packet: al_net_udp_buf_alloc failed "
		    "state %d/%d size %zd",
		    state->client_state, state->probe_state, size);
		return NULL;
	}
	memset(am, 0, size);
	return am;
}

/*
 * Allocate new packet for FMT_IV_KEY encapsulation.
 */
static void *np_new_packet_key(size_t size, size_t *p_am_len)
{
	struct notify_state *state = &notify_state;
	void *am;
	struct np_encaps_key *encaps;
	u8 crc;

	size += sizeof(crc) - sizeof(struct np_encaps_key);
	size += -size & (NP_PAD - 1);
	size += sizeof(struct np_encaps_key);
	am = np_new_packet(size);
	if (am) {
		encaps = am;
		encaps->format = NP_FMT_IV_KEY;
		encaps->reg_key = state->reg_key;
	}
	*p_am_len = size;
	return am;
}

static int np_check_resp(struct notify_state *state, void *am, size_t am_len,
	size_t min_len, enum notify_client_state exp_state)
{
	struct np_encaps_key *encaps = am;
	struct np_head *head = (struct np_head *)(encaps + 1);

	if (am_len < min_len) {
		np_log(LOG_WARN "check_resp: too short %u", am_len);
		return -1;
	}
	if (ntohs(head->seq) != state->sequence) {
		np_log(LOG_WARN "check_resp: wrong seq %x exp %x",
		    ntohs(head->seq), state->sequence); /* XXX */
		return -1;
	}
	if (state->client_state != exp_state) {
		np_log(LOG_WARN "check_resp: wrong state %d",
		    state->client_state);
		return -1;
	}
	if (state->reg_key && state->reg_key != encaps->reg_key) {
		np_log(LOG_WARN "check_resp: wrong key %lx exp %lx state %d",
		    encaps->reg_key, state->reg_key, state->client_state);
		return -1;
	}
	return 0;
}

static void np_post_event(struct notify_state *state)
{
	state->notify();
}

static void np_down_event(struct notify_state *state)
{
	np_down(state);
	al_net_addr_set_ipv4(&state->notify_addr, 0);
	state->restart_probing = 1;
	np_post_event(state);
}

static void np_probe_down(struct notify_state *state)
{
	np_close(&state->probe_pcb);
	state->probe_state = NCS_PROBE_DOWN;
	notify_timer_cancel(&state->probe_timer);
}

static void np_down_retry_event(struct notify_state *state)
{
	if (state->client_state == NCS_UP) {
		if (state->probe_state != NCS_PROBE_IDLE) {
			state->restart_probing = 1;
		} else {
			state->restart_probing = 0;
		}
	}
	state->probe_send = 0;
	notify_timer_cancel(&state->timer);
	state->client_state = NCS_DOWN_RETRY;
	np_probe_down(state);
	np_post_event(state);
}

static void np_down(struct notify_state *state)
{
	np_close(&state->pcb);
	state->client_state = NCS_DOWN;
	state->probe_send = 0;
	notify_timer_cancel(&state->timer);
	np_probe_down(state);
	np_log(LOG_DEBUG "down");
}

static void np_probe_init(struct notify_state *state)
{
	state->probe = NP_INIT_PROBE;
	state->min_probe = NP_MIN_PROBE;
	state->max_probe = NP_MAX_PROBE;
	state->retries_left = NP_MAX_TRY;
	state->probes_needed = NP_INIT_PROBES;
	state->probe_retries_left = NP_MAX_TRY;
}

static void np_req_probe(struct notify_state *state)
{
	void *am;
	size_t am_len;
	struct np_req_probe *probe;
	enum ada_err error;

	if (!state->probe_retries_left) {
		np_probe_down(state);
		return;
	}
	state->probe_retries_left--;

	state->sequence++;
	state->probe_seq = state->sequence;
	am = np_new_packet_key(sizeof(*probe), &am_len);
	if (!am) {
		/* temporary buf allocation problem, try later */
		goto reset_timer;
	}

	probe = am;
	np_init_head(&probe->head, NP_REQ_PROBE, state->probe_seq);
	probe->probe_delay = htons(state->probe);

	np_log(LOG_DEBUG
	    "req_probe: sending seq %x with probe %u min %u max %u",
	    state->probe_seq, state->probe, state->min_probe, state->max_probe);

	error = np_send(state, state->probe_pcb, am, am_len);
	if (error != AE_OK && error != AE_BUF &&
	    error != AE_INVAL_VAL) {
		np_down_retry_event(state);
		return;
	}
reset_timer:
	notify_timer_set(&state->probe_timer, NP_RESP_WAIT * 1000);
	state->probe_state = NCS_PROBE_SEND;
}

static void np_req_probe_new(struct notify_state *state)
{
	u16 probe_delay;

	/*
	 * If min_probe + 12.5% is less than max_probe, we're done probing.
	 */
	if (state->min_probe + state->min_probe / 8 >= state->max_probe) {
		state->probe = state->min_probe;
		state->keep_alive = state->min_probe;
		state->probe_state = NCS_PROBE_IDLE;
		np_log(LOG_DEBUG "req_probe_new: probe state set to idle");
		np_reset_keep_alive(state);
		return;
	}

	/*
	 * Either double the probe or take the average between min and max.
	 */
	probe_delay = (state->min_probe + state->max_probe) / 2;
	if (probe_delay > state->probe * 2) {
		probe_delay = state->probe * 2;
	}
	state->probe = probe_delay;

	if (state->keep_alive < state->min_probe) {
		state->keep_alive = state->min_probe;
		np_reset_keep_alive(state);
	}
	state->probes_needed = NP_INIT_PROBES;
	state->probe_retries_left = NP_MAX_TRY;
	np_req_probe(state);
}

static void np_probe_wait(struct notify_state *state)
{
	notify_timer_set(&state->probe_timer,
	    (state->probe + NP_PROBE_GRACE) * 1000);
	state->probe_state = NCS_PROBE_WAIT;
}

static void np_recv_req_probe_resp(struct notify_state *state,
		void *am, size_t am_len, struct al_net_udp *pcb)
{
	struct np_resp *resp = am;

	if (am_len < sizeof(*resp)) {
		np_log(LOG_WARN "recv_req_probe_resp: len %u too short",
		    am_len);
		return;
	}

	if (state->probe_state == NCS_PROBE_SEND &&
	    ntohs(resp->head.seq) == state->probe_seq) {
		if (resp->error) {
			np_log(LOG_WARN "recv_req_probe_resp: error %u",
			    resp->error);
			np_down_retry_event(state);
		} else {
			state->err_count = 0;
			np_probe_wait(state);
		}
	}
}

static void np_setup_probe_send(struct notify_state *state)
{
	u64 curtime = al_clock_get_total_ms();
	u32 next_timeout;

	/* Send probe to check for ANS reachability */
	state->probe_send = 1;
	/* don't retry if notify is not up */
	state->retries_left = (state->client_state == NCS_UP) ? NP_MAX_TRY : 1;
	/* fix the probe sequence to use for all the probes */
	state->probe_sequence = state->sequence;
	if (TSTAMP_LT(state->probe_time, curtime)) {
		next_timeout = 0;
	} else {
		next_timeout = state->probe_time - curtime;
	}
	notify_timer_set(&state->timer, next_timeout);
}

static void np_set_next_np_timeout(struct notify_state *state, int correction)
{
	u32 next_timeout;
	u64 curtime = al_clock_get_total_ms();
	u64 next_keep_alive_time;

	notify_timer_cancel(&state->timer);
	next_keep_alive_time = curtime +
	    (state->keep_alive - NP_KA_MARGIN + correction) * 1000;

	if (state->probe_send) {
		next_timeout = (NP_RESP_WAIT + correction) * 1000;
	} else if (TSTAMP_LT(state->probe_time, next_keep_alive_time)) {
		np_setup_probe_send(state);
		return;
	} else {
		state->probe_send = 0;
		if (state->keep_alive < NP_KA_MARGIN - correction) {
			next_timeout = 0;
		} else {
			next_timeout = (state->keep_alive - NP_KA_MARGIN +
			    correction) * 1000;
		}
	}
	notify_timer_set(&state->timer, next_timeout);
}

/*
 * Registration success or probe succcess.
 */
static void np_bring_up(struct notify_state *state)
{
	state->err_count = 0;
	state->change_wait = 0;
	state->client_state = NCS_UP;
	state->keep_alive = state->probe;
	np_reset_keep_alive(state);
	np_post_event(state);
}

static void np_recv_probe_resp(struct notify_state *state,
		void *am, size_t am_len, struct al_net_udp *pcb)
{
	struct np_resp *resp = am;

	if (am_len < sizeof(*resp)) {
		np_log(LOG_WARN "recv_probe_resp: len %u too short",
		    am_len);
exit:
		return;
	}
	if (!state->probe_send ||
	    state->probe_sequence != ntohs(resp->head.seq)) {
		np_log(LOG_WARN "recv_probe_resp: response without probe");
		goto exit;
	}
	state->probe_send = 0;
	state->probe_time = al_clock_get_total_ms() +
		state->poll_interval * 1000;
	if (state->client_state == NCS_DOWN_RETRY) {
		state->poll_interval = np_poll_default;
		if (state->restart_probing) {
			if (state->probe_pcb == NULL &&
			    notify_udp_open(&state->probe_pcb,
				&state->notify_addr, NP_UDP_PORT2, state)) {
				np_down_event(state);
				return;
			}
			np_probe_init(state);
			np_probe_wait(state);
		} else {
			state->probe_state = NCS_PROBE_IDLE;
		}
		np_bring_up(state);
		return;
	}

	/* need NP_RESP_WAIT correction to account for probe resp time */
	np_set_next_np_timeout(state, NP_RESP_WAIT * -1);
}

static void np_recv_event(struct notify_state *state, void *am,
			struct al_net_udp *pcb)
{
	struct np_event *event = am;
	void *resp_am;
	struct np_resp *resp;
	size_t am_len;

	/*
	 * Caller has verified buf contains np_head.
	 * Acknowledge event.
	 */
	resp_am = np_new_packet_key(sizeof(struct np_resp), &am_len);
	if (resp_am) {
		resp = resp_am;
		np_init_head(&resp->head, NP_NOTIFY_RESP,
		    ntohs(event->head.seq));
		np_send(state, pcb, resp_am, am_len);
	}
	np_post_event(state);
}

/*
 * Handle Probe.
 * Caller has verified buf contains np_head.
 */
static void np_recv_probe(struct notify_state *state, void *am,
			struct al_net_udp *pcb)
{
	struct np_probe *probe = am;
	struct np_resp *resp;
	void *resp_am;
	size_t am_len;

	np_log(LOG_DEBUG "recv_probe: seq %x", ntohs(probe->head.seq));

	/*
	 * Acknowledge probe.
	 */
	resp_am = np_new_packet_key(sizeof(*resp), &am_len);
	if (resp_am) {
		resp = resp_am;
		np_init_head(&resp->head, NP_PROBE_RESP,
		    ntohs(probe->head.seq));
		resp->error = NP_ERR_NONE;
		np_send(state, pcb, resp_am, am_len);
	}

	if (state->probe_state != NCS_PROBE_WAIT ||
	    ntohs(probe->head.seq) != state->probe_seq) {
		return;
	}

	if (state->probes_needed > 1) {
		state->probes_needed--;
		state->probe_retries_left = NP_MAX_TRY;
		np_req_probe(state);
	} else {
		state->min_probe = state->probe;
		np_req_probe_new(state);
	}
}

static void np_probe_timeout(struct timer *arg)
{
	struct notify_state *state = &notify_state;

	notify_lock();
	switch (state->probe_state) {
	case NCS_PROBE_SEND:
		np_log(LOG_DEBUG "probe_timeout: state probe_send probe %u",
		    state->probe);
		np_req_probe(state);
		break;
	case NCS_PROBE_WAIT:
		np_log(LOG_DEBUG "probe_timeout: state probe_wait new max %u",
		    state->probe);
		state->max_probe = state->probe;
		np_req_probe_new(state);
		break;
	default:
		break;
	}
	notify_unlock();
}

/*
 * Create the SHA-1 signature of the payload and key for the payload.
 */
static void np_sign(struct notify_state *state,
	const void *payload, size_t len, void *sign)
{
	/* adc_sha1(payload, len, state->aes_key, state->aes_key_len, sign); */

	struct al_hash_sha1_ctxt *ctx;
	ctx = al_hash_sha1_ctxt_alloc();
	ASSERT(ctx);
	al_hash_sha1_ctxt_init(ctx);
	al_hash_sha1_add(ctx, payload, len);
	al_hash_sha1_add(ctx, state->aes_key, state->aes_key_len);
	al_hash_sha1_final(ctx, sign);
	al_hash_sha1_ctxt_free(ctx);
}

/*
 * Registration message doesn't get encrypted because its how we get our
 * key and the notify protocol doesn't know who we are yet.  We do include
 * a signature, however, to verify that we know the key.
 */
static void np_register(struct notify_state *state)
{
	void *am;
	u8 *payload;
	struct np_encaps_dsn *encaps;
	struct np_register *req;
	size_t dsn_len;
	size_t len;
	size_t pad;
	enum ada_err error;

	len = strlen(ada_conf.dsn) + 1;
	pad = 0;
	if (len & 3) {
		pad = 4 - (len & 3);
		len += pad;
	}
	if (len == 0 || len > 255) {
		np_log(LOG_ERR "dev id too long");
		np_down_event(state);
		return;
	}

	state->sequence++;
	state->probe_seq = state->sequence;
	state->reg_key = 0;

	dsn_len = len;
	len += sizeof(*encaps) + sizeof(*req) + sizeof(struct np_sig);
	am = np_new_packet(len);
	if (!am) {
		/* temp buf alloc problem, try again later */
		goto set_timers;
	}
	payload = am;
	encaps = (struct np_encaps_dsn *)payload;
	encaps->format = NP_FMT_DSN;
	encaps->dsn_len = dsn_len;
	memcpy(encaps + 1, ada_conf.dsn, dsn_len - pad);

	req = (struct np_register *)(payload + sizeof(*encaps) + dsn_len);
	np_init_head(&req->head, NP_REG, state->sequence);
	req->probe_delay = htons(state->probe);

	len -= sizeof(struct np_sig);
	np_sign(state, payload, len, payload + len);

	np_log(LOG_DEBUG "register: sending seq %x with probe %u",
	    state->sequence, state->probe);

	len += sizeof(struct np_sig);
	error = np_send(state, state->pcb, am, len);
	if (error != AE_OK && error != AE_BUF && error != AE_INVAL_VAL) {
		np_log(LOG_WARN "register: send err %d", error);
		np_down_event(state);
		return;
	}
set_timers:
	notify_timer_set(&state->timer, NP_RESP_WAIT * 1000);
	state->client_state = NCS_REGISTER;
}

static void np_recv_reg_resp(struct notify_state *state, void *am,
	size_t am_len)
{
	struct np_reg_resp *resp;
	u16 ka_period;

	resp = am;
	if (np_check_resp(state, am, am_len, sizeof(*resp), NCS_REGISTER)) {
		return;
	}
	notify_timer_cancel(&state->timer);
	if (resp->error != NP_ERR_NONE) {
		np_log(LOG_WARN "recv_reg_resp: resp error %d", resp->error);
		np_down_event(state);
		return;
	}
	ka_period = ntohs(resp->ka_period);
	if (ka_period > NP_KA_MARGIN) {
		state->keep_alive = ka_period;
	}
	state->reg_key = resp->encaps.reg_key;
	np_bring_up(state);
	np_probe_wait(state);
}

/*
 * Notify client to check for ANS server change.
 * Wait a bit first.  Each time we do this, wait longer.
 */
static void np_change(struct notify_state *state)
{
	state->client_state = NCS_CHANGE_WAIT;
	al_net_addr_set_ipv4(&state->notify_addr, 0);
	state->probe_send = 0;
	if (state->change_wait == 0) {
		state->change_wait = NP_MIN_CHANGE_WAIT;
	} else {
		state->change_wait *= NP_CHANGE_WAIT_MULT;
		if (state->change_wait > NP_MAX_CHANGE_WAIT) {
			state->change_wait = NP_MAX_CHANGE_WAIT;
		}
	}
	notify_timer_set(&state->timer, state->change_wait * 1000);
}

static void np_error(struct notify_state *state)
{
	if (state->err_count++ > NP_ERR_LIMIT) {
		np_log(LOG_WARN "np_error: limit of %u exceeded - "
		    "requesting change", NP_ERR_LIMIT);
		np_change(state);
	}
}

/*
 * An error response was received for a registration.
 * The response is in format NS_FMT_DSN_ERR.
 * The payload should be the same as our request.
 */
static void np_recv_reg_err(struct notify_state *state, void *am,
		size_t am_len)
{
	struct np_encaps_dsn *encaps;
	struct np_register *reg;
	struct np_sig sig;
	size_t len;

	np_log(LOG_DEBUG "recv_reg_err");

	len = sizeof(*encaps) + sizeof(*reg) + sizeof(sig);
	if (am_len < len) {
		np_log(LOG_WARN "recv_reg_err:  len %u (<%d)", am_len, len);
		return;
	}
	encaps = am;
	len += encaps->dsn_len;
	if (am_len < len) {
		np_log(LOG_WARN "recv_reg_err:  len %u dsn_len %u",
		    am_len, encaps->dsn_len);
		return;
	}
	if (encaps->dsn[encaps->dsn_len - 1] != '\0') {
		np_log(LOG_WARN "recv_reg_err:  dsn not terminated");
		return;
	}
	if (strcmp(encaps->dsn, ada_conf.dsn)) {
		np_log(LOG_WARN "recv_reg_err:  dsn mismatch %s", encaps->dsn);
		return;
	}
	reg = (struct np_register *)((u8 *)am +
	    sizeof(*encaps) + encaps->dsn_len);
	if (reg->head.ver != NP_VERS || reg->head.op != NP_REG) {
		np_log(LOG_WARN "recv_reg_err:  ver/op mismatch ver %u op %u",
		    reg->head.ver, reg->head.op);
		return;
	}
	if (ntohs(reg->head.seq) != state->sequence) {
		np_log(LOG_WARN "recv_reg_err:  seq mismatch");
		return;
	}
	/*
	 * Check signature.
	 * A bad signature indicates a replay DOS attack or an old response.
	 */
	encaps->format = NP_FMT_DSN;	/* reset format so signature matches */
	np_sign(state, am, len - sizeof(sig), &sig);
	if (memcmp(reg + 1, &sig, sizeof(sig))) {
		np_log(LOG_WARN "recv_reg_err: bad signature");
		return;
	}
	np_change(state);
}

/*
 * Send NP packet after encrypting it.
 * This frees the buf even if error occurs.
 */
static enum ada_err np_send(struct notify_state *state,
			struct al_net_udp *pcb, void *am,
			size_t am_len)
{
	enum ada_err error;
	struct np_encaps_key *encaps;
	enum np_format format;
	u8 *crc;
	size_t offset;
	int rc;

	encaps = (struct np_encaps_key *)am;
	format = encaps->format;

#if NP_DEBUG
	if (format == NP_FMT_IV_KEY) {
		struct np_head *head;

		head = (struct np_head *)(encaps + 1);
		np_log(LOG_DEBUG "send: sending seq %x op %d %s",
		    ntohs(head->seq), head->op, np_op_name(head->op));
	}
#endif /* NP_DEBUG */

	if (format == NP_FMT_IV_KEY && state->aes_key_len) {

		/*
		 * If we cannot simply get the IV from the AES context,
		 * generate a new random one and re-init the context.
		 */
		if (al_aes_iv_get(state->aes_tx_ctx,
		    encaps->iv, sizeof(encaps->iv))) {
			al_random_fill(encaps->iv, sizeof(encaps->iv));
			if (al_aes_cbc_key_set(state->aes_tx_ctx,
			    state->aes_key, state->aes_key_len,
			    encaps->iv, 0)) {
				np_log(LOG_WARN "send: AES key init err");
				return AE_INVAL_VAL;		/* XXX */
			}
		}
		offset = sizeof(*encaps);

		/*
		 * Add CRC.
		 */
		crc = (u8 *)am + am_len - sizeof(*crc);
		*crc = crc8((u8 *)am + offset,
			am_len - offset - sizeof(*crc), CRC8_INIT);
		np_log(LOG_DEBUG "%s() Tx Packet-AES (len = %d, crc = 0x%02X)",
			__func__, am_len, (int)(*crc));
		/* log_bytes_in_hex(MOD_LOG_NOTIFY, am, am_len); */

		/*
		 * encrypt in place
		 */
		rc = al_aes_cbc_encrypt(state->aes_tx_ctx,
		    am + offset, am + offset, am_len - offset);
		if (rc < 0) {
			np_log(LOG_ERR "encrypt err %u", rc);
			error = AE_INVAL_VAL;
			goto err;
		}
	} else {
		np_log(LOG_DEBUG "%s() Tx Packet (len = %d)", __func__, am_len);
		/* log_bytes_in_hex(MOD_LOG_NOTIFY, am, am_len); */
	}

	error = notify_udp_send(pcb, am, am_len);
	if (error == AE_IN_PROGRESS) {
		np_log(LOG_DEBUG "udp_send err inprogress");
		error = AE_OK;
	} else if (error != AE_OK) {
		np_log(LOG_ERR "udp_send err %d", error);
	}

	/* After sent, the buffer will be freed by net-udp,
	 * so it should not be freed here.
	 */
	return error;

err:
	al_net_udp_buf_free(am);
	return error;
}

/*
 * Open PCBs
 */
static int np_open_pcbs(struct notify_state *state)
{
	enum ada_err err;

	err = notify_udp_open(&state->pcb, &state->notify_addr, NP_UDP_PORT,
	    &notify_state);
	if (err) {
		np_log(LOG_ERR "open: err %d port %u", err, NP_UDP_PORT);
		return -1;
	}
	err = notify_udp_open(&state->probe_pcb, &state->notify_addr,
	    NP_UDP_PORT2, &notify_state);
	if (err) {
		np_log(LOG_ERR "open: err %d port %u", err, NP_UDP_PORT2);
		np_close(&state->pcb);
		return -1;
	}
	return 0;
}

/*
 * Reset timer for next reach check
 */
static void np_setup_next_reach_check(struct notify_state *state)
{
	notify_timer_set(&state->timer, state->poll_interval * 1000);
	np_log(LOG_DEBUG "next reach check in %lu secs", state->poll_interval);
}

/*
 * ANS DNS resolved callback.
 */
static void np_dns_cb(struct al_net_dns_req *req)
{
	struct notify_state *state =
	    CONTAINER_OF(struct notify_state, dns_req, req);
	u8 *bp;
	u8 diffipaddr;
	u32 addr;

	client_lock();

	if (strcmp(req->hostname, state->notify_host)) {
		/* dns cb for old ANS server, drop result */
		np_log(LOG_WARN "%s result drop, %s != %s", __func__,
		    req->hostname, state->notify_host);
		req->hostname = NULL;
		client_unlock();
		return;
	}
	req->hostname = NULL;		/* indicates DNS not in progress */

	if (state->client_state == NCS_REGISTER ||
	    state->client_state == NCS_UP) {
		/* notifier is already up and running, drop the result */
		client_unlock();
		return;
	}
	if (req->error == AL_ERR_OK) {
		addr = al_net_addr_get_ipv4(&req->addr);
		bp = (u8 *)&addr;
		np_log(LOG_INFO "ANS server %s at %u.%u.%u.%u",
		    state->notify_host, bp[0], bp[1], bp[2], bp[3]);
		diffipaddr =
		    (al_net_addr_get_ipv4(&state->notify_addr) != addr);
		al_net_addr_set_ipv4(&state->notify_addr, addr);
		if (state->client_state == NCS_DOWN_RETRY) {
			if (diffipaddr) {
				np_down(state);
				state->client_state = NCS_DOWN_RETRY;
				np_open_pcbs(state);
			}
			state->probe_time = 0;
			np_setup_probe_send(state);
		} else if (state->client_state != NCS_REGISTER &&
		    state->client_state != NCS_UP) {
			state->client_state = NCS_DNS_PASS;
			np_post_event(state);
		}
	} else {
		np_log(LOG_WARN "ANS server %s DNS lookup failed",
		    state->notify_host);
		if (state->client_state == NCS_DOWN_RETRY) {
			np_setup_next_reach_check(state);
		}
	}

	client_unlock();
}

void np_set_server(const char *name)
{
	struct notify_state *state = &notify_state;
	struct al_net_dns_req *req = &state->dns_req;

	/* To eliminate error reported by valgrind (memory check tool) that
	   source and destination overlap in strncpy.
	*/
	if (name != state->notify_host) {
		strncpy(state->notify_host, name,
			sizeof(state->notify_host) - 1);
	}
	if (req->hostname) {		/* request pending */
		return;
	}
	if (name[0] == '\0') {
		state->notify_host[0] = '\0';
		al_net_addr_set_ipv4(&state->notify_addr, 0);
		np_down_event(state);
		return;
	}
	req->hostname = state->notify_host;
	req->callback = np_dns_cb;
	if (al_dns_req_ipv4_start(req)) {
		req->hostname = NULL;

		np_log(LOG_ERR "%s dns lookup failed", name);

		if (state->client_state == NCS_DOWN_RETRY) {
			np_setup_next_reach_check(state);
		} else {
			np_down_event(state);
		}
	}
	np_log(LOG_DEBUG "%s( %s )", __func__, name);
}

void np_retry_server(void)
{
	struct notify_state *state = &notify_state;

	np_set_server(state->notify_host);
}

/*
 * Sends keep alives OR probes
 */
static void np_send_keep_alive(struct notify_state *state)
{
	void *am;
	size_t am_len;
	struct np_keep_alive *ka;
	size_t pbuf_size = (state->probe_send) ?
	    sizeof(struct np_probe) : sizeof(struct np_keep_alive);
	enum ada_err np_send_err;

	am = np_new_packet_key(pbuf_size, &am_len);
	if (!am) {
		np_log(LOG_ERR "send_keep_alive: pbuf alloc failed");
		return;
	}
	ka = am;
	if (state->probe_send) {
		if (!state->retries_left) {
			np_down_retry_event(state);
			return;
		}
		np_init_head(&ka->head, NP_PROBE, state->probe_sequence);
		state->retries_left--;
	} else {
		state->sequence++;
		np_init_head(&ka->head, NP_KEEP_ALIVE, state->sequence);
		ka->ka_period = htons(state->keep_alive);
	}

	np_send_err = np_send(state, state->pcb, am, am_len);
	if (np_send_err != AE_OK && np_send_err != AE_BUF &&
	    np_send_err != AE_INVAL_VAL) {
		/* non-recoverable error */
		np_down_retry_event(state);
		return;
	}

	np_set_next_np_timeout(state, 0);
}

static void np_recv_keep_alive_resp(struct notify_state *state,
	void *am, size_t am_len, struct al_net_udp *pcb)
{
	struct np_resp *resp = am;

	if (np_check_resp(state, am, am_len, sizeof(*resp), NCS_UP)) {
		return;
	}
	if (resp->error != NP_ERR_NONE) {
		np_log(LOG_WARN
		    "recv_keep_alive_resp: resp error %u - restarting",
		    resp->error);
		np_probe_init(state);
		np_register(state);
	} else {
		state->err_count = 0;
	}
}

static void np_reset_keep_alive(struct notify_state *state)
{
	notify_timer_cancel(&state->timer);
	state->probe_time = al_clock_get_total_ms() +
		state->poll_interval * 1000;
	np_send_keep_alive(state);
}

static void np_timeout(struct timer *arg)
{
	struct notify_state *state = &notify_state;

	notify_lock();
	switch (state->client_state) {
	case NCS_REGISTER:
		if (!state->retries_left) {
			np_down_event(state);
		} else {
			state->retries_left--;
			np_register(state);
		}
		break;
	case NCS_CHANGE_WAIT:
		np_post_event(state);
		break;
	case NCS_UP:
		np_send_keep_alive(state);
		break;
	case NCS_DOWN_RETRY:
		if (state->probe_send) {
			np_send_keep_alive(state);
			break;
		}
		/* Clear out the DNS cache and check if DNS resolves */
		al_net_dns_delete_host(state->notify_host);
		np_retry_server();
		break;
	default:
		break;
	}
	notify_unlock();
}

enum np_decrypt_err {
	ND_SUCCESS = 0,
	ND_KEY_ERR = -1,
	ND_CRYPT_ERR = -2,
	ND_ARG_ERR = -3,
	ND_CRC_ERR = -4,
};

/*
 * Decrypt the payload in a buffer encapsuled in format NP_FMT_IV_KEY.
 * Returns a non-negative number on success.
 */
static enum np_decrypt_err np_decrypt(struct notify_state *state,
					void *buf, size_t len)
{
	struct np_encaps_key *encaps;
	u8 crc;

	if (len < sizeof(*encaps)) {
		np_log(LOG_WARN "np_decrypt: too short for IV. len %u", len);
		return ND_ARG_ERR;
	}
	encaps = buf;
	if (al_aes_cbc_key_set(state->aes_rx_ctx,
	    state->aes_key, state->aes_key_len, encaps->iv, 1)) {
		np_log(LOG_WARN "decrypt: init failed");
		return ND_KEY_ERR;
	}
	buf = (u8 *)buf + sizeof(*encaps);
	len -= sizeof(*encaps);
	if (al_aes_cbc_decrypt(state->aes_rx_ctx, buf, buf, len)) {
		return ND_CRYPT_ERR;
	}

	/*
	 * Check CRC.
	 */
	crc = crc8(buf, len, CRC8_INIT);
	if (crc) {
		np_log(LOG_WARN "decrypt: saw invalid crc %x", crc);
		return ND_CRC_ERR;
	}
	return ND_SUCCESS;
}

static void np_recv(void *arg, struct al_net_udp *pcb, void *am,
		size_t len, struct al_net_addr *addr, u16 port,
		struct al_net_if *net_if)
{
	struct notify_state *state = arg;
	struct np_head *head;
	struct np_resp *resp;
	struct np_encaps_key *encaps;
	enum np_decrypt_err rc;

	/* am = al_net_udp_buf_coalesce(am); */
	if (!am) {
		np_log(LOG_ERR "recv: coalesce failed");
		return;
	}
	np_log(LOG_DEBUG "%s( from = 0x%08lX, port = %d, len = %d )",
		__func__, al_net_addr_get_ipv4(addr), port, len);
	/* log_bytes_in_hex(MOD_LOG_NOTIFY, am, len); */

	encaps = am;
	switch (encaps->format) {
	case NP_FMT_IV_KEY:
		if (len < sizeof(*encaps)) {
			np_log(LOG_WARN
			    "recv: too short len %u for encaps iv key", len);
			goto drop;
		}
		if (state->client_state == NCS_UP &&
		    encaps->reg_key != state->reg_key) {
			np_log(LOG_WARN "recv: reg_key %x expected %x",
			    ntohl(encaps->reg_key), ntohl(state->reg_key));
			goto drop;
		}
		/* fall through */
	case NP_FMT_KEY_ERR:
		rc = np_decrypt(state, encaps, len);
		if (rc != ND_SUCCESS) {
			if (encaps->format == NP_FMT_KEY_ERR &&
			    rc == ND_CRC_ERR) {
				np_log(LOG_WARN
				    "recv: err from server not decrypted");
				np_error(state);
				goto drop;
			}
			np_log(LOG_WARN "recv: decrypt rc %d", rc);
			if (rc == ND_CRC_ERR) {
				np_change(state);
				goto drop;
			}
			np_error(state);
			goto drop;
		}
		break;
	case NP_FMT_DSN_ERR:
		notify_lock();
		np_recv_reg_err(state, am, len);
		notify_unlock();
		goto drop;
	case NP_FMT_DSN:
	case NP_FMT_KEY:
	default:
		np_log(LOG_WARN "recv: unexpected format %x", encaps->format);
		goto drop;
	}

	head = (struct np_head *)(encaps + 1);
	if (head->ver != NP_VERS) {
		np_log(LOG_WARN "recv: bad version %x", head->ver); /* XXX */
		goto drop;
	}

#if NP_DEBUG
	np_log(LOG_DEBUG "recv: op %d %s", head->op, np_op_name(head->op));
#endif /* NP_DEBUG */

	notify_lock();
	switch (head->op) {
	case NP_REG_RESP:
		np_recv_reg_resp(state, am, len);
		break;
	case NP_REQ_PROBE:
		if (encaps->format != NP_FMT_KEY_ERR) {
			goto error;
		}
		resp = am;
		resp->error = NP_ERR_NOT_REG;
		/* fall through */
	case NP_REQ_PROBE_RESP:
		np_recv_req_probe_resp(state, am, len, pcb);
		break;
	case NP_PROBE:
		np_recv_probe(state, am, pcb);
		break;
	case NP_KEEP_ALIVE:
		if (encaps->format != NP_FMT_KEY_ERR) {
			goto error;
		}
		resp = am;
		resp->error = NP_ERR_NOT_REG;
		/* fall through */
	case NP_KEEP_ALIVE_RESP:
		np_recv_keep_alive_resp(state, am, len, pcb);
		break;
	case NP_NOTIFY:
		np_recv_event(state, am, pcb);
		break;
	case NP_PROBE_RESP:
		np_recv_probe_resp(state, am, len, pcb);
		break;
	case NP_UNREG:		/* XXX TBD */
	case NP_UNREG_RESP:	/* XXX TBD */
	case NP_REG:
	case NP_NOTIFY_RESP:
	default:
error:
		np_log(LOG_WARN "recv: unexpected op %x", head->op);
		break;
	}
	notify_unlock();
drop:
	al_net_udp_buf_free(am);
	return;
}

enum notify_event np_event_get(void)
{
	struct notify_state *state = &notify_state;
	enum notify_event event;

	notify_lock();
	switch (state->client_state) {
	case NCS_DOWN:
		event = NS_EV_DOWN;
		break;
	case NCS_DOWN_RETRY:
		event = NS_EV_DOWN_RETRY;
		state->poll_interval = NP_RETRY_INTERVAL;
		np_setup_next_reach_check(state);
		break;
	case NCS_DNS_PASS:
		event = NS_EV_DNS_PASS;
		break;
	case NCS_CHANGE_WAIT:
		event = NS_EV_CHANGE;
		break;
	default:
		event = NS_EV_CHECK;
		break;
	}
	notify_unlock();
	return event;
}

void np_init(void (*notify)(void))
{
	struct notify_state *state = &notify_state;

	state->notify = notify;
	al_net_addr_set_ipv4(&state->notify_addr, 0);
	state->notify_host[0] = '\0';

	timer_init(&state->probe_timer, np_probe_timeout);
	timer_init(&state->timer, np_timeout);
	notify_lock_init();

	state->aes_tx_ctx = al_aes_ctxt_alloc();
	ASSERT(state->aes_tx_ctx);
	state->aes_rx_ctx = al_aes_ctxt_alloc();
	ASSERT(state->aes_rx_ctx);
}

void np_final(void)
{
	struct notify_state *state = &notify_state;

	al_aes_ctxt_free(state->aes_rx_ctx);
	al_aes_ctxt_free(state->aes_tx_ctx);
}

int np_start(u8 *cipher_key, size_t key_len)
{
	struct notify_state *state = &notify_state;
	u8 iv[16];

	if (state->notify_host[0] == '\0'
	    || !al_net_addr_get_ipv4(&state->notify_addr)) {
		return -1;
	}
#ifdef NP_DEBUG_KEY
	np_log(LOG_WARN "overriding ANS cipher key");
	cipher_key = (u8 *)NP_DEBUG_KEY;
	key_len = NP_KEY_LEN;
#endif /* DEBUG_KEY */
#ifdef NP_DEBUG_IP
	np_log(LOG_WARN "overriding ANS server address");
	addr->addr = htonl(NP_DEBUG_IP);
#endif /* DEBUG_IP */

	al_random_fill(iv, sizeof(iv));

	notify_lock();
	np_down(state);
	if (!key_len) {
		goto err;
	}

	/*
	 * Setup and test key.
	 */
	state->aes_key = cipher_key;
	state->aes_key_len = key_len;
	if (al_aes_cbc_key_set(state->aes_tx_ctx,
	    cipher_key, key_len, iv, 0)) {
		np_log(LOG_WARN "start: AES key init err");
		goto err;
	}
	if (np_open_pcbs(state)) {
		goto err;
	}
	np_probe_init(state);
	np_register(state);
	state->poll_interval = np_poll_default;
	goto unlock;
err:
	np_down_event(state);
unlock:
	notify_unlock();
	np_log(LOG_DEBUG "start");
	return 0;
}

void np_stop(void)
{
	struct notify_state *state = &notify_state;

	notify_lock();
	state->change_wait = 0;
	np_down(state);
	notify_unlock();
	np_log(LOG_DEBUG "stop");
}

/*
 * Wifi has started, and we can issue ioctl()'s to it.
 * There's some entropy available from processor chip. Feed it to SSL prng.
 * Note: both SSL and notify code run in tcpip_thread context, so
 * no locking needed.
 */
void np_prng_seed(void)
{
/*
	static u8 prng_seeded;
	int rc;
	u8 random_data[16 + sizeof(conf_sys_mac_addr)];

	if (prng_seeded) {
		return;
	}
	al_random_fill(random_data, 16);
	memcpy(random_data + 16, conf_sys_mac_addr, sizeof(conf_sys_mac_addr));
	rc = adc_aes_entropy_add(random_data, sizeof(random_data));
	if (rc) {
		np_log(LOG_ERR "add entropy fail %d", rc);
	} else {
		prng_seeded = 1;
	}
*/
}

/*
 * Print status for debugging / diagnostics. Invoked by CLI "client" command
 * or by the CLI "notify" command.
 */
void np_status(void)
{
	struct notify_state *state = &notify_state;
	u32 ip;
	u8 *bp;

	ip = al_net_addr_get_ipv4(&state->notify_addr);
	bp = (u8 *)ip;
	printcli("notify: server=%s, ip=%u.%u.%u.%u",
	    state->notify_host, bp[0], bp[1], bp[2], bp[3]);
	printcli("notify: client_state=%u probe_state=%u "
	    "keep_alive=%u", state->client_state, state->probe_state,
	    state->keep_alive);
}

static enum ada_err notify_udp_open(struct al_net_udp **pudp,
	struct al_net_addr *addr, u16 port, void *cb_arg)
{
	struct al_net_udp *udp;
	enum al_err err;

	udp = al_net_udp_new();
	if (udp) {
		err = al_net_udp_connect(udp, addr, port);
		if (err) {
			al_net_udp_free(udp);
			goto last;
		}
		al_net_udp_set_recv_cb(udp, np_recv, cb_arg);
	} else {
		err = AL_ERR_ALLOC;
		goto last;
	}
	*pudp = udp;
last:
	return (enum ada_err)(-err);
}

static enum ada_err notify_udp_send(struct al_net_udp *udp,
	void *buf, size_t len)
{
	int err;
	err = al_net_udp_send(udp, buf, len);
	return (enum ada_err)(-err);
}

