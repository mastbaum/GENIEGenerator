//____________________________________________________________________________
/*!

\class    genie::Messenger

\brief    A more convenient interface to the log4cpp Message Service

\author   Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
          CCLRC, Rutherford Appleton Laboratory

\created  June 16, 2004

*/
//____________________________________________________________________________

#include <iostream>
#include <vector>

#include "libxml/parser.h"
#include "libxml/xmlmemory.h"

#include <TSystem.h>

#include "Messenger/Messenger.h"
#include "Utils/StringUtils.h"
#include "Utils/PrintUtils.h"
#include "Utils/XmlParserUtils.h"

using namespace genie;

using std::cout;
using std::endl;
using std::vector;

//____________________________________________________________________________
Messenger * Messenger::fInstance = 0;
//____________________________________________________________________________
Messenger::Messenger()
{
  fInstance =  0;
}
//____________________________________________________________________________
Messenger::~Messenger()
{
  fInstance = 0;
}
//____________________________________________________________________________
Messenger * Messenger::Instance()
{
  if(fInstance == 0) {

    // the first thing that get's printed in a GENIE session is the banner
    print_utils::PrintBanner();

    static Messenger::Cleaner cleaner;
    cleaner.DummyMethodAndSilentCompiler();

    fInstance = new Messenger;

    log4cpp::Appender * appender;
    appender = new log4cpp::OstreamAppender("default", &cout);
    appender->setLayout(new log4cpp::BasicLayout());

    log4cpp::Category & MSG = log4cpp::Category::getRoot();

    MSG.setAdditivity(false);
    MSG.addAppender(appender);

    fInstance->Configure(); // set user-defined priority levels
  }
  return fInstance;
}
//____________________________________________________________________________
log4cpp::Category & Messenger::operator () (const char * stream)
{
  log4cpp::Category & MSG = log4cpp::Category::getInstance(stream);

  return MSG;
}
//____________________________________________________________________________
void Messenger::SetPriorityLevel(
                       const char * stream, log4cpp::Priority::Value priority)
{
  log4cpp::Category & MSG = log4cpp::Category::getInstance(stream);

  MSG.setPriority(priority);
}
//____________________________________________________________________________
void Messenger::Configure(void)
{
// The Configure() method will look for priority level xml config files, read
// the priority levels and set them.
// By default, first it will look for the $GENIE/config/messenger.xml file.
// Then it will look any messenger configuration xml file defined in the
// GMSGCONF env variable. This variable can contain more than one XML files
// (that should be delimited with a ';'). The full path for each file should
// be given. See the $GENIE/config/messenger.xml for the XML schema.
// The later each file is, the higher priority it has - eg. if the same stream
// is listed twice with conflicting priority then the one found last is used.

  bool ok = false;

  //-- get the default messenger configuration XML file
  string base_dir = string( gSystem->Getenv("GENIE") );
  string msg_config_file = base_dir + string("/config/messenger.xml");

  // parse & set the default priority levels
  ok = this->SetPrioritiesFromXmlFile(msg_config_file);
  if(!ok) {
    SLOG("Messenger", pERROR)
           << "Priority levels from: " << msg_config_file << " were not set!";
  }

  //-- checkout the GMSGCONF conf for additional messenger configuration files
  string gmsgconf = (gSystem->Getenv("GMSGCONF") ?
                                            gSystem->Getenv("GMSGCONF") : "");
  SLOG("Messenger", pINFO) << "$GMSGCONF env.var = " << gmsgconf;

  if(gmsgconf.size()>0) {
     //-- check for multiple files delimited with a ":"
     vector<string> conf_xmlv = string_utils::Split(gmsgconf, ":");

     //-- loop over messenger config files -- parse & set priorities
     vector<string>::const_iterator conf_iter;
     for(conf_iter = conf_xmlv.begin();
                                 conf_iter != conf_xmlv.end(); ++conf_iter) {
          string conf_xml = *conf_iter;
          ok = this->SetPrioritiesFromXmlFile(conf_xml);
          if(!ok) {
            SLOG("Messenger", pERROR)
                << "Priority levels from: " << conf_xml << " were not set!";
            }
     }
  } else {
    SLOG("Messenger", pINFO)
                  << "No additional messenger config XML file was specified";
  }
}
//____________________________________________________________________________
bool Messenger::SetPrioritiesFromXmlFile(string filename)
{
// Reads the XML config file and sets the priority levels
//
  SLOG("Messenger", pINFO)
            << "Reading msg stream priorities from XML file: " << filename;
  xmlDocPtr xml_doc = xmlParseFile(filename.c_str());

  if(xml_doc==NULL) {
    SLOG("Messenger", pERROR)
           << "XML file could not be parsed! [file: " << filename << "]";
    return false;
  }

  xmlNodePtr xml_root = xmlDocGetRootElement(xml_doc);

  if(xml_root==NULL) {
    SLOG("Messenger", pERROR)
         << "XML doc. has null root element! [file: " << filename << "]";
    return false;
  }

  if( xmlStrcmp(xml_root->name, (const xmlChar *) "messenger_config") ) {
    SLOG("Messenger", pERROR)
      << "XML doc. has invalid root element! [file: " << filename << "]";
    return false;
  }

  xmlNodePtr xml_msgp = xml_root->xmlChildrenNode; // <priority>'s

  // loop over all xml tree nodes that are children of the <spline> node
  while (xml_msgp != NULL) {

     // enter everytime you find a <priority> tag
     if( (!xmlStrcmp(xml_msgp->name, (const xmlChar *) "priority")) ) {

         string msgstream = string_utils::TrimSpaces(
                  XmlParserUtils::GetAttribute(xml_msgp, "msgstream"));
         string priority =
                XmlParserUtils::TrimSpaces( xmlNodeListGetString(
                               xml_doc, xml_msgp->xmlChildrenNode, 1));
         SLOG("Messenger", pINFO)
           << "Setting priority level: " << msgstream << " --> " << priority;

         log4cpp::Priority::Value pv = this->PriorityFromString(priority);
         this->SetPriorityLevel(msgstream.c_str(), pv);
     }
     xml_msgp = xml_msgp->next;
  }
  xmlFree(xml_msgp);

  return true;
}
//____________________________________________________________________________
log4cpp::Priority::Value Messenger::PriorityFromString(string p)
{
  if ( p.find("FATAL")  != string::npos ) return log4cpp::Priority::FATAL;
  if ( p.find("ALERT")  != string::npos ) return log4cpp::Priority::ALERT;
  if ( p.find("CRIT")   != string::npos ) return log4cpp::Priority::CRIT;
  if ( p.find("ERROR")  != string::npos ) return log4cpp::Priority::ERROR;
  if ( p.find("WARN")   != string::npos ) return log4cpp::Priority::WARN;
  if ( p.find("NOTICE") != string::npos ) return log4cpp::Priority::NOTICE;
  if ( p.find("INFO")   != string::npos ) return log4cpp::Priority::INFO;
  if ( p.find("DEBUG")  != string::npos ) return log4cpp::Priority::DEBUG;

  SLOG("Messenger", pWARN)
                    << "Unknown priority = " << p << " - Setting to INFO";
  return log4cpp::Priority::INFO;
}
//____________________________________________________________________________

