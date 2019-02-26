// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012 - 2018 Microchip Technology Inc., and its subsidiaries.
 * All rights reserved.
 */

#include <linux/etherdevice.h>

#include "wilc_wfi_netdevice.h"
#include "linux_wlan.h"
#include "wilc_wfi_cfgoperations.h"

#define WILC_HIF_SCAN_TIMEOUT_MS                    4000
#define WILC_HIF_CONNECT_TIMEOUT_MS                 9500

#define FALSE_FRMWR_CHANNEL			100

/* Generic success will return 0 */
#define WILC_SUCCESS		0	/* Generic success */

/* Negative numbers to indicate failures */
/* Generic Fail */
#define	WILC_FAIL		-100
/* Busy with another operation*/
#define	WILC_BUSY		-101
/* A given argument is invalid*/
#define	WILC_INVALID_ARGUMENT	-102
/* An API request would violate the Driver state machine
 * (i.e. to start PID while not camped)
 */
#define	WILC_INVALID_STATE	-103
/* In copy operations if the copied data is larger than the allocated buffer*/
#define	WILC_BUFFER_OVERFLOW	-104
/* null pointer is passed or used */
#define WILC_NULL_PTR		-105
#define	WILC_EMPTY		-107
#define WILC_FULL		-108
#define	WILC_TIMEOUT		-109
/* The required operation have been canceled by the user*/
#define WILC_CANCELED		-110
/* The Loaded file is corruped or having an invalid format */
#define WILC_INVALID_FILE	-112
/* Cant find the file to load */
#define WILC_NOT_FOUND		-113
#define WILC_NO_MEM		-114
#define WILC_UNSUPPORTED_VERSION -115
#define WILC_FILE_EOF		-116

#if KERNEL_VERSION(3, 17, 0) > LINUX_VERSION_CODE
struct ieee80211_wmm_ac_param {
	u8 aci_aifsn; /* AIFSN, ACM, ACI */
	u8 cw; /* ECWmin, ECWmax (CW = 2^ECW - 1) */
	__le16 txop_limit;
} __packed;

struct ieee80211_wmm_param_ie {
	u8 element_id; /* Element ID: 221 (0xdd); */
	u8 len; /* Length: 24 */
	u8 oui[3]; /* 00:50:f2 */
	u8 oui_type; /* 2 */
	u8 oui_subtype; /* 1 */
	u8 version; /* 1 for WMM version 1.0 */
	u8 qos_info; /* AP/STA specific QoS info */
	u8 reserved; /* 0 */
	/* AC_BE, AC_BK, AC_VI, AC_VO */
	struct ieee80211_wmm_ac_param ac[4];
} __packed;
#endif

struct send_buffered_eap {
	void (*frmw_to_linux)(struct wilc_vif *vif, u8 *buff, u32 size,
			      u32 pkt_offset, u8 status);
	void (*eap_buf_param)(void *priv);
	u8 *buff;
	unsigned int size;
	unsigned int pkt_offset;
	void *user_arg;
};

struct wilc_rcvd_mac_info {
	u8 status;
};

struct wilc_set_multicast {
	u32 enabled;
	u32 cnt;
	u8 *mc_list;
};

struct host_if_wowlan_trigger {
	u8 wowlan_trigger;
};

struct bt_coex_mode {
	u8 bt_coex;
};

struct host_if_set_ant {
	u8 mode;
	u8 antenna1;
	u8 antenna2;
	u8 gpio_mode;
};

struct del_all_sta {
	u8 assoc_sta;
	u8 mac[WILC_MAX_NUM_STA][ETH_ALEN];
};

struct wilc_op_mode {
	__le32 mode;
};

struct wilc_reg_frame {
	bool reg;
	u8 reg_id;
	__le16 frame_type;
} __packed;

struct wilc_drv_handler {
	__le32 handler;
	u8 mode;
} __packed;

struct wilc_wep_key {
	u8 index;
	u8 key_len;
	u8 key[0];
} __packed;

struct wilc_sta_wpa_ptk {
	u8 mac_addr[ETH_ALEN];
	u8 key_len;
	u8 key[0];
} __packed;

struct wilc_ap_wpa_ptk {
	u8 mac_addr[ETH_ALEN];
	u8 index;
	u8 key_len;
	u8 key[0];
} __packed;

struct wilc_gtk_key {
	u8 mac_addr[ETH_ALEN];
	u8 rsc[8];
	u8 index;
	u8 key_len;
	u8 key[0];
} __packed;

union message_body {
	struct wilc_rcvd_net_info net_info;
	struct wilc_rcvd_mac_info mac_info;
	struct wilc_set_multicast mc_info;
	struct remain_ch remain_on_ch;
	char *data;
	struct send_buffered_eap send_buff_eap;
	struct host_if_set_ant set_ant;
	struct host_if_wowlan_trigger wow_trigger;
	struct bt_coex_mode bt_coex_mode;
};

struct host_if_msg {
	union message_body body;
	struct wilc_vif *vif;
	struct work_struct work;
	void (*fn)(struct work_struct *ws);
	struct completion work_comp;
	bool is_sync;
};

struct wilc_noa_opp_enable {
	u8 ct_window;
	u8 cnt;
	__le32 duration;
	__le32 interval;
	__le32 start_time;
} __packed;

struct wilc_noa_opp_disable {
	u8 cnt;
	__le32 duration;
	__le32 interval;
	__le32 start_time;
} __packed;

struct wilc_join_bss_param {
	char ssid[IEEE80211_MAX_SSID_LEN];
	u8 ssid_terminator;
	u8 bss_type;
	u8 ch;
	__le16 cap_info;
	u8 sa[ETH_ALEN];
	u8 bssid[ETH_ALEN];
	__le16 beacon_period;
	u8 dtim_period;
	u8 supp_rates[MAX_RATES_SUPPORTED + 1];
	u8 wmm_cap;
	u8 uapsd_cap;
	u8 ht_capable;
	u8 rsn_found;
	u8 rsn_grp_policy;
	u8 mode_802_11i;
	u8 p_suites[3];
	u8 akm_suites[3];
	u8 rsn_cap[2];
	u8 noa_enabled;
	__le32 tsf_lo;
	u8 idx;
	u8 opp_enabled;
	union {
		struct wilc_noa_opp_disable opp_dis;
		struct wilc_noa_opp_enable opp_en;
	};
} __packed;

/* 'msg' should be free by the caller for syc */
static struct host_if_msg*
wilc_alloc_work(struct wilc_vif *vif, void (*work_fun)(struct work_struct *),
		bool is_sync)
{
	struct host_if_msg *msg;

	if (!work_fun)
		return ERR_PTR(-EINVAL);

	msg = kzalloc(sizeof(*msg), GFP_ATOMIC);
	if (!msg)
		return ERR_PTR(-ENOMEM);
	msg->fn = work_fun;
	msg->vif = vif;
	msg->is_sync = is_sync;
	if (is_sync)
		init_completion(&msg->work_comp);

	return msg;
}

static int wilc_enqueue_work(struct host_if_msg *msg)
{
	INIT_WORK(&msg->work, msg->fn);

	if (!msg->vif || !msg->vif->wilc || !msg->vif->wilc->hif_workqueue)
		return -EINVAL;

	if (!queue_work(msg->vif->wilc->hif_workqueue, &msg->work))
		return -EINVAL;

	return 0;
}

/* The idx starts from 0 to (NUM_CONCURRENT_IFC - 1), but 0 index used as
 * special purpose in wilc device, so we add 1 to the index to starts from 1.
 * As a result, the returned index will be 1 to NUM_CONCURRENT_IFC.
 */
int wilc_get_vif_idx(struct wilc_vif *vif)
{
	return vif->idx + 1;
}

/* We need to minus 1 from idx which is from wilc device to get real index
 * of wilc->vif[], because we add 1 when pass to wilc device in the function
 * wilc_get_vif_idx.
 * As a result, the index should be between 0 and (NUM_CONCURRENT_IFC - 1).
 */
static struct wilc_vif *wilc_get_vif_from_idx(struct wilc *wilc, int idx)
{
	int index = idx - 1;

	if (index < 0 || index >= WILC_NUM_CONCURRENT_IFC)
		return NULL;

	return wilc->vif[index];
}

static void handle_send_buffered_eap(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct send_buffered_eap *hif_buff_eap = &msg->body.send_buff_eap;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending bufferd eapol to WPAS\n");
	if (!hif_buff_eap->buff)
		goto out;

	if (hif_buff_eap->frmw_to_linux)
		hif_buff_eap->frmw_to_linux(vif, hif_buff_eap->buff,
					    hif_buff_eap->size,
					    hif_buff_eap->pkt_offset,
					    PKT_STATUS_BUFFERED);
	if (hif_buff_eap->eap_buf_param)
		hif_buff_eap->eap_buf_param(hif_buff_eap->user_arg);

	if (hif_buff_eap->buff != NULL) {
		kfree(hif_buff_eap->buff);
		hif_buff_eap->buff = NULL;
	}

out:
	kfree(msg);
}

int wilc_scan(struct wilc_vif *vif, u8 scan_source, u8 scan_type,
	      u8 *ch_freq_list, u8 ch_list_len, const u8 *ies, size_t ies_len,
	      void (*scan_result_fn)(enum scan_event,
				     struct wilc_rcvd_net_info *, void *),
	      void *user_arg, struct wilc_probe_ssid *search)
{
	int result = 0;
	struct wid wid_list[5];
	u32 index = 0;
	u32 i;
	u8 *buffer;
	u8 valuesize = 0;
	u8 *search_ssid_vals = NULL;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct host_if_drv *hif_drv_p2p = get_drv_hndl_by_ifc(vif->wilc,
							      WILC_P2P_IFC);
	struct host_if_drv *hif_drv_wlan = get_drv_hndl_by_ifc(vif->wilc,
							       WILC_WLAN_IFC);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting SCAN params\n");
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Scanning: In [%d] state\n",
		   hif_drv->hif_state);

	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state != HOST_IF_IDLE &&
		    hif_drv_p2p->hif_state != HOST_IF_CONNECTED) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Don't scan. P2P_IFC is in state [%d]\n",
				   hif_drv_p2p->hif_state);
			result = -EBUSY;
			goto error;
		}
	}

	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state != HOST_IF_IDLE &&
		    hif_drv_wlan->hif_state != HOST_IF_CONNECTED) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Don't scan. WLAN_IFC is in state [%d]\n",
				   hif_drv_wlan->hif_state);
			result = -EBUSY;
			goto error;
		}
	}
	if (vif->connecting) {
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "Don't do scan in (CONNECTING) state\n");
		result = -EBUSY;
		goto error;
	}
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	if (vif->obtaining_ip) {
		PRINT_ER(vif->ndev, "Don't do obss scan\n");
		result = -EBUSY;
		goto error;
	}
