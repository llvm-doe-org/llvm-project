// Define callback functions and supporting functions to print all info
// currently provided by the OpenACC Profiling Interface implementation.

#include <acc_prof.h>
#include <assert.h>
#include <stdio.h>

static const char *constructToStr(acc_construct_t v) {
  switch (v) {
#define CONSTRUCT_ENUMERATOR(Construct, Val) \
  case acc_construct_##Construct: return "acc_construct_"#Construct;
ACC_FOREACH_CONSTRUCT(CONSTRUCT_ENUMERATOR)
#undef CONSTRUCT_ENUMERATOR
  }
  assert(!"unexpected acc_construct_t");
  return 0;
}

static const char *deviceToStr(acc_device_t v) {
  switch (v) {
#define DEVICE_ENUMERATOR(Device) \
  case acc_device_##Device: return "acc_device_"#Device;
ACC_FOREACH_DEVICE(DEVICE_ENUMERATOR)
#undef DEVICE_ENUMERATOR
  }
  assert(!"unexpected acc_device_t");
  return 0;
}

static const char *asyncToStr(ssize_t v) {
  static char buf[100];
  switch (v) {
#define ASYNC_ENUMERATOR(Async, Val) \
  case acc_async_##Async: return "acc_async_"#Async;
ACC_FOREACH_ASYNC(ASYNC_ENUMERATOR)
#undef ASYNC_ENUMERATOR
  default:
    sprintf(buf, "%zd", v);
    return buf;
  }
}

static void print_prof_info(acc_prof_info *pi) {
  printf("  acc_prof_info\n"
         "    event_type=%d, valid_bytes=%d, version=%d,\n"
         "    device_type=%s, device_number=%d,\n"
         "    thread_id=%d, async=%s, async_queue=%zd,\n"
         "    src_file=%s, func_name=%s,\n"
         "    line_no=%d, end_line_no=%d,\n"
         "    func_line_no=%d, func_end_line_no=%d\n",
         pi->event_type, pi->valid_bytes, pi->version,
         deviceToStr(pi->device_type), pi->device_number,
         pi->thread_id, asyncToStr(pi->async), pi->async_queue,
         pi->src_file ? pi->src_file : "(null)",
         pi->func_name ? pi->func_name : "(null)",
         pi->line_no, pi->end_line_no,
         pi->func_line_no, pi->func_end_line_no);
}

static void print_other_event_info(acc_other_event_info *oi) {
  printf("    event_type=%d, valid_bytes=%d,\n"
         "    parent_construct=%s,\n"
         "    implicit=%d, tool_info=%p",
         oi->event_type, oi->valid_bytes, constructToStr(oi->parent_construct),
         oi->implicit, oi->tool_info);
}

static void print_event_info(acc_event_info *ei) {
  switch (ei->event_type) {
  case acc_ev_create:
  case acc_ev_delete:
  case acc_ev_alloc:
  case acc_ev_free:
  case acc_ev_enqueue_upload_start:
  case acc_ev_enqueue_upload_end:
  case acc_ev_enqueue_download_start:
  case acc_ev_enqueue_download_end:
    printf("  acc_data_event_info\n");
    print_other_event_info(&ei->other_event);
    printf(",\n"
           "    var_name=%s, bytes=%zu,\n"
           "    host_ptr=%p,\n"
           "    device_ptr=%p\n",
           ei->data_event.var_name ? ei->data_event.var_name : "(null)",
           ei->data_event.bytes, ei->data_event.host_ptr,
           ei->data_event.device_ptr);
    break;
  case acc_ev_enqueue_launch_start:
  case acc_ev_enqueue_launch_end:
    printf("  acc_launch_event_info\n");
    print_other_event_info(&ei->other_event);
    printf(",\n"
           "    kernel_name=%p\n",
           ei->launch_event.kernel_name);
    break;
  default:
    printf("  acc_other_event_info\n");
    print_other_event_info(&ei->other_event);
    printf("\n");
    break;
  }
}

