#include <arpa/inet.h>
#include <cerrno>    //errno
#include <cstring>
#include <unistd.h>     //STDERR_FILENO

#include "andro_func.h"
#include "andro_global.h"
#include "andro_macro.h"
#include "andro_memory.h"
#include "andro_socket.h"

void CSocket::read_request_handler(lp_connection_t conn_ptr) {
  ssize_t result = recv_proc(conn_ptr, conn_ptr->packet_recv_buf_ptr, conn_ptr->packet_recv_len);
  log_stderr(errno, "result:%d", result);
  if (result <= 0) {
	return;
  }

  if (conn_ptr->packet_stat == ANDRO_PKG_STAT_HEAD_INIT) {
	if (result == ANDRO_PKG_HEADER_LEN) {
	  proc_header_handler(conn_ptr);
	} else {
	  conn_ptr->packet_stat = ANDRO_PKG_STAT_HEAD_RECVING;
	  conn_ptr->DataOffset(result);
	}
  } else if (conn_ptr->packet_stat == ANDRO_PKG_STAT_HEAD_RECVING) {
	if (conn_ptr->packet_recv_len == result) {
	  proc_header_handler(conn_ptr);
	} else {
	  conn_ptr->DataOffset(result);
	}
  } else if (conn_ptr->packet_stat == ANDRO_PKG_STAT_BODY_INIT) {
	if (result == conn_ptr->packet_recv_len) {
	  proc_data_handler(conn_ptr);
	} else {
	  conn_ptr->packet_stat = ANDRO_PKG_STAT_BODY_RECVING;
	  conn_ptr->DataOffset(result);
	}
  } else if (conn_ptr->packet_stat == ANDRO_PKG_STAT_BODY_RECVING) {
	if (result == conn_ptr->packet_recv_len) {
	  proc_data_handler(conn_ptr);
	} else {
	  conn_ptr->DataOffset(result);
	}
  }
}

ssize_t CSocket::recv_proc(lp_connection_t conn_ptr, char *buffer, ssize_t buf_len) {
  ssize_t n;

  n = recv(conn_ptr->fd, buffer, buf_len, 0);
  if (n == 0) {
	log_stderr(errno, "the connection is closed through a proper four-way handshake, fd=%d", conn_ptr->fd);
	if (close(conn_ptr->fd) == -1) {
	  log_error_core(ANDRO_LOG_ALERT, errno, "close(%d) failed in CSocket::recv_proc()", conn_ptr->fd);
	}
	push_to_recy_connect_queue(conn_ptr);
	return -1;
  }

  if (n < 0) {
	if (errno == EAGAIN || errno == EWOULDBLOCK) {
	  log_stderr(errno,
				 "the condition errno == EAGAIN || errno == EWOULDBLOCK is true in the CSocekt::recv_proc()");
	  return -1;
	}

	/*
		The occurrence of the `EINTR` error:
		When a process, which is blocked in a slow system call,
		captures a signal and the corresponding signal handler returns,
		the system call may return an `EINTR` error.

		For example, in a socket server, if a signal capture mechanism is set up and there are child processes,
		when the parent process is blocked in a slow system call and captures a valid signal,
		the kernel will cause `accept` to return an `EINTR` error (interrupted system call).
	*/
	if (errno == EINTR) {
	  log_stderr(errno, "the condition errno == EINTR is true in the CSocekt::recv_proc()");
	  return -1;
	}

	if (errno == ECONNRESET) {
	  // If the client abruptly closes the entire running program without properly closing the socket connection, it can result in the ECONNRESET error.
	  // Do nothing.
	} else {
	  log_stderr(errno, "ouccur error in the CSocekt::recv_proc()");
	}

	log_stderr(errno, " the connection is closed by the client in an abnormal manner, fd=%d", conn_ptr->fd);
	if (close(conn_ptr->fd) == -1) {
	  log_error_core(ANDRO_LOG_ALERT, errno, "close(%d) failed in CSocket::recv_proc()", conn_ptr->fd);
	}
	push_to_recy_connect_queue(conn_ptr);
	return -1;
  }

  return n;
}

