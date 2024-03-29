
#include "andro_socket.h"

#include <arpa/inet.h>
#include <cerrno>//errno
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>      //localtime_r
#include <pthread.h>  //
#include <sys/ioctl.h>//ioctl
#include <sys/time.h> //gettimeofday
#include <unistd.h>   //STDERR_FILENO

#include <string>

#include "andro_conf.h"
#include "andro_func.h"
#include "andro_global.h"
#include "andro_lockmutex.h"
#include "andro_macro.h"
#include "andro_memory.h"

CSocket::CSocket() {
  worker_max_connections = 1;
  listen_port_count = 1;
  recy_connection_wait_time = 60;

  epoll_handler = -1;

  send_msg_queue_size = 0;
  total_recy_connection_size = 0;
  timer_queue_map_size = 0;
  timer_value = 0;
  discard_send_pkg_count = 0;

  online_user_count = 0;

  last_print_time = 0;
}

bool CSocket::Init() {
  read_conf();
  bool result = open_listening_sockets();
  return result;
}

bool CSocket::InitSubProc() {
  if (pthread_mutex_init(&send_msg_queue_mutex, nullptr) != 0) {
	log_stderr(0, "pthread_mutex_init(&send_msg_queue_mutex) failed in CSocket::InitSubProc");
	return false;
  }

  if (pthread_mutex_init(&connection_mutex, nullptr) != 0) {
	log_stderr(0, "pthread_mutex_init(&connection_mutex) failed in CSocket::InitSubProc");
	return false;
  }

  if (pthread_mutex_init(&recy_conn_queue_mutex, nullptr) != 0) {
	log_stderr(0, "pthread_mutex_init(&recy_conn_queue_mutex) failed in CSocket::InitSubProc");
	return false;
  }

  if (pthread_mutex_init(&timer_queue_mutex, nullptr) != 0) {
	log_stderr(0, "pthread_mutex_init(&timer_queue_mutex) failed in CSocket::InitSubProc");
	return false;
  }

  // Initialize the semaphore for sending messages. Semaphores are used for synchronization between processes/threads.
  // Although mutexes [pthread_mutex_lock] and condition variables [pthread_cond_wait] are also means of synchronization between threads,
  // using a semaphore here makes it easier to understand and simplifies the problem. The code written is shorter and clearer;
  // The second parameter = 0, indicating that the semaphore is shared between threads, which is indeed the case. If it is non-zero, it means the semaphore is shared between processes.
  // The third parameter = 0, which is the initial value of the semaphore. When it is 0, calling sem_wait() will cause it to block right there.
  if (sem_init(&sem_event_send_queue, 0, 0) == -1) {
	log_stderr(0, "sem_init(&sem_event_send_queue, 0, 0) failed in CSocket::InitSubProc");
	return false;
  }

  int err;
  lp_thread_t send_queue_ptr;
  thread_container.push_back(send_queue_ptr = new thread_t(this));
  err = pthread_create(&send_queue_ptr->handle, nullptr, ServerSendQueueThread, send_queue_ptr);
  if (err != 0) {
	log_stderr(0,
			   "pthread_create(&send_queue_ptr->handle, nullptr, ServerSendQueueThread, send_queue_ptr) failed in CSocket::InitSubProc");
	return false;
  }

  lp_thread_t recy_conn_ptr;
  thread_container.push_back(recy_conn_ptr = new thread_t(this));
  err = pthread_create(&recy_conn_ptr->handle, nullptr, ServerRecyConnectionThread, recy_conn_ptr);
  if (err != 0) {
	log_stderr(0,
			   "pthread_create(&recy_conn_ptr->handle, nullptr, ServerRecyConnectionThread, recy_conn_ptr) failed in CSocket::InitSubProc");
	return false;
  }

  if (kick_timer_enable == 1) {
	lp_thread_t heartbeat_monitor;
	thread_container.push_back(heartbeat_monitor = new thread_t(this));
	err = pthread_create(&heartbeat_monitor->handle, nullptr, ServerTimerQueueMonitorThread, heartbeat_monitor);
	if (err != 0) {
	  log_stderr(0,
				 "pthread_create(heartbeat_monitor->handle, nullptr, ServerTimerQueueMonitorThread, heartbeat_monitor) failed in CSocket::InitSubProc");
	  return false;
	}
  }

  return true;
}

