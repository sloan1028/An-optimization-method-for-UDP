#include "rudp.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define GENERAL_PACKAGE 512
//#define GENERAL_PACKAGE 128

struct message
{
	struct message *next;
	uint8_t *buffer;
	int sz;
	int cap;
	int id;
	int tick;
};

struct message_queue
{
	struct message *head;
	struct message *tail;
};

struct array
{
	int cap;
	int n; // n是array里元素的数量
	int *a;
};

struct rudp
{
	struct message_queue send_queue;   // user packages will send
	struct message_queue recv_queue;   // the packages recv
	struct message_queue send_history; // user packages already send

	struct rudp_package *send_package; // returns by rudp_update

	struct message *free_list; // recycle message struct 循环消息结构
	struct array send_again;   // package id need send again

	int corrupt;
	int current_tick;
	int last_send_tick;
	int last_expired_tick;
	int send_id;
	int recv_id_min;
	int recv_id_max;
	int send_delay;
	int expired;
};

// rudp_new 创建 rudp 对象时，有两个参数可配置。
// send delay 表示数据累积多少个时间周期 tick 数才打包在一起发送。
// expired time 表示已发送的包至少保留多少个时间周期
struct rudp *rudp_new(int send_delay, int expired_time)
{
	struct rudp *U = (rudp *)malloc(sizeof(*U));
	memset(U, 0, sizeof(*U));
	U->send_delay = send_delay;
	U->expired = expired_time;
	return U;
}

// 清理U的send_package
static void clear_outpackage(struct rudp *U)
{
	struct rudp_package *tmp = U->send_package;
	while (tmp)
	{
		struct rudp_package *next = tmp->next;
		free(tmp);
		tmp = next;
	}
	U->send_package = NULL;
}

// 清除message_list
static void free_message_list(struct message *m)
{
	while (m)
	{
		struct message *next = m->next;
		free(m);
		m = next;
	}
}

void rudp_delete(struct rudp *U)
{
	free_message_list(U->send_queue.head);
	free_message_list(U->recv_queue.head);
	free_message_list(U->send_history.head);
	free_message_list(U->free_list);
	clear_outpackage(U);
	free(U->send_again.a);
	free(U);
}

// 创建一个新消息并返回
static struct message *new_message(struct rudp *U, const uint8_t *buffer, int sz)
{
	struct message *tmp = U->free_list;
	if (tmp)
	{
		U->free_list = tmp->next; // 删除free_list的头部, 也就是把tmp在freelist中删掉
		if (tmp->cap < sz)
		{ //如果tmp的cap小于sz，就不要了   重复利用free_list的空间把
			free(tmp);
			tmp = NULL;
		}
	}
	if (tmp == NULL)
	{
		int cap = sz;
		if (cap < GENERAL_PACKAGE)
		{
			cap = GENERAL_PACKAGE;
		}
		tmp = (message *)malloc(sizeof(struct message) + cap);
		tmp->cap = cap;
	}
	tmp->sz = sz;
	tmp->buffer = (uint8_t *)(tmp + 1);
	if (sz > 0 && buffer)
	{
		memcpy(tmp->buffer, buffer, sz);
	}
	tmp->tick = 0;
	tmp->id = 0;
	tmp->next = NULL;
	return tmp;
}

// 把m添加到free_list的头节点
static void delete_message(struct rudp *U, struct message *m)
{
	m->next = U->free_list;
	U->free_list = m;
}

//把m添加到q的尾节点
static void queue_push(struct message_queue *q, struct message *m)
{
	if (q->tail == NULL)
	{
		q->head = q->tail = m;
	}
	else
	{
		q->tail->next = m;
		q->tail = m;
	}
}

// pop出头节点，返回头节点
static struct message *queue_pop(struct message_queue *q, int id)
{
	if (q->head == NULL)
		return NULL;
	struct message *m = q->head;
	if (m->id != id)
		return NULL;
	q->head = m->next;
	m->next = NULL;
	if (q->head == NULL)
		q->tail = NULL;
	return m;
}

// 将id插入array中
static void array_insert(struct array *a, int id)
{
	int i;
	for (i = 0; i < a->n; i++)
	{
		if (a->a[i] == id)
			return;
		if (a->a[i] > id)
		{
			break;
		}
	}
	// insert before i
	if (a->n >= a->cap)
	{
		if (a->cap == 0)
		{
			a->cap = 16;
		}
		else
		{
			a->cap *= 2;
		}
		a->a = (int *)realloc(a->a, sizeof(int) * a->cap);
	}
	int j;
	for (j = a->n; j > i; j--)
	{
		a->a[j] = a->a[j - 1];
	}
	a->a[i] = id;
	++a->n;
}

