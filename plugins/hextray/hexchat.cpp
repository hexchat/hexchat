/* X-Tray
 * Copyright (C) 1998, 2005 Peter Zelezny, Michael Hotaling <Mike.Hotaling@SinisterDevelopments.com>
 *
 * X-Tray is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * X-Tray is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with X-Tray; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <tchar.h>

#include "hexchat-plugin.h"
#include "hextray.h"
#include "resource.h"
#include "hexchat.h"
#include "utility.h"

// from util.c of HexChat source code ( slightly modified to fit HexTray Syntax )
char *hexchat_strip_color (char *text)
{
	int nc	= 0;
	int i	= 0;
	int col	= 0;
	int len	= strlen(text);
	char *new_str = (char *)malloc(len + 2);

	while (len > 0)
	{
		if ((col && isdigit(*text) && (nc < 2)) || (col && isdigit(*(text+1)) && (nc < 3) && (*text == ',')))
		{
			nc++;

			if(*text == ',')
			{
				nc = 0;
			}
		}
		else
		{
			col = 0;

			switch (*text)
			{
			case '\003':			  /*ATTR_COLOR: */
				{
					col = 1;
					nc = 0;
				}
				break;
			case '\007':			  /*ATTR_BEEP: */
			case '\017':			  /*ATTR_RESET: */
			case '\026':			  /*ATTR_REVERSE: */
			case '\002':			  /*ATTR_BOLD: */
			case '\037':			  /*ATTR_UNDERLINE: */
				break;
			default:
				{
					new_str[i] = *text;
					i++;
				}
				break;
			}
		}

		text++;
		len--;
	}

	new_str[i] = 0;

	return new_str;
}

void check_special_chars (char *cmd)
{
	int occur	= 0;
	int len		= strlen (cmd);
	int i = 0, j = 0;
	char *buf;

	if (!len)
		return;

	buf = (char *)malloc (len + 1);

	if (buf)
	{
		while (cmd[j])
		{
			switch (cmd[j])
			{
			case '%':
				{
					occur++;

					switch (cmd[j + 1])
					{
					case 'R':
						buf[i] = '\026';
						break;
					case 'U':
						buf[i] = '\037';
						break;
					case 'B':
						buf[i] = '\002';
						break;
					case 'C':
						buf[i] = '\003';
						break;
					case 'O':
						buf[i] = '\017';
						break;
					case '%':
						buf[i] = '%';
						break;
					default:
						buf[i] = '%';
						j--;
						break;
					}

					j++;
				}
				break;
			default:
				{
					buf[i] = cmd[j];
				}
				break;
			}

			j++;
			i++;
		}

		buf[i] = 0;

		if (occur)
			strcpy (cmd, buf);

		free (buf);
	}
}

void hexchat_globally_away(TCHAR *tszAway)
{
	char szTemp[512];
	char szAway[512];

	ConvertString(tszAway, szAway, 512);
	_snprintf(szTemp, 512, "ALLSERV AWAY %s\0", szAway);
	check_special_chars(szTemp);
	hexchat_exec(szTemp);
}

void hexchat_away(TCHAR *tszAway)
{
	char szTemp[512];
	char szAway[512];

	ConvertString(tszAway, szAway, 512);
	_snprintf(szTemp, 512, szAway);
	check_special_chars(szTemp);
	hexchat_commandf(ph, "AWAY %s\0", szTemp);
}

void hexchat_globally_back()
{
	std::vector<int> xs;
	std::vector<int>::iterator xsi;
	hexchat_list *xl = hexchat_list_get(ph, "channels");

	if(xl)
	{
		while(hexchat_list_next(ph, xl))
		{
			xsi = std::find(xs.begin(), xs.end(), hexchat_list_int(ph, xl, "id"));

			if((xsi == xs.end()) &&
				((strlen(hexchat_list_str(ph, xl, "server")) > 0) || 
				(strlen(hexchat_list_str(ph, xl, "channel")) > 0)))
			{
				xs.push_back(hexchat_list_int(ph, xl, "id"));
				hexchat_set_context(ph, (hexchat_context *)hexchat_list_str(ph, xl, "context"));
				hexchat_back();
			}
		}

		hexchat_list_free(ph, xl);
	}
}



void hexchat_back()
{
	if(hexchat_get_info(ph, "away"))
	{
		hexchat_command(ph, "BACK");
	}
}

HMENU setServerMenu()
{
	HMENU sTemp = CreateMenu();
	TCHAR wszServer[128];
	TCHAR wszNick[128];
	TCHAR wszMenuEntry[256];

	std::vector<int> xs;
	std::vector<int>::iterator xsi;
	hexchat_list *xl = hexchat_list_get(ph, "channels");

	AppendMenu(sTemp, MF_STRING, ACT_AWAY, _T("Set Globally Away"));
	AppendMenu(sTemp, MF_STRING, ACT_BACK, _T("Set Globally Back"));
	AppendMenu(sTemp, MF_SEPARATOR, 0, NULL);

	if(xl)
	{
		while(hexchat_list_next(ph, xl))
		{
			xsi = std::find(xs.begin(), xs.end(), hexchat_list_int(ph, xl, "id"));

			if( (xsi == xs.end()) &&
				((strlen(hexchat_list_str(ph, xl, "server")) > 0) || 
				(strlen(hexchat_list_str(ph, xl, "channel")) > 0)))
			{
				hexchat_set_context(ph, (hexchat_context *)hexchat_list_str(ph, xl, "context"));
				xs.push_back(hexchat_list_int(ph, xl, "id"));

				char *network	= _strdup(hexchat_list_str(ph, xl, "network"));
				char *server	= _strdup(hexchat_list_str(ph, xl, "server"));
				char *nick		= _strdup(hexchat_get_info(ph, "nick"));

				if(network != NULL)
				{
					ConvertString(network, wszServer, 128);
				}
				else
				{
					ConvertString(server, wszServer, 128);
				}

				if(server != NULL)
				{
					ConvertString(nick, wszNick, 128);
					_sntprintf(wszMenuEntry, 256, _T("%s @ %s\0"), wszNick, wszServer);

					if(!hexchat_get_info(ph, "away"))
					{
						AppendMenu(sTemp, MF_STRING, (hexchat_list_int(ph, xl, "id") + 1), wszMenuEntry);
					}
					else
					{
						AppendMenu(sTemp, (MF_CHECKED | MF_STRING), (hexchat_list_int(ph, xl, "id") + 1), wszMenuEntry);							
					}
				}

				free(network);
				free(server);
				free(nick);
			}
		}

		hexchat_list_free(ph, xl);
	}

	return sTemp;
}

struct _hexchat_context *hexchat_find_server(int find_id)
{
	hexchat_context *xc;
	hexchat_list *xl = hexchat_list_get(ph, "channels");
	int id;

	if(!xl)
		return NULL;

	while(hexchat_list_next(ph, xl))
	{
		id = hexchat_list_int(ph, xl, "id");
		
		if(id == -1)
		{
			return NULL;
		}
		else if(id == find_id)
		{
			xc = (hexchat_context *)hexchat_list_str(ph, xl, "context");
			
			hexchat_list_free(ph, xl);

			return xc;
		}
	}

	hexchat_list_free(ph, xl);

	return NULL;
}

void hexchat_exec(char *command)
{
	hexchat_set_context(ph, hexchat_find_context(ph, NULL, NULL));
	hexchat_command(ph, command);
}