CSocket::~CSocket() {
  std::vector<lp_listening_t>::iterator pos;
  for (pos = listen_socket_list.begin(); pos != listen_socket_list.end(); ++pos) {
	delete (*pos);
  }
  listen_socket_list.clear();
}

void CSocket::ShutdownSubProc() {
  if (sem_post(&sem_event_send_queue) == -1) {
	log_stderr(0, "sem_post(&sem_event_send_queue) failed in CSocket::InitSubProc");
  }

  std::vector<lp_thread_t>::iterator iter;
  for (iter = thread_container.begin(); iter != thread_container.end(); iter++) {
	pthread_join((*iter)->handle, nullptr);
  }

  for (iter = thread_container.begin(); iter != thread_container.end(); iter++) {
	if (*iter)
	  delete *iter;
  }
  thread_container.clear();

  clear_msg_send_queue();
  clear_connection_pool();
  clear_timer_queue();

  pthread_mutex_destroy(&connection_mutex);
  pthread_mutex_destroy(&send_msg_queue_mutex);
  pthread_mutex_destroy(&recy_conn_queue_mutex);
  pthread_mutex_destroy(&timer_queue_mutex);
  sem_destroy(&sem_event_send_queue);
}

void CSocket::clear_msg_send_queue() {
  char *tmp_ptr;

  while (!send_msg_queue.empty()) {
	tmp_ptr = send_msg_queue.front();
	send_msg_queue.pop_front();
	CMemory::FreeMemory(tmp_ptr);
  }
}

void CSocket::read_conf() {
  CConfig *config = CConfig::GetInstance();
  worker_max_connections = config->GetIntDefault(ANDRO_CONF_WORK_MAX_CONNECTIONS, worker_max_connections);
  listen_port_count = config->GetIntDefault(ANDRO_CONF_LISTEN_PORT_COUNT, listen_port_count);

  recy_connection_wait_time = config->GetIntDefault(ANDRO_CONF_SOCK_RECY_WAIT_TIME, recy_connection_wait_time);
  kick_timer_enable = config->GetIntDefault(ANDRO_CONF_SOCK_KICK_TIMER_ENABLE, kick_timer_enable);
  timeout_kick_enable = config->GetIntDefault(ANDRO_CONF_SOCK_TIMEOUT_KICK_ENABLE, timeout_kick_enable);
  timeout_wait_time = config->GetIntDefault(ANDRO_CONF_SOCK_MAX_WAIT_TIME, ANDRO_CONF_SOCK_MAX_WAIT_TIME_DEFAULT_VALUE);

  flood_attack_detection_enable =
	  config->GetIntDefault(ANDRO_CONF_SECURITY_FLOOD_ATTACK_DETECTION_ENABLE, flood_attack_detection_enable);
  flood_time_interval = config->GetIntDefault(ANDRO_CONF_SECURITY_FLOOD_TIME_INTERVAL, flood_time_interval);
  flood_kick_count = config->GetIntDefault(ANDRO_CONF_SECURITY_FLOOD_KICK_COUNTER, flood_kick_count);
}

bool CSocket::open_listening_sockets() {
  int socket_fd;
  struct sockaddr_in serv_addr{};
  int port;

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  CConfig *config = CConfig::GetInstance();
  for (int i = 0; i < listen_port_count; i++) {
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
	  log_stderr(errno, "socket() occur failed in CSocket::Init(), i=%d", i);
	}

	int reuse_addr = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuse_addr, sizeof(reuse_addr)) == -1) {
	  log_stderr(errno, "setsockopt(SO_REUSEADDR) failed in in CSocket::Init(), i=%d", i);
	  close(socket_fd);
	  return false;
	}

	if (!set_nonblocking(socket_fd)) {
	  log_stderr(errno, "set_nonblocking failed in in CSocket::Init(), i=%d", i);
	  close(socket_fd);
	  return false;
	}

	auto port_str = ANDRO_CONF_PORT_PRIFIX + std::to_string(i);
	port = config->GetIntDefault(port_str.c_str(), 10000);
	serv_addr.sin_port = htons(in_port_t(port));

	if (bind(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
	  log_stderr(errno, "bind() failed in in CSocket::Init(), i=%d", i);
	  close(socket_fd);
	  return false;
	}

	if (listen(socket_fd, ANDRO_LISTEN_BACKLOG) == -1) {
	  log_stderr(errno, "listen() failed in in CSocket::Init(), i=%d", i);
	  close(socket_fd);
	  return false;
	}

	auto listen_socket_item = new listening_t;
	memset(listen_socket_item, 0, sizeof(listening_t));
	listen_socket_item->fd = socket_fd;
	listen_socket_item->port = port;

	log_error_core(ANDRO_LOG_INFO, 0, "listen() successful, port=%d", port);
	listen_socket_list.push_back(listen_socket_item);
  }

  if (listen_socket_list.empty()) return false;

  return true;
}