#endif

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting SCAN params\n");
	hif_drv->usr_scan_req.ch_cnt = 0;

	if (search) {
		for (i = 0; i < search->n_ssids; i++)
			valuesize += ((search->ssid_info[i].ssid_len) + 1);
		search_ssid_vals = kmalloc(valuesize + 1, GFP_KERNEL);
		if (search_ssid_vals) {
			wid_list[index].id = WID_SSID_PROBE_REQ;
			wid_list[index].type = WID_STR;
			wid_list[index].val = search_ssid_vals;
			buffer = wid_list[index].val;

			*buffer++ = search->n_ssids;

		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "In Handle_ProbeRequest number of ssid %d\n",
			 search->n_ssids);
			for (i = 0; i < search->n_ssids; i++) {
				*buffer++ = search->ssid_info[i].ssid_len;
				memcpy(buffer, search->ssid_info[i].ssid,
				       search->ssid_info[i].ssid_len);
				buffer += search->ssid_info[i].ssid_len;
			}

			wid_list[index].size = (s32)(valuesize + 1);
			index++;
		}
	}

	wid_list[index].id = WID_INFO_ELEMENT_PROBE;
	wid_list[index].type = WID_BIN_DATA;
	wid_list[index].val = (s8 *)ies;
	wid_list[index].size = ies_len;
	index++;

	wid_list[index].id = WID_SCAN_TYPE;
	wid_list[index].type = WID_CHAR;
	wid_list[index].size = sizeof(char);
	wid_list[index].val = (s8 *)&scan_type;
	index++;

	wid_list[index].id = WID_SCAN_CHANNEL_LIST;
	wid_list[index].type = WID_BIN_DATA;

	if (ch_freq_list && ch_list_len > 0) {
		for (i = 0; i < ch_list_len; i++) {
			if (ch_freq_list[i] > 0)
				ch_freq_list[i] -= 1;
		}
	}

	wid_list[index].val = ch_freq_list;
	wid_list[index].size = ch_list_len;
	index++;

	wid_list[index].id = WID_START_SCAN_REQ;
	wid_list[index].type = WID_CHAR;
	wid_list[index].size = sizeof(char);
	wid_list[index].val = (s8 *)&scan_source;
	index++;

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, wid_list,
				      index,
				      wilc_get_vif_idx(vif));

	if (result) {
		PRINT_ER(vif->ndev, "Failed to send scan parameters\n");
		goto error;
	} else {
		hif_drv->usr_scan_req.scan_result = scan_result_fn;
		hif_drv->usr_scan_req.arg = user_arg;
		hif_drv->scan_timer_vif = vif;
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   ">> Starting the SCAN timer\n");
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
		hif_drv->scan_timer.data = (unsigned long)hif_drv;
#endif
		mod_timer(&hif_drv->scan_timer,
			  jiffies + msecs_to_jiffies(WILC_HIF_SCAN_TIMEOUT_MS));
	}

error:
	if (search) {
		kfree(search->ssid_info);
		kfree(search_ssid_vals);
	}

	return result;
}

s32 handle_scan_done(struct wilc_vif *vif, enum scan_event evt)
{
	s32 result = 0;
	u8 abort_running_scan;
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct user_scan_req *scan_req;
	u8 null_bssid[6] = {0};

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "handling scan done\n");

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return result;
	}

	if (evt == SCAN_EVENT_DONE) {
		if (memcmp(hif_drv->assoc_bssid, null_bssid, ETH_ALEN) == 0)
			hif_drv->hif_state = HOST_IF_IDLE;
		else
			hif_drv->hif_state = HOST_IF_CONNECTED;
	} else if (evt == SCAN_EVENT_ABORTED) {
		PRINT_INFO(vif->ndev, GENERIC_DBG, "Abort running scan\n");
		abort_running_scan = 1;
		wid.id = WID_ABORT_RUNNING_SCAN;
		wid.type = WID_CHAR;
		wid.val = (s8 *)&abort_running_scan;
		wid.size = sizeof(char);

		result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));

		if (result) {
			PRINT_ER(vif->ndev, "Failed to set abort running\n");
			result = -EFAULT;
		}
	}

	scan_req = &hif_drv->usr_scan_req;
	if (scan_req->scan_result) {
		scan_req->scan_result(evt, NULL, scan_req->arg);
		scan_req->scan_result = NULL;
	}

	return result;
}

static int wilc_send_connect_wid(struct wilc_vif *vif)
{
	int result = 0;
	struct wid wid_list[4];
	u32 wid_cnt = 0;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct wilc_conn_info *conn_attr = &hif_drv->conn_info;
	struct wilc_join_bss_param *bss_param = hif_drv->conn_info.param;
	struct host_if_drv *hif_drv_p2p = get_drv_hndl_by_ifc(vif->wilc,
							      WILC_P2P_IFC);
	struct host_if_drv *hif_drv_wlan = get_drv_hndl_by_ifc(vif->wilc,
							       WILC_WLAN_IFC);

	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Don't scan. P2P_IFC is in state [%d]\n",
			 hif_drv_p2p->hif_state);
			 result = -EFAULT;
			goto error;
		}
	}
	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Don't scan. WLAN_IFC is in state [%d]\n",
			 hif_drv_wlan->hif_state);
			result = -EFAULT;
			goto error;
		}
	}

	wid_list[wid_cnt].id = WID_INFO_ELEMENT_ASSOCIATE;
	wid_list[wid_cnt].type = WID_BIN_DATA;
	wid_list[wid_cnt].val = conn_attr->req_ies;
	wid_list[wid_cnt].size = conn_attr->req_ies_len;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_11I_MODE;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&conn_attr->security;
	wid_cnt++;

	PRINT_D(vif->ndev, HOSTINF_DBG, "Encrypt Mode = %x\n",
		conn_attr->security);
	wid_list[wid_cnt].id = WID_AUTH_TYPE;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&conn_attr->auth_type;
	wid_cnt++;

	PRINT_D(vif->ndev, HOSTINF_DBG, "Authentication Type = %x\n",
		conn_attr->auth_type);
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Connecting to network on channel %d\n", conn_attr->ch);

	wid_list[wid_cnt].id = WID_JOIN_REQ_EXTENDED;
	wid_list[wid_cnt].type = WID_STR;
	wid_list[wid_cnt].size = sizeof(*bss_param);
	wid_list[wid_cnt].val = (u8 *)bss_param;
	wid_cnt++;

	PRINT_INFO(vif->ndev, GENERIC_DBG, "send HOST_IF_WAITING_CONN_RESP\n");

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, wid_list,
				      wid_cnt,
				      wilc_get_vif_idx(vif));
	if (result) {
		PRINT_ER(vif->ndev, "failed to send config packet\n");
		goto error;
	} else {
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "set HOST_IF_WAITING_CONN_RESP\n");
		hif_drv->hif_state = HOST_IF_WAITING_CONN_RESP;
	}

	return 0;

error:

	kfree(conn_attr->req_ies);
	conn_attr->req_ies = NULL;

	return result;
}

static void handle_connect_timeout(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	int result;
	struct wid wid;
	u16 dummy_reason_code = 0;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		goto out;
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	if (hif_drv->conn_info.conn_result) {
		hif_drv->conn_info.conn_result(EVENT_CONN_RESP,
					       WILC_MAC_STATUS_DISCONNECTED,
					       hif_drv->conn_info.arg);

	} else {
		PRINT_ER(vif->ndev, "conn_result is NULL\n");
	}

	wid.id = WID_DISCONNECT;
	wid.type = WID_CHAR;
	wid.val = (s8 *)&dummy_reason_code;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending disconnect request\n");
	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send disconect\n");

	hif_drv->conn_info.req_ies_len = 0;
	kfree(hif_drv->conn_info.req_ies);
	hif_drv->conn_info.req_ies = NULL;

out:
	kfree(msg);
}

void *wilc_parse_join_bss_param(struct cfg80211_bss *bss,
				struct cfg80211_crypto_settings *crypto)
{
	struct wilc_join_bss_param *param;
	struct ieee80211_p2p_noa_attr noa_attr;
	u8 rates_len = 0;
	const u8 *tim_elm, *ssid_elm, *rates_ie, *supp_rates_ie;
	const u8 *ht_ie, *wpa_ie, *wmm_ie, *rsn_ie;
	int ret;
	const struct cfg80211_bss_ies *ies = bss->ies;

	param = kzalloc(sizeof(*param), GFP_KERNEL);
	if (!param)
		return NULL;

	param->beacon_period = bss->beacon_interval;
	param->cap_info = bss->capability;
	param->bss_type = WILC_FW_BSS_TYPE_INFRA;
	param->ch = ieee80211_frequency_to_channel(bss->channel->center_freq);
	ether_addr_copy(param->bssid, bss->bssid);

	ssid_elm = cfg80211_find_ie(WLAN_EID_SSID, ies->data, ies->len);
	if (ssid_elm) {
		if (ssid_elm[1] <= IEEE80211_MAX_SSID_LEN)
			memcpy(param->ssid, ssid_elm + 2, ssid_elm[1]);
	}

