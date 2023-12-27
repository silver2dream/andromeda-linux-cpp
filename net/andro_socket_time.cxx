//
// Created by Hank Lin on 2023/12/26.
// Copyright (c) 2023 RunOn Entertainment. All rights reserved.
//

#include "andro_func.h"
#include "andro_global.h"
#include "andro_lockmutex.h"
#include "andro_memory.h"
#include "andro_socket.h"

void *CSocket::ServerTimerQueueMonitorThread(void *thread_data) {
  auto thread = static_cast<lp_thread_t>(thread_data);
  auto socket_ptr = thread->This;

  time_t absolute_time, cur_time;
  int err;

  while (G_STOP_EVENT == 0) {
	if (socket_ptr->timer_queue_map_size > 0) {

	  absolute_time = socket_ptr->timer_value;
	  cur_time = time(nullptr);
	  if (absolute_time < cur_time) {
		std::list<lp_message_header_t> idle_list;
		lp_message_header_t result;

		err = pthread_mutex_lock(&socket_ptr->timer_queue_mutex);
		if (err != 0) {
		  log_stderr(err,
					 "pthread_mutex_lock(&socket_ptr->timer_queue_mutex) failed in CSocekt::ServerTimerQueueMonitorThread(), error code:%d",
					 err);
		}

		while ((result = socket_ptr->get_over_time_timer(cur_time)) != nullptr) {
		  idle_list.push_back(result);
		}

		err = pthread_mutex_unlock(&socket_ptr->timer_queue_mutex);
		if (err != 0) {
		  log_stderr(err,
					 "pthread_mutex_unlock(&socket_ptr->timer_queue_mutex) failed in CSocekt::ServerTimerQueueMonitorThread(), error code:%d",
					 err);
		}

		lp_message_header_t msg_header_ptr;
		while (!idle_list.empty()) {
		  msg_header_ptr = idle_list.front();
		  idle_list.pop_front();
		  socket_ptr->HeartBeatTimeoutChecking(msg_header_ptr, cur_time);
		}
	  }
	}
	usleep(500 * 1000);
  }
  return nullptr;
}

void CSocket::push_to_timer_queue(lp_connection_t conn_ptr) {
  time_t future_time = time(nullptr);
  future_time += timeout_wait_time;

  CLock lock(&timer_queue_mutex);
  auto msg_header_ptr = (lp_message_header_t) CMemory::AllocMemory(ANDRO_MSG_HEADER_LEN, false);
  msg_header_ptr->conn_ptr = conn_ptr;
  msg_header_ptr->sequence = conn_ptr->sequence;
  timer_queue_map.insert(std::make_pair(future_time, msg_header_ptr));
  timer_queue_map_size++;
  timer_value = get_earliest_time();
}

lp_message_header_t CSocket::get_over_time_timer(time_t cur_time) {
  lp_message_header_t msg_header_ptr;

  if (timer_queue_map_size == 0 || timer_queue_map.empty()) {
	return nullptr;
  }

  time_t earliest_time = get_earliest_time();
  if (earliest_time <= cur_time) {
	msg_header_ptr = remove_first_timer();

	if (timeout_kick_enable != 1) {
	  time_t new_in_queue_time = cur_time + (timeout_wait_time);
	  auto new_msg_header_ptr = (lp_message_header_t) CMemory::AllocMemory(ANDRO_MSG_HEADER_LEN, false);
	  new_msg_header_ptr->conn_ptr = msg_header_ptr->conn_ptr;
	  new_msg_header_ptr->sequence = msg_header_ptr->sequence;
	  timer_queue_map.insert(std::make_pair(new_in_queue_time, new_msg_header_ptr));
	  timer_queue_map_size++;
	}

	if (timer_queue_map_size > 0) {
	  timer_value = get_earliest_time();
	}

	return msg_header_ptr;
  }

  return nullptr;
}

time_t CSocket::get_earliest_time() {
  std::multimap<time_t, lp_message_header_t>::iterator pos;
  pos = timer_queue_map.begin();
  return pos->first;
}

lp_message_header_t CSocket::remove_first_timer() {
  std::multimap<time_t, lp_message_header_t>::iterator pos;
  lp_message_header_t msg_header_ptr;

  if (timer_queue_map_size <= 0) {
	return nullptr;
  }

  pos = timer_queue_map.begin();
  msg_header_ptr = pos->second;
  timer_queue_map.erase(pos);
  --timer_queue_map_size;

  return msg_header_ptr;
}

void CSocket::delete_from_timer_queue(lp_connection_t conn_ptr) {
  std::multimap<time_t, lp_message_header_t>::iterator pos, pos_end;
  CLock lock(&timer_queue_mutex);

lblMTQM:
  pos = timer_queue_map.begin();
  pos_end = timer_queue_map.end();
  for (; pos != pos_end; ++pos) {
	if (pos->second->conn_ptr == conn_ptr) {
	  CMemory::FreeMemory(pos->second);
	  timer_queue_map.erase(pos);
	  --timer_queue_map_size;
	  goto lblMTQM;
	}
  }

  if (timer_queue_map_size > 0) {
	timer_value = get_earliest_time();
  }
}

void CSocket::clear_timer_queue() {
  std::multimap<time_t, lp_message_header_t>::iterator pos, pos_end;
  pos = timer_queue_map.begin();
  pos_end = timer_queue_map.end();
  for (; pos != pos_end; ++pos) {
	CMemory::FreeMemory(pos->second);
	--timer_queue_map_size;
  }
  timer_queue_map.clear();
}

void CSocket::HeartBeatTimeoutChecking(lp_message_header_t message_header_ptr, time_t cur_time) {
  CMemory::FreeMemory(message_header_ptr);
}

