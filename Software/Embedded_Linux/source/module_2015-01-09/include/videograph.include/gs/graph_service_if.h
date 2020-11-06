/**
* @file graph_service_if.h
*   This file defines the graph-service interface for other modules.
* Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
*
* $Revision: 1.31 $
* $Date: 2014/12/29 07:03:32 $
*
* ChangeLog:
*  $Log: graph_service_if.h,v $
*  Revision 1.31  2014/12/29 07:03:32  foster_h
*  for 60i use ride_disp_scl_in buffer, reduce disp0_in buffer
*
*  Revision 1.30  2014/11/28 03:42:14  schumy_c
*  Pass ref.clock fps info to tick_handler.(support 60fps feature)
*  Rename some variables.
*
*  Revision 1.29  2014/11/11 02:33:58  klchu
*  add group job id for capture.
*
*  Revision 1.28  2014/10/30 08:27:13  klchu
*  use layout_info instead of buf_info for layout change.
*
*  Revision 1.27  2014/10/22 02:49:33  klchu
*  add support of more ddr number.
*
*  Revision 1.26  2014/09/16 07:02:05  schumy_c
*  Fix fake tick timer inaccurate issue.(used for multiwin)
*
*  Revision 1.25  2014/06/18 06:34:42  schumy_c
*  Add fake reference clock service.
*
*  Revision 1.24  2014/06/17 03:39:39  schumy_c
*  Add function. query fds by specifying an enode.
*
*  Revision 1.23  2014/06/04 03:18:24  schumy_c
*  Add job flow priority code. Set LV cap as high priority.
*
*  Revision 1.22  2014/04/17 03:16:25  schumy_c
*  Add new mode "direct callback with buffer" for entity.
*
*  Revision 1.21  2014/04/08 03:29:49  klchu
*  add gs_change_graph_name function.
*
*  Revision 1.20  2014/03/31 02:59:45  schumy_c
*  Add entity flow mode(direct putjob to next at callback).
*
*  Revision 1.19  2014/03/27 02:42:45  schumy_c
*  Add apply mode for gs.
*
*  Revision 1.18  2014/03/12 09:24:46  schumy_c
*  Add gs_reset_graph_entities() function.
*
*  Revision 1.17  2014/03/01 09:20:09  klchu
*  add datain entity setting.
*
*  Revision 1.16  2013/10/28 09:09:36  ivan
*  add gs reserved api
*
*  Revision 1.15  2013/09/05 10:05:44  klchu
*  add non-blocking flag for scaler in encode sub stream.
*
*  Revision 1.14  2013/09/02 09:55:25  klchu
*  add flag for 3DI to stop when version change.
*
*  Revision 1.13  2013/08/13 07:41:04  klchu
*  add do not care callback fail mode setting.
*
*  Revision 1.12  2013/03/20 06:02:49  schumy_c
*  Add graph name in API.
*
*  Revision 1.11  2013/03/18 09:25:28  schumy_c
*  Add graph log style.
*
*  Revision 1.10  2013/03/14 09:38:09  schumy_c
*  Modify graph log.
*
*  Revision 1.9  2013/01/15 06:15:42  schumy_c
*  Add start_pa for fragment detail.
*
*  Revision 1.8  2012/12/26 09:50:15  schumy_c
*  Return fragment more details.
*
*  Revision 1.7  2012/12/26 06:48:54  schumy_c
*  Add query pool layout detail funcitons.
*
*  Revision 1.6  2012/12/20 06:47:13  schumy_c
*  Change ENTITY_HDL to ENTITY_FD.
*
*  Revision 1.5  2012/12/11 01:46:19  schumy_c
*  Add entity notifier function.
*
*  Revision 1.4  2012/11/12 09:36:42  schumy_c
*  Add CVS header.
*
*/

#ifndef __GRAPH_SERVICE_IF_H__
#define __GRAPH_SERVICE_IF_H__


