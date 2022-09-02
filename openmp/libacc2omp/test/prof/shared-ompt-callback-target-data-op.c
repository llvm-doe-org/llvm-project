// Check registration/unregistration scenarios involving OpenACC events that
// share ompt_callback_target_data_op_emi.

// RUN: %clang-acc -o %t.exe %s

// DEFINE: %{check}( EVENT %) =                                                \
// DEFINE:   env EVENT=%{EVENT} %t.exe > %t.out 2> %t.err &&                   \
// DEFINE:   FileCheck -input-file %t.err -allow-empty -check-prefixes=ERR     \
// DEFINE:             %s &&                                                   \
// DEFINE:   FileCheck -input-file %t.out %s                                   \
// DEFINE:     -match-full-lines -strict-whitespace -allow-empty               \
// DEFINE:     -implicit-check-not=acc_ev_                                     \
// DEFINE:     -check-prefixes=%if-host<NONE|%{EVENT}>

// RUN: %{check}( NONE                   %)
// RUN: %{check}( CREATE                 %)
// RUN: %{check}( DELETE                 %)
// RUN: %{check}( ALLOC                  %)
// RUN: %{check}( FREE                   %)
// RUN: %{check}( ENQUEUE_UPLOAD_START   %)
// RUN: %{check}( ENQUEUE_UPLOAD_END     %)
// RUN: %{check}( ENQUEUE_DOWNLOAD_START %)
// RUN: %{check}( ENQUEUE_DOWNLOAD_END   %)
//
// END.

// expected-no-diagnostics

#include <acc_prof.h>
__attribute__((unused)) static void register_all_callbacks(acc_prof_reg reg);
#include "callbacks.h"

#include <stdlib.h>
#include <string.h>

#define FOREACH_EVENT(Macro)                                                   \
  Macro(NONE)                                                                  \
  Macro(CREATE)                                                                \
  Macro(DELETE)                                                                \
  Macro(ALLOC)                                                                 \
  Macro(FREE)                                                                  \
  Macro(ENQUEUE_UPLOAD_START)                                                  \
  Macro(ENQUEUE_UPLOAD_END)                                                    \
  Macro(ENQUEUE_DOWNLOAD_START)                                                \
  Macro(ENQUEUE_DOWNLOAD_END)

enum {
#define EventItr(Event)                                                        \
  EVENT_##Event,
  FOREACH_EVENT(EventItr)
#undef EventItr
};

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  // These events share ompt_callback_target_data_op_emi as a triggering OMPT
  // callback and are distinguished by the optype parameter in that callback
  // except that, within the pairs acc_ev_enqueue_upload_{start,end}_callback
  // and acc_ev_enqueue_download_{start,end}_callback, optype does not
  // distinguish the events.  That is, within each of those pairs, callbacks
  // for both events dispatch for the same optype, and the only distinction is
  // whether a callback is actually registered for each event.
  //
  // Check that registering callbacks for all of these events and then
  // unregistering callbacks for all but one of them doesn't cause the
  // remaining one to fail to dispatch.  Check that every event can be the only
  // event with a remaining callback in order to cover cases like the pairs
  // mentioned above.  Also check the case of unregistering all callbacks.
  reg(acc_ev_create, on_create, acc_reg);
  reg(acc_ev_delete, on_delete, acc_reg);
  reg(acc_ev_alloc, on_alloc, acc_reg);
  reg(acc_ev_free, on_free, acc_reg);
  reg(acc_ev_enqueue_upload_start, on_enqueue_upload_start, acc_reg);
  reg(acc_ev_enqueue_upload_end, on_enqueue_upload_end, acc_reg);
  reg(acc_ev_enqueue_download_start, on_enqueue_download_start, acc_reg);
  reg(acc_ev_enqueue_download_end, on_enqueue_download_end, acc_reg);

  const char *eventEnv = getenv("EVENT");
  assert(eventEnv && "expected env var EVENT to be set");
  int event;
  if (0)
    ;
#define EventItr(Event)                                                        \
  else if (!strcmp(eventEnv, #Event))                                          \
    event = EVENT_##Event;
  FOREACH_EVENT(EventItr)
#undef EventItr
  else
    assert(!"expected env var EVENT to have valid event");

  if (event != EVENT_CREATE)
    unreg(acc_ev_create, on_create, acc_reg);
  if (event != EVENT_DELETE)
    unreg(acc_ev_delete, on_delete, acc_reg);
  if (event != EVENT_FREE)
    unreg(acc_ev_free, on_free, acc_reg);
  if (event != EVENT_ALLOC)
    unreg(acc_ev_alloc, on_alloc, acc_reg);
  if (event != EVENT_ENQUEUE_UPLOAD_START)
    unreg(acc_ev_enqueue_upload_start, on_enqueue_upload_start, acc_reg);
  if (event != EVENT_ENQUEUE_UPLOAD_END)
    unreg(acc_ev_enqueue_upload_end, on_enqueue_upload_end, acc_reg);
  if (event != EVENT_ENQUEUE_DOWNLOAD_START)
    unreg(acc_ev_enqueue_download_start, on_enqueue_download_start, acc_reg);
  if (event != EVENT_ENQUEUE_DOWNLOAD_END)
    unreg(acc_ev_enqueue_download_end, on_enqueue_download_end, acc_reg);
}

//                 CREATE:acc_ev_create
//                 DELETE:acc_ev_delete
//                  ALLOC:acc_ev_alloc
//                   FREE:acc_ev_free
//   ENQUEUE_UPLOAD_START:acc_ev_enqueue_upload_start
//     ENQUEUE_UPLOAD_END:acc_ev_enqueue_upload_end
// ENQUEUE_DOWNLOAD_START:acc_ev_enqueue_download_start
//   ENQUEUE_DOWNLOAD_END:acc_ev_enqueue_download_end
int main() {
  int arr[2] = {10, 11};
  #pragma acc parallel copy(arr) num_gangs(1)
  for (int j = 0; j < 2; ++j)
    ;
  return 0;
}

// NONE-NOT: {{.}}
// ERR-NOT:{{.}}
