/*
 * Copyright (c) 1999 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#if !defined(WINNT)
#ident "$Id: sys_display.c,v 1.10 2000/02/13 19:18:27 steve Exp $"
#endif

# include  "vpi_user.h"
# include  <assert.h>
# include  <string.h>
# include  <ctype.h>
# include  <stdlib.h>

struct strobe_cb_info {
      char*name;
      vpiHandle*items;
      unsigned nitems;
};

static void array_from_iterator(struct strobe_cb_info*info, vpiHandle argv)
{
      vpiHandle item;
      unsigned nitems = 1;
      vpiHandle*items = malloc(sizeof(vpiHandle));
      items[0] = vpi_scan(argv);
      for (item = vpi_scan(argv) ;  item ;  item = vpi_scan(argv)) {
	    items = realloc(items, (nitems+1)*sizeof(vpiHandle));
	    items[nitems] = item;
	    nitems += 1;
      }

      info->nitems = nitems;
      info->items = items;
}


/*
 * If $display discovers a string as a parameter, this function is
 * called to process it as a format string. I need the argv handle as
 * well so that I can look for arguments as I move forward through the
 * string.
 */
static int format_str(char*fmt, int argc, vpiHandle*argv)
{
      s_vpi_value value;
      char buf[256];
      char*cp = fmt;
      int idx;

      assert(fmt);

      idx = 0;

      while (*cp) {
	    size_t cnt = strcspn(cp, "%\\");
	    if (cnt > 0) {
		  if (cnt >= sizeof buf)
			cnt = sizeof buf - 1;
		  strncpy(buf, cp, cnt);
		  buf[cnt] = 0;
		  vpi_printf("%s", buf);
		  cp += cnt;

	    } else if (*cp == '%') {
		  int fsize = -1;

		  cp += 1;
		  if (isdigit(*cp))
			fsize = strtoul(cp, &cp, 10);

		  switch (*cp) {
		      case 0:
			break;
		      case 'b':
		      case 'B':
			value.format = vpiBinStrVal;
			vpi_get_value(argv[idx++], &value);
			vpi_printf("%s", value.value.str);
			cp += 1;
			break;
		      case 'd':
		      case 'D':
			value.format = vpiDecStrVal;
			vpi_get_value(argv[idx++], &value);
			vpi_printf("%s", value.value.str);
			cp += 1;
			break;
		      case 'h':
		      case 'H':
		      case 'x':
		      case 'X':
			value.format = vpiHexStrVal;
			vpi_get_value(argv[idx++], &value);
			vpi_printf("%s", value.value.str);
			cp += 1;
			break;
		      case 'm':
			vpi_printf("%s", vpi_get_str(vpiFullName, argv[idx]));
			idx += 1;
			cp += 1;
			break;
		      case 'o':
		      case 'O':
			value.format = vpiOctStrVal;
			vpi_get_value(argv[idx++], &value);
			vpi_printf("%s", value.value.str);
			cp += 1;
			break;
		      case 't':
		      case 'T':
			value.format = vpiDecStrVal;
			vpi_get_value(argv[idx++], &value);
			vpi_printf("%s", value.value.str);
			cp += 1;
			break;
		      case '%':
			vpi_printf("%%");
			cp += 1;
			break;
		      default:
			vpi_printf("%c", *cp);
			cp += 1;
			break;
		  }

	    } else {

		  cp += 1;
		  switch (*cp) {
		      case 0:
			break;
		      case 'n':
			vpi_printf("\n");
			cp += 1;
			break;
		      default:
			vpi_printf("%c", *cp);
			cp += 1;
		  }
	    }
      }

      return idx;
}