static void print_api_info(acc_api_info *ai) {
  printf("  acc_api_info\n"
         "    device_api=%d, valid_bytes=%d,\n"
         "    device_type=%s\n",
         ai->device_api, ai->valid_bytes, deviceToStr(ai->device_type));
}

static void print_info(acc_prof_info *pi, acc_event_info *ei,
                       acc_api_info *ai) {
  print_prof_info(pi);
  print_event_info(ei);
  print_api_info(ai);
  fflush(stdout);
}

static void on_device_init_start(acc_prof_info *pi, acc_event_info *ei,
                                 acc_api_info *ai) {
  printf("acc_ev_device_init_start\n");
  print_info(pi, ei, ai);
}

static void on_device_init_end(acc_prof_info *pi, acc_event_info *ei,
                               acc_api_info *ai) {
  printf("acc_ev_device_init_end\n");
  print_info(pi, ei, ai);
}

static void on_device_shutdown_start(acc_prof_info *pi, acc_event_info *ei,
                                     acc_api_info *ai) {
  printf("acc_ev_device_shutdown_start\n");
  print_info(pi, ei, ai);
}

static void on_device_shutdown_end(acc_prof_info *pi, acc_event_info *ei,
                                   acc_api_info *ai) {
  printf("acc_ev_device_shutdown_end\n");
  print_info(pi, ei, ai);
}

static void on_runtime_shutdown(acc_prof_info *pi, acc_event_info *ei,
                                acc_api_info *ai) {
  printf("acc_ev_runtime_shutdown\n");
  print_info(pi, ei, ai);
}

static void on_create(acc_prof_info *pi, acc_event_info *ei,
                      acc_api_info *ai) {
  printf("acc_ev_create\n");
  print_info(pi, ei, ai);
}

static void on_delete(acc_prof_info *pi, acc_event_info *ei,
                      acc_api_info *ai) {
  printf("acc_ev_delete\n");
  print_info(pi, ei, ai);
}

static void on_alloc(acc_prof_info *pi, acc_event_info *ei, acc_api_info *ai) {
  printf("acc_ev_alloc\n");
  print_info(pi, ei, ai);
}

static void on_free(acc_prof_info *pi, acc_event_info *ei, acc_api_info *ai) {
  printf("acc_ev_free\n");
  print_info(pi, ei, ai);
}

static void on_enter_data_start(acc_prof_info *pi, acc_event_info *ei,
                                acc_api_info *ai) {
  printf("acc_ev_enter_data_start\n");
  print_info(pi, ei, ai);
}

static void on_enter_data_end(acc_prof_info *pi, acc_event_info *ei,
                              acc_api_info *ai) {
  printf("acc_ev_enter_data_end\n");
  print_info(pi, ei, ai);
}

static void on_exit_data_start(acc_prof_info *pi, acc_event_info *ei,
                               acc_api_info *ai) {
  printf("acc_ev_exit_data_start\n");
  print_info(pi, ei, ai);
}

static void on_exit_data_end(acc_prof_info *pi, acc_event_info *ei,
                             acc_api_info *ai) {
  printf("acc_ev_exit_data_end\n");
  print_info(pi, ei, ai);
}


static void on_update_start(acc_prof_info *pi, acc_event_info *ei,
                            acc_api_info *ai) {
  printf("acc_ev_update_start\n");
  print_info(pi, ei, ai);
}

static void on_update_end(acc_prof_info *pi, acc_event_info *ei,
                          acc_api_info *ai) {
  printf("acc_ev_update_end\n");
  print_info(pi, ei, ai);
}

static void on_compute_construct_start(acc_prof_info *pi, acc_event_info *ei,
                                       acc_api_info *ai) {
  printf("acc_ev_compute_construct_start\n");
  print_info(pi, ei, ai);
}