// 发送RUDP包（把包插入发送队列中等待周期发送）
void rudp_input(struct rudp *U, const char *buffer, int sz)
{
	assert(sz <= MAX_PACKAGE);
	struct message *m = new_message(U, (const uint8_t *)buffer, sz);
	m->id = U->send_id++;
	m->tick = U->current_tick;
	queue_push(&U->send_queue, m); //添加到发送队列中
}

//接受消息 放到para的buffer数组中
int rudp_recv(struct rudp *U, char buffer[MAX_PACKAGE])
{
	if (U->corrupt)
	{
		U->corrupt = 0;
		return -1;
	}
	struct message *tmp = queue_pop(&U->recv_queue, U->recv_id_min);
	if (tmp == NULL)
	{
		return 0;
	}
	++U->recv_id_min;
	int sz = tmp->sz;
	if (sz > 0)
	{
		memcpy(buffer, tmp->buffer, sz);
	}
	delete_message(U, tmp);
	return sz;
}

//清除在tick时间节点前的历史记录
static void clear_send_expired(struct rudp *U, int tick)
{
	struct message *m = U->send_history.head;
	struct message *last = NULL;
	while (m)
	{
		if (m->tick >= tick)
		{
			break;
		}
		last = m;
		m = m->next;
	}
	if (last)
	{
		// free all the messages before tick
		last->next = U->free_list;
		U->free_list = U->send_history.head;
	}
	U->send_history.head = m;
	if (m == NULL)
	{
		U->send_history.tail = NULL;
	}
}

// 如其名
static int get_id(struct rudp *U, const uint8_t *buffer)
{
	int id = buffer[0] * 256 + buffer[1];
	id |= U->recv_id_max & ~0xffff;
	if (id < U->recv_id_max - 0x8000)
		id += 0x10000;
	else if (id > U->recv_id_max + 0x8000)
		id -= 0x10000;
	return id;
}

static void add_request(struct rudp *U, int id)
{
	array_insert(&U->send_again, id);
}

static void insert_message(struct rudp *U, int id, const uint8_t *buffer, int sz)
{
	if (id < U->recv_id_min)
		return;
	if (id > U->recv_id_max || U->recv_queue.head == NULL)
	{
		struct message *m = new_message(U, buffer, sz);
		m->id = id;
		queue_push(&U->recv_queue, m);
		U->recv_id_max = id;
	}
	else //如果id在min和max之间
	{
		struct message *m = U->recv_queue.head;
		struct message **last = &U->recv_queue.head;
		do
		{
			if (m->id == id) // 已经有了，退出
			{
				return;
			}
			if (m->id > id) // 找到需要插入的地方
			{
				// insert here
				struct message *tmp = new_message(U, buffer, sz);
				tmp->id = id;
				tmp->next = m;
				*last = tmp;
				return;
			}
			last = &m->next;
			m = m->next;
		} while (m);
	}
}

static void add_missing(struct rudp *U, int id)
{
	insert_message(U, id, NULL, -1);
}

#define TYPE_IGNORE 0
#define TYPE_CORRUPT 1
#define TYPE_REQUEST 2
#define TYPE_MISSING 3
#define TYPE_NORMAL 4

//提取包
static void extract_package(struct rudp *U, const uint8_t *buffer, int sz)
{
	while (sz > 0)
	{
		int len = buffer[0];
		if (len > 127)
		{ // >127说明第一位为1，tag编码为两字节
			if (sz <= 1)
			{
				U->corrupt = 1;
				return;
			}
			len = (len * 256 + buffer[1]) & 0x7fff; // &7fff是为了把最高位的1消掉
			buffer += 2;
			sz -= 2;
		}
		else
		{
			buffer += 1;
			sz -= 1;
		}
		switch (len)
		{
		case TYPE_IGNORE: // len=0 为心跳包
			if (U->send_again.n == 0)
			{
				// request next package id
				//array_insert(&U->send_again, U->recv_id_min);
			}
			break;
		case TYPE_CORRUPT: // 连接异常
			U->corrupt = 1;
			return;
		case TYPE_REQUEST: //请求包
		case TYPE_MISSING: //异常包
			if (sz < 2)
			{
				U->corrupt = 1;
				return;
			}
			(len == TYPE_REQUEST ? add_request : add_missing)(U, get_id(U, buffer)); //如果等于2执行请求包，如果等于3执行异常包
			buffer += 2;
			sz -= 2;
			break;
		default:
			len -= TYPE_NORMAL;
			if (sz < len + 2)
			{
				U->corrupt = 1;
				return;
			}
			else
			{
				int id = get_id(U, buffer);
				insert_message(U, id, buffer + 4, len); // 把消息插入message中
				// 原
				// insert_message(U, id, buffer + 2, len); // 把消息插入message中
			}

			// 这里改过了分包组包
			buffer += len + 4;
			sz -= len + 4;

			break;
		}
	}
}