	tim_elm = cfg80211_find_ie(WLAN_EID_TIM, ies->data, ies->len);
	if (tim_elm && tim_elm[1] >= 2)
		param->dtim_period = tim_elm[3];

	memset(param->p_suites, 0xFF, 3);
	memset(param->akm_suites, 0xFF, 3);

	rates_ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, ies->data, ies->len);
	if (rates_ie) {
		rates_len = rates_ie[1];
		param->supp_rates[0] = rates_len;
		memcpy(&param->supp_rates[1], rates_ie + 2, rates_len);
	}

	supp_rates_ie = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, ies->data,
					 ies->len);
	if (supp_rates_ie) {
		if (supp_rates_ie[1] > (MAX_RATES_SUPPORTED - rates_len))
			param->supp_rates[0] = MAX_RATES_SUPPORTED;
		else
			param->supp_rates[0] += supp_rates_ie[1];

		memcpy(&param->supp_rates[rates_len + 1], supp_rates_ie + 2,
		       (param->supp_rates[0] - rates_len));
	}

	ht_ie = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, ies->data, ies->len);
	if (ht_ie)
		param->ht_capable = true;

	ret = cfg80211_get_p2p_attr(ies->data, ies->len,
				    IEEE80211_P2P_ATTR_ABSENCE_NOTICE,
				    (u8 *)&noa_attr, sizeof(noa_attr));
	if (ret > 0) {
		param->tsf_lo = cpu_to_le32(ies->tsf);
		param->noa_enabled = 1;
		param->idx = noa_attr.index;
		if (noa_attr.oppps_ctwindow & IEEE80211_P2P_OPPPS_ENABLE_BIT) {
			param->opp_enabled = 1;
			param->opp_en.ct_window = noa_attr.oppps_ctwindow;
			param->opp_en.cnt = noa_attr.desc[0].count;
			param->opp_en.duration = noa_attr.desc[0].duration;
			param->opp_en.interval = noa_attr.desc[0].interval;
			param->opp_en.start_time = noa_attr.desc[0].start_time;
		} else {
			param->opp_enabled = 0;
			param->opp_dis.cnt = noa_attr.desc[0].count;
			param->opp_dis.duration = noa_attr.desc[0].duration;
			param->opp_dis.interval = noa_attr.desc[0].interval;
			param->opp_dis.start_time = noa_attr.desc[0].start_time;
		}
	}
	wmm_ie = cfg80211_find_vendor_ie(WLAN_OUI_MICROSOFT,
					 WLAN_OUI_TYPE_MICROSOFT_WMM,
					 ies->data, ies->len);
	if (wmm_ie) {
		struct ieee80211_wmm_param_ie *ie;

		ie = (struct ieee80211_wmm_param_ie *)wmm_ie;
		if ((ie->oui_subtype == 0 || ie->oui_subtype == 1) &&
		    ie->version == 1) {
			param->wmm_cap = true;
			if (ie->qos_info & BIT(7))
				param->uapsd_cap = true;
		}
	}

	wpa_ie = cfg80211_find_vendor_ie(WLAN_OUI_MICROSOFT,
					 WLAN_OUI_TYPE_MICROSOFT_WPA,
					 ies->data, ies->len);
	if (wpa_ie) {
		param->mode_802_11i = 1;
		param->rsn_found = true;
	}

	rsn_ie = cfg80211_find_ie(WLAN_EID_RSN, ies->data, ies->len);
	if (rsn_ie) {
		int offset = 8;

		param->mode_802_11i = 2;
		param->rsn_found = true;
		//extract RSN capabilities
		offset += (rsn_ie[offset] * 4) + 2;
		offset += (rsn_ie[offset] * 4) + 2;
		memcpy(param->rsn_cap, &rsn_ie[offset], 2);
	}

	if (param->rsn_found) {
		int i;

		param->rsn_grp_policy = crypto->cipher_group & 0xFF;
		for (i = 0; i < crypto->n_ciphers_pairwise && i < 3; i++)
			param->p_suites[i] = crypto->ciphers_pairwise[i] & 0xFF;

		for (i = 0; i < crypto->n_akm_suites && i < 3; i++)
			param->akm_suites[i] = crypto->akm_suites[i] & 0xFF;
	}

	return (void *)param;
}

static void handle_rcvd_ntwrk_info(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_rcvd_net_info *rcvd_info = &msg->body.net_info;
	struct user_scan_req *scan_req = &msg->vif->hif_drv->usr_scan_req;
	const u8 *ch_elm;
	u8 *ies;
	int ies_len;
	size_t offset;

	PRINT_D(msg->vif->ndev, HOSTINF_DBG,
		"Handling received network info\n");

	if (ieee80211_is_probe_resp(rcvd_info->mgmt->frame_control))
		offset = offsetof(struct ieee80211_mgmt, u.probe_resp.variable);
	else if (ieee80211_is_beacon(rcvd_info->mgmt->frame_control))
		offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
	else
		goto done;

	ies = rcvd_info->mgmt->u.beacon.variable;
	ies_len = rcvd_info->frame_len - offset;
	if (ies_len <= 0)
		goto done;

	PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "New network found\n");
	/* extract the channel from recevied mgmt frame */
	ch_elm = cfg80211_find_ie(WLAN_EID_DS_PARAMS, ies, ies_len);
	if (ch_elm && ch_elm[1] > 0)
		rcvd_info->ch = ch_elm[2];

	if (scan_req->scan_result)
		scan_req->scan_result(SCAN_EVENT_NETWORK_FOUND,
				      rcvd_info, scan_req->arg);

done:
	kfree(rcvd_info->mgmt);
	kfree(msg);
}

static void host_int_get_assoc_res_info(struct wilc_vif *vif,
					u8 *assoc_resp_info,
					u32 max_assoc_resp_info_len,
					u32 *rcvd_assoc_resp_info_len)
{
	int result;
	struct wid wid;

	wid.id = WID_ASSOC_RES_INFO;
	wid.type = WID_STR;
	wid.val = assoc_resp_info;
	wid.size = max_assoc_resp_info_len;

	result = wilc_send_config_pkt(vif, WILC_GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result) {
		*rcvd_assoc_resp_info_len = 0;
		PRINT_ER(vif->ndev, "Failed to send association response\n");
		return;
	}

	*rcvd_assoc_resp_info_len = wid.size;
}

static s32 wilc_parse_assoc_resp_info(u8 *buffer, u32 buffer_len,
				      struct wilc_conn_info *ret_conn_info)
{
	u8 *ies;
	u16 ies_len;
	struct assoc_resp *res = (struct assoc_resp *)buffer;

	ret_conn_info->status = le16_to_cpu(res->status_code);
	if (ret_conn_info->status == WLAN_STATUS_SUCCESS) {
		ies = &buffer[sizeof(*res)];
		ies_len = buffer_len - sizeof(*res);

		ret_conn_info->resp_ies = kmemdup(ies, ies_len, GFP_KERNEL);
		if (!ret_conn_info->resp_ies)
			return -ENOMEM;

		ret_conn_info->resp_ies_len = ies_len;
	}

	return 0;
}

static inline void host_int_parse_assoc_resp_info(struct wilc_vif *vif,
						  u8 mac_status)
{
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct wilc_conn_info *conn_info = &hif_drv->conn_info;

	if (mac_status == WILC_MAC_STATUS_CONNECTED) {
		u32 assoc_resp_info_len;

		memset(hif_drv->assoc_resp, 0, WILC_MAX_ASSOC_RESP_FRAME_SIZE);

		host_int_get_assoc_res_info(vif, hif_drv->assoc_resp,
					    WILC_MAX_ASSOC_RESP_FRAME_SIZE,
					    &assoc_resp_info_len);

		PRINT_D(vif->ndev, HOSTINF_DBG,
			"Received association response = %d\n",
			assoc_resp_info_len);
		if (assoc_resp_info_len != 0) {
			s32 err = 0;

			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "Parsing association response\n");
			err = wilc_parse_assoc_resp_info(hif_drv->assoc_resp,
							 assoc_resp_info_len,
							 conn_info);
			if (err)
				PRINT_ER(vif->ndev,
					 "wilc_parse_assoc_resp_info() returned error %d\n",
					 err);
		}
	}

	del_timer(&hif_drv->connect_timer);
	conn_info->conn_result(EVENT_CONN_RESP, mac_status, conn_info->arg);

	if (mac_status == WILC_MAC_STATUS_CONNECTED &&
	    conn_info->status == WLAN_STATUS_SUCCESS) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "MAC status : CONNECTED and Connect Status : Successful\n");
		hif_drv->hif_state = HOST_IF_CONNECTED;
		ether_addr_copy(hif_drv->assoc_bssid, conn_info->bssid);
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
		handle_pwrsave_for_IP(vif, IP_STATE_OBTAINING);
#endif
	} else {
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "MAC status : %d and Connect Status : %d\n",
			   mac_status, conn_info->status);
		hif_drv->hif_state = HOST_IF_IDLE;
	}

	kfree(conn_info->resp_ies);
	conn_info->resp_ies = NULL;
	conn_info->resp_ies_len = 0;

	kfree(conn_info->req_ies);
	conn_info->req_ies = NULL;
	conn_info->req_ies_len = 0;
}

static inline void host_int_handle_disconnect(struct wilc_vif *vif)
{
	struct host_if_drv *hif_drv = vif->hif_drv;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Received WILC_MAC_STATUS_DISCONNECTED from the FW\n");
	if (hif_drv->usr_scan_req.scan_result) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "\n\n<< Abort the running OBSS Scan >>\n\n");
		del_timer(&hif_drv->scan_timer);
		handle_scan_done(vif, SCAN_EVENT_ABORTED);
	}

	if (hif_drv->conn_info.conn_result) {
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
		handle_pwrsave_for_IP(vif, IP_STATE_DEFAULT);
#endif

		hif_drv->conn_info.conn_result(EVENT_DISCONN_NOTIF,
					       0, hif_drv->conn_info.arg);
	} else {
		PRINT_ER(vif->ndev, "Connect result NULL\n");
	}

	eth_zero_addr(hif_drv->assoc_bssid);

	hif_drv->conn_info.req_ies_len = 0;
	kfree(hif_drv->conn_info.req_ies);
	hif_drv->conn_info.req_ies = NULL;
	hif_drv->hif_state = HOST_IF_IDLE;
}