bool CSocket::set_nonblocking(int socket_fd) {
  // 0: clean, 1: setup
  int non_blocking = 1;
  if (ioctl(socket_fd, FIONBIO, &non_blocking) == -1) {
	return false;
  }
  return true;
}

void CSocket::close_listening_sockets() {
  for (int i = 0; i < listen_port_count; i++) {
	close(listen_socket_list[i]->fd);
	log_error_core(ANDRO_LOG_INFO, 0, "close listen port[%d]", listen_socket_list[i]->port);
  }
}

void CSocket::msg_send(char *send_buffer) {
  CLock lock(&send_msg_queue_mutex);

  if (send_msg_queue_size > ANDRO_CONF_METRIC_MAX_SEND_MSG_QUEUE_SIZE_DEFAULT_VALUE) {
	discard_send_pkg_count++;
	CMemory::FreeMemory(send_buffer);
	return;
  }

  auto msg_header_ptr = (lp_message_header_t) send_buffer;
  lp_connection_t conn_ptr = msg_header_ptr->conn_ptr;
  if (conn_ptr->send_count > ANDRO_CONF_METRIC_MAX_CONNECTION_SEND_COUNT_DEFAULT_VALUE) {
	log_stderr(0,
			   "in CSocekt::msg_send(), it was found that user[%d] has accumulated a large number of packets pending to be sent, hence the connection with him was severed");

	discard_send_pkg_count++;
	CMemory::FreeMemory(send_buffer);
	kick(conn_ptr);
	return;
  }

  ++conn_ptr->send_count;
  send_msg_queue.push_back(send_buffer);
  ++send_msg_queue_size;

  if (sem_post(&sem_event_send_queue) == -1) {
	log_stderr(0, "sem_post(&sem_event_send_queue) failed in CSocket::msg_send");
  }
}

void CSocket::kick(lp_connection_t conn_ptr) {
  if (kick_timer_enable == 1) {
	delete_from_timer_queue(conn_ptr);
  }
  if (conn_ptr->fd != -1) {
	close(conn_ptr->fd);
	conn_ptr->fd = -1;
  }

  if (conn_ptr->throw_send_count > 0) {
	--conn_ptr->throw_send_count;
  }

  push_to_recy_connect_queue(conn_ptr);
}

bool CSocket::detect_flood(lp_connection_t conn_ptr) {
  struct timeval curr_time_data{};
  uint64_t curr_time;
  bool result = false;

  gettimeofday(&curr_time_data, nullptr);
  curr_time = (curr_time_data.tv_sec * 1000 + curr_time_data.tv_usec / 1000); //ms
  if ((curr_time - conn_ptr->flood_kick_last_time) < flood_time_interval) {
	conn_ptr->flood_attack_count++;
  } else {
	conn_ptr->flood_attack_count = 0;
  }
  conn_ptr->flood_kick_last_time = curr_time;

  if (conn_ptr->flood_kick_last_time >= flood_kick_count) {
	result = true;
  }

  return result;
}

void CSocket::PrintThreadInfo() {
  auto curr_time = time(nullptr);
  if (curr_time - last_print_time > ANDRO_CONF_METRIC_TIME_DEFAULT_VALUE) {
	auto recv_msg_queue_size = G_THREAD_POOL.GetMsgQueueSize();

	last_print_time = curr_time;
	int duplicate_online_user_count = online_user_count;
	int duplicate_send_msg_queue_size = send_msg_queue_size;
	log_stderr(0, "------------------------------------begin--------------------------------------");
	log_stderr(0, "online_user/max(%d/%d)", duplicate_online_user_count, worker_max_connections);
	log_stderr(0,
			   "free_connections/total_connections/recycle_connections(%d/%d/%d)",
			   free_connection_pool.size(),
			   connection_pool.size(),
			   recy_connection_pool.size());
	log_stderr(0, "timer_queue_size(%d)", timer_queue_map.size());
	log_stderr(0,
			   "recv_msg_queue_size/send_msg_queue_size(%d/%d), discard_send_pkg_count",
			   duplicate_online_user_count,
			   worker_max_connections,
			   discard_send_pkg_count);
	if (recv_msg_queue_size > ANDRO_CONF_METRIC_MAX_RECV_MSG_QUEUE_SIZE_DEFAULT_VALUE) {
	  log_stderr(0,
				 "the number of entries in the receive queue is too large (%d), consider rate limiting or increasing the number of processing threads",
				 recv_msg_queue_size);
	}
	log_stderr(0, "-------------------------------------end---------------------------------------");
  }
}