struct tmp_buffer
{
	uint8_t buf[GENERAL_PACKAGE];
	int sz;
	struct rudp_package *head;
	struct rudp_package *tail;
};

// new一个新的package出来
static void new_package(struct rudp *U, struct tmp_buffer *tmp)
{
	struct rudp_package *p = (rudp_package *)malloc(sizeof(*p) + tmp->sz);
	p->next = NULL;
	p->buffer = (char *)(p + 1);
	p->sz = tmp->sz;
	memcpy(p->buffer, tmp->buf, tmp->sz);
	if (tmp->tail == NULL)
	{ // 这里是为了让p插到tmp的尾部
		tmp->head = tmp->tail = p;
	}
	else
	{
		tmp->tail->next = p;
		tmp->tail = p;
	}
	tmp->sz = 0; //最后相当于把tmp剪切到p中，tmp成空
}

// 填充头部
static int fill_header(uint8_t *buf, int len, int id)
{
	int sz;
	if (len < 128)
	{
		buf[0] = len;
		++buf;
		sz = 1;
	}
	else
	{
		buf[0] = ((len & 0x7f00) >> 8) | 0x80;
		buf[1] = len & 0xff;
		buf += 2;
		sz = 2;
	}
	buf[0] = (id & 0xff00) >> 8;
	buf[1] = id & 0xff;

	//这里加了一行用来区分是否分包了
	buf[2] = 0x80;
	buf[3] = 0x00;

	return sz + 4;
}

// 打包请求包
static void pack_request(struct rudp *U, struct tmp_buffer *tmp, int id, int tag)
{
	int sz = GENERAL_PACKAGE - tmp->sz;
	if (sz < 3)
	{ //说明tmp的sz > GENERAL_PACKAGE-3 可能是超出空间了所以new一个新的，这个新的会放在tmp的队列中
		new_package(U, tmp);
	}
	uint8_t *buffer = tmp->buf + tmp->sz;
	tmp->sz += fill_header(buffer, tag, id);
}

// 打包信息，把m打进tmp中
static void pack_message(struct rudp *U, struct tmp_buffer *tmp, struct message *m)
{
	int sz = GENERAL_PACKAGE - tmp->sz; // 当前的tmp还能容纳的大小
	if (m->sz > GENERAL_PACKAGE - 4)
	{ // 如果m的sz太大了一个tmp容不下
		if (tmp->sz > 0)
			new_package(U, tmp);
		// big package
		sz = 4 + m->sz;
		struct rudp_package *p = (rudp_package *)malloc(sizeof(*p) + sz);
		p->next = NULL;
		p->buffer = (char *)(p + 1);
		p->sz = sz;
		fill_header((uint8_t *)p->buffer, m->sz + TYPE_NORMAL, m->id);
		memcpy(p->buffer + 4, m->buffer, m->sz);
		if (tmp->tail == NULL)
		{ // 把p加到tmp的尾部
			tmp->head = tmp->tail = p;
		}
		else
		{
			tmp->tail->next = p;
			tmp->tail = p;
		}
		return;
	}
	if (sz < 4 + m->sz)
	{ // tmp能容纳的大小不能容纳现在的m
		new_package(U, tmp);
	}
	uint8_t *buf = tmp->buf + tmp->sz;
	int len = fill_header(buf, m->sz + TYPE_NORMAL, m->id);
	tmp->sz += len + m->sz;
	buf += len;
	memcpy(buf, m->buffer, m->sz);
}

