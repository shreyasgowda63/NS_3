/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2007 INRIA
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation;
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  *
  * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
  */
 
 #include <iostream>
 #include <iomanip>
 #include <algorithm>
 #include <map>
 #include <climits>    // CHAR_BIT
 
 #include "ns3/command-line.h"
 #include "ns3/config.h"
 #include "ns3/global-value.h"
 #include "ns3/log.h"
 #include "ns3/object-vector.h"
 #include "ns3/object.h"
 #include "ns3/pointer.h"
 #include "ns3/string.h"
 #include "ns3/node-container.h"
 #include "ns3/simple-channel.h"
 #include "ns3/system-path.h"
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE ("PrintIntrospectedDoxygen");
 
 namespace
 {
   std::string anchor;              
   std::string argument;            
   std::string boldStart;           
   std::string boldStop;            
   std::string breakBoth;           
   std::string breakHtmlOnly;       
   std::string breakTextOnly;       
   std::string brief;               
   std::string classStart;          
   std::string classStop;           
   std::string codeWord;            
   std::string commentStart;        
   std::string commentStop;         
   std::string copyDoc;             
   std::string file;                
   std::string flagSpanStart;       
   std::string flagSpanStop;        
   std::string functionStart;       
   std::string functionStop;        
   std::string headingStart;        
   std::string headingStop;         
   std::string indentHtmlOnly;      
   std::string listLineStart;       
   std::string listLineStop;        
   std::string listStart;           
   std::string listStop;            
   std::string note;                
   std::string page;                
   std::string reference;           
   std::string returns;             
   std::string sectionStart;        
   std::string seeAlso;             
   std::string subSectionStart;     
   std::string templArgDeduced;     
   std::string templArgExplicit;    
   std::string templateArgument;    
   std::string variable;            
 
 }  // unnamed namespace
 
 
 void
 SetMarkup (bool outputText)
 {
   NS_LOG_FUNCTION (outputText);
   if (outputText)
     {
       anchor                       = "";
       argument                     = "  Arg: ";
       boldStart                    = "";
       boldStop                     = "";
       breakBoth                    = "\n";
       breakHtmlOnly                = "";
       breakTextOnly                = "\n";
       brief                        = "";
       classStart                   = "";
       classStop                    = "\n\n";
       codeWord                     = " ";
       commentStart                 = "===============================================================\n";
       commentStop                  = "";
       copyDoc                      = "  See: ";
       file                         = "File: ";
       flagSpanStart                = "";
       flagSpanStop                 = "";
       functionStart                = "";
       functionStop                 = "\n\n";
       headingStart                 = "";
       headingStop                  = "";
       indentHtmlOnly               = "";
       listLineStart                = "    * ";
       listLineStop                 = "";
       listStart                    = "";
       listStop                     = "";
       note                         = "Note: ";
       page                         = "Page ";
       reference                    = " ";
       returns                      = "  Returns: ";
       sectionStart                 = "Section ";
       seeAlso                      = "  See: ";
       subSectionStart              = "Subsection ";
       templArgDeduced              = "[deduced]  ";
       templArgExplicit             = "[explicit] ";
       templateArgument             = "Template Arg: ";
       variable                     = "Variable: ";
     }
   else
     {
       anchor                       = "\\anchor ";
       argument                     = "\\param ";
       boldStart                    = "<b>";
       boldStop                     = "</b>";
       breakBoth                    = "<br>";
       breakHtmlOnly                = "<br>";
       breakTextOnly                = "";
       brief                        = "\\brief ";
       classStart                   = "\\class ";
       classStop                    = "";
       codeWord                     = "\\p ";
       commentStart                 = "/*!\n";
       commentStop                  = "*/\n";
       copyDoc                      = "\\copydoc ";
       file                         = "\\file ";
       flagSpanStart                = "<span class=\"mlabel\">";
       flagSpanStop                 = "</span>";
       functionStart                = "\\fn ";
       functionStop                 = "";
       headingStart                 = "<h3>";
       headingStop                  = "</h3>";
       indentHtmlOnly               = "  ";
       listLineStart                = "<li>";
       listLineStop                 = "</li>";
       listStart                    = "<ul>";
       listStop                     = "</ul>";
       note                         = "\\note ";
       page                         = "\\page ";
       reference                    = " \\ref ";
       returns                      = "\\returns ";
       sectionStart                 = "\\ingroup ";
       seeAlso                      = "\\see ";
       subSectionStart              = "\\addtogroup ";
       templArgDeduced              = "\\deduced ";
       templArgExplicit             = "\\explicit ";
       templateArgument             = "\\tparam ";
       variable                     = "\\var ";
     }
 }  // SetMarkup ()
 
 
 /***************************************************************
  *        Aggregation and configuration paths
  ***************************************************************/
 
 class StaticInformation
 {
 public:
   void RecordAggregationInfo (std::string a, std::string b);
   void Gather (TypeId tid);
   void Print (void) const;
 
   std::vector<std::string> Get (TypeId tid) const;
 
   std::vector<std::string> GetNoTypeIds (void) const;
 
 private:
   std::string GetCurrentPath (void) const;
   void DoGather (TypeId tid);
   void RecordOutput (TypeId tid);
   bool HasAlreadyBeenProcessed (TypeId tid) const;
   std::vector<std::pair<TypeId,std::string> > m_output;
   std::vector<std::string> m_currentPath;
   std::vector<TypeId> m_alreadyProcessed;
   std::vector<std::pair<TypeId,TypeId> > m_aggregates;
   mutable std::vector<std::string> m_noTids;
   
 };  // class StaticInformation
 
 
 void 
 StaticInformation::RecordAggregationInfo (std::string a, std::string b)
 {
   NS_LOG_FUNCTION (this << a << b);
   TypeId aTid;
   bool found = TypeId::LookupByNameFailSafe (a, &aTid);
   if (!found)
     {
       m_noTids.push_back (a);
       return;
     }
   TypeId bTid;
   found = TypeId::LookupByNameFailSafe (b, &bTid);
   if (!found)
     {
       m_noTids.push_back (b);
       return;
     }
 
   m_aggregates.push_back (std::make_pair (aTid, bTid));
 }
 
 
 void 
 StaticInformation::Print (void) const
 {
   NS_LOG_FUNCTION (this);
   for (auto item : m_output)
     {
       std::cout << item.first.GetName () << " -> " << item.second << std::endl;
     }
 }
 
 
 std::string
 StaticInformation::GetCurrentPath (void) const
 {
   NS_LOG_FUNCTION (this);
   std::ostringstream oss;
   for (auto item : m_currentPath)
     {
       oss << "/" << item;
     }
   return oss.str ();
 }
 
 
 void
 StaticInformation::RecordOutput (TypeId tid)
 {
   NS_LOG_FUNCTION (this << tid);
   m_output.push_back (std::make_pair (tid, GetCurrentPath ()));
 }
 
 
 bool
 StaticInformation::HasAlreadyBeenProcessed (TypeId tid) const
 {
   NS_LOG_FUNCTION (this << tid);
   for (auto it : m_alreadyProcessed)
     {
       if (it == tid)
     {
       return true;
     }
     }
   return false;
 }
 
 
 std::vector<std::string> 
 StaticInformation::Get (TypeId tid) const
 {
   NS_LOG_FUNCTION (this << tid);
   std::vector<std::string> paths;
   for (auto item : m_output)
     {
       if (item.first == tid)
     {
       paths.push_back (item.second);
     }
     }
   return paths;
 }
 
 template <typename T>
 void
 Uniquefy (T t)
 {
   std::sort (t.begin (), t.end ());
   t.erase (std::unique (t.begin (), t.end ()), t.end ());
 }
 
 std::vector<std::string>
 StaticInformation::GetNoTypeIds (void) const
 {
   NS_LOG_FUNCTION (this);
   Uniquefy (m_noTids);
   return m_noTids;
 }
 
 
 void
 StaticInformation::Gather (TypeId tid)
 {
   NS_LOG_FUNCTION (this << tid);
   DoGather (tid);
   Uniquefy (m_output);
 }
 
 
 void 
 StaticInformation::DoGather (TypeId tid)
 {
   NS_LOG_FUNCTION (this << tid);
   if (HasAlreadyBeenProcessed (tid))
     {
       return;
     }
   RecordOutput (tid);
   for (uint32_t i = 0; i < tid.GetAttributeN (); ++i)
     {
       struct TypeId::AttributeInformation info = tid.GetAttribute(i);
       const PointerChecker *ptrChecker = dynamic_cast<const PointerChecker *> (PeekPointer (info.checker));
       if (ptrChecker != 0)
         {
           TypeId pointee = ptrChecker->GetPointeeTypeId ();
 
       // See if this is a pointer to an Object.
       Ptr<Object> object = CreateObject<Object> ();
       TypeId objectTypeId = object->GetTypeId ();
       if (objectTypeId == pointee)
         {
           // Stop the recursion at this attribute if it is a
           // pointer to an Object, which create too many spurious
           // paths in the list of attribute paths because any
           // Object can be in that part of the path.
           continue;
         }
 
           m_currentPath.push_back (info.name);
           m_alreadyProcessed.push_back (tid);
           DoGather (pointee);
           m_alreadyProcessed.pop_back ();
           m_currentPath.pop_back ();
           continue;
         }
       // attempt to cast to an object vector.
       const ObjectPtrContainerChecker *vectorChecker = dynamic_cast<const ObjectPtrContainerChecker *> (PeekPointer (info.checker));
       if (vectorChecker != 0)
         {
           TypeId item = vectorChecker->GetItemTypeId ();
           m_currentPath.push_back (info.name + "/[i]");
           m_alreadyProcessed.push_back (tid);
           DoGather (item);
           m_alreadyProcessed.pop_back ();
           m_currentPath.pop_back ();
           continue;
         }
     }
   for (uint32_t j = 0; j < TypeId::GetRegisteredN (); j++)
     {
       TypeId child = TypeId::GetRegistered (j);
       if (child.IsChildOf (tid))
         {
           std::string childName = "$" + child.GetName ();
           m_currentPath.push_back (childName);
           m_alreadyProcessed.push_back (tid);
           DoGather (child);
           m_alreadyProcessed.pop_back ();
           m_currentPath.pop_back ();
         }
     }
   for (auto item : m_aggregates)
     {
       if (item.first == tid || item.second == tid)
         {
           TypeId other;
           if (item.first == tid)
             {
               other = item.second;
             }
           if (item.second == tid)
             {
               other = item.first;
             }
           std::string name = "$" + other.GetName ();
           m_currentPath.push_back (name);
           m_alreadyProcessed.push_back (tid);
           DoGather (other);
           m_alreadyProcessed.pop_back ();
           m_currentPath.pop_back ();      
         }
     }
 }  // StaticInformation::DoGather ()
 
 
 StaticInformation GetTypicalAggregations ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   static StaticInformation info;
   static bool mapped = false;
 
   if (mapped)
     {
       return info;
     }
 
   // Short circuit next call
   mapped = true;
 
   // The below statements register typical aggregation relationships
   // in ns-3 programs, that otherwise aren't picked up automatically
   // by the creation of the above node.  To manually list other common
   // aggregation relationships that you would like to see show up in
   // the list of configuration paths in the doxygen, add additional
   // statements below.
   info.RecordAggregationInfo ("ns3::Node", "ns3::TcpSocketFactory");
   info.RecordAggregationInfo ("ns3::Node", "ns3::UdpSocketFactory");
   info.RecordAggregationInfo ("ns3::Node", "ns3::PacketSocketFactory");
   info.RecordAggregationInfo ("ns3::Node", "ns3::MobilityModel");
   info.RecordAggregationInfo ("ns3::Node", "ns3::Ipv4L3Protocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::Ipv4NixVectorRouting");
   info.RecordAggregationInfo ("ns3::Node", "ns3::Icmpv4L4Protocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::ArpL3Protocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::Icmpv4L4Protocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::UdpL4Protocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::Ipv6L3Protocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::Icmpv6L4Protocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::TcpL4Protocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::RipNg");
   info.RecordAggregationInfo ("ns3::Node", "ns3::GlobalRouter");
   info.RecordAggregationInfo ("ns3::Node", "ns3::aodv::RoutingProtocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::dsdv::RoutingProtocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::dsr::DsrRouting");
   info.RecordAggregationInfo ("ns3::Node", "ns3::olsr::RoutingProtocol");
   info.RecordAggregationInfo ("ns3::Node", "ns3::EnergyHarvesterContainer");
   info.RecordAggregationInfo ("ns3::Node", "ns3::EnergySourceContainer");
 
   // Create a channel object so that channels appear in the namespace
   // paths that will be generated here.
   Ptr<SimpleChannel> simpleChannel;
   simpleChannel = CreateObject<SimpleChannel> ();
 
   for (uint32_t i = 0; i < Config::GetRootNamespaceObjectN (); ++i)
     {
       Ptr<Object> object = Config::GetRootNamespaceObject (i);
       info.Gather (object->GetInstanceTypeId ());
     }
 
   return info;
 
 }  // GetTypicalAggregations ()
 
 
 typedef std::map< std::string, int32_t> NameMap;
 typedef NameMap::const_iterator         NameMapIterator; 
 
 
 NameMap
 GetNameMap (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   static NameMap nameMap;
   static bool mapped = false;
 
   if (mapped)
     {
       return nameMap;
     }
 
   // Short circuit next call
   mapped = true;
   
   // Get typical aggregation relationships.
   StaticInformation info = GetTypicalAggregations ();
 
   // Registered types
   for (uint32_t i = 0; i < TypeId::GetRegisteredN (); i++)
     {
       TypeId tid = TypeId::GetRegistered (i);
       if (tid.MustHideFromDocumentation ())
     {
       continue;
     }
       
       // Capitalize all of letters in the name so that it sorts
       // correctly in the map.
       std::string name = tid.GetName ();
       std::transform (name.begin (), name.end (), name.begin (), ::toupper);
       
       // Save this name's index.
       nameMap[name] = i;
     }
 
   // Type names without TypeIds
   std::vector<std::string> noTids = info.GetNoTypeIds ();
   for (auto item : noTids)
     {
       nameMap[item] = -1;
     }
        
   return nameMap;
 }  // GetNameMap ()
 
 
 /***************************************************************
  *        Docs for a single TypeId
  ***************************************************************/
 
 void
 PrintConfigPaths (std::ostream & os, const TypeId tid)
 {
   NS_LOG_FUNCTION (tid);
   std::vector<std::string> paths = GetTypicalAggregations ().Get (tid);
 
   // Config --------------
   if (paths.empty ())
     {
       os << "Introspection did not find any typical Config paths."
      << breakBoth
      << std::endl;
     }
   else
     {
       os << headingStart
      <<   "Config Paths"
      << headingStop
      << std::endl;
       os << std::endl;
       os << tid.GetName ()
      << " is accessible through the following paths"
      << " with Config::Set and Config::Connect:"
      << std::endl;
       os << listStart << std::endl;
       for (auto path : paths)
     {
       os << listLineStart
              <<   "\"" << path << "\""
          <<  listLineStop 
          << breakTextOnly
          << std::endl;
     }
       os << listStop << std::endl;
     }
 }  // PrintConfigPaths ()
       
 
 void
 PrintAttributesTid (std::ostream &os, const TypeId tid)
 {
   NS_LOG_FUNCTION (tid);
   os << listStart << std::endl;
   for (uint32_t j = 0; j < tid.GetAttributeN (); j++)
     {
       struct TypeId::AttributeInformation info = tid.GetAttribute(j);
       os << listLineStart
      <<   boldStart << info.name << boldStop << ": "
      <<   info.help
      <<   std::endl;
       os <<   "  "
      <<   listStart << std::endl;
       os <<     "    "
      <<     listLineStart
      <<       "Set with class: " << reference
      <<       info.checker->GetValueTypeName ()
      <<     listLineStop
      << std::endl;
       if (info.checker->HasUnderlyingTypeInformation ())
     {
       os << "    "
          << listLineStart
          <<   "Underlying type: ";
           
           std::string valType = info.checker->GetValueTypeName ();
           std::string underType = info.checker->GetUnderlyingTypeInformation ();
       if ((valType   != "ns3::EnumValue") && (underType != "std::string"))
         {
           // Indirect cases to handle
           bool handled = false;
               
           if (valType == "ns3::PointerValue")
         {
           const PointerChecker *ptrChecker =
             dynamic_cast<const PointerChecker *> (PeekPointer (info.checker));
           if (ptrChecker != 0)
             {
               os << reference << "ns3::Ptr" << "< "
              << reference << ptrChecker->GetPointeeTypeId ().GetName ()
              << ">";
               handled = true;
             }
         }
           else if (valType == "ns3::ObjectPtrContainerValue")
         {
           const ObjectPtrContainerChecker * ptrChecker =
             dynamic_cast<const ObjectPtrContainerChecker *> (PeekPointer (info.checker));
           if (ptrChecker != 0)
             {
               os << reference << "ns3::Ptr" << "< "
              << reference << ptrChecker->GetItemTypeId ().GetName ()
              << ">";
               handled = true;
             }
         }
               // Helper to match first part of string
               class StringBeginMatcher
               {
               public:
                 StringBeginMatcher (const std::string s)
                   : m_string (s) { };
                 bool operator () (const std::string t)
                 {
                   std::size_t pos = m_string.find (t);
                   return pos == 0;
                 };
               private:
                 std::string m_string;
               };
               StringBeginMatcher match (underType);
                   
               if ( match ("bool")     || match ("double")   ||
                    match ("int8_t")   || match ("uint8_t")  ||
                    match ("int16_t")  || match ("uint16_t") ||
                    match ("int32_t")  || match ("uint32_t") ||
                    match ("int64_t")  || match ("uint64_t")
                    )
                 {
                   os << underType;
                   handled = true;
                 }
           if (! handled)
         {
           os << reference << underType;
         }
         }
       os << listLineStop << std::endl;
     }
       if (info.flags & TypeId::ATTR_CONSTRUCT && info.accessor->HasSetter ())
     {
       os << "    "
          << listLineStart
          <<   "Initial value: "
          <<   info.initialValue->SerializeToString (info.checker)
          << listLineStop
          << std::endl;
     }
       os << "    " << listLineStart << "Flags: ";
       if (info.flags & TypeId::ATTR_CONSTRUCT && info.accessor->HasSetter ())
     {
       os << flagSpanStart << "construct " << flagSpanStop;
     }
       if (info.flags & TypeId::ATTR_SET && info.accessor->HasSetter ())
     {
       os << flagSpanStart << "write " << flagSpanStop;
     }
       if (info.flags & TypeId::ATTR_GET && info.accessor->HasGetter ())
     {
       os << flagSpanStart << "read " << flagSpanStop;
     }
       os << listLineStop << std::endl;
       os << "  "
      << listStop
      << " " << std::endl;
       
     }
   os << listStop << std::endl;
 }  // PrintAttributesTid ()
 
 
 void
 PrintAttributes (std::ostream & os, const TypeId tid)
 {
   NS_LOG_FUNCTION (tid);
   if (tid.GetAttributeN () == 0)
     {
       os << "No Attributes are defined for this type."
      << breakBoth
      << std::endl;
     }
   else
     {
       os << headingStart
      <<   "Attributes"
      << headingStop
      << std::endl;
       PrintAttributesTid (os, tid);
     }
 
   // Attributes from base classes
   TypeId tmp = tid.GetParent ();
   while (tmp.GetParent () != tmp)
     {
       if (tmp.GetAttributeN () != 0)
     {
       os << headingStart
          <<   "Attributes defined in parent class "
          <<   tmp.GetName ()
          << headingStop
          << std::endl;
       PrintAttributesTid (os, tmp);
     }
       tmp = tmp.GetParent ();
 
     }  // Attributes
 } // PrintAttributes ()
 
 
 void
 PrintTraceSourcesTid (std::ostream & os, const TypeId tid)
 {
   NS_LOG_FUNCTION (tid);
   os << listStart << std::endl;
   for (uint32_t i = 0; i < tid.GetTraceSourceN (); ++i)
     {
       struct TypeId::TraceSourceInformation info = tid.GetTraceSource (i);
       os << listLineStart
      <<   boldStart << info.name << boldStop << ": "
      <<   info.help << breakBoth
     //    '%' prevents doxygen from linking to the Callback class...
      <<   "%Callback signature: " 
      <<   info.callback
      <<   std::endl;
       os << listLineStop << std::endl;
     }
   os << listStop << std::endl;
 }  // PrintTraceSourcesTid ()
 
 
 void
 PrintTraceSources (std::ostream & os, const TypeId tid)
 {
   NS_LOG_FUNCTION (tid);
   if (tid.GetTraceSourceN () == 0)
     {
       os << "No TraceSources are defined for this type."
      << breakBoth
      << std::endl;
     }
   else
     {
       os << headingStart
      <<   "TraceSources"
      << headingStop  << std::endl;
       PrintTraceSourcesTid (os, tid);
     }
 
   // Trace sources from base classes
   TypeId tmp = tid.GetParent ();
   while (tmp.GetParent () != tmp)
     {
       if (tmp.GetTraceSourceN () != 0)
     {
       os << headingStart
          << "TraceSources defined in parent class "
          << tmp.GetName ()
          << headingStop << std::endl;
       PrintTraceSourcesTid (os, tmp);
     }
       tmp = tmp.GetParent ();
     }
 
 }  // PrintTraceSources ()
 
 void PrintSize (std::ostream & os, const TypeId tid)
 {
   NS_LOG_FUNCTION (tid);
   NS_ASSERT_MSG (CHAR_BIT != 0, "CHAR_BIT is zero");
   
   std::size_t arch = (sizeof (void *) * CHAR_BIT);
   
   os << boldStart << "Size" << boldStop
      << " of this type is " << tid.GetSize ()
      << " bytes (on a " << arch << "-bit architecture)."
      << std::endl;
 }  // PrintSize ()
 
 
 void
 PrintTypeIdBlocks (std::ostream & os)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   NameMap nameMap = GetNameMap ();
 
   // Iterate over the map, which will print the class names in
   // alphabetical order.
   for (auto item : nameMap)
     {
       // Handle only real TypeIds
       if (item.second < 0)
         {
           continue ;
         }
       // Get the class's index out of the map;
       TypeId tid = TypeId::GetRegistered (item.second);
       std::string name = tid.GetName ();
       
       std::cout << commentStart << std::endl;
       
       std::cout << classStart << name << std::endl;
       std::cout << std::endl;
 
       PrintConfigPaths (std::cout, tid);
       PrintAttributes (std::cout, tid);
       PrintTraceSources (std::cout, tid);
       PrintSize (std::cout, tid);
       
       std::cout << commentStop << std::endl;
     }  // for class documentation
 
 }  // PrintTypeIdBlocks

 void
 PrintTypeIdBlock (std::ostream & os, const TypeId tid)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   NameMap nameMap = GetNameMap ();
 
   // Iterate over the map, which will print the class names in
   // alphabetical order.
   for (auto item : nameMap)
     {
       // Handle only real TypeIds
       if (item.second < 0)
         {
           continue ;
         }
       // Get the class's index out of the map;
       TypeId tid = TypeId::GetRegistered (item.second);
       std::string name = tid.GetName ();
       
       std::cout << commentStart << std::endl;
       
       std::cout << classStart << name << std::endl;
       std::cout << std::endl;
 
       PrintConfigPaths (std::cout, tid);
       PrintAttributes (std::cout, tid);
       PrintTraceSources (std::cout, tid);
       PrintSize (std::cout, tid);
       
       std::cout << commentStop << std::endl;
     }  // for class documentation
 
 }  // PrintTypeIdBlock
 
 
 /***************************************************************
  *        Lists of All things
  ***************************************************************/
 
 void
 PrintAllTypeIds (std::ostream & os)
 {
   NS_LOG_FUNCTION_NOARGS ();
   os << commentStart << page << "TypeIdList All TypeIds\n"
      << std::endl;
   os << "This is a list of all" << reference << "TypeIds.\n"
      << "For more information see the" << reference << "TypeId "
      << "section of this API documentation and the TypeId section "
      << "in the Configuration and Attributes chapter of the Manual.\n"
      << std::endl;
 
   os << listStart << std::endl;
   
   NameMap nameMap = GetNameMap ();
   // Iterate over the map, which will print the class names in
   // alphabetical order.
   for (auto item : nameMap)
     {
       // Handle only real TypeIds
       if (item.second < 0)
         {
           continue ;
         }
       // Get the class's index out of the map;
       TypeId tid = TypeId::GetRegistered (item.second);
       
       os << indentHtmlOnly
      <<   listLineStart
          <<     boldStart
          <<       tid.GetName ()
          <<     boldStop
      <<   listLineStop
      << std::endl;
       
     }
   os << commentStop << std::endl;
 
 }  // PrintAllTypeIds ()
 
 
 void
 PrintAllAttributes (std::ostream & os)
 {
   NS_LOG_FUNCTION_NOARGS ();
   os << commentStart << page << "AttributeList All Attributes\n"
      << std::endl;
   os << "This is a list of all" << reference << "attribute by class.  "
      << "For more information see the" << reference << "attribute "
      << "section of this API documentation and the Attributes sections "
      << "in the Tutorial and Manual.\n"
      << std::endl;
 
   NameMap nameMap = GetNameMap ();
   // Iterate over the map, which will print the class names in
   // alphabetical order.
   for (auto item: nameMap)
     {
       // Handle only real TypeIds
       if (item.second < 0)
         {
           continue ;
         }
       // Get the class's index out of the map;
       TypeId tid = TypeId::GetRegistered (item.second);
       
       if (tid.GetAttributeN () == 0 )
     {
       continue;
     }
       os << boldStart << tid.GetName () << boldStop << breakHtmlOnly
      << std::endl;
       
       os << listStart << std::endl;
       for (uint32_t j = 0; j < tid.GetAttributeN (); ++j)
     {
       struct TypeId::AttributeInformation info = tid.GetAttribute(j);
       os << listLineStart
          <<   boldStart << info.name << boldStop
          <<   ": "      << info.help
          << listLineStop
          << std::endl;
     }
       os << listStop << std::endl;
     }
   os << commentStop << std::endl;
 
 }  // PrintAllAttributes ()
 
 
 void
 PrintAllGlobals (std::ostream & os)
 {
   NS_LOG_FUNCTION_NOARGS ();
   os << commentStart << page << "GlobalValueList All GlobalValues\n"
      << std::endl;
   os << "This is a list of all" << reference << "ns3::GlobalValue instances.\n"
      << std::endl;
   
   os << listStart << std::endl;
   for (GlobalValue::Iterator i = GlobalValue::Begin ();
        i != GlobalValue::End ();
        ++i)
     {
       StringValue val;
       (*i)->GetValue (val);
       os << indentHtmlOnly
      <<   listLineStart
      <<     boldStart
      <<       anchor
      <<       "GlobalValue" << (*i)->GetName () << " " << (*i)->GetName ()
      <<     boldStop
      <<     ": "            << (*i)->GetHelp ()
      <<     ".  Default value: " << val.Get () << "."
      <<   listLineStop
      << std::endl;
     }
   os << listStop << std::endl;
   os << commentStop << std::endl;
 
 }  // PrintAllGlobals ()
 
 
 void
 PrintAllLogComponents (std::ostream & os)
 {
   NS_LOG_FUNCTION_NOARGS ();
   os << commentStart << page << "LogComponentList All LogComponents\n"
      << std::endl;
   os << "This is a list of all" << reference << "ns3::LogComponent instances.\n"
      << std::endl;
 
   LogComponent::ComponentList * logs = LogComponent::GetComponentList ();
   // Find longest log name
   std::size_t widthL = std::string ("Log Component").size ();
   std::size_t widthR = std::string ("file").size ();
   for (auto it : (*logs))
     {
       widthL = std::max (widthL, it.first.size ());
       std::string file = it.second->File ();
       // Strip leading "../" related to depth in build directory
       // since doxygen only sees the path starting with "src/", etc.
       while (file.find ("../") == 0)
         {
           file = file.substr (3);
         }
       widthR  = std::max (widthR, file.size ());
     }
   const std::string sep (" | ");
 
   os << std::setw (widthL) << std::left << "Log Component" << sep
     // Header line has to be padded to same length as separator line
      << std::setw (widthR) << std::left << "File " << std::endl;
   os << ":" << std::string (widthL - 1, '-') << sep
      << ":" << std::string (widthR - 1, '-') << std::endl;
   
   LogComponent::ComponentList::const_iterator it;
   for (auto it : (*logs))
     {
       std::string file = it.second->File ();
       // Strip leading "../" related to depth in build directory
       // since doxygen only sees the path starting with "src/", etc.
       while (file.find ("../") == 0)
         {
           file = file.substr (3);
         }
       
       os << std::setw (widthL) << std::left << it.first << sep << file << std::endl;
     }
   os << std::right << std::endl;
   os << commentStop << std::endl;
 }  // PrintAllLogComponents ()
 
 
 void
 PrintAllTraceSources (std::ostream & os)
 {
   NS_LOG_FUNCTION_NOARGS ();
   os << commentStart << page << "TraceSourceList All TraceSources\n"
      << std::endl;
   os << "This is a list of all" << reference << "tracing sources.  "
      << "For more information see the " << reference << "tracing "
      << "section of this API documentation and the Tracing sections "
      << "in the Tutorial and Manual.\n"
      << std::endl;
 
   NameMap nameMap = GetNameMap ();
 
   // Iterate over the map, which will print the class names in
   // alphabetical order.
   for (auto item : nameMap)
     {
       // Handle only real TypeIds
       if (item.second < 0)
         {
           continue ;
         }
       // Get the class's index out of the map;
       TypeId tid = TypeId::GetRegistered (item.second);
 
       if (tid.GetTraceSourceN () == 0 )
     {
       continue;
     }
       os << boldStart << tid.GetName () << boldStop  << breakHtmlOnly
      << std::endl;
       
       os << listStart << std::endl;
       for (uint32_t j = 0; j < tid.GetTraceSourceN (); ++j)
     {
       struct TypeId::TraceSourceInformation info = tid.GetTraceSource(j);
       os << listLineStart 
          <<   boldStart << info.name << boldStop
          <<   ": "      << info.help
          << listLineStop
          << std::endl;
     }
       os << listStop << std::endl;
     }
   os << commentStop << std::endl;
 
 }  // PrintAllTraceSources ()
 
 
 /***************************************************************
  *        Docs for Attribute classes
  ***************************************************************/
 
 
 void
 PrintAttributeValueSection (std::ostream & os,
                             const std::string & name,
                             const bool seeBase = true)
 {
   NS_LOG_FUNCTION (name);
   std::string section = "attribute_" + name;
 
   // \ingroup attribute
   // \defgroup attribute_<name>Value <name> Attribute
   os << commentStart << sectionStart << "attribute\n"
      <<   subSectionStart << "attribute_" << name << " "
      <<     name << " Attribute\n"
      <<     "Attribute implementation for " << name << "\n";
   if (seeBase)
     {
       // Some classes don't live in ns3::.  Yuck
       if (name != "IeMeshId")
         {
           os << seeAlso << "ns3::" << name << "\n";
         }
       else
         {
           os << seeAlso << "ns3::dot11s::" << name << "\n";
         }
     }
   os << commentStop;
 
 }  // PrintAttributeValueSection ()
 
 
 void
 PrintAttributeValueWithName (std::ostream & os,
                              const std::string & name,
                              const std::string & type,
                              const std::string & header)
 {
   NS_LOG_FUNCTION (name << type << header);
   std::string sectAttr = sectionStart + "attribute_" + name;
   
   // \ingroup attribute_<name>Value
   // \class ns3::<name>Value "header"
   std::string valClass  = name + "Value";
   std::string qualClass = " ns3::" + valClass;
   
   os << commentStart << sectAttr << std::endl;
   os <<   classStart << qualClass << " \"" << header << "\"" << std::endl;
   os <<   "AttributeValue implementation for " << name << "." << std::endl;
   os <<   seeAlso << "AttributeValue" << std::endl;
   os << commentStop;
 
   // Copy ctor: <name>Value::<name>Value
   os << commentStart
      <<   functionStart << name
      <<     qualClass << "::" << valClass;
   if ( (name == "EmptyAttribute") ||
        (name == "ObjectPtrContainer") )
     {
       // Just default constructors.
       os << "(void)\n";
     }
   else
     {
       // Copy constructors
       os << "(const " << type << " & value)\n"
          << "Copy constructor.\n"
          << argument << "[in] value The " << name << " value to copy.\n";
     }
   os << commentStop;
 
   // <name>Value::Get (void) const
   os << commentStart
      <<   functionStart << type
      <<     qualClass << "::Get (void) const\n"
      <<   returns << "The " << name << " value.\n"
      << commentStop;
 
   // <name>Value::GetAccessor (T & value) const
   os << commentStart
      <<   functionStart << "bool"
      <<     qualClass << "::GetAccessor (T & value) const\n"
      <<   "Access the " << name << " value as type " << codeWord << "T.\n"
      <<   templateArgument << "T " << templArgExplicit << "The type to cast to.\n"
      <<   argument << "[out] value The " << name << " value, as type "
      <<     codeWord << "T.\n"
      <<   returns << "true.\n"
      << commentStop;
 
   // <name>Value::Set (const name & value)
   if (type != "Callback")  // Yuck
     {
       os << commentStart
          <<   functionStart << "void"
          <<     qualClass << "::Set (const " << type << " & value)\n"
          <<   "Set the value.\n"
          <<   argument << "[in] value The value to adopt.\n"
          << commentStop;
     }
 
   // <name>Value::m_value
   os << commentStart
      <<   variable << type
      <<     qualClass << "::m_value\n" 
      <<   "The stored " << name << " instance.\n"
      << commentStop
      << std::endl;
   
 }  // PrintAttributeValueWithName ()
 
 
 void
 PrintMakeAccessors (std::ostream & os, const std::string & name)
 {
   NS_LOG_FUNCTION (name);
   std::string sectAttr = sectionStart + "attribute_" + name + "\n";
   std::string make = "ns3::Make" + name + "Accessor ";
   
   // \ingroup attribute_<name>Value
   // Make<name>Accessor (T1 a1)
   os << commentStart << sectAttr
      <<   functionStart << "ns3::Ptr<const ns3::AttributeAccessor> "
      <<     make << "(T1 a1)\n"
      <<   copyDoc << "ns3::MakeAccessorHelper(T1)\n"
      <<   seeAlso << "AttributeAccessor\n"
      << commentStop;
 
   // \ingroup attribute_<name>Value
   // Make<name>Accessor (T1 a1)
   os << commentStart << sectAttr
      <<   functionStart << "ns3::Ptr<const ns3::AttributeAccessor> "
      <<     make << "(T1 a1, T2 a2)\n"
      <<   copyDoc << "ns3::MakeAccessorHelper(T1,T2)\n"
      <<   seeAlso << "AttributeAccessor\n"
      << commentStop;
 }  // PrintMakeAccessors ()
 
 
 void
 PrintMakeChecker (std::ostream & os,
                   const std::string & name,
                   const std::string & header)
 {
   NS_LOG_FUNCTION (name << header);
   std::string sectAttr = sectionStart + "attribute_" + name + "\n";
   std::string make = "ns3::Make" + name + "Checker ";
 
   // \ingroup attribute_<name>Value
   // class <name>Checker
   os << commentStart << sectAttr << std::endl;
   os <<   classStart << " ns3::" << name << "Checker"
      <<   " \"" << header << "\"" << std::endl;
   os <<   "AttributeChecker implementation for " << name << "Value." << std::endl;
   os <<   seeAlso << "AttributeChecker" << std::endl;
   os << commentStop;
     
   // \ingroup attribute_<name>Value
   // Make<name>Checker (void)
   os << commentStart << sectAttr
      <<   functionStart << "ns3::Ptr<const ns3::AttributeChecker> "
      <<     make << "(void)\n"
      <<   returns << "The AttributeChecker.\n"
      <<   seeAlso << "AttributeChecker\n"
      << commentStop;
 }  // PrintMakeChecker ()
 
 
 typedef struct {
   const std::string m_name;   
   const std::string m_type;   
   const bool m_seeBase;       
   const std::string m_header; 
 } AttributeDescriptor;
 
 
 void
 PrintAttributeHelper (std::ostream & os,
                       const AttributeDescriptor & attr)
 {
   NS_LOG_FUNCTION (attr.m_name << attr.m_type << attr.m_seeBase <<
                    attr.m_header);
   PrintAttributeValueSection  (os, attr.m_name, attr.m_seeBase);
   PrintAttributeValueWithName (os, attr.m_name, attr.m_type, attr.m_header);
   PrintMakeAccessors          (os, attr.m_name);
   PrintMakeChecker            (os, attr.m_name, attr.m_header);
 }  // PrintAttributeHelper ()
 
 
 void
 PrintAttributeImplementations (std::ostream & os)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   const AttributeDescriptor attributes [] =
     {
       // Name             Type             see Base  header-file
       // Users of ATTRIBUTE_HELPER_HEADER
       //
       { "Address",        "Address",        true,  "address.h"          },
       { "Box",            "Box",            true,  "box.h"              },
       { "DataRate",       "DataRate",       true,  "data-rate.h"        },
       { "DsssParameterSet",
                           "DsssParameterSet",
                                             true,  "dsss-parameter-set.h"},
       { "EdcaParameterSet",
                           "EdcaParameterSet",
                                             true,  "edca-parameter-set.h"},
       { "ErpInformation", "ErpInformation", true,  "erp-information.h"  },
       { "ExtendedCapabilities", "ExtendedCapabilities", true,  "extended-capabilities.h"  },
       { "HeCapabilities", "HeCapabilities", true,  "he-capabilities.h"  },
       { "VhtCapabilities","VhtCapabilities",true,  "vht-capabilities.h" },
       { "HtCapabilities", "HtCapabilities", true,  "ht-capabilities.h"  },
       { "IeMeshId",       "IeMeshId",       true,  "ie-dot11s-id.h"     },
       { "Ipv4Address",    "Ipv4Address",    true,  "ipv4-address.h"     },
       { "Ipv4Mask",       "Ipv4Mask",       true,  "ipv4-address.h"     },
       { "Ipv6Address",    "Ipv6Address",    true,  "ipv6-address.h"     },
       { "Ipv6Prefix",     "Ipv6Prefix",     true,  "ipv6-address.h"     },
       { "Mac16Address",   "Mac16Address",   true,  "mac16-address.h"    },
       { "Mac48Address",   "Mac48Address",   true,  "mac48-address.h"    },
       { "Mac64Address",   "Mac64Address",   true,  "mac64-address.h"    },
       { "ObjectFactory",  "ObjectFactory",  true,  "object-factory.h"   },
       { "OrganizationIdentifier",
                           "OrganizationIdentifier",
                                             true,  "vendor-specific-action.h" },
       { "Rectangle",      "Rectangle",      true,  "rectangle.h"        },
       { "Ssid",           "Ssid",           true,  "ssid.h"             },
       { "TypeId",         "TypeId",         true,  "type-id.h"          },
       { "UanModesList",   "UanModesList",   true,  "uan-tx-mode.h"      },
       // { "ValueClassTest", "ValueClassTest", false, "" /* outside ns3 */ },
       { "Vector",         "Vector",         true,  "vector.h"           },
       { "Vector2D",       "Vector2D",       true,  "vector.h"           },
       { "Vector3D",       "Vector3D",       true,  "vector.h"           },
       { "HeOperation",    "HeOperation",    true,  "he-operation.h"    },
       { "VhtOperation",   "VhtOperation",   true,  "vht-operation.h"    },
       { "HtOperation",    "HtOperation",    true,  "ht-operation.h"  },
       { "Waypoint",       "Waypoint",       true,  "waypoint.h"         },
       { "WifiMode",       "WifiMode",       true,  "wifi-mode.h"        },
       
       // All three (Value, Access and Checkers) defined, but custom
       { "Boolean",        "Boolean",        false, "boolean.h"          },
       { "Callback",       "Callback",       true,  "callback.h"         },
       { "Double",         "double",         false, "double.h"           },
       { "Enum",           "int",            false, "enum.h"             },
       { "Integer",        "int64_t",        false, "integer.h"          },
       { "Pointer",        "Pointer",        false, "pointer.h"          },
       { "RandomVariable", "RandomVariable", true,  "random-variable-stream.h"  },
       { "String",         "std::string",    false, "string.h"           },
       { "Time",           "Time",           true,  "nstime.h"           },
       { "Uinteger",       "uint64_t",       false, "uinteger.h"         },
       { "",               "",               false, "last placeholder"   }
     };
 
   int i = 0;
   while (attributes[i].m_name != "")
     {
       PrintAttributeHelper (os, attributes[i]);
       ++i;
     }
 
   // Special cases
   PrintAttributeValueSection  (os, "EmptyAttribute", false);
   PrintAttributeValueWithName (os, "EmptyAttribute", "EmptyAttribute",
                                    "attribute.h");
 
   PrintAttributeValueSection  (os, "ObjectPtrContainer", false);
   PrintAttributeValueWithName (os, "ObjectPtrContainer", "ObjectPtrContainer", "object-ptr-container.h");
   PrintMakeChecker            (os, "ObjectPtrContainer",  "object-ptr-container.h");
 
   PrintAttributeValueSection  (os, "ObjectVector", false);
   PrintMakeAccessors          (os, "ObjectVector");
   PrintMakeChecker            (os, "ObjectVector", "object-vector.h");
 
   PrintAttributeValueSection  (os, "ObjectMap", false);
   PrintMakeAccessors          (os, "ObjectMap");
   PrintMakeChecker            (os, "ObjectMap", "object-map.h");
   
 }  // PrintAttributeImplementations ()
 
 
 /***************************************************************
  *        Main
  ***************************************************************/
 
 int main (int argc, char *argv[])
 {
   NS_LOG_FUNCTION_NOARGS ();
   bool outputText = false;
   std::string typeId;
 
   CommandLine cmd;
   cmd.Usage ("Generate documentation for all ns-3 registered types, "
          "trace sources, attributes and global variables.");
   cmd.AddValue ("output-text", "format output as plain text", outputText);
   cmd.AddValue ("TypeId", "Print docs for just the given TypeId", typeId);
   cmd.Parse (argc, argv);
     
   SetMarkup (outputText);
   
   TypeId tid;

   bool validTypeId = TypeId::LookupByNameFailSafe (typeId, &tid);
   if (!validTypeId)
   {
      std::cerr << "Invalid TypeId name: " << typeId << std::endl;
      std::cerr << cmd;
      exit (1);
   }
   else
   {
      PrintTypeIdBlock (std::cout, tid);
      return 0;
   }

 
   // Create a Node, to force linking and instantiation of our TypeIds
   NodeContainer c;
   c.Create (1);
 
   // mode-line:  helpful when debugging introspected-doxygen.h
   if (!outputText)
     {
       std::cout << "/* -*- Mode:C++; c-file-style:\"gnu\"; "
                "indent-tabs-mode:nil; -*- */"
         << std::endl;
     }
 
   // Doxygen file header
   std::cout << std::endl;
   std::cout << commentStart
             << file << "\n"
             << sectionStart << "utils\n"
             << "Doxygen docs generated from the TypeId database.\n"
             << note << "This file is automatically generated by "
             << codeWord << "print-introspected-doxygen.cc. Do not edit this file! "
             << "Edit that file instead.\n"
             << commentStop
             << std::endl;
 
   PrintTypeIdBlocks (std::cout);
   
   PrintAllTypeIds (std::cout);
   PrintAllAttributes (std::cout);
   PrintAllGlobals (std::cout);
   PrintAllLogComponents (std::cout);
   PrintAllTraceSources (std::cout);
   PrintAttributeImplementations (std::cout);
 
   return 0;
 }