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
#include "iostream"
#include <map>

  int main(int argc, char* argv[])
  {
    char* pSourceXMLFilename = NULL;
    char* pOutputPOFilename = NULL;
    char* pForeignXMLFilename = NULL;

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
      }
      ++argv; --argc;
    }
  if ((pSourceXMLFilename == NULL) || (pOutputPOFilename == NULL))
  {
    printf("Wrong Arguments given !");
    printf("Usage: xbmc-xml2po -s <name> -o <name> (-f <name>)\n");
    printf("parameter -s <name> for source input English XML filename\n");
    printf("parameter -o <name> for output PO or POT filename\n");
    printf("parameter -f <name> for foreign input XML filename\n");
    return 1;
  }
  bool bHasForeignInput = pForeignXMLFilename != NULL;

  TiXmlDocument xmlDocSourceInput;
  if (!xmlDocSourceInput.LoadFile(pSourceXMLFilename))
  {
    std::cout << xmlDocSourceInput.ErrorDesc() << " " << pSourceXMLFilename << std::endl;
    return 1;
  }

  // Check if we really have "strings" root element and we have at least one child element
  TiXmlElement* pRootElementSourceInput = xmlDocSourceInput.RootElement();
  if (!pRootElementSourceInput || pRootElementSourceInput->NoChildren() ||
            pRootElementSourceInput->ValueTStr()!="strings")
  {
    std::cout << "error: No root element called: strings or no child found in source input XML" << std::endl;
    return 1;
  }

  std::multimap<std::string, int> mapXmlData;

  const TiXmlElement *pChildSourceInput = pRootElementSourceInput->FirstChildElement("string");
  const char* pAttrIdSourceInput = NULL;
  const char* pValueSourceInput = NULL;
  while (pChildSourceInput)
  {
    pAttrIdSourceInput=pChildSourceInput->Attribute("id");
    if (pAttrIdSourceInput && !pChildSourceInput->NoChildren())
    {
      int id = atoi(pAttrIdSourceInput);
      pValueSourceInput = pChildSourceInput->FirstChild()->Value();
      mapXmlData.insert(std::pair<std::string,int>( pValueSourceInput,id));
    }
    pChildSourceInput = pChildSourceInput->NextSiblingElement("string");
  }

  // Initalize the output xml document
  FILE * pPOTFile;
  pPOTFile = fopen (pOutputPOFilename,"w");
  if (pPOTFile == NULL) return 1;
  fprintf(pPOTFile,"# Converted from xbmc strings.xml &amp;");

  pChildSourceInput = pRootElementSourceInput->FirstChildElement("string");
  pAttrIdSourceInput = NULL;
  pValueSourceInput = NULL;
  int previd = -1;
  while (pChildSourceInput)
  {
    pAttrIdSourceInput=pChildSourceInput->Attribute("id");
    if (pAttrIdSourceInput && !pChildSourceInput->NoChildren())
    {
      int id=atoi(pAttrIdSourceInput);
      pValueSourceInput = pChildSourceInput->FirstChild()->Value();

      //create comment note if empty string or strings ids found
      if (previd !=-1 && (id-previd >= 2))
      {
        if (id-previd == 2) fprintf(pPOTFile,"#Empty string with id %i\n\n", id-1);
        if (id-previd > 2) fprintf(pPOTFile,"#Empty strings from id %i to %i\n\n", previd+1, id-1);
      }
      //create node trans-unit
      fprintf(pPOTFile,"# id:%s\n", pAttrIdSourceInput);

      if (mapXmlData.count( pValueSourceInput) > 1)        // if we have multiple IDs for the same string value
      {
        //create node context-group id with value
        fprintf(pPOTFile,"msgctxt \"Auto generated context for string id %i\"\n", id);
      }
      //create node source with value
      fprintf(pPOTFile,"msgid \"%s\"\n", pValueSourceInput);
      fprintf(pPOTFile,"msgstr \"\"\n\n");
      // if we have multiple IDs for the same string value, we create a context node
      previd =id;
    }
    pChildSourceInput = pChildSourceInput->NextSiblingElement("string");
  }

  fclose(pPOTFile);
  // Free up the allocated memory for the XML file
  xmlDocSourceInput.Clear();

  return 0;
}
