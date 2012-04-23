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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

#if defined( WIN32 ) && defined( TUNE )
  #include <crtdbg.h>
  _CrtMemState startMemState;
  _CrtMemState endMemState;
#endif

#include "tinyxml.h"
#include <string>
#include <map>
#include "xbmclangcodes.h"
#include "ctime"

#ifdef _MSC_VER
  std::string DirSepChar = "\\";
  #include "dirent.h"
#else
  std::string DirSepChar = "/";
  #include <dirent.h>
#endif

FILE * pPOTFile;
char* pSourceDirectory = NULL;
char* pProjectName =NULL;
char* pVersionNumber = NULL;

std::string WorkingDir;

TiXmlDocument xmlDocSourceInput;
TiXmlDocument xmlDocForeignInput;

class CCommentEntry
{
public:
  std::string text;
  bool bInterLineComment;
};

std::multimap<std::string, int> multimapSourceXmlStrings;
std::map<int, std::string> mapSourceXmlId;
std::map<int, std::string>::iterator itSourceXmlId;
std::map<int, std::string> mapForeignXmlId;
std::map<int, std::string>::iterator itForeignXmlId;
std::multimap<int, CCommentEntry> mapComments;
std::multimap<int, CCommentEntry>::iterator itComments;
std::pair<std::multimap<int, CCommentEntry>::iterator, std::multimap<int, CCommentEntry>::iterator> itRangeComments;

// remove trailing and leading whitespaces
std::string UnWhitespace(std::string strInput)
{
  int offset_end = strInput.size();
  int offset_start = 0;

  while (strInput[offset_start] == ' ')
    offset_start++; // check first non-whitespace char
  while (strInput[offset_end-1] == ' ')
    offset_end--; // check last non whitespace char

  strInput = strInput.substr(offset_start, offset_end - offset_start);
  return strInput;
}

void GetComment(const TiXmlNode *pCommentNode, int id)
{
  int nodeType;
  while (pCommentNode)
  {
    nodeType = pCommentNode->Type();
    if (nodeType == TiXmlNode::TINYXML_ELEMENT)
      break;
    if (nodeType == TiXmlNode::TINYXML_COMMENT)
    {
      CCommentEntry ce;
      ce.text = UnWhitespace(pCommentNode->Value());
      ce.bInterLineComment = pCommentNode->m_CommentLFPassed;
      // we store if the comment was in a new line or not

      mapComments.insert(std::pair<int,CCommentEntry>(id, ce));
    }
    pCommentNode = pCommentNode->NextSibling();
  }
}

std::string EscapeLF(const char * StrToEscape)
{
  std::string strIN(StrToEscape);
  std::string strOut;
  std::string::iterator it;
  for (it = strIN.begin(); it != strIN.end(); it++)
  {
    if (*it == '\n')
    {
      strOut.append("\\n");
      continue;
    }
    if (*it == '\r')
      continue;
    strOut += *it;
  }
  return strOut;
}

bool loadXMLFile (TiXmlDocument &pXMLDoc, std::string XMLFilename, std::map<int,
                  std::string> * pMapXmlStrings, bool isSourceFile)
{
  if (!pXMLDoc.LoadFile(XMLFilename.c_str()))
  {
    printf ("%s %s\n", pXMLDoc.ErrorDesc(), XMLFilename.c_str());
    return false;
  }

  TiXmlElement* pRootElement = pXMLDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="strings")
  {
    printf ("error: No root element called: \"strings\" or no child found in input XML file: %s\n",
            XMLFilename.c_str());
    return false;
  }

  if (isSourceFile) GetComment(pRootElement->FirstChild(), -1);

  const TiXmlElement *pChildElement = pRootElement->FirstChildElement("string");
  const char* pAttrId = NULL;
  const char* pValue = NULL;
  std::string valueString;
  int id;

  while (pChildElement)
  {
    pAttrId=pChildElement->Attribute("id");
    if (pAttrId && !pChildElement->NoChildren())
    {
      id = atoi(pAttrId);
      if (pMapXmlStrings->find(id) == pMapXmlStrings->end())
      {
        pValue = pChildElement->FirstChild()->Value();
        valueString = EscapeLF(pValue);
        if (isSourceFile)
          multimapSourceXmlStrings.insert(std::pair<std::string,int>( valueString,id));

        (*pMapXmlStrings)[id] = valueString;

        if (pChildElement && isSourceFile) GetComment(pChildElement->NextSibling(), id);
      }
    }
    pChildElement = pChildElement->NextSiblingElement("string");
  }
  // Free up the allocated memory for the XML file
  pXMLDoc.Clear();
  return true;
}

