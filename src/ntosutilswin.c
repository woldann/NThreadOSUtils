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
 * The above copyright notice and this permission notice shall be included in all
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

#include "ntosutilswin.h"

#include <psapi.h>
#include <tlhelp32.h>

void *NTHREAD_API find_gadget(uint16_t opcode)
{
	HANDLE proc = GetCurrentProcess();
	if (NULL == proc)
		return NULL;

	HMODULE mods[1024];
	DWORD needed;

	if (!EnumProcessModules(proc, mods, sizeof(mods), &needed))
		return NULL;

	MEMORY_BASIC_INFORMATION mbi;
	DWORD oldProtect;

	register int8_t *addr;
	unsigned int i;
	for (i = 1; i < (needed / sizeof(HMODULE)); i++) {
		HMODULE mod = mods[i];
		addr = (void *)mod;
		while (true) {
			memset(&mbi, 0, sizeof(mbi));
			if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0)
				break;

			DWORD protect = mbi.Protect;
			uint64_t l =
				(uint64_t)(mbi.BaseAddress - (void *)addr) +
				mbi.RegionSize;

			bool readable = (protect == PAGE_EXECUTE_READ) ||
					(protect == PAGE_EXECUTE_READWRITE);
			bool executable = (protect == PAGE_EXECUTE) || readable;

			if (executable) {
				if (!readable &&
				    !VirtualProtect(addr, l,
						    PAGE_EXECUTE_READWRITE,
						    &protect))
					continue;

				void *ret = memmem(addr, l, (void *)&opcode,
						   sizeof(opcode));
				if (ret != NULL)
					return ret;

				if (!readable)
					VirtualProtect(addr, l, protect,
						       &protect);
			}

			addr += l;
		}
	}

	return NULL;
}

ntid_t NTHREAD_API nosu_dummy_thread()
{
	STARTUPINFOA si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);

	BOOL ok = CreateProcessA(NULL, "cmd.exe /c ping 127.0.0.1 -n 6 >nul",
				 NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL,
				 NULL, &si, &pi);

	if (!ok)
		return 0;

	return pi.dwThreadId;
}

void NTHREAD_API nosu_kill_dummy(ntid_t thread_id)
{
	HANDLE thread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, thread_id);
	if (thread == NULL)
		return;

	DWORD pid = GetProcessIdOfThread(thread);
	HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (handle == NULL)
		return;

	TerminateProcess(handle, 0);
}

nerror_t NTHREAD_API nosu_upgrade(HANDLE thread)
{
	DWORD tid = GetThreadId(thread);
	return nosu_attach((ntid_t)tid);
}

HANDLE NTHREAD_API nosu_find_available_thread(ntid_t *thread_ids,
					      uint16_t thread_id_count)
{
	HANDLE threads[MAX_THREAD_COUNT];
	void *last_rips[MAX_THREAD_COUNT];
	uint16_t thread_count = 0;

	for (uint16_t i = 0; i < thread_id_count; i++) {
		ntid_t tid = thread_ids[i];
		HANDLE thread = OpenThread(NTHREAD_ACCESS, false, tid);
		if (thread != NULL) {
			threads[thread_count] = thread;
			last_rips[i] = 0;
			thread_count++;
		}
	}

	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_CONTROL;

	uint16_t re_thread_count = thread_count;
	HANDLE sel_thread = NULL;

	while (re_thread_count > 0) {
		for (uint16_t i = 0; i < thread_count; i++) {
			HANDLE thread = threads[i];
			if (thread == NULL)
				continue;

			if (!GetThreadContext(thread, &ctx)) {
				re_thread_count--;
				threads[i] = NULL;
				continue;
			}

			void *rip = (void *)ctx.Rip;
			void *last_rip = last_rips[i];
			if (last_rip != NULL && last_rip != rip) {
				sel_thread = thread;
				goto nosu_find_avaible_thread_exit;
			} else
				last_rips[i] = rip;
		}
	}

nosu_find_avaible_thread_exit:

	for (uint16_t i = 0; i < thread_count; i++) {
		HANDLE thread = threads[i];
		if (thread != sel_thread)
			CloseHandle(threads[i]);
	}

	return sel_thread;
}

uint16_t NTHREAD_API nosu_get_process_threads(ntid_t *thread_ids, DWORD pid)
{
	HANDLE thread_snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (thread_snap == INVALID_HANDLE_VALUE)
		return 0;

	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);

	uint32_t count = 0;
	if (Thread32First(thread_snap, &te32)) {
		do {
			if (te32.th32OwnerProcessID == pid) {
				thread_ids[count] = (ntid_t)te32.th32ThreadID;
				count++;

				if (count >= MAX_THREAD_COUNT)
					break;
			}
		} while (Thread32Next(thread_snap, &te32));
	}

	CloseHandle(thread_snap);
	return count;
}

nerror_t NTHREAD_API nosu_find_thread_and_upgrade(DWORD pid)
{
	ntid_t ids[MAX_THREAD_COUNT];
	uint16_t thread_count = nosu_get_process_threads(ids, pid);

	HANDLE thread = nosu_find_available_thread(ids, thread_count);
	if (thread == NULL)
		return GET_ERR(NTOSUTILS_WIN_FIND_AVAIBLE_THREAD_ERROR);

	return nosu_upgrade(thread);
}