// 寻找漏掉的包，请求对面重新发送, 打包到tmp里面拉
static void
request_missing(struct rudp *U, struct tmp_buffer *tmp)
{
	int id = U->recv_id_min;
	struct message *m = U->recv_queue.head;
	while (m)
	{
		assert(m->id >= id);
		if (m->id > id)
		{ //如果m->id更大的话说明中间有漏的消息需要处理
			int i;
			for (i = id; i < m->id; i++)
			{
				pack_request(U, tmp, i, TYPE_REQUEST); //会把漏的消息打包到tmp里面
			}
		}
		id = m->id + 1;
		m = m->next;
	}
}

// 根据对面请求需要重发的包进行回应
static void
reply_request(struct rudp *U, struct tmp_buffer *tmp)
{
	int i;
	struct message *history = U->send_history.head;
	for (i = 0; i < U->send_again.n; i++)
	{
		int id = U->send_again.a[i];
		if (id < U->recv_id_min)
		{
			// alreay recv, ignore
			continue;
		}
		for (;;)
		{
			if (history == NULL || id < history->id)
			{ // id < history的id或为空说明history里没有等于id的了，这个id的包已经废弃了发送异常包
				// expired
				pack_request(U, tmp, id, TYPE_MISSING); // 打包异常包（这个id的包已经废弃了）到tmp里
				break;
			}
			else if (id == history->id)
			{
				pack_message(U, tmp, history); // 会把请求需要的消息history打包到tmp里
				break;
			}
			history = history->next;
		}
	}

	U->send_again.n = 0;
}

static void
send_message(struct rudp *U, struct tmp_buffer *tmp)
{
	struct message *m = U->send_queue.head;
	while (m)
	{
		pack_message(U, tmp, m);
		m = m->next;
	}
	if (U->send_queue.head)
	{
		if (U->send_history.tail == NULL)
		{ // 如果历史队列为空
			U->send_history = U->send_queue;
		}
		else
		{ // 不为空就插入历史队列的尾端
			U->send_history.tail->next = U->send_queue.head;
			U->send_history.tail = U->send_queue.tail;
		}
		U->send_queue.head = NULL; // 把发送队列清空
		U->send_queue.tail = NULL;
	}
}

/*
	1. request missing ( lookup U->recv_queue )
	2. reply request ( U->send_again )
	3. send message ( U->send_queue )
	4. send heartbeat
 */
static struct rudp_package *gen_outpackage(struct rudp *U)
{
	struct tmp_buffer tmp;
	tmp.sz = 0;
	tmp.head = NULL;
	tmp.tail = NULL;

	request_missing(U, &tmp);
	reply_request(U, &tmp);
	send_message(U, &tmp);
	printf("tmp -> sz : %d\n", tmp.sz); 
	if(tmp.head) printf("tmp.head.size: %d\n", tmp.head->sz);
	// close tmp

	if (tmp.head == NULL)
	{ //如果为空就发送心跳包
		if (tmp.sz == 0)
		{
			tmp.buf[0] = TYPE_IGNORE;
			tmp.sz = 1;
		}
	}
	new_package(U, &tmp); // 最后这一步把tmp设置成虚拟头节点
	return tmp.head;
}

// rudp_update的api要求业务层按时间周期调用，当然也可以在同一时间片内调用多次，用传入的参数 tick 做区分。
//如果tick为0表示是在同一时间片内，不用急着处理数据，
//当 tick 大于 0 时，才表示时间流逝，这时可以合并上个时间周期内的数据集中处理。

//每次调用都有可能输出一系列需要发送出去的 UDP 包。这些数据包是由过去的 rudp_input 调用压入的数据产生的，
//同时也包含了，对端请求重传的数据，以及在没有通讯数据时插入的心跳包等。
struct rudp_package *rudp_update(struct rudp *U, const void *buffer, int sz, int tick)
{
	U->current_tick += tick;
	clear_outpackage(U); // 先把U的send_package清空
	extract_package(U, (uint8_t *)buffer, sz);
	if (U->current_tick >= U->last_expired_tick + U->expired)
	{
		clear_send_expired(U, U->last_expired_tick);
		U->last_expired_tick = U->current_tick;
	}
	if (U->current_tick >= U->last_send_tick + U->send_delay)
	{
		U->send_package = gen_outpackage(U);
		U->last_send_tick = U->current_tick;
		return U->send_package;
	}
	else
	{
		return NULL;
	}
}
