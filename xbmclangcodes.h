/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "string"

#define MAXLANGCOUNT 2;

enum
{
  LANG_COUNT = 46,
};

struct CLangcodes
{
  std::string Langname;
  std::string Langcode;
};

CLangcodes Langcodes [LANG_COUNT] =
{
  {"Afrikaans", "af"},
  {"Arabic", "ar"},
  {"Basque", "eu"},
  {"Bosnian", "bs"},
  {"Bulgarian", "bg"},
  {"Catalan", "ca"},
  {"Chinese (Simple)", "zh"},
  {"Chinese (Traditional)", "zh_CN"},
  {"Croatian", "hr"},
  {"Czech", "cs"},
  {"Danish", "da"},
  {"Dutch", "nl"},
  {"English", "en"},
  {"English (US)", "en_US"},
  {"Esperanto", "eo"},
  {"Finnish", "fi"},
  {"French", "fr"},
  {"German", "de"},
  {"Greek", "el"},
  {"Hebrew", "he"},
  {"Hindi (Devanagiri)", "hi"},
  {"Hungarian", "hu"},
  {"Icelandic", "is"},
  {"Indonesian", "id"},
  {"Italian", "it"},
  {"Japanese", "ja"},
  {"Korean", "ko"},
  {"Lithuanian", "lt"},
  {"Maltese", "mt"},
  {"Norwegian", "no"},
  {"Polish", "pl"},
  {"Portuguese", "pt"},
  {"Portuguese (Brazil)", "pt_BR"},
  {"Romanian", "ro"},
  {"Russian", "ru"},
  {"Serbian", "sr"},
  {"Serbian (Cyrillic)", "sr_RS"},
  {"Slovak", "sk"},
  {"Slovenian", "sl"},
  {"Spanish", "es"},
  {"Spanish (Mexico)", "es_MX"},
  {"Swedish", "sv"},
  {"Thai", "th"},
  {"Turkish", "tr"},
  {"Ukrainian", "uk"}
};

bool bUnknownLangFound = false;

std::string FindLangCode(std::string LangToLook)
{
  for (int i=0; i<LANG_COUNT ; i++)
  {
    if (LangToLook == Langcodes[i].Langname) return Langcodes[i].Langcode;
  }
  bUnknownLangFound = true;
  return "UNKNOWN";
}


