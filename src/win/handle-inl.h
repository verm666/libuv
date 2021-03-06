/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef UV_WIN_HANDLE_INL_H_
#define UV_WIN_HANDLE_INL_H_

#include <assert.h>

#include "uv.h"
#include "internal.h"


#define DECREASE_ACTIVE_COUNT(loop, handle)                             \
  do {                                                                  \
    if (--(handle)->activecnt == 0 &&                                   \
        !((handle)->flags & UV_HANDLE_CLOSING)) {                       \
      uv__handle_stop((handle));                                        \
    }                                                                   \
    assert((handle)->activecnt >= 0);                                   \
  } while (0)


#define INCREASE_ACTIVE_COUNT(loop, handle)                             \
  do {                                                                  \
    if ((handle)->activecnt++ == 0) {                                   \
      uv__handle_start((handle));                                       \
    }                                                                   \
    assert((handle)->activecnt > 0);                                    \
  } while (0)


#define DECREASE_PENDING_REQ_COUNT(handle)                              \
  do {                                                                  \
    assert(handle->reqs_pending > 0);                                   \
    handle->reqs_pending--;                                             \
                                                                        \
    if (handle->flags & UV_HANDLE_CLOSING &&                            \
        handle->reqs_pending == 0) {                                    \
      uv_want_endgame(loop, (uv_handle_t*)handle);                      \
    }                                                                   \
  } while (0)


#define uv__handle_close(handle)                                        \
  do {                                                                  \
    ngx_queue_remove(&(handle)->handle_queue);                          \
    (handle)->flags |= UV_HANDLE_CLOSED;                                \
    if ((handle)->close_cb) {                                           \
      (handle)->close_cb((uv_handle_t*)(handle));                       \
    }                                                                   \
  } while (0)


INLINE static void uv_want_endgame(uv_loop_t* loop, uv_handle_t* handle) {
  if (!(handle->flags & UV_HANDLE_ENDGAME_QUEUED)) {
    handle->flags |= UV_HANDLE_ENDGAME_QUEUED;

    handle->endgame_next = loop->endgame_handles;
    loop->endgame_handles = handle;
  }
}


INLINE static void uv_process_endgames(uv_loop_t* loop) {
  uv_handle_t* handle;

  while (loop->endgame_handles) {
    handle = loop->endgame_handles;
    loop->endgame_handles = handle->endgame_next;

    handle->flags &= ~UV_HANDLE_ENDGAME_QUEUED;

    switch (handle->type) {
      case UV_TCP:
        uv_tcp_endgame(loop, (uv_tcp_t*) handle);
        break;

      case UV_NAMED_PIPE:
        uv_pipe_endgame(loop, (uv_pipe_t*) handle);
        break;

      case UV_TTY:
        uv_tty_endgame(loop, (uv_tty_t*) handle);
        break;

      case UV_UDP:
        uv_udp_endgame(loop, (uv_udp_t*) handle);
        break;

      case UV_POLL:
        uv_poll_endgame(loop, (uv_poll_t*) handle);
        break;

      case UV_TIMER:
        uv_timer_endgame(loop, (uv_timer_t*) handle);
        break;

      case UV_PREPARE:
      case UV_CHECK:
      case UV_IDLE:
        uv_loop_watcher_endgame(loop, handle);
        break;

      case UV_ASYNC:
        uv_async_endgame(loop, (uv_async_t*) handle);
        break;

      case UV_PROCESS:
        uv_process_endgame(loop, (uv_process_t*) handle);
        break;

      case UV_FS_EVENT:
        uv_fs_event_endgame(loop, (uv_fs_event_t*) handle);
        break;

      default:
        assert(0);
        break;
    }
  }
}

#endif /* UV_WIN_HANDLE_INL_H_ */