static void do_display(struct strobe_cb_info*info)
{
      s_vpi_value value;
      int idx;

      for (idx = 0 ;  idx < info->nitems ;  idx += 1) {
	    vpiHandle item = info->items[idx];

	    switch (vpi_get(vpiType, item)) {

		case 0:
		  vpi_printf(" ");
		  break;

		case vpiConstant:
		  if (vpi_get(vpiConstType, item) == vpiStringConst) {
			value.format = vpiStringVal;
			vpi_get_value(item, &value);
			idx += format_str(value.value.str,
					  info->nitems-idx-1,
					  info->items+idx+1);
		  } else {
			value.format = vpiBinStrVal;
			vpi_get_value(item, &value);
			vpi_printf("%s", value.value.str);
		  }
		  break;

		case vpiNet:
		case vpiReg:
		case vpiMemoryWord:
		  value.format = vpiBinStrVal;
		  vpi_get_value(item, &value);
		  vpi_printf("%s", value.value.str);
		  break;

		case vpiTimeVar:
		  value.format = vpiTimeVal;
		  vpi_get_value(item, &value);
		  vpi_printf("%u", value.value.time->low);
		  break;

		default:
		  vpi_printf("?");
		  break;
	    }
      }
}

static int sys_display_calltf(char *name)
{
      struct strobe_cb_info info;
      vpiHandle sys = vpi_handle(vpiSysTfCall, 0);

      vpiHandle argv = vpi_iterate(vpiArgument, sys);

      array_from_iterator(&info, argv);

      do_display(&info);

      free(info.items);

      if (strcmp(name,"$display") == 0)
	    vpi_printf("\n");

      return 0;
}

/*
 * The strobe implementation takes the parameter handles that are
 * passed to the calltf and puts them in to an array for safe
 * keeping. That array (and other bookkeeping) is passed, via the
 * struct_cb_info object, to the REadOnlySych function strobe_cb,
 * where it is use to perform the actual formatting and printing.
 */

static int strobe_cb(p_cb_data cb)
{
      struct strobe_cb_info*info = (struct strobe_cb_info*)cb->user_data;

      do_display(info);

      vpi_printf("\n");

      free(info->name);
      free(info->items);
      free(info);

      return 0;
}

static int sys_strobe_calltf(char*name)
{
      struct t_cb_data cb;
      struct t_vpi_time time;

      vpiHandle sys = vpi_handle(vpiSysTfCall, 0);

      vpiHandle argv = vpi_iterate(vpiArgument, sys);

      struct strobe_cb_info*info = calloc(1, sizeof(struct strobe_cb_info));

      array_from_iterator(info, argv);
      info->name = strdup(name);

      time.type = vpiSimTime;
      time.low = 0;
      time.high = 0;

      cb.reason = cbReadOnlySynch;
      cb.cb_rtn = strobe_cb;
      cb.time = &time;
      cb.obj = 0;
      cb.user_data = (char*)info;
      vpi_register_cb(&cb);
      return 0;
}

/*
 * The $monitor system task works by managing these static variables,
 * and the cbValueChange callbacks associated with registers and
 * nets. Note that it is proper to keep the state in static variables
 * because there can only be one monitor at a time pending (even
 * though that monitor may be watching many variables).
 */

static struct strobe_cb_info monitor_info = { 0, 0, 0 };
static int monitor_scheduled = 0;
static vpiHandle *monitor_callbacks = 0;

static int monitor_cb_2(p_cb_data cb)
{
      do_display(&monitor_info);
      vpi_printf("\n");
      monitor_scheduled = 0;
      return 0;
}

static int monitor_cb_1(p_cb_data cause)
{
      struct t_cb_data cb;
      struct t_vpi_time time;

	/* Reschedule this event so that it happens for the next
	   trigger on this variable. */
      cb = *cause;
      vpi_register_cb(&cb);
      
      if (monitor_scheduled) return 0;

	/* This this action caused the first trigger, then schedule
	   the monitor to happen at the end of the time slice and mark
	   it as scheduled. */
      monitor_scheduled += 1;
      time.type = vpiSimTime;
      time.low = 0;
      time.high = 0;

      cb.reason = cbReadOnlySynch;
      cb.cb_rtn = monitor_cb_2;
      cb.time = &time;
      cb.obj = 0;
      vpi_register_cb(&cb);

      return 0;
}

