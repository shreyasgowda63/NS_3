/*
 * Copyright (c) 2010 The Boeing Company
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Tom Goff <thomas.goff@boeing.com>
 * Modified by: Eduardo Nuno Almeida <enmsa@outlook.pt>
 *              Merged the files "unix-fd-reader.cc" and "win32-fd-reader.cc".
 */

#include "fd-reader.h"

#include "fatal-error.h"
#include "log.h"
#include "simulator.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <thread>

#ifdef __WIN32__

#include <BaseTsd.h>
#include <WinSock2.h>
using ssize_t = SSIZE_T;

#else

#include <sys/select.h>
#include <unistd.h> // close()

#endif

/**
 * @file
 * @ingroup system
 * ns3::FdReader implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FdReader");

/**
 * Write a buffer of data to a pipe.
 *
 * @param fd File descriptor of the pipe.
 * @param buf Buffer with the data to write to the pipe.
 * @param n Number of bytes to write.
 * @return Number of bytes written to the pipe.
 */
inline ssize_t
WritePipe(int fd, const void* buf, size_t n)
{
#ifdef __WIN32__
    return send(fd, (char*)buf, static_cast<int>(n), 0);
#else
    return write(fd, buf, n);
#endif
}

/**
 * Read data from a pipe to a buffer.
 *
 * @param fd File descriptor of the pipe.
 * @param buf Buffer where the data read will be saved.
 * @param n Number of bytes to read.
 * @return Number of bytes read from the pipe.
 */
inline ssize_t
ReadPipe(int fd, void* buf, size_t n)
{
#ifdef __WIN32__
    return recv(fd, (char*)buf, static_cast<int>(n), 0);
#else
    return read(fd, buf, n);
#endif
}

/**
 * Close pipe.
 *
 * @param fd File descriptor of the pipe.
 * @return 0 on success, -1 on error.
 */
inline int
ClosePipe(int fd)
{
#ifdef __WIN32__
    return closesocket(fd);
#else
    return close(fd);
#endif
}

#ifdef __WIN32__
bool FdReader::winsock_initialized = false;
#endif

FdReader::FdReader()
    : m_fd(-1),
      m_stop(false)
{
    NS_LOG_FUNCTION(this);
    m_evpipe[0] = -1;
    m_evpipe[1] = -1;
}

FdReader::~FdReader()
{
    NS_LOG_FUNCTION(this);
    Stop();
}