bool WriteComments(int comm_id, bool bInterLine)
{
  bool bHadCommWrite = false;
  itComments = mapComments.find(comm_id);
  if (itComments != mapComments.end()) // check if we have at least one comment for the xml id
  {
    itRangeComments = mapComments.equal_range(comm_id);
    for (itComments = itRangeComments.first; itComments != itRangeComments.second;
         ++itComments)
         {
           if (bInterLine == itComments->second.bInterLineComment)
           {
             bHadCommWrite = true;
             fprintf(pPOTFile,bInterLine ? "#%s\n": "#. %s\n", itComments->second.text.c_str());
           }
         }
  }
  return bHadCommWrite;
}

void PrintUsage()
{
  printf
  (
  "Usage: xbmc-xml2po -s <sourcedirectoryname> (-p <projectname>) (-v <version>)\n"
  "parameter -s <name> for source root language directory, which contains the language dirs\n"
  "parameter -p <name> for project name, eg. xbmc.skin.confluence\n"
  "parameter -v <name> for project version, eg. FRODO_GIT251373f9c3\n\n"
  );
#ifdef _MSC_VER
  printf
  (
  "Note for Windows users: In case you have whitespace or any special character\n"
  "in any of the arguments, please use apostrophe around them. For example:\n"
  "xbmc-xml2po.exe -s \"C:\\xbmc dir\\language\" -p xbmc.core -v Frodo_GIT\n"
  );
#endif
return;
}

std::string GetCurrTime()
{
  std::string strTime(64, '\0');
  time_t now = std::time(0);
  struct std::tm* gmtm = std::gmtime(&now);

  if (gmtm != NULL)
  {
    sprintf(&strTime[0], "%04i-%02i-%02i %02i:%02i+0000", gmtm->tm_year + 1900, gmtm->tm_mon + 1,
            gmtm->tm_mday, gmtm->tm_hour, gmtm->tm_min);
  }
  return strTime;
}

bool  ConvertXML2PO(std::string LangDir, std::string LCode, bool bIsForeignLang)
{
  int contextCount = 0;
  int stringCountSource = 0;
  int stringCountForeign = 0;
  std::string  OutputPOFilename;

  OutputPOFilename = LangDir + "strings.po";
  
  // Initalize the output po document
  pPOTFile = fopen (OutputPOFilename.c_str(),"wb");
  if (pPOTFile == NULL)
  {
    printf("Error opening output file: %s\n", OutputPOFilename.c_str());
    return false;
  }
  printf("%s\t\t", LCode.c_str()); 

  fprintf(pPOTFile,
    "# XBMC Media Center language file\n"
    "msgid \"\"\n"
    "msgstr \"\"\n"
    "\"Project-Id-Version: %s-%s\\n\"\n"
    "\"Report-Msgid-Bugs-To: alanwww1@xbmc.org\\n\"\n"
    "\"POT-Creation-Date: %s\\n\"\n"
    "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
    "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
    "\"Language-Team: LANGUAGE\\n\"\n"
    "\"Language: %s\\n\"\n"
    "\"MIME-Version: 1.0\\n\"\n"
    "\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
    "\"Content-Transfer-Encoding: 8bit\\n\"\n\n",
    (pProjectName != NULL) ? pProjectName : "xbmc-unnamed",
    (pVersionNumber != NULL) ?  pVersionNumber : "rev_unknown",
    GetCurrTime().c_str(),
    (!LCode.empty()) ? LCode.c_str() : "LANGUAGE");

  int previd = -1;
  bool bCommentWritten = false;
  for (itSourceXmlId = mapSourceXmlId.begin(); itSourceXmlId != mapSourceXmlId.end(); itSourceXmlId++)
  {
    int id = itSourceXmlId->first;
    std::string value = itSourceXmlId->second;

    //create comment lines, if empty string id or ids found and
    //re-create original xml comments between entries. Only for the source language
    bCommentWritten = false;
    if (!bIsForeignLang) bCommentWritten = WriteComments(previd, true);

    if ((id-previd >= 2) && !bIsForeignLang)
    {
      if (id-previd == 2 && previd > -1)
        fprintf(pPOTFile,"#empty string with id %i\n", id-1);
      if (id-previd > 2 && previd > -1)
        fprintf(pPOTFile,"#empty strings from id %i to %i\n", previd+1, id-1);
      bCommentWritten = true;
    }
    if (bCommentWritten) fprintf(pPOTFile, "\n");

    //create comment, including string id
    fprintf(pPOTFile,"#: id:%i\n", id);

    //write comment originally placed next to the string entry
    //convert it into #. style gettext comment
    if (!bIsForeignLang) WriteComments(id, false);

    if (multimapSourceXmlStrings.count(value) > 1) // if we have multiple IDs for the same string value
    {
      //create autogenerated context message for multiple msgid entries
      fprintf(pPOTFile,"msgctxt \"Auto context with id %i\"\n", id);
      contextCount++;
    }
    //create msgid and msgstr lines
    fprintf(pPOTFile,"msgid \"%s\"\n", value.c_str());
    if (bIsForeignLang)
    {
      itForeignXmlId = mapForeignXmlId.find(id);
      if (itForeignXmlId != mapForeignXmlId.end())
      {
        stringCountForeign++;
        fprintf(pPOTFile,"msgstr \"%s\"\n\n", itForeignXmlId->second.c_str());
      }
      else fprintf(pPOTFile,"msgstr \"\"\n\n");
    }
    else fprintf(pPOTFile,"msgstr \"\"\n\n");

    stringCountSource++;
    previd =id;
  }
  fclose(pPOTFile);

  printf("%i\t\t", bIsForeignLang ? stringCountForeign : stringCountSource);
  printf("%i\t\t", contextCount);
  printf("%s\n", OutputPOFilename.erase(0,WorkingDir.length()).c_str());

  mapForeignXmlId.clear();
  return true;
}