#ifdef __GRAPH_SERVICE_C__
#define GS_EXT
#else
#define GS_EXT extern
#endif


#ifdef WIN32
#include <stdarg.h>
#include "linux_fnc.h"
#define private priv_5417
#else
#include <linux/types.h>
#endif


#define GS_MAX_NAME_LEN         31
#define MAX_PRIVATE_POOL_COUNT  3
#define DONT_CARE               0xFF5417FF
#define ENTITY_LOWEST_PRIORITY  0xFA5417AF


#define REMOVE_OLD_VER          0
#define KEEP_OLD_VER            1

#define POOL_FIXED_SIZE         0
#define POOL_VAR_SIZE           1

#define AS_INPUT                0
#define AS_OUTPUT               1

#define METHOD_BY_TIMESTAMP     0
#define METHOD_BY_RATIO         1

#define GRAPH_VER_IDLE          0
#define GRAPH_VER_START         1

#define MAX_BUF_COUNT           0
#define MIN_BUF_COUNT           1

#define UPPER_STREAM            0
#define DOWN_STREAM             1

#if 0
#define MODE_NONE               0
#define MODE_SOURCE_AUTO_REPEAT 1	/* value1: TRUE, FALSE */
#define MODE_GROUP_ACTION		2	/* value1: AS_INPUT, AS_OUTPUT  value2: 0~31 (priority) */
#define MODE_ENODE_STATUS       3   /* value1: ENODE_ENABLE, ENODE_NO_PJ, ENODE_DIRECT_CB */
#endif

typedef enum entity_mode_tag {
    MODE_NONE = 0,
    MODE_KEEP_INPUT_BUFFER = 1,         /* value1: TRUE, FALSE */
    MODE_FLOW_BY_TICK = 2,              /* value1: TRUE, FALSE */
    MODE_ENODE_STATUS = 3,              /* value1: ENODE_ENABLE, ENODE_NO_PJ, ENODE_DIRECT_CB */
    MODE_ACCEPT_DIFF_VER_INPUT = 4,     /* value1: TRUE, FALSE */
    MODE_DO_NOT_CARE_CALLBACK_FAIL = 5, /* value1: TRUE, FALSE */
    MODE_ENTITY_KEEP_JOB = 6,           /* value1: TRUE, FALSE */
    MODE_ENTITY_NON_BLOCKING = 7,       /* value1: TRUE, FALSE */
    MODE_ENTITY_DATAIN = 8,             /* value1: TRUE, FALSE */
    MODE_DIR_STOP_WHEN_APPLY = 9,       /* value1: TRUE, FALSE */
    MODE_DIR_PUT_NEXT_AT_CALLBACK = 10, /* value1: TRUE, FALSE */
    MODE_JOB_FLOW_PRIORITY = 11         /* value1: 0(low) ~ 1(high) */
} entity_mode_t;

// Define priority for 'MODE_JOB_FLOW_PRIORITY'
#define PRIO_NORMAL     0
#define PRIO_HIGH       1

#define POOL_OPTION_CAN_FROM_FREE_AT_APPLY   (1<<0)

//entity node status
#define ENODE_ENABLE        0 // define entity node status, normal operation
#define ENODE_NO_PJ         1 // entity node disable and does not put job
#define ENODE_DIRECT_CB     2 // entity node disable and direct call back
#define ENODE_DIRECT_CB_WITH_BUF    3 // aquire buffer but not to putjob

#ifndef TRUE
#define TRUE (1==1)
#endif
#ifndef FALSE
#define FALSE (1==0)
#endif

typedef int  (*fn_graph_notify_t)(int status, int graph_ver_id, void *private_data);


typedef   unsigned int      ENTITY_FD;
typedef   void*             GRAPH_HDL;
typedef   void*             BUFFER_HDL;
typedef   void*             APPLY_LIST_HDL;
typedef   void*             BUFFER_POOL_HDL;
typedef   void*             POOL_LAYOUT_HDL;