static void handle_rcvd_gnrl_async_info(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct wilc_rcvd_mac_info *mac_info = &msg->body.mac_info;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		netdev_err(vif->ndev, "%s: hif driver is NULL\n", __func__);
		goto free_msg;
	}

	PRINT_INFO(vif->ndev, GENERIC_DBG,
		   "Current State = %d,Received state = %d\n",
		   hif_drv->hif_state, mac_info->status);

	if (!hif_drv->conn_info.conn_result) {
		PRINT_ER(vif->ndev, "conn_result is NULL\n");
		goto free_msg;
	}
	if (hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP) {
		host_int_parse_assoc_resp_info(vif, mac_info->status);
	} else if (mac_info->status == WILC_MAC_STATUS_DISCONNECTED) {
		if (hif_drv->hif_state == HOST_IF_CONNECTED) {
			host_int_handle_disconnect(vif);
		} else if (hif_drv->usr_scan_req.scan_result) {
			PRINT_WRN(vif->ndev, HOSTINF_DBG,
				  "Received WILC_MAC_STATUS_DISCONNECTED. Abort the running Scan");
			del_timer(&hif_drv->scan_timer);
			handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}

free_msg:
	kfree(msg);
}

int wilc_disconnect(struct wilc_vif *vif)
{
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct user_scan_req *scan_req;
	struct wilc_conn_info *conn_info;
	int result;
	u16 dummy_reason_code = 0;
	struct host_if_drv *hif_drv_p2p = get_drv_hndl_by_ifc(vif->wilc,
							      WILC_P2P_IFC);
	struct host_if_drv *hif_drv_wlan = get_drv_hndl_by_ifc(vif->wilc,
							       WILC_WLAN_IFC);

	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Abort Scan. WLAN_IFC is in state [%d]\n",
				   hif_drv_wlan->hif_state);
			del_timer(&hif_drv_wlan->scan_timer);
			handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}
	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Abort Scan. P2P_IFC is in state [%d]\n",
				   hif_drv_p2p->hif_state);
			del_timer(&hif_drv_p2p->scan_timer);
			handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}
	wid.id = WID_DISCONNECT;
	wid.type = WID_CHAR;
	wid.val = (s8 *)&dummy_reason_code;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending disconnect request\n");

#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	handle_pwrsave_for_IP(vif, IP_STATE_DEFAULT);
#endif

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));

	if (result) {
		PRINT_ER(vif->ndev, "Failed to send dissconect\n");
		return -ENOMEM;
	}

	scan_req = &hif_drv->usr_scan_req;
	conn_info = &hif_drv->conn_info;

	if (scan_req->scan_result) {
		del_timer(&hif_drv->scan_timer);
		scan_req->scan_result(SCAN_EVENT_ABORTED, NULL, scan_req->arg);
		scan_req->scan_result = NULL;
	}

	if (conn_info->conn_result) {
		if (hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "supplicant requested disconnection\n");
			del_timer(&hif_drv->connect_timer);
			conn_info->conn_result(EVENT_CONN_RESP,
					       WILC_MAC_STATUS_DISCONNECTED,
					       conn_info->arg);

		} else if (hif_drv->hif_state == HOST_IF_CONNECTED) {
			conn_info->conn_result(EVENT_DISCONN_NOTIF,
					       WILC_MAC_STATUS_DISCONNECTED,
					       conn_info->arg);
		}
	} else {
		PRINT_ER(vif->ndev, "conn_result = NULL\n");
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	eth_zero_addr(hif_drv->assoc_bssid);

	conn_info->req_ies_len = 0;
	kfree(conn_info->req_ies);
	conn_info->req_ies = NULL;

	return 0;
}

void wilc_resolve_disconnect_aberration(struct wilc_vif *vif)
{
	if (!vif->hif_drv)
		return;
	if (vif->hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP ||
	    vif->hif_drv->hif_state == HOST_IF_CONNECTING) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "\n\n<< correcting Supplicant state machine >>\n\n");
		wilc_disconnect(vif);
	}
}

int wilc_get_statistics(struct wilc_vif *vif, struct rf_info *stats)
{
	struct wid wid_list[5];
	u32 wid_cnt = 0, result;

	wid_list[wid_cnt].id = WID_LINKSPEED;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&stats->link_speed;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_RSSI;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&stats->rssi;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_SUCCESS_FRAME_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->tx_cnt;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_RECEIVED_FRAGMENT_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->rx_cnt;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_FAILED_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->tx_fail_cnt;
	wid_cnt++;

	result = wilc_send_config_pkt(vif, WILC_GET_CFG, wid_list,
				      wid_cnt,
				      wilc_get_vif_idx(vif));

	if (result) {
		PRINT_ER(vif->ndev, "Failed to send scan parameters\n");
		return result;
	}

	if (stats->link_speed > TCP_ACK_FILTER_LINK_SPEED_THRESH &&
	    stats->link_speed != DEFAULT_LINK_SPEED) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Enable TCP filter\n");
		wilc_enable_tcp_ack_filter(vif, true);
	} else if (stats->link_speed != DEFAULT_LINK_SPEED) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Disable TCP filter %d\n",
			   stats->link_speed);
		wilc_enable_tcp_ack_filter(vif, false);
	}

	return result;
}

static void handle_get_statistics(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct rf_info *stats = (struct rf_info *)msg->body.data;

	wilc_get_statistics(vif, stats);
	kfree(msg);
}

static void wilc_hif_pack_sta_param(struct wilc_vif *vif, u8 *cur_byte,
				    const u8 *mac,
				    struct station_parameters *params)
{
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Packing STA params\n");
	ether_addr_copy(cur_byte, mac);
	cur_byte +=  ETH_ALEN;

	put_unaligned_le16(params->aid, cur_byte);
	cur_byte += 2;

	*cur_byte++ = params->supported_rates_len;
	if (params->supported_rates_len > 0)
		memcpy(cur_byte, params->supported_rates,
		       params->supported_rates_len);
	cur_byte += params->supported_rates_len;

	if (params->ht_capa) {
		*cur_byte++ = true;
		memcpy(cur_byte, &params->ht_capa,
		       sizeof(struct ieee80211_ht_cap));
	} else {
		*cur_byte++ = false;
	}
	cur_byte += sizeof(struct ieee80211_ht_cap);

	put_unaligned_le16(params->sta_flags_mask, cur_byte);
	cur_byte += 2;
	put_unaligned_le16(params->sta_flags_set, cur_byte);
}

static int handle_remain_on_chan(struct wilc_vif *vif,
				 struct remain_ch *hif_remain_ch)
{
	int result;
	u8 remain_on_chan_flag;
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct host_if_drv *hif_drv_p2p = get_drv_hndl_by_ifc(vif->wilc,
							      WILC_P2P_IFC);
	struct host_if_drv *hif_drv_wlan = get_drv_hndl_by_ifc(vif->wilc,
							       WILC_WLAN_IFC);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return -EFAULT;
	}

	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "IFC busy scanning P2P_IFC state %d\n",
				   hif_drv_p2p->hif_state);
			return -EBUSY;
		} else if ((hif_drv_p2p->hif_state != HOST_IF_IDLE) &&
		(hif_drv_p2p->hif_state != HOST_IF_CONNECTED)) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "IFC busy connecting. P2P_IFC state %d\n",
				   hif_drv_p2p->hif_state);
			return -EBUSY;
		}
	}
	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "IFC busy scanning. WLAN_IFC state %d\n",
				   hif_drv_wlan->hif_state);
			return -EBUSY;
		} else if ((hif_drv_wlan->hif_state != HOST_IF_IDLE) &&
		(hif_drv_wlan->hif_state != HOST_IF_CONNECTED)) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "IFC busy connecting. WLAN_IFC %d\n",
				   hif_drv_wlan->hif_state);
			return -EBUSY;
		}
	}

	if (vif->connecting) {
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "Don't do scan in (CONNECTING) state\n");
		return -EBUSY;
	}
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	if (vif->obtaining_ip) {
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "Don't obss scan until IP adresss is obtained\n");
		return -EBUSY;
	}
#endif

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting channel [%d] duration[%d] [%llu]\n",
		   hif_remain_ch->ch, hif_remain_ch->duration,
		   hif_remain_ch->cookie);
	remain_on_chan_flag = true;
	wid.id = WID_REMAIN_ON_CHAN;
	wid.type = WID_STR;
	wid.size = 2;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		return -ENOMEM;

	wid.val[0] = remain_on_chan_flag;
	wid.val[1] = (s8)hif_remain_ch->ch;

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	kfree(wid.val);
	if (result) {
		PRINT_ER(vif->ndev, "Failed to set remain on channel\n");
		return -EBUSY;
	}

	hif_drv->remain_on_ch.arg = hif_remain_ch->arg;
	hif_drv->remain_on_ch.expired = hif_remain_ch->expired;
	hif_drv->remain_on_ch.ch = hif_remain_ch->ch;
	hif_drv->remain_on_ch.cookie = hif_remain_ch->cookie;
	hif_drv->hif_state = HOST_IF_P2P_LISTEN;

	hif_drv->remain_on_ch_timer_vif = vif;

	return result;
}

