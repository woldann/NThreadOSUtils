/**
 * MIT License
 *
 * Copyright (c) 2025 Serkan Aksoy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission nice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ntosutils.h"

void *nosu_push_addr;
void *nosu_sleep_addr;
nthread_reg_offset_t nosu_push_offset;

uint16_t nosu_sleep_opcode;
uint16_t nosu_push_ret_opcode;

uint16_t push_ret_opcodes[] = { PUSH_RBX_RET_OPCODE, PUSH_RBP_RET_OPCODE,
				PUSH_RSI_RET_OPCODE, PUSH_RDI_RET_OPCODE };
uint16_t sleep_opcodes[] = { SLEEP_OPCODE };

nerror_t nosu_global_init()
{
	int8_t i;

	uint16_t sleep_opcode;
	void *sleep_addr;

	int8_t sleep_opcode_count =
		sizeof(push_ret_opcodes) / sizeof(*push_ret_opcodes);
	for (i = 0; i < sleep_opcode_count; i++) {
		sleep_opcode = sleep_opcodes[i];
		sleep_addr = find_gadget(sleep_opcode);
		if (sleep_addr != NULL)
			break;
	}

	if (i == sleep_opcode_count)
		return GET_ERR(NTOSUTILS_FIND_PUSH_RET_GADGET_ERROR);

	uint16_t push_ret_opcode;
	void *push_addr;

	int8_t push_ret_opcode_count =
		sizeof(push_ret_opcodes) / sizeof(*push_ret_opcodes);
	for (i = 0; i < push_ret_opcode_count; i++) {
		push_ret_opcode = push_ret_opcodes[i];
		push_addr = find_gadget(push_ret_opcode);
		if (push_addr != NULL)
			break;
	}

	if (i == push_ret_opcode_count)
		return GET_ERR(NTOSUTILS_FIND_PUSH_RET_GADGET_ERROR);

#ifdef LOG_LEVEL_2

	if (push_ret_opcode == PUSH_RDI_RET_OPCODE ||
	    push_ret_opcode == PUSH_RSI_RET_OPCODE) {
		LOG_WARN(
			"A push gadget was found, but using this register may lead to instability");
	}

#endif /* ifdef LOG_LEVEL_2 */

	nosu_push_addr = push_addr;
	nosu_sleep_addr = sleep_addr;

	nosu_push_ret_opcode = push_ret_opcode;
	nosu_sleep_opcode = sleep_opcode;

	nosu_push_offset = REG_OFFSET_FROM_PUSH_RET_OPCODE(push_ret_opcodes[i]);

#ifdef LOG_LEVEL_3

	LOG_INFO("Push Address(%p)", nosu_push_addr);
	LOG_INFO("Push Opcode(%02X)", nosu_push_ret_opcode);

	LOG_INFO("Sleep Address(%p)", nosu_sleep_addr);
	LOG_INFO("Sleep Opcode(%02X)", nosu_sleep_opcode);

#endif /* ifdef LOG_LEVEL_3 */

#ifdef LOG_INFO_2
	LOG_INFO("NThread dummy process test started");
#endif /* ifdef LOG_INFO_2 */

	if (nosu_test())
		return GET_ERR(NTOSUTILS_TEST_ERROR);

	return N_OK;
}

nerror_t nosu_init(nthread_t *nthread, ntid_t thread_id)
{
	return nthread_init(nthread, thread_id, nosu_push_offset,
			    nosu_push_addr, nosu_sleep_addr);
}

nerror_t nosu_attach(ntid_t thread_id)
{
	return ntu_attach_ex(thread_id, nosu_push_offset, nosu_push_addr,
			     nosu_sleep_addr);
}

bool nosu_test()
{
	ntid_t tid = nosu_dummy_thread();
	if (tid == 0)
		return GET_ERR(NTOSUTILS_DUMMY_THREAD_ERROR);

	nthread_t nthread;
	if (HAS_ERR(nosu_init(&nthread, tid)))
		return GET_ERR(NTOSUTILS_NOSU_INIT_ERROR);

	nosu_kill_dummy(tid);
	return N_OK;
}