typedef struct fragment_tag {
    int  is_used;
    unsigned long start_va;
    unsigned long start_pa;
} fragment_t;



GS_EXT int    gs_init(void);
GS_EXT int    gs_exit(void);
GS_EXT GRAPH_HDL  gs_create_graph(fn_graph_notify_t fn_graph_notify, void *private_data, char *name);
GS_EXT int    gs_destroy_graph(GRAPH_HDL graph);
GS_EXT int    gs_change_graph_name(GRAPH_HDL graph, char *name);
GS_EXT int    gs_new_graph_version(GRAPH_HDL graph, int ver_no);
GS_EXT int    gs_del_graph_version(int graph_ver_id);
GS_EXT int    gs_get_ver_id(GRAPH_HDL graph, int ver_no);
GS_EXT int    gs_get_graph_ver_no(int graph_ver_id, GRAPH_HDL *graph, int *ver_no);
GS_EXT int    gs_add_link(int graph_ver_id, ENTITY_FD from_entity, ENTITY_FD to_entity,
                          BUFFER_POOL_HDL buf_pool, void *p_buffer_info, char *buf_name,
                          BUFFER_HDL *ret_bnode_hdl);
GS_EXT int    gs_remove_link(int graph_ver_id, ENTITY_FD from_entity, ENTITY_FD to_entity);
GS_EXT int    gs_verify_link(int graph_ver_id);
GS_EXT int    gs_create_buffer_sea(int ddr_no, int max_size);
GS_EXT int    gs_destroy_buffer_sea(int ddr_no);
GS_EXT BUFFER_POOL_HDL gs_create_buffer_pool(int ddr_no, int size, int type,
                                             char *name, int option,int flag);
GS_EXT int    gs_destroy_buffer_pool(BUFFER_POOL_HDL buf_pool);
GS_EXT u32    gs_which_buffer_sea(u32 va);
GS_EXT u32    gs_offset_in_buffer_sea(u32 va);
GS_EXT u32    gs_buffer_sea_pa_base(u8 ddr_no);
GS_EXT u32    gs_buffer_sea_size(u8 ddr_no);
GS_EXT POOL_LAYOUT_HDL  gs_new_pool_layout(BUFFER_POOL_HDL buf_pool);
GS_EXT int    gs_del_pool_layout(POOL_LAYOUT_HDL layout);
GS_EXT int    gs_reset_pool_layout(POOL_LAYOUT_HDL layout);
GS_EXT int gs_set_ms_extra_layout(BUFFER_POOL_HDL src_pool_hdl, BUFFER_POOL_HDL extra_pool_hdl, 
                             unsigned int size, unsigned int count);
GS_EXT int    gs_change_fix_pool_layout_count(POOL_LAYOUT_HDL pool_layout, unsigned int size, 
                                              unsigned int count, int idx);
GS_EXT int    gs_change_var_pool_layout_count(POOL_LAYOUT_HDL pool_layout, unsigned int size, 
                                              unsigned int win_size, ENTITY_FD entity, int idx);
GS_EXT int    gs_get_pool_fragment_entry(BUFFER_POOL_HDL buf_pool, fragment_t fragment_ary[],
                                         int *p_count);
GS_EXT int    gs_buffer_is_reserved(void *);
GS_EXT APPLY_LIST_HDL gs_create_apply_list(void);
GS_EXT int    gs_destroy_apply_list(APPLY_LIST_HDL apply_list);
GS_EXT int    gs_reset_apply_list(APPLY_LIST_HDL apply_list);
GS_EXT int    gs_add_graph_to_apply_list(APPLY_LIST_HDL apply_list, int graph_ver_id);
GS_EXT int    gs_apply(APPLY_LIST_HDL apply_list);
GS_EXT int    gs_set_flow_ratio(int graph_ver_id, ENTITY_FD entity, int method, int input_rate,
                                int output_rate);