static void handle_listen_state_expired(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct remain_ch *hif_remain_ch = &msg->body.remain_on_ch;
	u8 remain_on_chan_flag;
	struct wid wid;
	int result;
	struct host_if_drv *hif_drv = vif->hif_drv;
	u8 null_bssid[6] = {0};

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "CANCEL REMAIN ON CHAN\n");

	if (hif_drv->hif_state == HOST_IF_P2P_LISTEN) {
		remain_on_chan_flag = false;
		wid.id = WID_REMAIN_ON_CHAN;
		wid.type = WID_STR;
		wid.size = 2;
		wid.val = kmalloc(wid.size, GFP_KERNEL);

		if (!wid.val) {
			PRINT_ER(vif->ndev, "Failed to allocate memory\n");
			goto free_msg;
		}

		wid.val[0] = remain_on_chan_flag;
		wid.val[1] = FALSE_FRMWR_CHANNEL;

		result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));
		kfree(wid.val);
		if (result != 0) {
			PRINT_ER(vif->ndev, "Failed to set remain channel\n");
			goto free_msg;
		}

		if (hif_drv->remain_on_ch.expired)
			hif_drv->remain_on_ch.expired(hif_drv->remain_on_ch.arg,
						      hif_remain_ch->cookie);

		if (memcmp(hif_drv->assoc_bssid, null_bssid, ETH_ALEN) == 0)
			hif_drv->hif_state = HOST_IF_IDLE;
		else
			hif_drv->hif_state = HOST_IF_CONNECTED;
	} else {
		PRINT_D(vif->ndev, GENERIC_DBG,  "Not in listen state\n");
	}

free_msg:
	kfree(msg);
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void listen_timer_cb(struct timer_list *t)
#else
static void listen_timer_cb(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct host_if_drv *hif_drv = from_timer(hif_drv, t,
						      remain_on_ch_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->remain_on_ch_timer_vif;
	int result;
	struct host_if_msg *msg;

	del_timer(&vif->hif_drv->remain_on_ch_timer);

	msg = wilc_alloc_work(vif, handle_listen_state_expired, false);
	if (IS_ERR(msg))
		return;

	msg->body.remain_on_ch.cookie = vif->hif_drv->remain_on_ch.cookie;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
		kfree(msg);
	}
}

static void handle_set_mcast_filter(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct wilc_set_multicast *set_mc = &msg->body.mc_info;
	int result;
	struct wid wid;
	u8 *cur_byte;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setup Multicast Filter\n");

	wid.id = WID_SETUP_MULTICAST_FILTER;
	wid.type = WID_BIN;
	wid.size = sizeof(struct wilc_set_multicast) + (set_mc->cnt * ETH_ALEN);
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	put_unaligned_le32(set_mc->enabled, cur_byte);
	cur_byte += 4;

	put_unaligned_le32(set_mc->cnt, cur_byte);
	cur_byte += 4;

	if (set_mc->cnt > 0 && set_mc->mc_list)
		memcpy(cur_byte, set_mc->mc_list, set_mc->cnt * ETH_ALEN);

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send setup multicast\n");

error:
	kfree(set_mc->mc_list);
	kfree(wid.val);
	kfree(msg);
}

void wilc_set_wowlan_trigger(struct wilc_vif *vif, u8 wowlan_trigger)
{
	int ret;
	struct wid wid;

	wid.id = WID_WOWLAN_TRIGGER;
	wid.type = WID_CHAR;
	wid.val = &wowlan_trigger;
	wid.size = sizeof(s8);

	ret = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev,
			 "Failed to send wowlan trigger config packet\n");
}

static void handle_scan_timer(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	int ret;

	PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "handling scan timer\n");
	ret = handle_scan_done(msg->vif, SCAN_EVENT_ABORTED);
	if (ret)
		PRINT_ER(msg->vif->ndev, "Failed to handle scan done\n");
	kfree(msg);
}

static void handle_scan_complete(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);

	del_timer(&msg->vif->hif_drv->scan_timer);
	PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "scan completed\n");

	handle_scan_done(msg->vif, SCAN_EVENT_DONE);

	kfree(msg);
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void timer_scan_cb(struct timer_list *t)
#else
static void timer_scan_cb(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct host_if_drv *hif_drv = from_timer(hif_drv, t, scan_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->scan_timer_vif;
	struct host_if_msg *msg;
	int result;

	msg = wilc_alloc_work(vif, handle_scan_timer, false);
	if (IS_ERR(msg))
		return;

	result = wilc_enqueue_work(msg);
	if (result)
		kfree(msg);
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void timer_connect_cb(struct timer_list *t)
#else
static void timer_connect_cb(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct host_if_drv *hif_drv = from_timer(hif_drv, t, connect_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->connect_timer_vif;
	struct host_if_msg *msg;
	int result;

	msg = wilc_alloc_work(vif, handle_connect_timeout, false);
	if (IS_ERR(msg))
		return;

	result = wilc_enqueue_work(msg);
	if (result)
		kfree(msg);
}

signed int wilc_send_buffered_eap(struct wilc_vif *vif,
				  void (*frmw_to_linux)(struct wilc_vif *, u8 *,
							u32, u32, u8),
				  void (*eap_buf_param)(void *), u8 *buff,
				  unsigned int size, unsigned int pkt_offset,
				  void *user_arg)
{
	int result;
	struct host_if_msg *msg;

	if (!vif || !frmw_to_linux || !eap_buf_param)
		return -EFAULT;

	msg = wilc_alloc_work(vif, handle_send_buffered_eap, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);
	msg->body.send_buff_eap.frmw_to_linux = frmw_to_linux;
	msg->body.send_buff_eap.eap_buf_param = eap_buf_param;
	msg->body.send_buff_eap.size = size;
	msg->body.send_buff_eap.pkt_offset = pkt_offset;
	msg->body.send_buff_eap.buff = kmalloc(size + pkt_offset,
						  GFP_ATOMIC);
	memcpy(msg->body.send_buff_eap.buff, buff, size + pkt_offset);
	msg->body.send_buff_eap.user_arg = user_arg;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg->body.send_buff_eap.buff);
		kfree(msg);
	}
	return result;
}

int wilc_remove_wep_key(struct wilc_vif *vif, u8 index)
{
	struct wid wid;
	int result;

	wid.id = WID_REMOVE_WEP_KEY;
	wid.type = WID_STR;
	wid.size = sizeof(char);
	wid.val = &index;

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev,
			 "Failed to send remove wep key config packet\n");
	return result;
}

int wilc_set_wep_default_keyid(struct wilc_vif *vif, u8 index)
{
	struct wid wid;
	int result;

	wid.id = WID_KEY_ID;
	wid.type = WID_CHAR;
	wid.size = sizeof(char);
	wid.val = &index;
	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev,
			 "Failed to send wep default key config packet\n");

	return result;
}

int wilc_add_wep_key_bss_sta(struct wilc_vif *vif, const u8 *key, u8 len,
			     u8 index)
{
	struct wid wid;
	int result;
	struct wilc_wep_key *wep_key;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling WEP key\n");
	wid.id = WID_ADD_WEP_KEY;
	wid.type = WID_STR;
	wid.size = sizeof(*wep_key) + len;
	wep_key = kzalloc(wid.size, GFP_KERNEL);
	if (!wep_key) {
		PRINT_ER(vif->ndev, "No buffer to send Key\n");
		return -ENOMEM;
	}
	wid.val = (u8 *)wep_key;

	wep_key->index = index;
	wep_key->key_len = len;
	memcpy(wep_key->key, key, len);

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		netdev_err(vif->ndev,
			   "Failed to add wep key config packet\n");

	kfree(wep_key);
	return result;
}

int wilc_add_wep_key_bss_ap(struct wilc_vif *vif, const u8 *key, u8 len,
			    u8 index, u8 mode, enum authtype auth_type)
{
	struct wid wid_list[3];
	int result;
	struct wilc_wep_key *wep_key;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling WEP key index: %d\n",
		   index);
	wid_list[0].id = WID_11I_MODE;
	wid_list[0].type = WID_CHAR;
	wid_list[0].size = sizeof(char);
	wid_list[0].val = &mode;

	wid_list[1].id = WID_AUTH_TYPE;
	wid_list[1].type = WID_CHAR;
	wid_list[1].size = sizeof(char);
	wid_list[1].val = (s8 *)&auth_type;

	wid_list[2].id = WID_WEP_KEY_VALUE;
	wid_list[2].type = WID_STR;
	wid_list[2].size = sizeof(*wep_key) + len;
	wep_key = kzalloc(wid_list[2].size, GFP_KERNEL);
	if (!wep_key) {
		PRINT_ER(vif->ndev, "No buffer to send Key\n");
		return -ENOMEM;
	}

	wid_list[2].val = (u8 *)wep_key;

	wep_key->index = index;
	wep_key->key_len = len;
	memcpy(wep_key->key, key, len);
	result = wilc_send_config_pkt(vif, WILC_SET_CFG, wid_list,
				      ARRAY_SIZE(wid_list),
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev,
			 "Failed to add wep ap key config packet\n");

	kfree(wep_key);
	return result;
}