static void on_compute_construct_end(acc_prof_info *pi, acc_event_info *ei,
                                     acc_api_info *ai) {
  printf("acc_ev_compute_construct_end\n");
  print_info(pi, ei, ai);
}

static void on_enqueue_launch_start(acc_prof_info *pi, acc_event_info *ei,
                                    acc_api_info *ai) {
  printf("acc_ev_enqueue_launch_start\n");
  print_info(pi, ei, ai);
}

static void on_enqueue_launch_end(acc_prof_info *pi, acc_event_info *ei,
                                  acc_api_info *ai) {
  printf("acc_ev_enqueue_launch_end\n");
  print_info(pi, ei, ai);
}

static void on_enqueue_upload_start(acc_prof_info *pi, acc_event_info *ei,
                                    acc_api_info *ai) {
  printf("acc_ev_enqueue_upload_start\n");
  print_info(pi, ei, ai);
}

static void on_enqueue_upload_end(acc_prof_info *pi, acc_event_info *ei,
                                  acc_api_info *ai) {
  printf("acc_ev_enqueue_upload_end\n");
  print_info(pi, ei, ai);
}

static void on_enqueue_download_start(acc_prof_info *pi, acc_event_info *ei,
                                      acc_api_info *ai) {
  printf("acc_ev_enqueue_download_start\n");
  print_info(pi, ei, ai);
}

static void on_enqueue_download_end(acc_prof_info *pi, acc_event_info *ei,
                                    acc_api_info *ai) {
  printf("acc_ev_enqueue_download_end\n");
  print_info(pi, ei, ai);
}

static void on_wait_start(acc_prof_info *pi, acc_event_info *ei,
                          acc_api_info *ai) {
  assert(!"acc_ev_wait_start is not yet implemented\n");
}

static void on_wait_end(acc_prof_info *pi, acc_event_info *ei,
                        acc_api_info *ai) {
  assert(!"acc_ev_wait_end is not yet implemented\n");
}

static void register_all_callbacks(acc_prof_reg reg) {
  reg(acc_ev_device_init_start, on_device_init_start, acc_reg);
  reg(acc_ev_device_init_end, on_device_init_end, acc_reg);
  reg(acc_ev_device_shutdown_start, on_device_shutdown_start, acc_reg);
  reg(acc_ev_device_shutdown_end, on_device_shutdown_end, acc_reg);
  reg(acc_ev_runtime_shutdown, on_runtime_shutdown, acc_reg);
  reg(acc_ev_create, on_create, acc_reg);
  reg(acc_ev_delete, on_delete, acc_reg);
  reg(acc_ev_alloc, on_alloc, acc_reg);
  reg(acc_ev_free, on_free, acc_reg);
  reg(acc_ev_enter_data_start, on_enter_data_start, acc_reg);
  reg(acc_ev_enter_data_end, on_enter_data_end, acc_reg);
  reg(acc_ev_exit_data_start, on_exit_data_start, acc_reg);
  reg(acc_ev_exit_data_end, on_exit_data_end, acc_reg);
  reg(acc_ev_update_start, on_update_start, acc_reg);
  reg(acc_ev_update_end, on_update_end, acc_reg);
  reg(acc_ev_compute_construct_start, on_compute_construct_start, acc_reg);
  reg(acc_ev_compute_construct_end, on_compute_construct_end, acc_reg);
  reg(acc_ev_enqueue_launch_start, on_enqueue_launch_start, acc_reg);
  reg(acc_ev_enqueue_launch_end, on_enqueue_launch_end, acc_reg);
  reg(acc_ev_enqueue_upload_start, on_enqueue_upload_start, acc_reg);
  reg(acc_ev_enqueue_upload_end, on_enqueue_upload_end, acc_reg);
  reg(acc_ev_enqueue_download_start, on_enqueue_download_start, acc_reg);
  reg(acc_ev_enqueue_download_end, on_enqueue_download_end, acc_reg);
}
