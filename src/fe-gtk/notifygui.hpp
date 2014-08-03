/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef HEXCHAT_NOTIFYGUI_H
#define HEXCHAT_NOTIFYGUI_H

namespace hexchat{
namespace fe{
namespace notify{
	void fe_notify_ask(char *name, char *networks);
} // ::hexchat::fe::notify
} // ::hexchat::fe

namespace gui{
namespace notify{

void notify_gui_update (void);
void notify_opengui (void);

} // ::hexchat::gui::notify
} // ::hexchat::gui
} // ::hexchat

#endif
