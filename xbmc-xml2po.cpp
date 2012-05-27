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
#include "CharsetUtils.h"
#include <string>
#include <map>
#include "xbmclangcodes.h"
#include "ctime"
#include <algorithm>
#include <sys/stat.h>

enum
{
  SKIN = 0,
  ADDON = 1,
  CORE = 2,
  ADDON_NOSTRINGS = 3,
  UNKNOWN = 4
};

struct CLangData
{
  std::string LLangCode;
  int LPlurals;
  std::string LPlurForm;
};

#ifdef _MSC_VER
  std::string DirSepChar = "\\";
  #include "dirent.h"
#else
  std::string DirSepChar = "/";
  #include <dirent.h>
#endif

FILE * pPOTFile;
char* pSourceDirectory = NULL;

std::string WorkingDir;
std::string ProjRootDir;
std::string ProjName;
std::string ProjVersion;
std::string ProjTextName;
std::string ProjProvider;
int projType;
bool bhasLFWritten;

TiXmlDocument xmlDocSourceInput;
TiXmlDocument xmlDocForeignInput;

class CCommentEntry
{
public:
  std::string text;
  bool bInterLineComment;
};

struct CAddonXMLEntry
{
  std::string strSummary;
  std::string strDescription;
  std::string strDisclaimer;
};

std::map<int, std::string> mapSourceXmlId;
std::map<int, std::string>::iterator itSourceXmlId;
std::map<int, std::string> mapForeignXmlId;
std::map<int, std::string>::iterator itForeignXmlId;
std::map<std::string, CAddonXMLEntry> mapAddonXMLData;
std::map<std::string, CAddonXMLEntry>::iterator itAddonXMLData;
std::multimap<int, CCommentEntry> mapComments;
std::multimap<int, CCommentEntry>::iterator itComments;
std::pair<std::multimap<int, CCommentEntry>::iterator, std::multimap<int, CCommentEntry>::iterator> itRangeComments;
std::string sourceXMLEncoding;
std::string foreignXMLEncoding;
std::string addonXMLEncoding;

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

void WriteLF(FILE* pfile)
{
  if (!bhasLFWritten)
  {
    bhasLFWritten = true;
    fprintf (pfile, "\n");
  }
};