int wilc_add_ptk(struct wilc_vif *vif, const u8 *ptk, u8 ptk_key_len,
		 const u8 *mac_addr, const u8 *rx_mic, const u8 *tx_mic,
		 u8 mode, u8 cipher_mode, u8 index)
{
	int result = 0;
	u8 t_key_len = ptk_key_len + RX_MIC_KEY_LEN + TX_MIC_KEY_LEN;

	if (mode == WILC_AP_MODE) {
		struct wid wid_list[2];
		struct wilc_ap_wpa_ptk *key_buf;

		wid_list[0].id = WID_11I_MODE;
		wid_list[0].type = WID_CHAR;
		wid_list[0].size = sizeof(char);
		wid_list[0].val = (s8 *)&cipher_mode;

		key_buf = kzalloc(sizeof(*key_buf) + t_key_len, GFP_KERNEL);
		if (!key_buf) {
			PRINT_ER(vif->ndev,
				 "NO buffer to keep Key buffer - AP\n");
			return -ENOMEM;
		}
		ether_addr_copy(key_buf->mac_addr, mac_addr);
		key_buf->index = index;
		key_buf->key_len = t_key_len;
		memcpy(&key_buf->key[0], ptk, ptk_key_len);

		if (rx_mic)
			memcpy(&key_buf->key[ptk_key_len], rx_mic,
			       RX_MIC_KEY_LEN);

		if (tx_mic)
			memcpy(&key_buf->key[ptk_key_len + RX_MIC_KEY_LEN],
			       tx_mic, TX_MIC_KEY_LEN);

		wid_list[1].id = WID_ADD_PTK;
		wid_list[1].type = WID_STR;
		wid_list[1].size = sizeof(*key_buf) + t_key_len;
		wid_list[1].val = (u8 *)key_buf;
		result = wilc_send_config_pkt(vif, WILC_SET_CFG, wid_list,
					      ARRAY_SIZE(wid_list),
					      wilc_get_vif_idx(vif));
		kfree(key_buf);
	} else if (mode == WILC_STATION_MODE) {
		struct wid wid;
		struct wilc_sta_wpa_ptk *key_buf;

		key_buf = kzalloc(sizeof(*key_buf) + t_key_len, GFP_KERNEL);
		if (!key_buf) {
			PRINT_ER(vif->ndev,
				 "No buffer to keep Key buffer - Station\n");
			return -ENOMEM;
		}

		ether_addr_copy(key_buf->mac_addr, mac_addr);
		key_buf->key_len = t_key_len;
		memcpy(&key_buf->key[0], ptk, ptk_key_len);

		if (rx_mic)
			memcpy(&key_buf->key[ptk_key_len], rx_mic,
			       RX_MIC_KEY_LEN);

		if (tx_mic)
			memcpy(&key_buf->key[ptk_key_len + RX_MIC_KEY_LEN],
			       tx_mic, TX_MIC_KEY_LEN);

		wid.id = WID_ADD_PTK;
		wid.type = WID_STR;
		wid.size = sizeof(*key_buf) + t_key_len;
		wid.val = (s8 *)key_buf;
		result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));
		kfree(key_buf);
	}

	return result;
}

int wilc_add_rx_gtk(struct wilc_vif *vif, const u8 *rx_gtk, u8 gtk_key_len,
		    u8 index, u32 key_rsc_len, const u8 *key_rsc,
		    const u8 *rx_mic, const u8 *tx_mic, u8 mode,
		    u8 cipher_mode)
{
	int result = 0;
	struct wilc_gtk_key *gtk_key;
	int t_key_len = gtk_key_len + RX_MIC_KEY_LEN + TX_MIC_KEY_LEN;

	gtk_key = kzalloc(sizeof(*gtk_key) + t_key_len, GFP_KERNEL);
	if (!gtk_key) {
		PRINT_ER(vif->ndev, "No buffer to send GTK Key\n");
		return -ENOMEM;
	}

	/* fill bssid value only in station mode */
	if (mode == WILC_STATION_MODE &&
	    vif->hif_drv->hif_state == HOST_IF_CONNECTED)
		memcpy(gtk_key->mac_addr, vif->hif_drv->assoc_bssid, ETH_ALEN);

	if (key_rsc)
		memcpy(gtk_key->rsc, key_rsc, 8);
	gtk_key->index = index;
	gtk_key->key_len = t_key_len;
	memcpy(&gtk_key->key[0], rx_gtk, gtk_key_len);

	if (rx_mic)
		memcpy(&gtk_key->key[gtk_key_len], rx_mic, RX_MIC_KEY_LEN);

	if (tx_mic)
		memcpy(&gtk_key->key[gtk_key_len + RX_MIC_KEY_LEN],
		       tx_mic, TX_MIC_KEY_LEN);

	if (mode == WILC_AP_MODE) {
		struct wid wid_list[2];

		wid_list[0].id = WID_11I_MODE;
		wid_list[0].type = WID_CHAR;
		wid_list[0].size = sizeof(char);
		wid_list[0].val = (s8 *)&cipher_mode;

		wid_list[1].id = WID_ADD_RX_GTK;
		wid_list[1].type = WID_STR;
		wid_list[1].size = sizeof(*gtk_key) + t_key_len;
		wid_list[1].val = (u8 *)gtk_key;

		result = wilc_send_config_pkt(vif, WILC_SET_CFG, wid_list,
					      ARRAY_SIZE(wid_list),
					      wilc_get_vif_idx(vif));
		kfree(gtk_key);
	} else if (mode == WILC_STATION_MODE) {
		struct wid wid;

		wid.id = WID_ADD_RX_GTK;
		wid.type = WID_STR;
		wid.size = sizeof(*gtk_key) + t_key_len;
		wid.val = (u8 *)gtk_key;
		result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));
		kfree(gtk_key);
	}

	return result;
}

int wilc_set_pmkid_info(struct wilc_vif *vif, struct wilc_pmkid_attr *pmkid)
{
	struct wid wid;
	int result;

	wid.id = WID_PMKID_INFO;
	wid.type = WID_STR;
	wid.size = (pmkid->numpmkid * sizeof(struct wilc_pmkid)) + 1;
	wid.val = (u8 *)pmkid;

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));

	return result;
}

int wilc_get_mac_address(struct wilc_vif *vif, u8 *mac_addr)
{
	int result;
	struct wid wid;

	wid.id = WID_MAC_ADDR;
	wid.type = WID_STR;
	wid.size = ETH_ALEN;
	wid.val = mac_addr;

	result = wilc_send_config_pkt(vif, WILC_GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		netdev_err(vif->ndev, "Failed to get mac address\n");

	return result;
}

int wilc_set_mac_address(struct wilc_vif *vif, u8 *mac_addr)
{
	struct wid wid;
	int result;

	wid.id = WID_MAC_ADDR;
	wid.type = WID_STR;
	wid.size = ETH_ALEN;
	wid.val = mac_addr;

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to set mac address\n");

	return result;
}

int wilc_set_join_req(struct wilc_vif *vif, u8 *bssid, const u8 *ies,
		      size_t ies_len)
{
	int result;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct wilc_conn_info *conn_info = &hif_drv->conn_info;

	if (bssid)
		ether_addr_copy(conn_info->bssid, bssid);

	if (ies) {
		conn_info->req_ies_len = ies_len;
		conn_info->req_ies = kmemdup(ies, ies_len, GFP_KERNEL);
		if (!conn_info->req_ies) {
			result = -ENOMEM;
			return result;
		}
	}

	result = wilc_send_connect_wid(vif);
	if (result) {
		PRINT_ER(vif->ndev, "Failed to send connect wid\n");
		goto free_ies;
	}

#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
	hif_drv->connect_timer.data = (unsigned long)hif_drv;
#endif
	hif_drv->connect_timer_vif = vif;
	mod_timer(&hif_drv->connect_timer,
		  jiffies + msecs_to_jiffies(WILC_HIF_CONNECT_TIMEOUT_MS));

	return 0;

free_ies:
	kfree(conn_info->req_ies);

	return result;
}

int wilc_set_mac_chnl_num(struct wilc_vif *vif, u8 channel)
{
	struct wid wid;
	int result;

	wid.id = WID_CURRENT_CHANNEL;
	wid.type = WID_CHAR;
	wid.size = sizeof(char);
	wid.val = &channel;

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to set channel\n");

	return result;
}

int wilc_set_wfi_drv_handler(struct wilc_vif *vif, int index, u8 mode,
			     u8 ifc_id)
{
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	int result;
	struct wilc_drv_handler drv;

	if (!hif_drv)
		return -EFAULT;

	wid.id = WID_SET_DRV_HANDLER;
	wid.type = WID_STR;
	wid.size = sizeof(drv);
	wid.val = (u8 *)&drv;

	drv.handler = cpu_to_le32(index);
	drv.mode = (ifc_id | (mode << 1));

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      hif_drv->driver_handler_id);
	if (result)
		PRINT_ER(vif->ndev, "Failed to set driver handler\n");

	return result;
}

int wilc_set_operation_mode(struct wilc_vif *vif, u32 mode)
{
	struct wid wid;
	struct wilc_op_mode op_mode;
	int result;

	wid.id = WID_SET_OPERATION_MODE;
	wid.type = WID_INT;
	wid.size = sizeof(op_mode);
	wid.val = (u8 *)&op_mode;

	op_mode.mode = cpu_to_le32(mode);

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to set operation mode\n");

	return result;
}