void
FdReader::Start(int fd, Callback<void, uint8_t*, ssize_t> readCallback)
{
    NS_LOG_FUNCTION(this << fd << &readCallback);

    NS_ASSERT_MSG(!m_readThread.joinable(), "read thread already exists");

    int tmp;

#ifdef __WIN32__
    if (!winsock_initialized)
    {
        WSADATA wsaData;
        tmp = WSAStartup(MAKEWORD(2, 2), &wsaData);
        NS_ASSERT_MSG(tmp != NO_ERROR, "Error at WSAStartup()");
        winsock_initialized = true;
    }

    // Create a pipe for inter-thread event notification
    m_evpipe[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    m_evpipe[1] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ((static_cast<uint64_t>(m_evpipe[0]) == INVALID_SOCKET) ||
        (static_cast<uint64_t>(m_evpipe[1]) == INVALID_SOCKET))
    {
        NS_FATAL_ERROR("pipe() failed: " << std::strerror(errno));
    }

    // Make the read end non-blocking
    ULONG iMode = 1;
    tmp = ioctlsocket(m_evpipe[0], FIONBIO, &iMode);
    if (tmp != NO_ERROR)
    {
        NS_FATAL_ERROR("fcntl() failed: " << std::strerror(errno));
    }

#else  // Not __WIN32__
    // Create a pipe for inter-thread event notification
    tmp = pipe(m_evpipe);
    if (tmp == -1)
    {
        NS_FATAL_ERROR("pipe() failed: " << std::strerror(errno));
    }

    // Make the read end non-blocking
    tmp = fcntl(m_evpipe[0], F_GETFL);
    if (tmp == -1)
    {
        NS_FATAL_ERROR("fcntl() failed: " << std::strerror(errno));
    }
    if (fcntl(m_evpipe[0], F_SETFL, tmp | O_NONBLOCK) == -1)
    {
        NS_FATAL_ERROR("fcntl() failed: " << std::strerror(errno));
    }
#endif // __WIN32__

    m_fd = fd;
    m_readCallback = readCallback;

    //
    // We're going to spin up a thread soon, so we need to make sure we have
    // a way to tear down that thread when the simulation stops.  Do this by
    // scheduling a "destroy time" method to make sure the thread exits before
    // proceeding.
    //
    if (!m_destroyEvent.IsPending())
    {
        // Hold a reference to ensure that this object is not
        // deallocated before the destroy-time event fires
        this->Ref();
        m_destroyEvent = Simulator::ScheduleDestroy(&FdReader::DestroyEvent, this);
    }

    //
    // Now spin up a thread to read from the fd
    //
    NS_LOG_LOGIC("Spinning up read thread");

    m_readThread = std::thread(&FdReader::Run, this);
}

void
FdReader::DestroyEvent()
{
    NS_LOG_FUNCTION(this);
    Stop();
    this->Unref();
}

void
FdReader::Stop()
{
    NS_LOG_FUNCTION(this);
    m_stop = true;

    // Signal the read thread
    if (m_evpipe[1] != -1)
    {
        char zero = 0;
        ssize_t len = WritePipe(m_evpipe[1], &zero, sizeof(zero));

        if (len != sizeof(zero))
        {
            NS_LOG_WARN("incomplete write(): " << std::strerror(errno));
        }
    }

    // Join the read thread
    if (m_readThread.joinable())
    {
        m_readThread.join();
    }

    // Close the write end of the event pipe
    if (m_evpipe[1] != -1)
    {
        ClosePipe(m_evpipe[1]);
        m_evpipe[1] = -1;
    }

    // Close the read end of the event pipe
    if (m_evpipe[0] != -1)
    {
        ClosePipe(m_evpipe[0]);
        m_evpipe[0] = -1;
    }

    // Reset everything else
    m_fd = -1;
    m_readCallback.Nullify();
    m_stop = false;
}

// This runs in a separate thread
void
FdReader::Run()
{
    NS_LOG_FUNCTION(this);

    fd_set rfds;
    int nfds = (m_fd > m_evpipe[0] ? m_fd : m_evpipe[0]) + 1;

    FD_ZERO(&rfds);
    FD_SET(m_fd, &rfds);
    FD_SET(m_evpipe[0], &rfds);

    for (;;)
    {
        fd_set readfds = rfds;
        int r = select(nfds, &readfds, nullptr, nullptr, nullptr);

        if (r == -1 && errno != EINTR)
        {
            NS_FATAL_ERROR("select() failed: " << std::strerror(errno));
        }

        if (FD_ISSET(m_evpipe[0], &readfds))
        {
            // Drain the event pipe
            for (;;)
            {
                char buf[1024];
                ssize_t len = ReadPipe(m_evpipe[0], buf, sizeof(buf));

                if (len == 0)
                {
                    NS_FATAL_ERROR("event pipe closed");
                }
                if (len < 0)
                {
                    if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
                    {
                        break;
                    }

                    NS_FATAL_ERROR("read() failed: " << std::strerror(errno));
                }
            }
        }

        if (m_stop)
        {
            // This thread is done
            break;
        }

        if (FD_ISSET(m_fd, &readfds))
        {
            FdReader::Data data = DoRead();

            // Reading stops when m_len is zero
            if (data.m_len == 0)
            {
                break;
            }

            // The callback is only called when m_len is positive
            // (data is ignored if m_len is negative)
            if (data.m_len > 0)
            {
                m_readCallback(data.m_buf, data.m_len);
            }
        }
    }
}

} // namespace ns3
