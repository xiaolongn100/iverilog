/*
 * Copyright (c) 2008 Stephen Williams (steve@icarus.com)
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

# include "config.h"

# include  "AStatement.h"
# include  "util.h"

# include  <typeinfo>

NetAnalog* AStatement::elaborate(Design*des, NetScope*scope) const
{
      cerr << get_fileline() << ": sorry: I don't yet know how to elaborate"
	   << " his kind of analog statement." << endl;
      cerr << get_fileline() << ":      : typeid = " << typeid(*this).name() << endl;
      return 0;
}

bool AProcess::elaborate(Design*des, NetScope*scope) const
{
      NetAnalog*statement = statement_->elaborate(des, scope);
      if (statement == 0)
	    return false;

      NetAnalogTop*top = new NetAnalogTop(scope, type_, statement);

	// Evaluate the attributes for this process, if there
	// are any. These attributes are to be attached to the
	// NetProcTop object.
      struct attrib_list_t*attrib_list = 0;
      unsigned attrib_list_n = 0;
      attrib_list = evaluate_attributes(attributes, attrib_list_n, des, scope);

      for (unsigned adx = 0 ;  adx < attrib_list_n ;  adx += 1)
	    top->attribute(attrib_list[adx].key,
			   attrib_list[adx].val);

      delete[]attrib_list;

      top->set_line(*this);
      des->add_process(top);

      return true;
}