s32 wilc_get_inactive_time(struct wilc_vif *vif, const u8 *mac, u32 *out_val)
{
	struct wid wid;
	s32 result;

	wid.id = WID_SET_STA_MAC_INACTIVE_TIME;
	wid.type = WID_STR;
	wid.size = ETH_ALEN;
	wid.val = kzalloc(wid.size, GFP_KERNEL);
	if (!wid.val) {
		PRINT_ER(vif->ndev, "Failed to allocate buffer\n");
		return -ENOMEM;
	}

	ether_addr_copy(wid.val, mac);
	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	kfree(wid.val);
	if (result) {
		PRINT_ER(vif->ndev, "Failed to set inactive mac\n");
		return result;
	}

	wid.id = WID_GET_INACTIVE_TIME;
	wid.type = WID_INT;
	wid.val = (s8 *)out_val;
	wid.size = sizeof(u32);
	result = wilc_send_config_pkt(vif, WILC_GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to get inactive time\n");

	PRINT_INFO(vif->ndev, CFG80211_DBG, "Getting inactive time : %d\n",
		   *out_val);

	return result;
}

int wilc_get_rssi(struct wilc_vif *vif, s8 *rssi_level)
{
	struct wid wid;
	int result;

	if (!rssi_level) {
		PRINT_ER(vif->ndev, "RSS pointer value is null\n");
		return -EFAULT;
	}

	wid.id = WID_RSSI;
	wid.type = WID_CHAR;
	wid.size = sizeof(char);
	wid.val = rssi_level;
	result = wilc_send_config_pkt(vif, WILC_GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		netdev_err(vif->ndev, "Failed to get RSSI value\n");

	return result;
}

int wilc_get_stats_async(struct wilc_vif *vif, struct rf_info *stats)
{
	int result;
	struct host_if_msg *msg;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, " getting async statistics\n");
	msg = wilc_alloc_work(vif, handle_get_statistics, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.data = (char *)stats;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
		return result;
	}

	return result;
}

int wilc_hif_set_cfg(struct wilc_vif *vif, struct cfg_param_attr *param)
{
	struct wid wid_list[4];
	int i = 0;
	int result;

	if (param->flag & WILC_CFG_PARAM_RETRY_SHORT) {
		wid_list[i].id = WID_SHORT_RETRY_LIMIT;
		wid_list[i].val = (s8 *)&param->short_retry_limit;
		wid_list[i].type = WID_SHORT;
		wid_list[i].size = sizeof(u16);
		i++;
	}
	if (param->flag & WILC_CFG_PARAM_RETRY_LONG) {
		wid_list[i].id = WID_LONG_RETRY_LIMIT;
		wid_list[i].val = (s8 *)&param->long_retry_limit;
		wid_list[i].type = WID_SHORT;
		wid_list[i].size = sizeof(u16);
		i++;
	}
	if (param->flag & WILC_CFG_PARAM_FRAG_THRESHOLD) {
		wid_list[i].id = WID_FRAG_THRESHOLD;
		wid_list[i].val = (s8 *)&param->frag_threshold;
		wid_list[i].type = WID_SHORT;
		wid_list[i].size = sizeof(u16);
		i++;
	}
	if (param->flag & WILC_CFG_PARAM_RTS_THRESHOLD) {
		wid_list[i].id = WID_RTS_THRESHOLD;
		wid_list[i].val = (s8 *)&param->rts_threshold;
		wid_list[i].type = WID_SHORT;
		wid_list[i].size = sizeof(u16);
		i++;
	}

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, wid_list,
				      i, wilc_get_vif_idx(vif));

	return result;
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void get_periodic_rssi(struct timer_list *t)
#else
static void get_periodic_rssi(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct wilc_vif *vif = from_timer(vif, t, periodic_rssi);
#else
	struct wilc_vif *vif = (struct wilc_vif *)arg;
#endif

	if (!vif->hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return;
	}

	if (vif->hif_drv->hif_state == HOST_IF_CONNECTED)
		wilc_get_stats_async(vif, &vif->periodic_stats);

	mod_timer(&vif->periodic_rssi, jiffies + msecs_to_jiffies(5000));
}

int wilc_init(struct net_device *dev, struct host_if_drv **hif_drv_handler)
{
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif = netdev_priv(dev);
	struct wilc *wilc = vif->wilc;
	int i;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Initializing host interface for client %d\n",
		   wilc->clients_count + 1);

	hif_drv  = kzalloc(sizeof(*hif_drv), GFP_KERNEL);
	if (!hif_drv) {
		PRINT_ER(dev, "hif driver is NULL\n");
		return -ENOMEM;
	}
	*hif_drv_handler = hif_drv;
	for (i = 0; i <= wilc->vif_num; i++)
		if (dev == wilc->vif[i]->ndev) {
			wilc->vif[i]->hif_drv = hif_drv;
			hif_drv->driver_handler_id = i + 1;
			break;
		}

#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	vif->obtaining_ip = false;
#endif

	if (wilc->clients_count == 0)
		mutex_init(&wilc->deinit_lock);

	#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
		timer_setup(&vif->periodic_rssi, get_periodic_rssi, 0);
	#else
		setup_timer(&vif->periodic_rssi, get_periodic_rssi,
			    (unsigned long)vif);
	#endif
		mod_timer(&vif->periodic_rssi,
			  jiffies + msecs_to_jiffies(5000));

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	timer_setup(&hif_drv->scan_timer, timer_scan_cb, 0);
	timer_setup(&hif_drv->connect_timer, timer_connect_cb, 0);
	timer_setup(&hif_drv->remain_on_ch_timer, listen_timer_cb, 0);
#else
	setup_timer(&hif_drv->scan_timer, timer_scan_cb, 0);
	setup_timer(&hif_drv->connect_timer, timer_connect_cb, 0);
	setup_timer(&hif_drv->remain_on_ch_timer, listen_timer_cb, 0);
#endif

	hif_drv->hif_state = HOST_IF_IDLE;

	hif_drv->p2p_timeout = 0;

	wilc->clients_count++;

	return 0;
}

int wilc_deinit(struct wilc_vif *vif)
{
	int result = 0;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	mutex_lock(&vif->wilc->deinit_lock);

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "De-initializing host interface for client %d\n",
		   vif->wilc->clients_count);

	del_timer_sync(&hif_drv->scan_timer);
	del_timer_sync(&hif_drv->connect_timer);
	del_timer_sync(&vif->periodic_rssi);
	del_timer_sync(&hif_drv->remain_on_ch_timer);

	if (hif_drv->usr_scan_req.scan_result) {
		hif_drv->usr_scan_req.scan_result(SCAN_EVENT_ABORTED, NULL,
						  hif_drv->usr_scan_req.arg);
		hif_drv->usr_scan_req.scan_result = NULL;
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	kfree(hif_drv);
	vif->hif_drv = NULL;

	vif->wilc->clients_count--;
	mutex_unlock(&vif->wilc->deinit_lock);
	return result;
}

void wilc_network_info_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	int result;
	struct host_if_msg *msg;
	int id;
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif;

	id = get_unaligned_le32(&buffer[length - 4]);
	vif = wilc_get_vif_from_idx(wilc, id);
	if (!vif)
		return;
	hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "driver not init[%p]\n", hif_drv);
		return;
	}

	msg = wilc_alloc_work(vif, handle_rcvd_ntwrk_info, false);
	if (IS_ERR(msg))
		return;

	msg->body.net_info.frame_len = get_unaligned_le16(&buffer[6]) - 1;
	msg->body.net_info.rssi = buffer[8];
	msg->body.net_info.mgmt = kmemdup(&buffer[9],
					  msg->body.net_info.frame_len,
					  GFP_KERNEL);
	if (!msg->body.net_info.mgmt) {
		kfree(msg);
		return;
	}

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "message parameters (%d)\n", result);
		kfree(msg->body.net_info.mgmt);
		kfree(msg);
	}
}

void wilc_gnrl_async_info_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	int result;
	struct host_if_msg *msg;
	int id;
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif;

	mutex_lock(&wilc->deinit_lock);

	id = get_unaligned_le32(&buffer[length - 4]);
	vif = wilc_get_vif_from_idx(wilc, id);
	if (!vif) {
		mutex_unlock(&wilc->deinit_lock);
		return;
	}
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "General asynchronous info packet received\n");

	hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		mutex_unlock(&wilc->deinit_lock);
		return;
	}

	if (!hif_drv->conn_info.conn_result) {
		PRINT_ER(vif->ndev, "there is no current Connect Request\n");
		mutex_unlock(&wilc->deinit_lock);
		return;
	}

	msg = wilc_alloc_work(vif, handle_rcvd_gnrl_async_info, false);
	if (IS_ERR(msg)) {
		mutex_unlock(&wilc->deinit_lock);
		return;
	}

	msg->body.mac_info.status = buffer[7];
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Received MAC status= %d Reason= %d Info = %d\n",
		   buffer[7], buffer[8], buffer[9]);
	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	mutex_unlock(&wilc->deinit_lock);
}

void wilc_scan_complete_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	int result;
	int id;
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif;

	id = get_unaligned_le32(&buffer[length - 4]);
	vif = wilc_get_vif_from_idx(wilc, id);
	if (!vif)
		return;
	hif_drv = vif->hif_drv;
	PRINT_INFO(vif->ndev, GENERIC_DBG, "Scan notification received\n");

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return;
	}

	if (hif_drv->usr_scan_req.scan_result) {
		struct host_if_msg *msg;

		msg = wilc_alloc_work(vif, handle_scan_complete, false);
		if (IS_ERR(msg))
			return;

		result = wilc_enqueue_work(msg);
		if (result) {
			PRINT_ER(vif->ndev, "enqueue work failed\n");
			kfree(msg);
		}
	}
}

int wilc_remain_on_channel(struct wilc_vif *vif, u64 cookie,
			   u32 duration, u16 chan,
			   void (*expired)(void *, u64), void *user_arg)
{
	struct remain_ch roc;
	int result;

	PRINT_INFO(vif->ndev, CFG80211_DBG, "%s called\n", __func__);
	roc.ch = chan;
	roc.expired = expired;
	roc.arg = user_arg;
	roc.duration = duration;
	roc.cookie = cookie;
	result = handle_remain_on_chan(vif, &roc);
	if (result)
		PRINT_ER(vif->ndev, "%s: failed to set remain on channel\n",
			 __func__);

	return result;
}

int wilc_listen_state_expired(struct wilc_vif *vif, u64 cookie)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	del_timer(&hif_drv->remain_on_ch_timer);

	msg = wilc_alloc_work(vif, handle_listen_state_expired, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.remain_on_ch.cookie = cookie;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	return result;
}

void wilc_frame_register(struct wilc_vif *vif, u16 frame_type, bool reg)
{
	struct wid wid;
	int result;
	struct wilc_reg_frame reg_frame;

	wid.id = WID_REGISTER_FRAME;
	wid.type = WID_STR;
	wid.size = sizeof(reg_frame);
	wid.val = (u8 *)&reg_frame;

	memset(&reg_frame, 0x0, sizeof(reg_frame));
	reg_frame.reg = reg;

	switch (frame_type) {
	case IEEE80211_STYPE_ACTION:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "ACTION\n");
		reg_frame.reg_id = WILC_FW_ACTION_FRM_IDX;
		break;

	case IEEE80211_STYPE_PROBE_REQ:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "PROBE REQ\n");
		reg_frame.reg_id = WILC_FW_PROBE_REQ_IDX;
		break;

	default:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Not valid frame type\n");
		break;
	}
	reg_frame.frame_type = cpu_to_le16(frame_type);
	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to frame register\n");
}

