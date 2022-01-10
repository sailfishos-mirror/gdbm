/* Placeholders for libintl calls
   Copyright (C) 2022 Free Software Foundation, Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.    */

char *
gettext (const char *msgid)
{
  return (char *) msgid;
}

char *
dgettext (const char *domainname, const char *msgid)
{
  return (char *) msgid;
}  

char *
dcgettext (const char *domainname, const char *msgid, int category)
{
  return (char *) msgid;
}  
  
char *
textdomain (const char *domainname)
{
  return (char *) domainname;
}

char *
bindtextdomain (const char *domainname, const char *dirname)
{
  return (char *) domainname;
}
  