int CSocket::EpollInit() {
  epoll_handler = epoll_create(worker_max_connections);
  if (epoll_handler == -1) {
	log_stderr(errno, "epoll_create() failed in EpollInit()");
	exit(2);
  }

  init_connection_pool();

  std::vector<lp_listening_t>::iterator pos;
  for (pos = listen_socket_list.begin(); pos != listen_socket_list.end(); ++pos) {
	lp_connection_t conn = get_connection((*pos)->fd);
	if (conn == nullptr) {
	  log_stderr(errno, "get_free_connection() failed in EpollInit()");
	  exit(2);
	}

	conn->listening = (*pos);
	(*pos)->connection = conn;

	conn->rhandler = &CSocket::event_accept;

	if (EpollOperateEvent((*pos)->fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP, 0, conn) == -1) {
	  exit(2);
	}
  }

  return 1;
}

int CSocket::EpollOperateEvent(int fd,
							   uint32_t event_type,
							   uint32_t other_flag,
							   int bcation,
							   lp_connection_t conn_ptr) const {
  struct epoll_event ev{};
  memset(&ev, 0, sizeof(ev));

  if (event_type == EPOLL_CTL_ADD) {
	ev.data.ptr = (void *) conn_ptr;
	ev.events = other_flag;
	conn_ptr->events = other_flag;
  } else if (event_type == EPOLL_CTL_MOD) {
	// Do nothing.
  } else {
	// Do nothing.
	return 1;
  }

  if (epoll_ctl(epoll_handler, event_type, fd, &ev) == -1) {
	log_stderr(errno, "epoll_ctl(%d,%ud,%ud,%d) failed in EpollOperateEvent()", fd, event_type, other_flag, bcation);
	return -1;
  }
  return 1;
}

int CSocket::EpollProcessEvents(int timer) {
  int events = epoll_wait(epoll_handler, epoll_events, ANDRO_MAX_EPOLL_WAIT_EVENTS, timer);
  if (events == -1) {
	if (errno == EINTR) {
	  log_error_core(ANDRO_LOG_INFO, errno, "epoll_wait() failed in CSocket::EpollProcessEvents()");
	  return 1;
	} else {
	  log_error_core(ANDRO_LOG_ALERT, errno, "epoll_wait() failed in CSocket::EpollProcessEvents()");
	  return 0;
	}
  }

  if (events == 0) {
	if (timer != -1) {
	  return 1;
	}
	log_error_core(ANDRO_LOG_ALERT,
				   0,
				   "epoll_wait() is not timeout and have no events return, in CSocket::EpollProcessEvents()");
	return 0;
  }

  lp_connection_t conn;
  uint32_t revents;
  for (int i = 0; i < events; ++i) {
	conn = (lp_connection_t) (epoll_events[i].data.ptr);
	revents = epoll_events[i].events;

	if (revents & EPOLLIN) {
	  (this->*(conn->rhandler))(conn);
	}

	if (revents & EPOLLOUT) {
	  if (revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		--conn->throw_send_count;
	  } else {
		(this->*(conn->whandler))(conn);
	  }
	}
  }
  return 1;
}

