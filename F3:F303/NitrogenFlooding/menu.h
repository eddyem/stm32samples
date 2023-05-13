/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>

struct _menu_;
typedef struct{
    const char *text;                   // name of menu item
    struct _menu_ *submenu;             // submenu if selected
    void (*action)(struct _menu_ *top); // action when item selected (be carefull when both `submenu` and `action` !=NULL), top - parent menu
} menuitem;

typedef struct _menu_{
    struct _menu_ *parent;  // parent menu (when `ESC` pressed) or NULL if should return to main window
    int nitems;             // amount of `menuitem`
    int selected;           // selected item
    menuitem *items;        // array of `menuitem` - menu itself
} menu;

extern menu mainmenu;

// subwindow handler
typedef void (*window_handler)(uint8_t evtmask);