int main(int argc, char* argv[])
{
  // Basic syntax checking for "-x name" format
  while ((argc > 2) && (argv[1][0] == '-') && (argv[1][1] != '\x00') && (argv[1][2] == '\x00') &&
         (argv[2][0] != '\x00') && (argv[2][0] != '-'))
  {
    switch (argv[1][1])
    {
      case 's':
        --argc; ++argv;
        pSourceDirectory = argv[1];
        break;
      case 'p':
        --argc; ++argv;
        pProjectName = argv[1];
        break;
      case 'v':
        --argc; ++argv;
        pVersionNumber = argv[1];
        break;
    }
    ++argv; --argc;
  }

  if (pSourceDirectory == NULL)
  {
    printf("\nWrong Arguments given !\n");
    PrintUsage();
    return 1;
  }

  printf("\nXBMC-XML2PO by Team XBMC\n");
  printf("\nResults:\n\n");
  printf("Langcode\tString match\tAuto contexts\tOutput file\n");
  printf("--------------------------------------------------------------\n");

  WorkingDir = pSourceDirectory;
  if (WorkingDir[0] != DirSepChar[0]) WorkingDir.append(DirSepChar);

  if (!loadXMLFile(xmlDocSourceInput, WorkingDir + "English" + DirSepChar + "strings.xml",
      &mapSourceXmlId, true))
  {
    printf("Fatal error: no English source xml file found or it is corrupted\n");
    return 1;
  }
  ConvertXML2PO(WorkingDir + "English" + DirSepChar, "en", false);

  DIR* Dir;
  struct dirent *DirEntry;
  Dir = opendir(WorkingDir.c_str());
  int langcounter =0;

  while((DirEntry=readdir(Dir)))
  {
    std::string LName = "";
    if (DirEntry->d_type == DT_DIR && DirEntry->d_name[0] != '.' && strcmp(DirEntry->d_name, "English"))
    {
      if (loadXMLFile(xmlDocForeignInput, WorkingDir + DirEntry->d_name + DirSepChar + "strings.xml",
          &mapForeignXmlId, false))
      {
        ConvertXML2PO(WorkingDir + DirEntry->d_name + DirSepChar, FindLangCode(DirEntry->d_name).c_str(), true);
        langcounter++;
      }
    }
  }

  printf("\nReady. Processed %i languages.\n", langcounter+1);

  if (bUnknownLangFound)
    printf("\nWarning: At least one language found with unpaired language code !\n"
      "Please edit the .po file manually and correct the language code !\n"
      "Also please report this to alanwww1@xbmc.org if possible !\n\n");
  return 0;
}