void *CSocket::ServerSendQueueThread(void *thread_data) {
  auto thread = static_cast<lp_thread_t>(thread_data);
  CSocket *socket_ptr = thread->This;
  int err;
  std::list<char *>::iterator pos, pos2, posend;

  char *msg_buffer;
  lp_message_header_t msg_header_ptr;
  lp_packet_header_t pkg_header_ptr;
  lp_connection_t conn_ptr;
  unsigned short tmp;
  ssize_t send_size;

  while (G_STOP_EVENT == 0) {
	if (sem_wait(&socket_ptr->sem_event_send_queue) == -1) {
	  if (errno != EINTR) {
		log_stderr(errno, "sem_wait(&socket_ptr->sem_event_send_queue failed in CSocket::ServerSendQueueThread");
	  }
	}

	if (G_STOP_EVENT != 0) {
	  break;
	}

	if (socket_ptr->send_msg_queue_size > 0) {
	  err = pthread_mutex_lock(&socket_ptr->send_msg_queue_mutex);
	  if (err != 0) {
		log_stderr(err,
				   "pthread_mutex_lock(&socket_ptr->send_msg_queue_mutex) failed in CSocket::ServerSendQueueThread, err=%d",
				   err);
	  }

	  pos = socket_ptr->send_msg_queue.begin();
	  posend = socket_ptr->send_msg_queue.end();

	  while (pos != posend) {
		msg_buffer = (*pos);
		msg_header_ptr = (lp_message_header_t) msg_buffer;
		pkg_header_ptr = (lp_packet_header_t) (msg_buffer + ANDRO_MSG_HEADER_LEN);
		conn_ptr = msg_header_ptr->conn_ptr;

		if (conn_ptr->sequence != msg_header_ptr->sequence) {
		  pos2 = pos;
		  pos++;
		  socket_ptr->send_msg_queue.erase(pos2);
		  --socket_ptr->send_msg_queue_size;
		  CMemory::FreeMemory(msg_buffer);
		  continue;
		}

		if (conn_ptr->throw_send_count > 0) {
		  pos++;
		  continue;
		}

		--conn_ptr->send_count;

		conn_ptr->allocated_packet_send_mem_ptr = msg_buffer;
		pos2 = pos;
		pos++;
		socket_ptr->send_msg_queue.erase(pos2);
		--socket_ptr->send_msg_queue_size;
		conn_ptr->packet_send_buf_ptr = (char *) pkg_header_ptr;
		tmp = ntohs(pkg_header_ptr->pkg_len);
		conn_ptr->packet_send_len = tmp;

		//log_stderr(errno, "about to send data %ud", conn_ptr->packet_send_len);

		send_size = socket_ptr->send_proc(conn_ptr, conn_ptr->packet_send_buf_ptr, conn_ptr->packet_send_len);
		if (send_size > 0) {
		  if (send_size == conn_ptr->packet_send_len) {
			CMemory::FreeMemory(conn_ptr->allocated_packet_send_mem_ptr);
			conn_ptr->allocated_packet_send_mem_ptr = nullptr;
			conn_ptr->throw_send_count = 0;
			//log_stderr(0, "sended data finish in CSocket::ServerSendQueueThread");
		  } else {
			conn_ptr->packet_send_buf_ptr = conn_ptr->packet_send_buf_ptr + send_size;
			conn_ptr->packet_send_len = conn_ptr->packet_send_len - send_size;
			++conn_ptr->throw_send_count;
			if (socket_ptr->EpollOperateEvent(conn_ptr->fd, EPOLL_CTL_MOD, EPOLLOUT, 0, conn_ptr) == -1) {
			  log_stderr(errno, "EpollOperateEvent() failed in CSocket::ServerSendQueueThread");
			}
			//log_stderr(errno, "send's buffer was full, part[%d] of total[%d]", send_size, conn_ptr->packet_send_len);
		  }
		  continue;
		} else if (send_size == 0) {
		  CMemory::FreeMemory(conn_ptr->allocated_packet_send_mem_ptr);
		  conn_ptr->allocated_packet_send_mem_ptr = nullptr;
		  conn_ptr->throw_send_count = 0;
		  continue;
		} else if (send_size == -1) {
		  ++conn_ptr->throw_send_count;
		  if (socket_ptr->EpollOperateEvent(conn_ptr->fd, EPOLL_CTL_MOD, EPOLLOUT, 0, conn_ptr) == -1) {
			log_stderr(errno, "EpollOperateEvent() failed in CSocket::ServerSendQueueThread");
		  }
		  continue;
		} else {
		  CMemory::FreeMemory(conn_ptr->allocated_packet_send_mem_ptr);
		  conn_ptr->allocated_packet_send_mem_ptr = nullptr;
		  conn_ptr->throw_send_count = 0;
		}
	  }

	  err = pthread_mutex_unlock(&socket_ptr->send_msg_queue_mutex);
	  if (err != 0) {
		log_stderr(err,
				   "pthread_mutex_unlock(&socket_ptr->send_msg_queue_mutex) failed in failed in CSocket::ServerSendQueueThread, err=%d",
				   err);
	  }
	}
  }

  return (void *) nullptr;
}






