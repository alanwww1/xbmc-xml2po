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

#include <stdio.h>

#if defined( WIN32 ) && defined( TUNE )
  #include <crtdbg.h>
  _CrtMemState startMemState;
  _CrtMemState endMemState;
#endif

#include "tinyxml.h"
#include "string"
#include <map>

FILE * pPOTFile;
char* pSourceXMLFilename = NULL;
char* pOutputPOFilename = NULL;
char* pForeignXMLFilename = NULL;
char* pProjectName =NULL;
char* pVersionNumber = NULL;
char* pLanguage =NULL;

TiXmlDocument xmlDocSourceInput;
TiXmlDocument xmlDocForeignInput;
bool bHasForeignInput = false;
std::multimap<std::string, int> mapSourceXmlStrings;
std::map<int, std::string> mapSourceXmlId;
std::map<int, std::string>::iterator itSourceXmlId;
std::map<int, std::string> mapForeignXmlId;


bool loadXMLFile (TiXmlDocument &pXMLDoc, char* pFilename, std::map<int, std::string> * pMapXmlStrings,
  bool isSourceFile)
{
  if (!pXMLDoc.LoadFile(pFilename))
  {
    printf ("%s %s\n", pXMLDoc.ErrorDesc(), pFilename);
    return false;
  }

  TiXmlElement* pRootElement = pXMLDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="strings")
  {
    printf ("error: No root element called: \"strings\" or no child found in input XML file: %s\n", pFilename);
    return false;
  }

  const TiXmlElement *pChildElement = pRootElement->FirstChildElement("string");
  const char* pAttrId = NULL;
  const char* pValue = NULL;
  while (pChildElement)
  {
    pAttrId=pChildElement->Attribute("id");
    if (pAttrId && !pChildElement->NoChildren())
    {
      int id = atoi(pAttrId);
      pValue = pChildElement->FirstChild()->Value();
      mapSourceXmlStrings.insert(std::pair<std::string,int>( pValue,id));
      (*pMapXmlStrings)[id] = pValue;
    }
    pChildElement = pChildElement->NextSiblingElement("string");
  }
  return true;
}

int main(int argc, char* argv[])
{
  // Basic syntax checking for "-x name" format
  while ((argc > 2) && (argv[1][0] == '-') && (argv[1][1] != '\x00') && (argv[1][2] == '\x00')
    && (argv[2][0] != '\x00') && (argv[2][0] != '-'))
  {
    switch (argv[1][1])
    {
    case 's':
      --argc; ++argv;
      pSourceXMLFilename = argv[1];
      break;
    case 'o':
      --argc; ++argv;
      pOutputPOFilename = argv[1];
      break;
    case 'f':
      --argc; ++argv;
      pForeignXMLFilename = argv[1];
      break;
    case 'p':
      --argc; ++argv;
      pProjectName = argv[1];
      break;
    case 'v':
      --argc; ++argv;
      pVersionNumber = argv[1];
      break;
    case 'l':
      --argc; ++argv;
      pLanguage = argv[1];
      break;
    }
    ++argv; --argc;
  }
  if ((pSourceXMLFilename == NULL) || (pOutputPOFilename == NULL))
  {
    printf("Wrong Arguments given !\n");
    printf("Usage: xbmc-xml2po -s <name> -o <name> (-f <name>)\n");
    printf("parameter -s <name> for source input English XML filename\n");
    printf("parameter -o <name> for output PO or POT filename\n");
    printf("parameter -f <name> for foreign input XML filename\n");
    return 1;
  }
  bHasForeignInput = pForeignXMLFilename != NULL;

  if (!loadXMLFile(xmlDocSourceInput, pSourceXMLFilename, &mapSourceXmlId, true)) return 1;

  // Initalize the output xml document
  pPOTFile = fopen (pOutputPOFilename,"wb");
  if (pPOTFile == NULL) return 1;
  fprintf(pPOTFile,
  "# Converted from xbmc strings.xml with xbmc-xml2po coded by Team-XBMC\n"
  "msgid \"\"\n"
  "msgstr \"\"\n"
  "\"Project-Id-Version: %s-%s\\n\"\n"
  "\"Report-Msgid-Bugs-To: alanwww1@xbmc.org\\n\"\n"
  "\"POT-Creation-Date: 2012-02-15 19:43+0100\\n\"\n"
  "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
  "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
  "\"Language-Team: LANGUAGE\\n\"\n"
  "\"Language: %s\\n\"\n"
  "\"MIME-Version: 1.0\\n\"\n"
  "\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
  "\"Content-Transfer-Encoding: 8bit\\n\"\n\n"
  ,pProjectName, pVersionNumber, pLanguage);

  int previd = -1;
  for (itSourceXmlId = mapSourceXmlId.begin(); itSourceXmlId != mapSourceXmlId.end(); itSourceXmlId++)
  {
    int id = itSourceXmlId->first;
    std::string value = itSourceXmlId->second;

    //create comment note if empty string or strings ids found
    if (previd !=-1 && (id-previd >= 2))
    {
      if (id-previd == 2) fprintf(pPOTFile,"#Empty string with id %i\n\n", id-1);
      if (id-previd > 2) fprintf(pPOTFile,"#Empty strings from id %i to %i\n\n", previd+1, id-1);
    }
    //create node trans-unit
    fprintf(pPOTFile,"# id:%i\n", id);

    if (mapSourceXmlStrings.count(value) > 1)        // if we have multiple IDs for the same string value
    {
      //create node context-group id with value
      fprintf(pPOTFile,"msgctxt \"Auto generated context for string id %i\"\n", id);
    }
    //create node source with value
    fprintf(pPOTFile,"msgid \"%s\"\n", value.c_str());
    fprintf(pPOTFile,"msgstr \"\"\n\n");
    // if we have multiple IDs for the same string value, we create a context node
    previd =id;
  }

  fclose(pPOTFile);
  // Free up the allocated memory for the XML file
//  xmlDocSourceInput.Clear();

  return 0;
}
