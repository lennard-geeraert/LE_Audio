/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

/* Dit was eerst 10 ms, maar dan werkte de code niet */
#define BUF_ALLOC_TIMEOUT (50) /* 10 ms */
#define BIG_TERMINATE_TIMEOUT_US (60 * USEC_PER_SEC) /* 60 s */
#define BIG_SDU_INTERVAL_US (10000) /* 10 ms */

#define BIS_ISO_CHAN_COUNT 2
/* Een bufferpool is een verzameling van vooraf gedefinieerde geheugenblokken (buffers) die in één keer worden toegewezen en die vervolgens worden beheerd en hergebruikt door de applicatie. Dit voorkomt constante dynamische geheugenallocatie en -deallocatie tijdens de uitvoering van de applicatie */
NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, BIS_ISO_CHAN_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* Voor het synchroniseren wanneer een BIG is aangemaakt */
static K_SEM_DEFINE(sem_big_cmplt, 0, BIS_ISO_CHAN_COUNT);
/* Voor het synchroniseren bij het beëindigen van een BIG */
static K_SEM_DEFINE(sem_big_term, 0, BIS_ISO_CHAN_COUNT);
/* Om aan te geven wanneer ISO-data verzonden kan worden */
static K_SEM_DEFINE(sem_iso_data, CONFIG_BT_ISO_TX_BUF_COUNT,
				   CONFIG_BT_ISO_TX_BUF_COUNT);

#define INITIAL_TIMEOUT_COUNTER (BIG_TERMINATE_TIMEOUT_US / BIG_SDU_INTERVAL_US)

/* sequentienummer bij voor het verzenden van ISO-data */
static uint16_t seq_num;

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);
	seq_num = 0U;
	/* incrementeer semafoor met 1 als de max nog niet bereikt is */
	k_sem_give(&sem_big_cmplt);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected with reason 0x%02x\n", chan, reason);
	k_sem_give(&sem_big_term);
}

static void iso_sent(struct bt_iso_chan *chan)
{
	// printk("ISO Channel %p send data\n", chan);
	k_sem_give(&sem_iso_data);
}

static struct bt_iso_chan_ops iso_ops = {
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
	.sent           = iso_sent,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = sizeof(uint32_t), /* maximale grootte van SDU in bytes */
	.rtn = 1, /* Channel Retransmission Number => 1 retry */
	.phy = BT_GAP_LE_PHY_2M, /* 2 Mbps => hogere snelheid, maar lager bereik */
};

static struct bt_iso_chan_qos bis_iso_qos = {
	.tx = &iso_tx_qos, /* Channel Transmission QoS (Quality of Service) */
};

/* Definieert twee ISO-kanalen met de bovenstaande instellingen en callbacks */
static struct bt_iso_chan bis_iso_chan[] = {
	{ .ops = &iso_ops, .qos = &bis_iso_qos, },
	{ .ops = &iso_ops, .qos = &bis_iso_qos, },
};

static struct bt_iso_chan *bis[] = {
	&bis_iso_chan[0],
	&bis_iso_chan[1],
};

static struct bt_iso_big_create_param big_create_param = {
	.num_bis = BIS_ISO_CHAN_COUNT,
	.bis_channels = bis,
	.interval = BIG_SDU_INTERVAL_US, /* in microseconds */
	/* tijd tss data ingevoerd en verzonden in de BIS => ms */
	.latency = 10,
	.packing = 0, /* 0 - sequential, 1 - interleaved */
	.framing = 0, /* 0 - unframed, 1 - framed */
};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE /* 0x09 */, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int main(void)
{
	uint32_t timeout_counter = INITIAL_TIMEOUT_COUNTER; /* 6000 */
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;
	int err;

	uint32_t iso_send_count = 0;
	uint8_t iso_data[sizeof(iso_send_count)] = { 0 };

	printk("Starting ISO Broadcast Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return 0;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return 0;
	}

	/* Create BIG */
	err = bt_iso_big_create(adv, &big_create_param, &big);
	if (err) {
		printk("Failed to create BIG (err %d)\n", err);
		return 0;
	}

	for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
		printk("Waiting for BIG complete chan %u...\n", chan);
		/* blokkeert de code totdat de semafoor wordt vrijgegeven */
		err = k_sem_take(&sem_big_cmplt, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("BIG create complete chan %u.\n", chan);
	}

	while (true) {
		for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
			struct net_buf *buf;
			int ret;

			buf = net_buf_alloc(&bis_tx_pool, K_MSEC(BUF_ALLOC_TIMEOUT));
			if (!buf) {
				printk("Data buffer allocate timeout on channel %u\n", chan);
				return 0;
			}

			ret = k_sem_take(&sem_iso_data, K_MSEC(BUF_ALLOC_TIMEOUT));
			if (ret) {
				printk("k_sem_take for ISO data sent failed\n");
				/* Decrements the reference count of a buffer => The buffer is put back into the pool if the reference count reaches zero*/
				net_buf_unref(buf);
				return 0;
			}

			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
			/* Zet uint32 om in array van bytes in little-endian formaat */
			sys_put_le32(iso_send_count, iso_data);
			/* Voeg ISO data toe aan buffer */
			net_buf_add_mem(buf, iso_data, sizeof(iso_data));
			/* Verzend de bufferinhoud via het BIS ISO-kanaal */
			ret = bt_iso_chan_send(&bis_iso_chan[chan], buf, seq_num);
			if (ret < 0) {
				printk("Unable to broadcast data on channel %u : %d", chan,ret);
				net_buf_unref(buf);
				return 0;
			}
		}

		/* ISO_PRINT_INTERVAL staat in Kconfig file */
		if ((iso_send_count % CONFIG_ISO_PRINT_INTERVAL) == 0) {
			printk("Sending value %u with sequence nr %u\n", iso_send_count, seq_num);
		}

		iso_send_count++;
		seq_num++;

		timeout_counter--;
		if (!timeout_counter) {
			timeout_counter = INITIAL_TIMEOUT_COUNTER;

			printk("BIG Terminate...");
			err = bt_iso_big_terminate(big);
			if (err) {
				printk("failed (err %d)\n", err);
				return 0;
			}
			printk("done.\n");

			for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT;
			     chan++) {
				printk("Waiting for BIG terminate complete chan %u...\n", chan);
				err = k_sem_take(&sem_big_term, K_FOREVER);
				if (err) {
					printk("failed (err %d)\n", err);
					return 0;
				}
				printk("BIG terminate complete chan %u.\n", chan);
			}

			printk("Create BIG...");
			err = bt_iso_big_create(adv, &big_create_param, &big);
			if (err) {
				printk("failed (err %d)\n", err);
				return 0;
			}
			printk("done.\n");

			for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
				printk("Waiting for BIG complete chan %u...\n", chan);
				err = k_sem_take(&sem_big_cmplt, K_FOREVER);
				if (err) {
					printk("failed (err %d)\n", err);
					return 0;
				}
				printk("BIG create complete chan %u.\n", chan);
			}
		}
	}
}
