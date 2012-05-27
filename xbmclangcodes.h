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
  LANG_COUNT = 47,
};

struct CLangcodes
{
  std::string Langname;
  std::string Langcode;
  int nplurals;
  std::string Pluralform;
};

CLangcodes Langcodes [LANG_COUNT] =
{
  {"Afrikaans", "af", 2, "(n != 1)"},
  {"Arabic", "ar", 6, "n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5"},
  {"Basque", "eu", 2, "(n != 1)"},
  {"Bosnian", "bs", 3, "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)"},
  {"Bulgarian", "bg", 2, "(n != 1)"},
  {"Catalan", "ca", 2, "(n != 1)"},
  {"Chinese (Simple)", "zh", 1, "0"},
  {"Chinese (Traditional)", "zh_CN", 1, "0"},
  {"Croatian", "hr", 3, "n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2"},
  {"Czech", "cs", 3, "(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2"},
  {"Danish", "da", 2, "(n != 1)"},
  {"Dutch", "nl", 2, "(n != 1)"},
  {"English", "en", 2, "(n != 1)"},
  {"English (US)", "en_US", 2, "(n != 1)"},
  {"Esperanto", "eo", 2, "(n != 1)"},
  {"Finnish", "fi", 2, "(n != 1)"},
  {"French", "fr", 2, "(n > 1)"},
  {"German", "de", 2, "(n != 1)"},
  {"Greek", "el", 2, "(n != 1)"},
  {"Hebrew", "he", 2, "(n != 1)"},
  {"Hindi (Devanagiri)", "hi", 2, "(n != 1)"},
  {"Hungarian", "hu", 2, "(n != 1)"},
  {"Icelandic", "is", 2, "(n != 1)"},
  {"Indonesian", "id", 1, "0"},
  {"Italian", "it", 2, "(n != 1)"},
  {"Japanese", "ja", 1, "0"},
  {"Korean", "ko", 1, "0"},
  {"Lithuanian", "lt", 3, "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2)"},
  {"Maltese", "mt", 4, "(n==1 ? 0 : n==0 || ( n%100>1 && n%100<11) ? 1 : (n%100>10 && n%100<20 ) ? 2 : 3)"},
  {"Norwegian", "no", 2, "(n != 1)"},
  {"Polish", "pl", 3, "(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)"},
  {"Portuguese", "pt", 2, "(n != 1)"},
  {"Portuguese (Brazil)", "pt_BR", 2, "(n > 1)"},
  {"Romanian", "ro", 3, "(n==1?0:(((n%100>19)||((n%100==0)&&(n!=0)))?2:1))"},
  {"Russian", "ru", 3, "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)"},
  {"Northern Sami", "se", 2, "(n != 1)"},
  {"Serbian", "sr", 3, "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)"},
  {"Serbian (Cyrillic)", "sr_RS", 3, "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)"},
  {"Slovak", "sk", 3, "(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2"},
  {"Slovenian", "sl", 4, "(n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3)"},
  {"Spanish", "es", 2, "(n != 1)"},
  {"Spanish (Mexico)", "es_MX", 2, "(n != 1)"},
  {"Swedish", "sv", 2, "(n != 1)"},
  {"Thai", "th", 1, "0"},
  {"Turkish", "tr", 1, "0"},
  {"Ukrainian", "uk", 3, "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)"}
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

std::string FindLang(std::string LangCode)
{
  for (int i=0; i<LANG_COUNT ; i++)
  {
    if (LangCode == Langcodes[i].Langcode) return Langcodes[i].Langname;
  }
  printf ("unknown language code found in addon.xml data. Language Code: %s", LangCode.c_str());
  return "UNRECOGNIZED_" + LangCode;
}

int GetnPlurals(std::string LangToLook)
{
  for (int i=0; i<LANG_COUNT ; i++)
  {
    if (LangToLook == Langcodes[i].Langname)
      return Langcodes[i].nplurals;
  }
  return 2;
}

std::string GetPlurForm(std::string LangToLook)
{
  for (int i=0; i<LANG_COUNT ; i++)
  {
    if (LangToLook == Langcodes[i].Langname)
      return Langcodes[i].Pluralform;
  }
  return "(n != 1)";
}