void CSocket::proc_header_handler(lp_connection_t conn_ptr) {
  lp_packet_header_t pkg_header_ptr;
  pkg_header_ptr = (lp_packet_header_t) (conn_ptr->packet_head_info);

  unsigned short pkg_len;
  pkg_len = ntohs(pkg_header_ptr->pkg_len);

  if (pkg_len < ANDRO_PKG_HEADER_LEN) {
	conn_ptr->ResetRecv();
  } else if (pkg_len > (ANDRO_PKG_MAX_LEN - 1000)) {
	conn_ptr->ResetRecv();
  } else {
	char *tmp_buff_ptr = (char *) CMemory::AllocMemory(ANDRO_MSG_HEADER_LEN + pkg_len, false);
	conn_ptr->allocated_packet_recv_mem_ptr = tmp_buff_ptr;

	auto msg_header_ptr = (lp_message_header_t) tmp_buff_ptr;
	msg_header_ptr->conn_ptr = conn_ptr;
	msg_header_ptr->sequence = conn_ptr->sequence;

	tmp_buff_ptr += ANDRO_MSG_HEADER_LEN;
	memcpy(tmp_buff_ptr, pkg_header_ptr, ANDRO_PKG_HEADER_LEN);
	if (pkg_len == ANDRO_PKG_HEADER_LEN) {
	  proc_data_handler(conn_ptr);
	} else {
	  conn_ptr->packet_stat = ANDRO_PKG_STAT_BODY_INIT;
	  conn_ptr->packet_recv_buf_ptr = tmp_buff_ptr + ANDRO_PKG_HEADER_LEN;
	  conn_ptr->packet_recv_len = pkg_len - ANDRO_PKG_HEADER_LEN;
	}
  }

}

void CSocket::proc_data_handler(lp_connection_t conn_ptr) {
  G_THREAD_POOL.PushToMsgQueueAndAwake(conn_ptr->allocated_packet_recv_mem_ptr);

  conn_ptr->allocated_packet_recv_mem_ptr = nullptr;
  conn_ptr->ResetRecv();
}

void CSocket::write_request_handler(lp_connection_t conn_ptr) {
  ssize_t send_size = send_proc(conn_ptr, conn_ptr->packet_send_buf_ptr, conn_ptr->packet_send_len);

  if (send_size > 0 && send_size != conn_ptr->packet_send_len) {
	conn_ptr->packet_send_buf_ptr = conn_ptr->packet_send_buf_ptr + send_size;
	conn_ptr->packet_send_len = conn_ptr->packet_send_len - send_size;
	return;
  } else if (send_size == -1) {
	log_stderr(errno, "send_size == -1 in CSocket::write_request_handler");
	return;
  }

  if (send_size > 0 && send_size == conn_ptr->packet_send_len) {
	if (EpollOperateEvent(conn_ptr->fd, EPOLL_CTL_MOD, EPOLLOUT, 1, conn_ptr) == -1) {
	  log_stderr(errno, "EpollOperateEvent() failed in CSocket::write_request_handler");
	}
  }

  if (sem_post(&sem_event_send_queue) == -1) {
	log_stderr(0, "sem_post(&sem_event_send_queue) failed in CSocket::write_request_handler");
  }

  CMemory::FreeMemory(conn_ptr->allocated_packet_send_mem_ptr);
  conn_ptr->allocated_packet_send_mem_ptr = nullptr;
  --conn_ptr->throw_send_count;
}

ssize_t CSocket::send_proc(lp_connection_t conn_ptr, char *buffer, ssize_t size) {
  ssize_t n;

  for (;;) {
	n = send(conn_ptr->fd, buffer, size, 0);
	if (n > 0) {
	  return n;
	}

	if (n == 0) {
	  return 0;
	}

	if (errno == EAGAIN) {
	  // Core buffer was full.
	  return -1;
	}

	if (errno == EINTR) {
	  log_stderr(errno, "send() failed in send_proc()");
	} else {
	  return -2;
	}
  }
}

void CSocket::ThreadRecvProcFunc(char *msg_buffer) {
}