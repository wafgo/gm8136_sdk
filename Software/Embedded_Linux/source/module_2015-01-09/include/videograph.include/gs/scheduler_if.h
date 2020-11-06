/**
* @file scheduler_if.h
*   This file defines the scheduler interface for other modules.
* Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
*
* $Revision: 1.20 $
* $Date: 2014/07/16 07:18:48 $
*
* ChangeLog:
*  $Log: scheduler_if.h,v $
*  Revision 1.20  2014/07/16 07:18:48  schumy_c
*  Add sch_check_buffer_existence API.
*
*  Revision 1.19  2014/07/15 09:02:15  schumy_c
*  Add sch_get_entity_job_count() api.
*
*  Revision 1.18  2014/07/15 07:52:47  schumy_c
*  Add sch_get_upstream_entity_job_count() api.
*
*  Revision 1.17  2014/06/27 10:25:03  schumy_c
*  Check and add timesync frame when doing clear bs.
*
*  Revision 1.16  2014/06/23 06:24:15  schumy_c
*  Fix build error. Add sch_dump_buffer_to_log().
*
*  Revision 1.15  2014/04/02 02:38:21  schumy_c
*  Add new define for wait buffer.
*
*  Revision 1.14  2013/05/29 08:32:58  schumy_c
*  Update log message.
*
*  Revision 1.13  2013/04/09 02:40:51  schumy_c
*  Add sch-list define.
*
*  Revision 1.12  2013/04/02 08:44:37  klchu
*  modify timer_delay setting.
*
*  Revision 1.11  2013/02/20 09:45:43  schumy_c
*  Modify print log interface.
*
*  Revision 1.10  2013/02/05 01:42:48  klchu
*  fixed compile error.
*
*  Revision 1.9  2013/02/04 09:38:35  klchu
*  add interface for vplib to check if there is any job not callback when timeout.
*
*  Revision 1.8  2013/01/16 06:52:29  schumy_c
*  Fix build error.
*
*  Revision 1.7  2013/01/16 06:09:06  schumy_c
*  Move from source folder.
*
*  Revision 1.2  2013/01/02 07:34:22  schumy_c
*  1.Put job whenever all entity are reached.
*  2.Add interface for updating gs configurations.
*
*  Revision 1.1  2012/11/27 02:33:32  schumy_c
*  Change file location.
*
*  Revision 1.5  2012/11/22 03:26:11  schumy_c
*  Add new log items.
*
*  Revision 1.4  2012/11/13 10:15:57  schumy_c
*  Update dump function API.
*
*  Revision 1.3  2012/11/12 09:36:42  schumy_c
*  Add CVS header.
*
*/

#ifndef __SCHEDULER_IF_H__
#define __SCHEDULER_IF_H__

#ifdef __SCHEDULRE_C__
#define SCH_IF_EXT
#else
#define SCH_IF_EXT extern
#endif

#ifdef WIN32
#include <stdarg.h>
//#include "list.h"
#define private priv_5417
#endif
#include "graph_service_if.h"

#define SCH_MAX_NAME_LEN         31

SCH_IF_EXT int sch_init(void);
SCH_IF_EXT int sch_exit(void);
SCH_IF_EXT int sch_job_callback(void *data);
SCH_IF_EXT void sch_print_all_structure_values(int level, int log_where, char *message, int *len,
                                               char *page, int apply);

#define LOG_TYPE_ENTITY             0
#define LOG_TYPE_BUFFER             1
#define LOG_TYPE_JOB                2
#define LOG_TYPE_RUNBUFFER          3
#define LOG_TYPE_GRAPH_OVERVIEW     4
#define LOG_TYPE_STATISTIC          5
#define LOG_TYPE_TIME_SYNC_HEAD_TS  6
#define LOG_TYPE_ENODE_DATA         7
#define LOG_TYPE_BNODE_DATA         8
#define LOG_TYPE_SCH_LIST           9

SCH_IF_EXT void sch_print_log(int log_where, int item, int *len, char *page);

#define IN      0
#define OUT     1
#define BOTH    2
SCH_IF_EXT int sch_dump_buffer(char *p_entity_name, int in_out);
SCH_IF_EXT int sch_dump_buffer_to_log(char *p_entity_name, int in_out, int on_off);

#define CONFIG_JOB_TIMER_DELAY      0
#define CONFIG_TICK_TIMER_DELAY     1
#define CONFIG_GROUP_TIMER_DELAY    2
#define CONFIG_WAITBUF_TIMER_DELAY  3
#define CONFIG_WAITBUF_TIMEOUT      4
SCH_IF_EXT int sch_get_config(int config_type);
SCH_IF_EXT int sch_set_config(int config_type, int value);


SCH_IF_EXT int sch_change_pool_layout(void/*BUFFER_POOL_HDL*/ *pool_hdl);
SCH_IF_EXT int sch_check_job_schduler(ENTITY_FD entity_fd);

SCH_IF_EXT int sch_check_and_add_timesync_buffer(ENTITY_FD entity_fd);
SCH_IF_EXT int sch_get_upstream_entity_job_count(ENTITY_FD entity_fd);
SCH_IF_EXT int sch_get_entity_job_count(ENTITY_FD entity_fd);

#define BUF_READY_TO_USE    0
#define BUF_ONGOING         1
SCH_IF_EXT int sch_check_buffer_existence(ENTITY_FD entity_fd, int in_out, int which);

#endif /* __SCHEDULER_IF_H__ */