int MakeDir(std::string Path)
{
  return mkdir(Path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
};

bool DirExists(std::string Path)
{
  #ifdef _MSC_VER
    return (INVALID_FILE_ATTRIBUTES == GetFileAttributes(Path) && GetLastError()==ERROR_FILE_NOT_FOUND);
  #else
    struct stat st;
    return (stat(Path.c_str(), &st) == 0);
  #endif
};

std::string RemoveSlash(std::string strIn)
{
  if (strIn[strIn.size()-1] != DirSepChar[0])
    return strIn;
  return strIn.substr(0,strIn.size()-2);
}

std::string AddSlash(std::string strIn)
{
  if (strIn[strIn.size()-1] == DirSepChar[0])
    return strIn;
  return strIn + DirSepChar;
}

std::string GetDirAtLevel(std::string strIn, int Level)
{
  if (Level == 0)
    return strIn;
  for (int i=0; i<Level;i++)
  {
    strIn = strIn.substr(0, RemoveSlash(strIn).find_last_of(DirSepChar[0]) +1);
  }
  return strIn;
}

bool FileExist(std::string filename) 
{
  FILE* pfileToTest = fopen (filename.c_str(),"rb");
  if (pfileToTest == NULL)
    return false;
  fclose(pfileToTest);
  return true;
}

bool LoadCoreVersion(std::string filename)
{
  std::string strBuffer;
  FILE * file;

  file = fopen(filename.c_str(), "rb");
  if (!file)
    return false;

  fseek(file, 0, SEEK_END);
  int64_t fileLength = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (fileLength < 10)
    {
      fclose(file);
      printf("non valid length found for GUIInfoManager file");
      return false;
    }

  strBuffer.resize(fileLength+1);

  unsigned int readBytes =  fread(&strBuffer[1], 1, fileLength, file);
  fclose(file);

  if (readBytes != fileLength)
  {
    printf("actual read data differs from file size, for GUIInfoManager file");
    return false;
  }
  size_t startpos = strBuffer.find("#define VERSION_MAJOR ") + 22;
  size_t endpos = strBuffer.find_first_of(" \n\r", startpos);
  ProjVersion = strBuffer.substr(startpos, endpos-startpos);
  ProjVersion += ".";

  startpos = strBuffer.find("#define VERSION_MINOR ") + 22;
  endpos = strBuffer.find_first_of(" \n\r", startpos);
  ProjVersion += strBuffer.substr(startpos, endpos-startpos);

  startpos = strBuffer.find("#define VERSION_TAG \"") + 21;
  endpos = strBuffer.find_first_of(" \n\r\"", startpos);
  ProjVersion += strBuffer.substr(startpos, endpos-startpos);
  return true;
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

/*!
 * Returns true if the encoding of the document is other then UTF-8.
 * /param strEncoding Returns the encoding of the document. Empty if UTF-8
 */
bool GetEncoding(const TiXmlDocument* pDoc, std::string& strEncoding)
{
  const TiXmlNode* pNode=NULL;
  while ((pNode=pDoc->IterateChildren(pNode)) && pNode->Type()!=TiXmlNode::TINYXML_DECLARATION) {}
  if (!pNode) return false;
  const TiXmlDeclaration* pDecl=pNode->ToDeclaration();
  if (!pDecl) return false;
  strEncoding=pDecl->Encoding();
  if (strEncoding.compare("UTF-8") ==0 || strEncoding.compare("UTF8") == 0 ||
    strEncoding.compare("utf-8") ==0 || strEncoding.compare("utf8") == 0)
    strEncoding.clear();
  std::transform(strEncoding.begin(), strEncoding.end(), strEncoding.begin(), ::toupper);
  return !strEncoding.empty(); // Other encoding then UTF8?
}

bool loadXMLFile (TiXmlDocument &pXMLDoc, std::string XMLFilename, std::map<int,
                  std::string> * pMapXmlStrings, bool isSourceFile)
{
  if (!pXMLDoc.LoadFile(XMLFilename.c_str()))
  {
    printf ("%s %s\n", pXMLDoc.ErrorDesc(), XMLFilename.c_str());
    return false;
  }
  if (isSourceFile) GetEncoding(&pXMLDoc, sourceXMLEncoding);
    else GetEncoding(&pXMLDoc, foreignXMLEncoding);

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

bool loadAddonXMLFile (std::string AddonXMLFilename)
{
  TiXmlDocument xmlAddonXML;

  if (!xmlAddonXML.LoadFile(AddonXMLFilename.c_str()))
  {
    printf ("%s %s\n", xmlAddonXML.ErrorDesc(), (WorkingDir + "addon.xml").c_str());
    return false;
  }

  GetEncoding(&xmlAddonXML, addonXMLEncoding);

  TiXmlElement* pRootElement = xmlAddonXML.RootElement();

  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="addon")
  {
    printf ("error: No root element called: \"addon\" or no child found in AddonXML file: %s\n",
            AddonXMLFilename.c_str());
    return false;
  }
  const char* pMainAttrId = NULL;

  pMainAttrId=pRootElement->Attribute("id");
  if (!pMainAttrId)
  {
    printf ("warning: No addon name was available in addon.xml file: %s\n", AddonXMLFilename.c_str());
    ProjName = "xbmc-unnamed";
  }
  else
    ProjName = EscapeLF(pMainAttrId);

  pMainAttrId=pRootElement->Attribute("version");
  if (!pMainAttrId)
  {
    printf ("warning: No version name was available in addon.xml file: %s\n", AddonXMLFilename.c_str());
    ProjVersion = "rev_unknown";
  }
  else
    ProjVersion = EscapeLF(pMainAttrId);

  pMainAttrId=pRootElement->Attribute("name");
  if (!pMainAttrId)
  {
    printf ("warning: No addon name was available in addon.xml file: %s\n", AddonXMLFilename.c_str());
    ProjTextName = "unknown";
  }
  else
    ProjTextName = EscapeLF(pMainAttrId);

  pMainAttrId=pRootElement->Attribute("provider-name");
  if (!pMainAttrId)
  {
    printf ("warning: No addon provider was available in addon.xml file: %s\n", AddonXMLFilename.c_str());
    ProjProvider = "unknown";
  }
  else
    ProjProvider = EscapeLF(pMainAttrId);

  std::string strAttrToSearch = "xbmc.addon.metadata";

  const TiXmlElement *pChildElement = pRootElement->FirstChildElement("extension");
  while (pChildElement && strcmp(pChildElement->Attribute("point"), "xbmc.addon.metadata") != 0)
    pChildElement = pChildElement->NextSiblingElement("extension");

  const TiXmlElement *pChildSummElement = pChildElement->FirstChildElement("summary");
  while (pChildSummElement)
  {
    std::string strLang = pChildSummElement->Attribute("lang");
    if (pChildSummElement->FirstChild())
    {
      std::string strValue = EscapeLF(pChildSummElement->FirstChild()->Value());
      mapAddonXMLData[strLang].strSummary = strValue;
    }
    pChildSummElement = pChildSummElement->NextSiblingElement("summary");
  }

  const TiXmlElement *pChildDescElement = pChildElement->FirstChildElement("description");
  while (pChildDescElement)
  {
    std::string strLang = pChildDescElement->Attribute("lang");
    if (pChildDescElement->FirstChild())
    {
      std::string strValue = EscapeLF(pChildDescElement->FirstChild()->Value());
      mapAddonXMLData[strLang].strDescription = strValue;
    }
    pChildDescElement = pChildDescElement->NextSiblingElement("description");
  }

  const TiXmlElement *pChildDisclElement = pChildElement->FirstChildElement("disclaimer");
  while (pChildDisclElement)
  {
    std::string strLang = pChildDisclElement->Attribute("lang");
    if (pChildDisclElement->FirstChild())
    {
      std::string strValue = EscapeLF(pChildDisclElement->FirstChild()->Value());
      mapAddonXMLData[strLang].strDisclaimer = strValue;
    }
    pChildDisclElement = pChildDisclElement->NextSiblingElement("disclaimer");
  }

  return true;
}

void WriteComments(int comm_id, bool bInterLine)
{
  itComments = mapComments.find(comm_id);
  if (itComments == mapComments.end()) // check if we have at least one comment for the xml id
    return;

  itRangeComments = mapComments.equal_range(comm_id);
  for (itComments = itRangeComments.first; itComments != itRangeComments.second;++itComments)
  {
    if (bInterLine == itComments->second.bInterLineComment)
    {
      WriteLF(pPOTFile);
      fprintf(pPOTFile,bInterLine ? "#%s\n": "#. %s\n", itComments->second.text.c_str());
    }
  }

  return;
};

// we write str lines into the file
void WriteStrLine(std::string prefix, std::string linkedString, std::string encoding)
{
  linkedString = ToUTF8(encoding, linkedString);
  fprintf (pPOTFile, "%s", prefix.c_str());
  fprintf (pPOTFile, "\"%s\"\n", linkedString.c_str());
  return;
}

void PrintUsage()
{
  printf
  (
  "Usage: xbmc-xml2po -s <sourcedirectoryname>\n"
  "parameter -s <name> for source root directory\n\n"
  "The source directory for addons should be where the addon.xml file exists"
  "For core language file, use the main xbmc source tree as a directory"

  );
#ifdef _MSC_VER
  printf
  (
  "Note for Windows users: In case you have whitespace or any special character\n"
  "in any of the arguments, please use apostrophe around them. For example:\n"
  "xbmc-xml2po.exe -s \"C:\\xbmc dir\\\"\n"
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

bool ConvertXML2PO(std::string LangDir, std::string LCode, int nPlurals,
                    std::string PluralForm, bool bIsForeignLang)
{
  int stringCount = 0;
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
    "%s"
    "%s%s%s%s%s%s%s%s%s"
    "msgid \"\"\n"
    "msgstr \"\"\n"
    "\"Project-Id-Version: %s\\n\"\n"
    "\"Report-Msgid-Bugs-To: alanwww1@xbmc.org\\n\"\n"
    "\"POT-Creation-Date: %s\\n\"\n"
    "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
    "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
    "\"Language-Team: LANGUAGE\\n\"\n"
    "\"MIME-Version: 1.0\\n\"\n"
    "\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
    "\"Content-Transfer-Encoding: 8bit\\n\"\n"
    "\"Language: %s\\n\"\n"
    "\"Plural-Forms: nplurals=%i; plural=%s\\n\"\n",
    (projType == CORE) ? ("XBMC " + ProjVersion + "\n").c_str() : "",
    (projType == CORE || projType == UNKNOWN) ? "" : "# Addon Name: ",
    (projType == CORE || projType == UNKNOWN) ? "" : ProjTextName.c_str(),
    (projType == CORE || projType == UNKNOWN) ? "" : "\n# Addon id: ",
    (projType == CORE || projType == UNKNOWN) ? "" : ProjName.c_str(),
    (projType == CORE || projType == UNKNOWN) ? "" : "\n# Addon version: ",
    (projType == CORE || projType == UNKNOWN) ? "" : ProjVersion.c_str(),
    (projType == CORE || projType == UNKNOWN) ? "" : "\n# Addon Provider: ",
    (projType == CORE || projType == UNKNOWN) ? "" : ProjProvider.c_str(),
    (projType == CORE || projType == UNKNOWN) ? "" : "\n",
    (projType == CORE) ? "XBMC-Main" : "XBMC-Addons",
    GetCurrTime().c_str(),
    (!LCode.empty()) ? LCode.c_str() : "LANGUAGE",
    nPlurals, PluralForm.c_str());
    bhasLFWritten =false;

  if (!mapAddonXMLData["en"].strSummary.empty())
  {
    WriteLF(pPOTFile);
    WriteStrLine("msgctxt ", "Addon Summary", addonXMLEncoding);
    WriteStrLine("msgid ", mapAddonXMLData["en"].strSummary.c_str(), addonXMLEncoding);
    WriteStrLine("msgstr ", LCode == "en" ? "": mapAddonXMLData[LCode].strSummary.c_str(), addonXMLEncoding);
    bhasLFWritten =false;
    stringCount++;
  }

  if (!mapAddonXMLData["en"].strDescription.empty())
  {
    WriteLF(pPOTFile);
    WriteStrLine("msgctxt ", "Addon Description", addonXMLEncoding);
    WriteStrLine("msgid ", mapAddonXMLData["en"].strDescription.c_str(), addonXMLEncoding);
    WriteStrLine("msgstr ", LCode == "en" ? "": mapAddonXMLData[LCode].strDescription.c_str(), addonXMLEncoding);
    bhasLFWritten =false;
    stringCount++;
  }

  if (!mapAddonXMLData["en"].strDisclaimer.empty())
  {
    WriteLF(pPOTFile);
    WriteStrLine("msgctxt ", "Addon Disclaimer", addonXMLEncoding);
    WriteStrLine("msgid ", mapAddonXMLData["en"].strDisclaimer.c_str(), addonXMLEncoding);
    WriteStrLine("msgstr ", LCode == "en" ? "": mapAddonXMLData[LCode].strDisclaimer.c_str(), addonXMLEncoding);
    bhasLFWritten =false;
    stringCount++;
  }

//  if (projType == ADDON_NOSTRINGS)
//    return true;

  int previd = -1;

  for (itSourceXmlId = mapSourceXmlId.begin(); itSourceXmlId != mapSourceXmlId.end(); itSourceXmlId++)
  {
    int id = itSourceXmlId->first;
    std::string value = itSourceXmlId->second;
    bhasLFWritten = false;

    //create comment lines, if empty string id or ids found and
    //re-create original xml comments between entries. Only for the source language
    if (!bIsForeignLang)
      WriteComments(previd, true);

    if ((id-previd >= 2) && !bIsForeignLang)
    {
      WriteLF(pPOTFile);
      if (id-previd == 2 && previd > -1)
        fprintf(pPOTFile,"#empty string with id %i\n", id-1);
      if (id-previd > 2 && previd > -1)
        fprintf(pPOTFile,"#empty strings from id %i to %i\n", previd+1, id-1);
    }
    bhasLFWritten = false;

    //write comment originally placed next to the string entry
    //convert it into #. style gettext comment
    if (!bIsForeignLang)
      WriteComments(id, false);

    //create msgctxt, including the string id
    WriteLF(pPOTFile);
    fprintf(pPOTFile,"msgctxt \"#%i\"\n", id);

    //create msgid and msgstr lines
    WriteLF(pPOTFile);
    WriteStrLine("msgid ", value.c_str(), sourceXMLEncoding);
    if (bIsForeignLang)
    {
      itForeignXmlId = mapForeignXmlId.find(id);
      if (itForeignXmlId != mapForeignXmlId.end())
      {
        stringCount++;
        WriteStrLine("msgstr ", itForeignXmlId->second.c_str(), foreignXMLEncoding);
      }
      else fprintf(pPOTFile,"msgstr \"\"\n");
    }
    else fprintf(pPOTFile,"msgstr \"\"\n");

    if (!bIsForeignLang)
      stringCount++;
    previd =id;
  }

  fclose(pPOTFile);

  printf("%i\t\t", stringCount);
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
    }
    ++argv; --argc;
  }

  if (pSourceDirectory == NULL)
  {
    printf("\nWrong Arguments given !\n");
    PrintUsage();
    return 1;
  }

  printf("\nXBMC-XML2PO v0.96 by Team XBMC\n");

  ProjRootDir = pSourceDirectory;
  ProjRootDir = AddSlash(ProjRootDir);

  if ((FileExist(ProjRootDir + "addon.xml")) &&
      (FileExist(ProjRootDir + "resources" + DirSepChar + "language" +
       DirSepChar + "English" + DirSepChar + "strings.xml")))
    projType = ADDON;
  else if ((FileExist(ProjRootDir + "addon.xml")) &&
           (FileExist(ProjRootDir + "language" + DirSepChar + "English" +
            DirSepChar + "strings.xml")))
    projType = SKIN;
  else if (FileExist(ProjRootDir + "addon.xml"))
    projType = ADDON_NOSTRINGS;
  else if (FileExist(ProjRootDir + "xbmc" + DirSepChar + "GUIInfoManager.h"))
    projType = CORE;
  else
    projType = UNKNOWN;

  std::string strprojType;

  switch (projType)
  {
    case ADDON:
      strprojType = "Addon with translatable strings";
      WorkingDir = ProjRootDir + DirSepChar + "resources" + DirSepChar + "language"+ DirSepChar;
      break;
    case SKIN:
      strprojType = "Skin addon";
      WorkingDir = ProjRootDir + DirSepChar + "language"+ DirSepChar;
      break;
    case ADDON_NOSTRINGS:
      strprojType = "Addon without any translatable strings";
      break;
    case CORE:
      strprojType = "XBMC core";
      WorkingDir = ProjRootDir + DirSepChar + "language" + DirSepChar;
      break;
    default:
      strprojType = "Unknown";
      WorkingDir = ProjRootDir;
      break;
  }

  if (projType == ADDON || projType == ADDON_NOSTRINGS || projType == SKIN)
    loadAddonXMLFile(ProjRootDir + "addon.xml");
  else if (projType == CORE)
  {
    ProjTextName = "XBMC Core";
    ProjProvider = "Team XBMC";
    ProjName = "xbmc.core";
    LoadCoreVersion(ProjRootDir + "xbmc" + DirSepChar + "GUIInfoManager.h");
  }

  printf ("Project type detected:\t%s\n", strprojType.c_str());
  printf ("\nProject name:\t\t%s\n", ProjTextName.c_str());
  printf ("Project id:\t\t%s\n", ProjName.c_str());
  printf ("Project version:\t%s\n", ProjVersion.c_str());
  printf ("Project provider:\t%s\n", ProjProvider.c_str());

  if (projType == ADDON_NOSTRINGS)
  {
    if (!DirExists(ProjRootDir + "resources") && (MakeDir(ProjRootDir + "resources") != 0))
    {
      printf ("fatal error: not able to create resources directory at dir: %s", ProjRootDir.c_str());
      return 1;
    }
    if (!DirExists(ProjRootDir + "resources" + DirSepChar + "language") &&
      (MakeDir(ProjRootDir + "resources"+ DirSepChar + "language") != 0))
    {
      printf ("fatal error: not able to create language directory at dir: %s", (ProjRootDir + "resources").c_str());
      return 1;
    }
    WorkingDir = ProjRootDir + "resources"+ DirSepChar + "language" + DirSepChar;
    for (itAddonXMLData = mapAddonXMLData.begin(); itAddonXMLData != mapAddonXMLData.end(); itAddonXMLData++)
    {
      if (!DirExists(WorkingDir + FindLang(itAddonXMLData->first)) && (MakeDir(WorkingDir +
          FindLang(itAddonXMLData->first)) != 0))
      {
        printf ("fatal error: not able to create %s language directory at dir: %s", itAddonXMLData->first.c_str(),
                WorkingDir.c_str());
        return 1;
      }
    }
  }

  if (projType !=ADDON_NOSTRINGS && !loadXMLFile(xmlDocSourceInput, WorkingDir + "English" + DirSepChar + "strings.xml",
      &mapSourceXmlId, true))
  {
    printf("Fatal error: no English source xml file found or it is corrupted\n");
    return 1;
  }

  printf("\nResults:\n\n");
  printf("Langcode\tString match\tOutput file\n");
  printf("--------------------------------------------------------------\n");

  ConvertXML2PO(WorkingDir + "English" + DirSepChar, "en", 2, "(n != 1)", false);

  DIR* Dir;
  struct dirent *DirEntry;
  Dir = opendir(WorkingDir.c_str());
  int langcounter =0;
  std::map<std::string, CLangData> mapLangList;
  std::map<std::string, CLangData>::iterator itmapLangList;

  while((DirEntry=readdir(Dir)))
  {
    if (DirEntry->d_type == DT_DIR && DirEntry->d_name[0] != '.' && strcmp(DirEntry->d_name, "English"))
    {
      CLangData Langdat;
      Langdat.LLangCode = FindLangCode(DirEntry->d_name);
      Langdat.LPlurals = GetnPlurals(DirEntry->d_name);
      Langdat.LPlurForm = GetPlurForm(DirEntry->d_name);
      mapLangList [DirEntry->d_name] = Langdat;
    }
  }

  for (itmapLangList = mapLangList.begin(); itmapLangList != mapLangList.end(); itmapLangList++)
  {
    bool bRunConvert = false;;
    if (projType == ADDON_NOSTRINGS || projType == ADDON || projType == SKIN)
      bRunConvert = true;
    if ((projType != ADDON_NOSTRINGS && loadXMLFile(xmlDocForeignInput, WorkingDir +
        itmapLangList->first + DirSepChar + "strings.xml", &mapForeignXmlId, false)) || bRunConvert)
    {
      ConvertXML2PO(WorkingDir + itmapLangList->first + DirSepChar, itmapLangList->second.LLangCode,
                    itmapLangList->second.LPlurals, itmapLangList->second.LPlurForm, true);
      langcounter++;
    }
  }

  printf("\nReady. Processed %i languages.\n", langcounter+1);

  if (bUnknownLangFound)
    printf("\nWarning: At least one language found with unpaired language code !\n"
      "Please edit the .po file manually and correct the language code, plurals !\n"
      "Also please report this to alanwww1@xbmc.org if possible !\n\n");
  return 0;
}