static int sys_monitor_calltf(char*name)
{
      unsigned idx;
      struct t_cb_data cb;
      struct t_vpi_time time;

      vpiHandle sys = vpi_handle(vpiSysTfCall, 0);
      vpiHandle argv = vpi_iterate(vpiArgument, sys);

      if (monitor_callbacks) {
	    for (idx = 0 ;  idx < monitor_info.nitems ;  idx += 1)
		  if (monitor_callbacks[idx])
			vpi_remove_cb(monitor_callbacks[idx]);

	    free(monitor_callbacks);
	    monitor_callbacks = 0;

	    free(monitor_info.items);
	    free(monitor_info.name);
	    monitor_info.items = 0;
	    monitor_info.nitems = 0;
	    monitor_info.name = 0;
      }

      array_from_iterator(&monitor_info, argv);
      monitor_info.name = strdup(name);

      monitor_callbacks = calloc(monitor_info.nitems, sizeof(vpiHandle));

      time.type = vpiSuppressTime;
      cb.reason = cbValueChange;
      cb.cb_rtn = monitor_cb_1;
      cb.time = &time;
      for (idx = 0 ;  idx < monitor_info.nitems ;  idx += 1) {

	    switch (vpi_get(vpiType, monitor_info.items[idx])) {
		case vpiNet:
		case vpiReg:
		  cb.obj = monitor_info.items[idx];
		  monitor_callbacks[idx] = vpi_register_cb(&cb);
		  break;

	    }
      }

      return 0;
}

void sys_display_register()
{
      s_vpi_systf_data tf_data;

      tf_data.type      = vpiSysTask;
      tf_data.tfname    = "$display";
      tf_data.calltf    = sys_display_calltf;
      tf_data.compiletf = 0;
      tf_data.sizetf    = 0;
      tf_data.user_data = "$display";
      vpi_register_systf(&tf_data);

      tf_data.type      = vpiSysTask;
      tf_data.tfname    = "$write";
      tf_data.calltf    = sys_display_calltf;
      tf_data.compiletf = 0;
      tf_data.sizetf    = 0;
      tf_data.user_data = "$write";
      vpi_register_systf(&tf_data);

      tf_data.type      = vpiSysTask;
      tf_data.tfname    = "$strobe";
      tf_data.calltf    = sys_strobe_calltf;
      tf_data.compiletf = 0;
      tf_data.sizetf    = 0;
      tf_data.user_data = "$strobe";
      vpi_register_systf(&tf_data);

      tf_data.type      = vpiSysTask;
      tf_data.tfname    = "$monitor";
      tf_data.calltf    = sys_monitor_calltf;
      tf_data.compiletf = 0;
      tf_data.sizetf    = 0;
      tf_data.user_data = "$monitor";
      vpi_register_systf(&tf_data);
}


/*
 * $Log: sys_display.c,v $
 * Revision 1.10  2000/02/13 19:18:27  steve
 *  Accept memory words as parameter to $display.
 *
 * Revision 1.9  1999/11/07 02:25:07  steve
 *  Add the $monitor implementation.
 *
 * Revision 1.8  1999/11/06 23:32:14  steve
 *  Unify display and strobe format routines.
 *
 * Revision 1.7  1999/11/06 22:16:50  steve
 *  Get the $strobe task working.
 *
 * Revision 1.6  1999/10/29 03:37:22  steve
 *  Support vpiValueChance callbacks.
 *
 * Revision 1.5  1999/10/28 00:47:25  steve
 *  Rewrite vvm VPI support to make objects more
 *  persistent, rewrite the simulation scheduler
 *  in C (to interface with VPI) and add VPI support
 *  for callbacks.
 *
 * Revision 1.4  1999/10/10 14:50:50  steve
 *  Add Octal dump format.
 *
 * Revision 1.3  1999/10/08 17:47:49  steve
 *  Add the %t formatting escape.
 *
 * Revision 1.2  1999/09/29 01:41:18  steve
 *  Support the $write system task, and have the
 *  vpi_scan function free iterators as needed.
 *
 * Revision 1.1  1999/08/15 01:23:56  steve
 *  Convert vvm to implement system tasks with vpi.
 *
 */

