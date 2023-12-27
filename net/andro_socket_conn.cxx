#include <cerrno>  //errno
#include <ctime>   //localtime_r
#include <unistd.h>//STDERR_FILENO

#include "andro_func.h"
#include "andro_global.h"
#include "andro_lockmutex.h"
#include "andro_macro.h"
#include "andro_memory.h"
#include "andro_socket.h"

andro_connection_s::andro_connection_s() {
  sequence = 0;
  pthread_mutex_init(&logic_proc_mutex, nullptr);
}

andro_connection_s::~andro_connection_s() {
  pthread_mutex_destroy(&logic_proc_mutex);
}

void andro_connection_s::GetOneToUse() {
  ++sequence;
  packet_stat = ANDRO_PKG_STAT_HEAD_INIT;
  packet_recv_buf_ptr = packet_head_info;
  packet_recv_len = ANDRO_PKG_HEADER_LEN;

  allocated_packet_recv_mem_ptr = nullptr;
  throw_send_count = 0;
  allocated_packet_send_mem_ptr = nullptr;
  events = 0;
}

void andro_connection_s::PutOneToFree() {
  ++sequence;
  if (allocated_packet_recv_mem_ptr != nullptr) {
	CMemory::FreeMemory(allocated_packet_recv_mem_ptr);
	allocated_packet_recv_mem_ptr = nullptr;
  }

  if (allocated_packet_send_mem_ptr != nullptr) {
	CMemory::FreeMemory(allocated_packet_send_mem_ptr);
	allocated_packet_send_mem_ptr = nullptr;
  }

  throw_send_count = 0;
}

void CSocket::init_connection_pool() {
  lp_connection_t conn_ptr;

  int conn_pool_size = sizeof(connection_t);
  for (int i = 0; i < worker_max_connections; ++i) {
	conn_ptr = (lp_connection_t) CMemory::AllocMemory(conn_pool_size, true);
	conn_ptr = new(conn_ptr) connection_t();
	conn_ptr->GetOneToUse();
	connection_pool.push_back(conn_ptr);
	free_connection_pool.push_back(conn_ptr);
  }

  free_connection_size = total_connection_size = connection_pool.size();
}

void CSocket::clear_connection_pool() {
  lp_connection_t conn_ptr;

  while (!connection_pool.empty()) {
	conn_ptr = connection_pool.front();
	connection_pool.pop_front();
	conn_ptr->~andro_connection_s();
	CMemory::FreeMemory(conn_ptr);
  }
}

lp_connection_t CSocket::get_connection(int socket_fd) {
  CLock lock(&connection_mutex);

  if (!free_connection_pool.empty()) {
	lp_connection_t conn_ptr = free_connection_pool.front();
	free_connection_pool.pop_front();
	conn_ptr->GetOneToUse();
	--free_connection_size;
	conn_ptr->fd = socket_fd;
	return conn_ptr;
  }

  auto conn_ptr = (lp_connection_t) CMemory::AllocMemory(sizeof(connection_t), true);
  conn_ptr = new(conn_ptr) connection_t();
  conn_ptr->GetOneToUse();
  connection_pool.push_back(conn_ptr);
  ++total_connection_size;
  conn_ptr->fd = socket_fd;
  return conn_ptr;
}

void CSocket::free_connection(lp_connection_t conn_ptr) {
  CLock lock(&connection_mutex);

  conn_ptr->PutOneToFree();

  free_connection_pool.push_back(conn_ptr);
  ++free_connection_size;
}

void CSocket::push_to_recy_connect_queue(lp_connection_t conn_ptr) {
  std::list<lp_connection_t>::iterator pos;
  bool is_found = false;

  CLock lock(&recy_conn_queue_mutex);

  for (pos = recy_connection_pool.begin(); pos != recy_connection_pool.end(); ++pos) {
	if ((*pos) == conn_ptr) {
	  is_found = true;
	  break;
	}
  }

  if (is_found) {
	return;
  }

  conn_ptr->recy_time = time(nullptr);
  ++conn_ptr->sequence;
  recy_connection_pool.push_back(conn_ptr);
  ++total_recy_connection_size;
  --online_user_count;
}