GS_EXT int    gs_set_head_latency(int graph_ver_id, ENTITY_FD head_entity, int mini_sec,
                                  int target_group);
GS_EXT int    gs_select_target_latency_entity(int graph_ver_id, ENTITY_FD target_entity, int group);
GS_EXT int    gs_set_head_entity_max_count(int graph_ver_id, ENTITY_FD head_entity, int count);
GS_EXT int    gs_set_tail_entity_min_count(int graph_ver_id, ENTITY_FD tail_entity, int count);
GS_EXT int    gs_set_buffer_count(int graph_ver_id, char *buf_name, int count, int max_min);
GS_EXT int    gs_set_buffer_count_by_buffer_handle(BUFFER_HDL buf_hdl, int count, int max_min);

//macros for 'fps_or_timer_value'
#define FPI_VALUE(d,v)      ((d) << 16 | (v))       /* frames per interval(ms) */
#define FPS_VALUE(v)        FPI_VALUE(1000, v)      /* frames per 1000ms       */
#define TIMER_VALUE(v)      (v)                     /* interval in ms          */
#define GET_INTERVAL(v)     ((0xffff0000 & (v)) >> 16)
#define GET_FPS(v)          (GET_INTERVAL(v) ? (0x0000ffff & v) : (1000 / (v)))
GS_EXT int    gs_set_reference_clock(int graph_ver_id, ENTITY_FD entity, int fps_or_interval);
GS_EXT int    gs_set_entity_mode(int graph_ver_id, ENTITY_FD entity, int mode, int value1, int value2);
GS_EXT int    gs_set_entity_group_action(int graph_ver_id, ENTITY_FD entity, unsigned int graph_type, 
                                         unsigned int vch, unsigned int path, unsigned int group_type, int priority);
GS_EXT int    gs_query_entity_fd(ENTITY_FD entity_fd, int up_down, ENTITY_FD *out_fd_ary, int max_count);
GS_EXT int    gs_sync_tuning_create_scope(void);
GS_EXT void   gs_sync_tuning_destroy_scope(int scope);
GS_EXT int    gs_sync_tuning_register(int scope, ENTITY_FD entity);
GS_EXT int    gs_sync_tuning_deregister(ENTITY_FD entity);
GS_EXT int    gs_sync_tuning_set_attribute(ENTITY_FD entity, int threshold, int range, int level);
GS_EXT int    gs_sync_tuning_set_base(ENTITY_FD entity);
GS_EXT int    gs_reset_graph_entities(unsigned int graph_ver_id);
GS_EXT void   gs_print_all_structure_values(void);

#define MODE_GRAPH              0
#define MODE_PERF               1
#define MODE_STAT               2
#define MODE_GRAPH_OLD_STYLE    3
GS_EXT void   gs_print_graph(int log_where, int *len, char *page, int mode);

typedef enum when_to_notify_tag {
    WHEN_GS_CALL_ENTITY_STOP       = 0,
    WHEN_JOB_PENDING_FOR_A_WHILE //not implement
} when_to_notify_t;
GS_EXT int    gs_register_entity_notifier(ENTITY_FD entity, int when, void *data,
          void (*fn_entity_notify)(void *data, int when, int version, unsigned long arg1) );

/**
  * Error code
  */
#define GS_OK                       ( 0)
#define GS_CANNOT_FIND_GRAPH_VER    (-1)    /* Must be an odd number */
#define GS_INVALID_ARGUMENT         (-2)
#define GS_DUPLICATED_LINK          (-3)
#define GS_LINK_CONFLICT            (-4)
#define GS_INTERNAL_ERROR           (-5)
#define GS_WRONG_GRAPH              (-6)
#define GS_OUT_OF_RANGE             (-7)
#define GS_RESOURCE_LEAK            (-8)
#define GS_OLD_VERSION_NOT_FINISH   (-9)
#define GS_PARTIAL_FDS              (-10)



#endif /* __GRAPH_SERVICE_IF_H__ */