int wilc_add_beacon(struct wilc_vif *vif, u32 interval, u32 dtim_period,
		    struct cfg80211_beacon_data *params)
{
	struct wid wid;
	int result;
	u8 *cur_byte;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting adding beacon\n");

	wid.id = WID_ADD_BEACON;
	wid.type = WID_BIN;
	wid.size = params->head_len + params->tail_len + 16;
	wid.val = kzalloc(wid.size, GFP_KERNEL);
	if (!wid.val) {
		PRINT_ER(vif->ndev, "Failed to allocate buffer\n");
		return -ENOMEM;
	}

	cur_byte = wid.val;
	put_unaligned_le32(interval, cur_byte);
	cur_byte += 4;
	put_unaligned_le32(dtim_period, cur_byte);
	cur_byte += 4;
	put_unaligned_le32(params->head_len, cur_byte);
	cur_byte += 4;

	if (params->head_len > 0)
		memcpy(cur_byte, params->head, params->head_len);
	cur_byte += params->head_len;

	put_unaligned_le32(params->tail_len, cur_byte);
	cur_byte += 4;

	if (params->tail_len > 0)
		memcpy(cur_byte, params->tail, params->tail_len);

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send add beacon\n");

	kfree(wid.val);

	return result;
}

int wilc_del_beacon(struct wilc_vif *vif)
{
	int result;
	struct wid wid;
	u8 del_beacon = 0;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deleting beacon message queue params\n");

	wid.id = WID_DEL_BEACON;
	wid.type = WID_CHAR;
	wid.size = sizeof(char);
	wid.val = &del_beacon;
	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send delete beacon\n");

	return result;
}

int wilc_add_station(struct wilc_vif *vif, const u8 *mac,
		     struct station_parameters *params)
{
	struct wid wid;
	int result;
	u8 *cur_byte;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting adding station message queue params\n");

	wid.id = WID_ADD_STA;
	wid.type = WID_BIN;
	wid.size = WILC_ADD_STA_LENGTH + params->supported_rates_len;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		return -ENOMEM;

	cur_byte = wid.val;
	wilc_hif_pack_sta_param(vif, cur_byte, mac, params);

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result != 0)
		PRINT_ER(vif->ndev, "Failed to send add station\n");

	kfree(wid.val);

	return result;
}

int wilc_del_station(struct wilc_vif *vif, const u8 *mac_addr)
{
	struct wid wid;
	int result;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deleting station message queue params\n");

	wid.id = WID_REMOVE_STA;
	wid.type = WID_BIN;
	wid.size = ETH_ALEN;
	wid.val = kzalloc(wid.size, GFP_KERNEL);
	if (!wid.val) {
		PRINT_ER(vif->ndev, "Failed to allocate buffer\n");
		return -ENOMEM;
	}

	if (!mac_addr)
		eth_broadcast_addr(wid.val);
	else
		ether_addr_copy(wid.val, mac_addr);

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to del station\n");

	kfree(wid.val);

	return result;
}

int wilc_del_allstation(struct wilc_vif *vif, u8 mac_addr[][ETH_ALEN])
{
	struct wid wid;
	int result;
	int i;
	u8 assoc_sta = 0;
	struct del_all_sta del_sta;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deauthenticating station message queue params\n");
	memset(&del_sta, 0x0, sizeof(del_sta));
	for (i = 0; i < WILC_MAX_NUM_STA; i++) {
		if (!is_zero_ether_addr(mac_addr[i])) {
			PRINT_INFO(vif->ndev,
				   CFG80211_DBG, "BSSID = %x%x%x%x%x%x\n",
				   mac_addr[i][0], mac_addr[i][1],
				   mac_addr[i][2], mac_addr[i][3],
				   mac_addr[i][4], mac_addr[i][5]);
			assoc_sta++;
			ether_addr_copy(del_sta.mac[i], mac_addr[i]);
		}
	}
	if (!assoc_sta) {
		PRINT_INFO(vif->ndev, CFG80211_DBG, "NO ASSOCIATED STAS\n");
		return 0;
	}
	del_sta.assoc_sta = assoc_sta;

	wid.id = WID_DEL_ALL_STA;
	wid.type = WID_STR;
	wid.size = (assoc_sta * ETH_ALEN) + 1;
	wid.val = (u8 *)&del_sta;

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send delete all station\n");

	return result;
}

int wilc_edit_station(struct wilc_vif *vif, const u8 *mac,
		      struct station_parameters *params)
{
	struct wid wid;
	int result;
	u8 *cur_byte;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting editing station message queue params\n");

	wid.id = WID_EDIT_STA;
	wid.type = WID_BIN;
	wid.size = WILC_ADD_STA_LENGTH + params->supported_rates_len;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		return -ENOMEM;

	cur_byte = wid.val;
	wilc_hif_pack_sta_param(vif, cur_byte, mac, params);

	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send edit station\n");

	kfree(wid.val);
	return result;
}

int wilc_set_power_mgmt(struct wilc_vif *vif, bool enabled, u32 timeout)
{
	struct wid wid;
	int result;
	s8 power_mode;

	if (wilc_wlan_get_num_conn_ifcs(vif->wilc) == 2 && enabled)
		return 0;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "\n\n>> Setting PS to %d <<\n\n",
		   enabled);
	if (enabled)
		power_mode = WILC_FW_MIN_FAST_PS;
	else
		power_mode = WILC_FW_NO_POWERSAVE;

	wid.id = WID_POWER_MANAGEMENT;
	wid.val = &power_mode;
	wid.size = sizeof(char);
	result = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send power management\n");
	else
		store_power_save_current_state(vif, power_mode);

	return result;
}

int wilc_setup_multicast_filter(struct wilc_vif *vif, u32 enabled, u32 count,
				u8 *mc_list)
{
	int result;
	struct host_if_msg *msg;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting Multicast Filter params\n");
	msg = wilc_alloc_work(vif, handle_set_mcast_filter, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.mc_info.enabled = enabled;
	msg->body.mc_info.cnt = count;
	msg->body.mc_info.mc_list = mc_list;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}
	return result;
}

void handle_powersave_state_changes(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;

	PRINT_INFO(vif->ndev, GENERIC_DBG, "Recover PS = %d\n",
		   vif->pwrsave_current_state);

	/* Recover PS previous state */
	wilc_set_power_mgmt(vif, vif->pwrsave_current_state, 0);
}

void wilc_powersave_state_changes(struct wilc_vif *vif)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_powersave_state_changes, false);
	if (IS_ERR(msg))
		return;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}
}

int wilc_set_tx_power(struct wilc_vif *vif, u8 tx_power)
{
	int ret;
	struct wid wid;

	wid.id = WID_TX_POWER;
	wid.type = WID_CHAR;
	wid.val = &tx_power;
	wid.size = sizeof(char);

	ret = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	return ret;
}

int wilc_get_tx_power(struct wilc_vif *vif, u8 *tx_power)
{
	int ret;
	struct wid wid;

	wid.id = WID_TX_POWER;
	wid.type = WID_CHAR;
	wid.val = tx_power;
	wid.size = sizeof(char);

	ret = wilc_send_config_pkt(vif, WILC_GET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	return ret;
}

bool is_valid_gpio(struct wilc_vif *vif, u8 gpio)
{
	switch (vif->wilc->chip) {
	case WILC_1000:
		if (gpio == 0 || gpio == 1 || gpio == 4 || gpio == 6)
			return true;
		else
			return false;
	case WILC_3000:
		if (gpio == 0 || gpio == 3 || gpio == 4 ||
		    (gpio >= 17 && gpio <= 20))
			return true;
		else
			return false;
	default:
		return false;
	}
}

int wilc_set_antenna(struct wilc_vif *vif, u8 mode)
{
	struct wid wid;
	int ret;
	struct sysfs_attr_group *attr_syfs_p = &vif->attr_sysfs;
	struct host_if_set_ant set_ant;

	set_ant.mode = mode;

	if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_INVALID_GPIO_CTRL) {
		PRINT_ER(vif->ndev, "Ant switch GPIO mode is invalid.\n");
		PRINT_ER(vif->ndev, "Set it using /sys/wilc/ant_swtch_mode\n");
		return WILC_FAIL;
	}

	if (is_valid_gpio(vif, attr_syfs_p->antenna1)) {
		set_ant.antenna1 = attr_syfs_p->antenna1;
	} else {
		PRINT_ER(vif->ndev, "Invalid GPIO%d\n", attr_syfs_p->antenna1);
		return WILC_FAIL;
	}

	if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_DUAL_GPIO_CTRL) {
		if ((attr_syfs_p->antenna2 != attr_syfs_p->antenna1) &&
		    is_valid_gpio(vif, attr_syfs_p->antenna2)) {
			set_ant.antenna2 = attr_syfs_p->antenna2;
		} else {
			PRINT_ER(vif->ndev, "Invalid GPIO %d\n",
				 attr_syfs_p->antenna2);
			return WILC_FAIL;
		}
	}

	set_ant.gpio_mode = attr_syfs_p->ant_swtch_mode;

	wid.id = WID_ANTENNA_SELECTION;
	wid.type = WID_BIN;
	wid.val = (u8 *)&set_ant;
	wid.size = sizeof(struct host_if_set_ant);
	if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_SNGL_GPIO_CTRL)
		PRINT_INFO(vif->ndev, CFG80211_DBG,
			   "set antenna %d on GPIO %d\n", set_ant.mode,
			   set_ant.antenna1);
	else if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_DUAL_GPIO_CTRL)
		PRINT_INFO(vif->ndev, CFG80211_DBG,
			   "set antenna %d on GPIOs %d and %d\n",
			   set_ant.mode, set_ant.antenna1,
			   set_ant.antenna2);

	ret = wilc_send_config_pkt(vif, WILC_SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	if (ret)
		PRINT_ER(vif->ndev, "Failed to set antenna mode\n");

	return ret;
}
