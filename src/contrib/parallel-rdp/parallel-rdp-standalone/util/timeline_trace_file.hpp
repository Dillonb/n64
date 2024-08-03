/* Copyright (c) 2017-2023 Hans-Kristian Arntzen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <queue>
#include "object_pool.hpp"

namespace Util
{
class TimelineTraceFile
{
public:
	explicit TimelineTraceFile(const std::string &path);
	~TimelineTraceFile();

	static void set_tid(const char *tid);
	static TimelineTraceFile *get_per_thread();
	static void set_per_thread(TimelineTraceFile *file);

	struct Event
	{
		char desc[256];
		char tid[32];
		uint32_t pid;
		uint64_t start_ns, end_ns;

		void set_desc(const char *desc);
		void set_tid(const char *tid);
	};
	Event *begin_event(const char *desc, uint32_t pid = 0);
	void end_event(Event *e);

	Event *allocate_event();
	void submit_event(Event *e);

	struct ScopedEvent
	{
		ScopedEvent(TimelineTraceFile *file, const char *tag, uint32_t pid = 0);
		ScopedEvent() = default;
		~ScopedEvent();
		void operator=(const ScopedEvent &) = delete;
		ScopedEvent(const ScopedEvent &) = delete;
		ScopedEvent(ScopedEvent &&other) noexcept;
		ScopedEvent &operator=(ScopedEvent &&other) noexcept;
		TimelineTraceFile *file = nullptr;
		Event *event = nullptr;
	};

private:
	void looper(std::string path);
	std::thread thr;
	std::mutex lock;
	std::condition_variable cond;

	ThreadSafeObjectPool<Event> event_pool;
	std::queue<Event *> queued_events;
};

#ifndef GRANITE_SHIPPING

#define GRANITE_MACRO_CONCAT_IMPL(a, b) a##b
#define GRANITE_MACRO_CONCAT(a, b) GRANITE_MACRO_CONCAT_IMPL(a, b)
#define GRANITE_SCOPED_TIMELINE_EVENT(str) \
	::Util::TimelineTraceFile::ScopedEvent GRANITE_MACRO_CONCAT(_timeline_scoped_count_, __COUNTER__){GRANITE_THREAD_GROUP() ? GRANITE_THREAD_GROUP()->get_timeline_trace_file() : nullptr, str}
#define GRANITE_SCOPED_TIMELINE_EVENT_FILE(file, str) \
	::Util::TimelineTraceFile::ScopedEvent GRANITE_MACRO_CONCAT(_timeline_scoped_count_, __COUNTER__){file, str}
#else
#define GRANITE_SCOPED_TIMELINE_EVENT(...) ((void)0)
#define GRANITE_SCOPED_TIMELINE_EVENT_FILE(...) ((void)0)
#endif
}