void *CSocket::ServerRecyConnectionThread(void *thread_data) {
  auto thread = static_cast<lp_thread_t>(thread_data);
  CSocket *socket_ptr = thread->This;

  time_t curr_time;
  int err;
  std::list<lp_connection_t>::iterator pos, posend;
  lp_connection_t conn_ptr;

  while (true) {
	usleep(200 * 1000);// 200ms

	if (socket_ptr->total_recy_connection_size > 0) {
	  curr_time = time(nullptr);
	  err = pthread_mutex_lock(&socket_ptr->recy_conn_queue_mutex);
	  if (err != 0) {
		log_stderr(err,
				   "pthread_mutex_lock(&socket_ptr->recy_conn_queue_mutex) failed in CSocket::ServerRecyConnectionThread, err=%d",
				   err);
	  }

lblRRTD:
	  pos = socket_ptr->recy_connection_pool.begin();
	  posend = socket_ptr->recy_connection_pool.end();
	  for (; pos != posend; ++pos) {
		conn_ptr = *pos;
		if (((conn_ptr->recy_time + socket_ptr->recy_connection_wait_time) > curr_time)
			&& (G_STOP_EVENT == 0)) {
		  continue;
		}

		if(conn_ptr->throw_send_count>0){
		  log_stderr(0, "throw_send_count of connection[%d] was not zero that should not be happening, in CSocket::ServerRecyConnectionThread()", conn_ptr->fd);
		}

		--socket_ptr->total_recy_connection_size;
		socket_ptr->recy_connection_pool.erase(pos);

		log_stderr(0, "connection[%d] has been returned in CSocket::ServerRecyConnectionThread()", conn_ptr->fd);

		socket_ptr->free_connection(conn_ptr);
		goto lblRRTD;
	  }
	  err = pthread_mutex_unlock(&socket_ptr->recy_conn_queue_mutex);
	  if (err != 0) {
		log_stderr(err,
				   "pthread_mutex_unlock(&socket_ptr->recy_conn_queue_mutex) failed in CSocket::ServerRecyConnectionThread, err=%d",
				   err);
	  }
	}

	if (G_STOP_EVENT == 1) {
	  if (socket_ptr->total_recy_connection_size > 0) {
		err = pthread_mutex_lock(&socket_ptr->recy_conn_queue_mutex);
		if (err != 0) {
		  log_stderr(err,
					 "pthread_mutex_lock(&socket_ptr->recy_conn_queue_mutex) failed in CSocket::ServerRecyConnectionThread, err=%d",
					 err);
		}

lblRRTD2:
		pos = socket_ptr->recy_connection_pool.begin();
		posend = socket_ptr->recy_connection_pool.end();
		for (; pos != posend; ++pos) {
		  conn_ptr = *pos;
		  --socket_ptr->total_recy_connection_size;
		  socket_ptr->recy_connection_pool.erase(pos);
		  socket_ptr->free_connection(conn_ptr);
		  goto lblRRTD2;
		}
		err = pthread_mutex_unlock(&socket_ptr->recy_conn_queue_mutex);
		if (err != 0) {
		  log_stderr(err,
					 "pthread_mutex_unlock(&socket_ptr->recy_conn_queue_mutex) failed in CSocket::ServerRecyConnectionThread, err=%d",
					 err);
		}
	  }
	  break;
	}
  }

  return (void *) nullptr;
}

void CSocket::close_connection(lp_connection_t conn_ptr) {
  free_connection(conn_ptr);
  if (close(conn_ptr->fd) == -1) {
	log_error_core(ANDRO_LOG_ALERT, errno, "close(%d) failed in CSocket::close_connection", conn_ptr->fd);
  }
}